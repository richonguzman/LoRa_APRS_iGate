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

/*___________________________________________________________________

██╗      ██████╗ ██████╗  █████╗      █████╗ ██████╗ ██████╗ ███████╗
██║     ██╔═══██╗██╔══██╗██╔══██╗    ██╔══██╗██╔══██╗██╔══██╗██╔════╝
██║     ██║   ██║██████╔╝███████║    ███████║██████╔╝██████╔╝███████╗
██║     ██║   ██║██╔══██╗██╔══██║    ██╔══██║██╔═══╝ ██╔══██╗╚════██║
███████╗╚██████╔╝██║  ██║██║  ██║    ██║  ██║██║     ██║  ██║███████║
╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝
                                                                     
                ██╗ ██████╗  █████╗ ████████╗███████╗
                ██║██╔════╝ ██╔══██╗╚══██╔══╝██╔════╝
                ██║██║  ███╗███████║   ██║   █████╗
                ██║██║   ██║██╔══██║   ██║   ██╔══╝
                ██║╚██████╔╝██║  ██║   ██║   ███████╗
                ╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝


                       Ricardo Guzman - CA2RXU
           https://github.com/richonguzman/LoRa_APRS_iGate
             (donations : http://paypal.me/richonguzman)
___________________________________________________________________*/

#include <ElegantOTA.h>
#include <TinyGPS++.h>
#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>
#include "configuration.h"
#include "network_manager.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "power_utils.h"
#include "sleep_utils.h"
#include "mqtt_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "web_utils.h"
#include "tnc_utils.h"
#include "ntp_utils.h"
#include "wx_utils.h"
#include "display.h"
#include "utils.h"
#ifdef HAS_A7670
    #include "A7670_utils.h"
#endif


String              versionDate             = "2025-12-29";
String              versionNumber           = "3.1.7";
Configuration       Config;
WiFiClient          aprsIsClient;
WiFiClient          mqttClient;
#ifdef HAS_GPS
    HardwareSerial  gpsSerial(1);
    TinyGPSPlus     gps;
    uint32_t        gpsSatelliteTime        = 0;
    bool            gpsInfoToggle           = false;
#endif

NetworkManager      *networkManager;

bool                isUpdatingOTA           = false;
uint32_t            lastBatteryCheck        = 0;

bool                backUpDigiMode          = false;
bool                modemLoggedToAPRSIS     = false;

#ifdef HAS_EPAPER
    uint32_t        lastEpaperTime          = 0;
    extern String   lastEpaperText;
#endif

std::vector<ReceivedPacket> receivedPackets;

String firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine;


void setup() {
    Serial.begin(115200);
    networkManager = new NetworkManager();
    networkManager->setup();
    if (Config.wifiAutoAP.enabled) {
        networkManager->setAPTimeout(Config.wifiAutoAP.timeout * 60 * 1000); // Convert minutes to milliseconds
    }
    networkManager->setHostName("iGATE-" + Config.callsign);
    POWER_Utils::setup();
    Utils::setupDisplay();
    LoRa_Utils::setup();
    Utils::validateFreqs();
    GPS_Utils::setup();
    STATION_Utils::loadBlacklistAndManagers();
    Utils::startupDelay();
    SLEEP_Utils::setup();
    WIFI_Utils::setup();
    NTP_Utils::setup();
    SYSLOG_Utils::setup();
    WX_Utils::setup();
    WEB_Utils::setup();
    TNC_Utils::setup();
    MQTT_Utils::setup();
    #ifdef HAS_A7670
        A7670_Utils::setup();
    #endif
    Utils::checkRebootMode();
    APRS_IS_Utils::firstConnection();
    SLEEP_Utils::checkSerial();
}

void loop() {
    if (Config.digi.ecoMode == 1) {
        SLEEP_Utils::checkWakeUpFlag();
        Utils::checkBeaconInterval();
        STATION_Utils::processOutputPacketBufferUltraEcoMode();
        Utils::checkSleepByLowBatteryVoltage(1);
        SLEEP_Utils::startSleeping();
    } else {
        networkManager->loop();

        if (isUpdatingOTA) {
            ElegantOTA.loop();
            return; // Don't process IGate and Digi during OTA update
        }
        
        #ifdef HAS_GPS
            if (Config.beacon.gpsActive) {
                if (millis() - gpsSatelliteTime > 5000) {
                    gpsInfoToggle = !gpsInfoToggle;
                    gpsSatelliteTime = millis();
                }
                if (gpsInfoToggle) {
                    thirdLine = "Satellite(s): ";
                    String gpsData = String(gps.satellites.value());
                    if (gpsData.length() < 2) gpsData = "0" + gpsData;  // Ensure two-digit formatting
                    thirdLine += gpsData;
                } else {
                    thirdLine = Utils::getLocalIP();
                }
            } else {
                thirdLine = Utils::getLocalIP();
            }
        #else
            thirdLine = Utils::getLocalIP();
        #endif

        #ifdef HAS_A7670
            // TODO: Make this part of Network manager, and use ESP-IDF network stack instead manual AT commands
            if (Config.aprs_is.active && !modemLoggedToAPRSIS) A7670_Utils::APRS_IS_connect();
        #else
            WIFI_Utils::checkWiFi();
            if (networkManager->isConnected()) {
                if (Config.aprs_is.active && !aprsIsClient.connected()) APRS_IS_Utils::connect();
                if (Config.mqtt.active && !mqttClient.connected()) MQTT_Utils::connect();
            }
        #endif

        NTP_Utils::update();
        TNC_Utils::loop();
        MQTT_Utils::loop();

        Utils::checkDisplayInterval();
        Utils::checkBeaconInterval();

        APRS_IS_Utils::checkStatus(); // Need that to update display, maybe split this and send APRSIS status to display func?

        String packet = "";
        if (Config.loramodule.rxActive) {
            packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
        }

        if (packet != "") {
            if (Config.aprs_is.active) {    // If APRSIS enabled
                APRS_IS_Utils::processLoRaPacket(packet); // Send received packet to APRSIS
            }

            if (Config.loramodule.txActive && (Config.digi.mode == 2 || Config.digi.mode == 3 || backUpDigiMode)) { // If Digi enabled
                STATION_Utils::clean25SegBuffer();
                DIGI_Utils::processLoRaPacket(packet); // Send received packet to Digi
            }

            if (Config.tnc.enableServer) TNC_Utils::sendToClients(packet, true);    // Send received packet to TNC KISS
            if (Config.tnc.enableSerial) TNC_Utils::sendToSerial(packet, true);     // Send received packet to Serial KISS
            if (Config.mqtt.active) MQTT_Utils::sendToMqtt(packet);                 // Send received packet to MQTT
        }

        if (Config.aprs_is.active) APRS_IS_Utils::listenAPRSIS();           // listen received packet from APRSIS

        STATION_Utils::processOutputPacketBuffer();

        #ifdef HAS_EPAPER   // Only consider updating every 10 seconds (when data to show is different from before)
            if(lastEpaperTime == 0 || millis() - lastEpaperTime > 10000) {
                String posibleEpaperText = firstLine + secondLine + thirdLine + fourthLine + fifthLine + sixthLine + seventhLine;
                if (lastEpaperText != posibleEpaperText) {
                    displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
                    lastEpaperText = posibleEpaperText;
                    lastEpaperTime = millis();
                }
            }
        #else
            displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
        #endif

        Utils::checkRebootTime();
        Utils::checkSleepByLowBatteryVoltage(1);
    }
}
