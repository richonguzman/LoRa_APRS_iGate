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

#include <HTTPClient.h>
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
extern bool             backUpDigiMode;

bool        WiFiConnected       = false;
uint32_t    WiFiAutoAPTime      = millis();
bool        WiFiAutoAPStarted   = false;
uint32_t    previousWiFiMillis  = 0;
uint8_t     wifiCounter         = 0;
uint32_t    lastBackupDigiTime  = millis();


namespace WIFI_Utils {

    bool checkInternetConnection() {
        HTTPClient http;
        http.begin("http://connectivitycheck.gstatic.com/generate_204");    // Google Connectivity Detection Endpoint
        http.setTimeout(2000);
        http.setConnectTimeout(2000);
        
        int httpCode = http.GET();
        http.end();
        
        if (httpCode == 204 || httpCode == HTTP_CODE_OK) {
            return true;
        } else {
            Serial.printf("Internet: FAIL (Code: %d)\n", httpCode);
            return false;
        }
    }

    void checkWiFi() {
        if (Config.digi.ecoMode != 0) return;

        if (backUpDigiMode) {
            if (WiFi.status() != WL_CONNECTED && ((millis() - lastBackupDigiTime) >= 15 * 60 * 1000)) {
                Serial.println("*** Stopping BackUp Digi Mode ***");
                backUpDigiMode = false;
                wifiCounter = 0;
            } else if (WiFi.status() == WL_CONNECTED) {
                Serial.println("*** WiFi Reconnect Success (Stopping Backup Digi Mode) ***");
                backUpDigiMode = false;
                wifiCounter = 0;
            }
        }

        if (!backUpDigiMode && ((millis() - previousWiFiMillis) >= 60 * 1000) && !WiFiAutoAPStarted) {
            previousWiFiMillis = millis();
            if (WiFi.status() == WL_CONNECTED) {
                if (Config.backupDigiMode) {
                    bool internetOK = checkInternetConnection();
                    if (!internetOK && Config.backupDigiMode) {
                        Serial.println("*** Internet LOST → Backup Digi Mode ***");
                        backUpDigiMode = true;
                        WiFi.disconnect();
                    }
                }                
            } else {
                Serial.println("Reconnecting to WiFi...");
                WiFi.disconnect();
                WIFI_Utils::startWiFi();

                if (Config.backupDigiMode) wifiCounter++;
                if (wifiCounter >= 2) {
                    Serial.println("*** Starting BackUp Digi Mode ***");
                    backUpDigiMode = true;
                    lastBackupDigiTime = millis();
                }
            }
        }
    }

    void startAutoAP() {
        WiFi.mode(WIFI_MODE_NULL);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(Config.callsign + "-AP", Config.wifiAutoAP.password);

        WiFiAutoAPTime = millis();
        WiFiAutoAPStarted = true;
    }

    void startWiFi() {
        bool startAP = false;
        if (currentWiFi->ssid == "") {
            startAP = true;
        } else {
            uint8_t wifiCounter = 0;
            String hostName = "iGATE-" + Config.callsign;
            WiFi.setHostname(hostName.c_str());
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(500);
            unsigned long start = millis();
            displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
            Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.print("' ");
            WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
            while (WiFi.status() != WL_CONNECTED && wifiCounter<myWiFiAPSize) {
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN,HIGH);
                #endif
                Serial.print('.');
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN,LOW);
                #endif
                if ((millis() - start) > 10000){
                    delay(1000);
                    if(myWiFiAPIndex >= (myWiFiAPSize - 1)) {
                        myWiFiAPIndex = 0;
                        wifiCounter++;
                    } else {
                        myWiFiAPIndex++;
                    }
                    wifiCounter++;
                    currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
                    start = millis();
                    Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
                    displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
                    WiFi.disconnect();
                    WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
                }
            }
        }
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("\nConnected as ");
            Serial.print(WiFi.localIP());
            Serial.print(" / MAC Address: ");
            Serial.println(WiFi.macAddress());
            displayShow("", "     Connected!!", "" , "     loading ...", 1000);
        } else if (WiFi.status() != WL_CONNECTED) {
            startAP = true;

            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", " WiFi Not Connected!", "" , "     loading ...", 1000);
        }
        WiFiConnected = !startAP;
        if (startAP) {
            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", "   Starting Auto AP", " Please connect to it " , "     loading ...", 1000);

            startAutoAP();
        }
    }

    void checkAutoAPTimeout() {
        if (WiFiAutoAPStarted && Config.wifiAutoAP.timeout > 0) {
            if (WiFi.softAPgetStationNum() > 0) {
                WiFiAutoAPTime = 0;
            } else {
                if (WiFiAutoAPTime == 0) {
                    WiFiAutoAPTime = millis();
                } else if ((millis() - WiFiAutoAPTime) > Config.wifiAutoAP.timeout * 60 * 1000) {
                    Serial.println("Stopping auto AP");

                    WiFiAutoAPStarted = false;
                    WiFi.softAPdisconnect(true);

                    Serial.println("Auto AP stopped (timeout)");
                }
            }
        }
    }

    void setup() {
        if (Config.digi.ecoMode == 0) startWiFi();
        btStop();
    }

}