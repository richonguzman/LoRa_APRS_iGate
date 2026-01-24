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
#include "serial_ports.h"
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

    void checkWiFi() {
        if (Config.digi.ecoMode == 0) {
            if (backUpDigiMode) {
                uint32_t WiFiCheck = millis() - lastBackupDigiTime;
                if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15 * 60 * 1000) {
                    DEBUG_PRINTLN("*** Stopping BackUp Digi Mode ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                } else if (WiFi.status() == WL_CONNECTED) {
                    DEBUG_PRINTLN("*** WiFi Reconnect Success (Stopping Backup Digi Mode) ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                }
            }

            if (!backUpDigiMode && (WiFi.status() != WL_CONNECTED) && ((millis() - previousWiFiMillis) >= 30 * 1000) && !WiFiAutoAPStarted) {
                DEBUG_PRINT(millis());
                DEBUG_PRINTLN("Reconnecting to WiFi...");
                WiFi.disconnect();
                WIFI_Utils::startWiFi();
                previousWiFiMillis = millis();

                if (Config.backupDigiMode) {
                    wifiCounter++;
                }
                if (wifiCounter >= 2) {
                    DEBUG_PRINTLN("*** Starting BackUp Digi Mode ***");
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
            DEBUG_PRINT("\nConnecting to WiFi '"); DEBUG_PRINT(currentWiFi->ssid); DEBUG_PRINT("' ");
            WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
            while (WiFi.status() != WL_CONNECTED && wifiCounter<myWiFiAPSize) {
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN,HIGH);
                #endif
                DEBUG_PRINT('.');
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
                    DEBUG_PRINT("\nConnecting to WiFi '"); DEBUG_PRINT(currentWiFi->ssid); DEBUG_PRINTLN("' ...");
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
            DEBUG_PRINT("\nConnected as ");
            DEBUG_PRINT(WiFi.localIP());
            DEBUG_PRINT(" / MAC Address: ");
            DEBUG_PRINTLN(WiFi.macAddress());
            displayShow("", "     Connected!!", "" , "     loading ...", 1000);
        } else if (WiFi.status() != WL_CONNECTED) {
            startAP = true;

            DEBUG_PRINTLN("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", " WiFi Not Connected!", "" , "     loading ...", 1000);
        }
        WiFiConnected = !startAP;
        if (startAP) {
            DEBUG_PRINTLN("\nNot connected to WiFi! Starting Auto AP");
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
                    DEBUG_PRINTLN("Stopping auto AP");

                    WiFiAutoAPStarted = false;
                    WiFi.softAPdisconnect(true);

                    DEBUG_PRINTLN("Auto AP stopped (timeout)");
                }
            }
        }
    }

    void setup() {
        if (Config.digi.ecoMode == 0) startWiFi();
        btStop();
    }

}