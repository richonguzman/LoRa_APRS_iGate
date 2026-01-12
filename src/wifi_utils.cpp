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
#include "network_manager.h"
#include "board_pinout.h"
#include "wifi_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration    Config;
extern NetworkManager   *networkManager;

extern uint8_t          myWiFiAPIndex;
extern int              myWiFiAPSize;
extern WiFi_AP          *currentWiFi;
extern bool             backUpDigiMode;

bool        WiFiConnected       = false;
uint32_t    WiFiAutoAPTime      = millis();
uint32_t    previousWiFiMillis  = 0;
uint8_t     wifiCounter         = 0;
uint32_t    lastBackupDigiTime  = millis();


namespace WIFI_Utils {

    void checkWiFi() {
        if (Config.digi.ecoMode == 0) {
            if (backUpDigiMode) {
                uint32_t WiFiCheck = millis() - lastBackupDigiTime;
                if (!networkManager->isWiFiConnected() && WiFiCheck >= 15 * 60 * 1000) {
                    Serial.println("*** Stopping BackUp Digi Mode ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                } else if (networkManager->isWiFiConnected()) {
                    Serial.println("*** WiFi Reconnect Success (Stopping Backup Digi Mode) ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                }
            }

            if (!backUpDigiMode && (!networkManager->isWiFiConnected()) && ((millis() - previousWiFiMillis) >= 30 * 1000) && !networkManager->isWifiAPActive()) {
                Serial.print(millis());
                Serial.println("Reconnecting to WiFi...");
                WIFI_Utils::startWiFi();
                previousWiFiMillis = millis();

                if (Config.backupDigiMode) {
                    wifiCounter++;
                }
                if (wifiCounter >= 2) {
                    Serial.println("*** Starting BackUp Digi Mode ***");
                    backUpDigiMode = true;
                    lastBackupDigiTime = millis();
                }
            }
        }
    }

    void startAutoAP() {
        displayShow("", "   Starting Auto AP", " Please connect to it " , "     loading ...", 1000);

        networkManager->setupAP(Config.callsign + "-AP", Config.wifiAutoAP.password);
        WiFiAutoAPTime = millis();
    }

    void startWiFi() {
        if (currentWiFi->ssid.isEmpty()) {
            Serial.println("WiFi SSID not set! Starting Auto AP");
            startAutoAP();
            return;
        }

        String hostName = "iGATE-" + Config.callsign;
        WiFi.setHostname(hostName.c_str());

        // TODO: Create generic multi-SSID support in Network Manager
        while (!networkManager->isWiFiConnected()) {
            displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
            networkManager->disconnectWiFi();
            networkManager->connectWiFi(currentWiFi->ssid, currentWiFi->password);

            if(myWiFiAPIndex >= (myWiFiAPSize - 1)) {
                break;
            }

            myWiFiAPIndex++;
            currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
        }

        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        if (networkManager->isWiFiConnected()) {
            Serial.print("\nConnected as ");
            Serial.print(networkManager->getWiFiIP());
            Serial.print(" / MAC Address: ");
            Serial.println(networkManager->getWiFimacAddress());
            displayShow("", "     Connected!!", "" , "     loading ...", 1000);
        } else {
            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", " WiFi Not Connected!", "" , "     loading ...", 1000);
            startAutoAP();
        }

        //TODO: Use network manager whenever this variable is used
        WiFiConnected = networkManager->isWiFiConnected();
    }

    void checkAutoAPTimeout() {
        if (networkManager->isWifiAPActive() && Config.wifiAutoAP.timeout > 0) {
            if (WiFi.softAPgetStationNum() > 0) {
                WiFiAutoAPTime = 0;
            } else {
                if (WiFiAutoAPTime == 0) {
                    WiFiAutoAPTime = millis();
                } else if ((millis() - WiFiAutoAPTime) > Config.wifiAutoAP.timeout * 60 * 1000) {
                    Serial.println("Stopping auto AP");

                    networkManager->disableAP();

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
