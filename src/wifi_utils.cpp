/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <WiFi.h>
#include "configuration.h"
#include "board_pinout.h"
#include "wifi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern uint8_t          myWiFiAPIndex;
extern int              myWiFiAPSize;
extern WiFi_AP          *currentWiFi;

bool        WiFiConnected       = false;
bool        WiFiAutoAPStarted   = false;
uint32_t    WiFiAutoAPTime      = 0;

// OPTIMIERUNG: Non-Blocking WiFi-Verbindung
namespace {
    uint32_t lastConnectionAttempt = 0;
    const uint32_t CONNECTION_TIMEOUT = 10000; // 10 Sekunden
    const uint32_t RECONNECT_DELAY = 30000;    // 30 Sekunden zwischen Versuchen
    bool isConnecting = false;
}

namespace WIFI_Utils {

    void checkIfWiFiAP() {
        if (Config.wifiAPs[0].ssid == "") {
            startAutoAP();
        }
    }

    void startAutoAP() {
        String hostName = "iGATE-";
        hostName += Config.callsign;
        hostName += "-AP";
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP(hostName.c_str(), Config.wifiAutoAP.password.c_str());
        Serial.print("\nAccess Point '");
        Serial.print(hostName);
        Serial.println("' started");
        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());

        displayShow("", "WiFi AP Started!", "AP: " + hostName, 
                   "Pass: " + Config.wifiAutoAP.password, 
                   "IP: " + WiFi.softAPIP().toString(), "", 0);

        WiFiAutoAPTime = millis();
        WiFiAutoAPStarted = true;
    }

    void checkAutoAPTimeout() {
        if (WiFiAutoAPStarted && Config.wifiAutoAP.timeout != 0) {
            if (WiFi.softAPgetStationNum() == 0) {
                if ((millis() - WiFiAutoAPTime) >= (Config.wifiAutoAP.timeout * 60 * 1000)) {
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_OFF);
                    WiFiAutoAPStarted = false;
                    Serial.println("\nAuto AP Timeout - WiFi turned OFF");
                    displayShow("", "Auto AP Timeout", "WiFi turned OFF", "", "", "", 2000);
                }
            } else {
                WiFiAutoAPTime = millis();
            }
        }
    }

    // OPTIMIERUNG: Non-Blocking WiFi Connect
    void startWiFi() {
        bool startAP = false;
        
        if (currentWiFi->ssid == "") {
            startAP = true;
        } else {
            String hostName = "iGATE-";
            hostName += Config.callsign;
            
            WiFi.setHostname(hostName.c_str());
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(500);

            Serial.print("\nConnecting to WiFi '");
            Serial.print(currentWiFi->ssid);
            Serial.println("' ...");
            
            displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
            
            WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
            
            // OPTIMIERUNG: Non-Blocking mit Timeout
            unsigned long start = millis();
            uint8_t wifiCounter = 0;
            
            while (WiFi.status() != WL_CONNECTED && wifiCounter < myWiFiAPSize) {
                if (millis() - start > CONNECTION_TIMEOUT) {
                    if (myWiFiAPIndex >= (myWiFiAPSize - 1)) {
                        myWiFiAPIndex = 0;
                        wifiCounter++;
                    } else {
                        myWiFiAPIndex++;
                    }
                    
                    if (wifiCounter >= myWiFiAPSize) {
                        break;
                    }
                    
                    currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
                    start = millis();
                    
                    Serial.print("\nTrying next WiFi: '");
                    Serial.print(currentWiFi->ssid);
                    Serial.println("'");
                    
                    displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
                    
                    WiFi.disconnect();
                    delay(100);
                    WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
                }
                
                // Non-Blocking delay
                delay(100);
                
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN, !digitalRead(INTERNAL_LED_PIN));
                #endif
            }
            
            #ifdef INTERNAL_LED_PIN
                digitalWrite(INTERNAL_LED_PIN, LOW);
            #endif
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Connected as ");
            Serial.print(WiFi.localIP());
            Serial.print(" / MAC Address: ");
            Serial.println(WiFi.macAddress());
            
            displayShow("", "     Connected!!", "", "     loading ...", 1000);
            WiFiConnected = true;
            isConnecting = false;
            lastConnectionAttempt = millis();
        } else {
            startAP = true;
            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", " WiFi Not Connected!", "", "     loading ...", 1000);
            WiFiConnected = false;
        }

        if (startAP) {
            Serial.println("\nStarting Auto AP Mode");
            startAutoAP();
        }
    }

    // OPTIMIERUNG: Non-Blocking WiFi Check mit Reconnect-Logic
    void checkWiFi() {
        if (Config.digi.ecoMode == 1 || Config.digi.ecoMode == 2) {
            return;
        }

        if (WiFiAutoAPStarted) {
            return;
        }

        // Verbindung verloren?
        if (WiFiConnected && WiFi.status() != WL_CONNECTED) {
            WiFiConnected = false;
            Serial.println("WiFi connection lost!");
        }

        // OPTIMIERUNG: Rate-Limited Reconnect Attempts
        if (!WiFiConnected && !isConnecting) {
            uint32_t now = millis();
            
            if (now - lastConnectionAttempt > RECONNECT_DELAY) {
                lastConnectionAttempt = now;
                isConnecting = true;
                
                Serial.println("Attempting to reconnect WiFi...");
                
                WiFi.disconnect();
                delay(100);
                WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
                
                // Non-Blocking Check
                unsigned long start = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - start < CONNECTION_TIMEOUT) {
                    delay(100);
                }
                
                if (WiFi.status() == WL_CONNECTED) {
                    WiFiConnected = true;
                    Serial.println("WiFi reconnected successfully!");
                    displayShow("", "WiFi Reconnected!", "", WiFi.localIP().toString(), 2000);
                } else {
                    Serial.println("WiFi reconnection failed");
                    
                    // Try next AP
                    if (myWiFiAPIndex >= (myWiFiAPSize - 1)) {
                        myWiFiAPIndex = 0;
                    } else {
                        myWiFiAPIndex++;
                    }
                    currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
                }
                
                isConnecting = false;
            }
        }
    }

    void setup() {
        if (Config.wifiAPs.size() == 0) {
            Serial.println("No WiFi APs configured!");
            startAutoAP();
            return;
        }

        #ifdef INTERNAL_LED_PIN
            pinMode(INTERNAL_LED_PIN, OUTPUT);
            digitalWrite(INTERNAL_LED_PIN, LOW);
        #endif

        startWiFi();
    }

}
