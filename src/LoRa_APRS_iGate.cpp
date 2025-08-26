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
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "power_utils.h"
#include "sleep_utils.h"
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


String              versionDate             = "2025-08-20";
Configuration       Config;
WiFiClient          espClient;
#ifdef HAS_GPS
    HardwareSerial  gpsSerial(1);
    TinyGPSPlus     gps;
    uint32_t        gpsSatelliteTime        = 0;
    bool            gpsInfoToggle           = false;
#endif

uint8_t             myWiFiAPIndex           = 0;
int                 myWiFiAPSize            = Config.wifiAPs.size();
WiFi_AP             *currentWiFi            = &Config.wifiAPs[myWiFiAPIndex];

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
//#define STARTUP_DELAY 5 //min

#ifdef HAS_TWO_CORES
    QueueHandle_t aprsIsTxQueue = NULL;
    QueueHandle_t aprsIsRxQueue = NULL;
#endif


void setup() {
    Serial.begin(115200);
    POWER_Utils::setup();
    Utils::setupDisplay();
    LoRa_Utils::setup();
    Utils::validateFreqs();
    GPS_Utils::setup();
    STATION_Utils::loadBlacklistAndManagers();

    #ifdef STARTUP_DELAY    // (TEST) just to wait for WiFi init of Routers
        displayShow("", "  STARTUP DELAY ...", "", "", 0);
        delay(STARTUP_DELAY * 60 * 1000);
    #endif

    SLEEP_Utils::setup();
    WIFI_Utils::setup();
    NTP_Utils::setup();
    SYSLOG_Utils::setup();
    WX_Utils::setup();
    WEB_Utils::setup();
    TNC_Utils::setup();
    #ifdef HAS_A7670
        A7670_Utils::setup();
    #endif
    Utils::checkRebootMode();

    APRS_IS_Utils::firstConnection();
    SLEEP_Utils::checkSerial();

    // Crear queues con verificación detallada
    //Serial.println("Creando aprsIsTxQueue...");
    aprsIsTxQueue = xQueueCreate(50, sizeof(String));
    //Serial.printf("aprsIsTxQueue = %p\n", aprsIsTxQueue);

    //Serial.println("Creando aprsIsRxQueue...");
    aprsIsRxQueue = xQueueCreate(50, sizeof(String));
    //Serial.printf("aprsIsRxQueue = %p\n", aprsIsRxQueue);

    // Verificación crítica
    if (aprsIsRxQueue == NULL || aprsIsTxQueue == NULL) {
        Serial.println("FATAL: Error creando queues!");
        while(1) {
            Serial.println("STUCK - Queues failed");
            delay(1000);
        }
    }
    Serial.println("Queues creadas OK");

    // Iniciar el task de APRSIS
    if (!APRS_IS_Utils::startListenerAPRSISTask()) {
        Serial.println("Error: No se pudo crear el task de APRSIS");
    }
}

void loop() {
    //Serial.println("Loop tick: " + String(millis()));
    //delay(1000);

    if (Config.digi.ecoMode == 1) {
        SLEEP_Utils::checkWakeUpFlag();
        Utils::checkBeaconInterval();
        STATION_Utils::processOutputPacketBufferUltraEcoMode();
        Utils::checkSleepByLowBatteryVoltage(1);
        SLEEP_Utils::startSleeping();
    } else {
        WIFI_Utils::checkAutoAPTimeout();

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
            if (Config.aprs_is.active && !modemLoggedToAPRSIS) A7670_Utils::APRS_IS_connect();
        #else
            WIFI_Utils::checkWiFi();
            if (Config.aprs_is.active && (WiFi.status() == WL_CONNECTED) && !espClient.connected()) APRS_IS_Utils::connect();
        #endif

        NTP_Utils::update();
        TNC_Utils::loop();

        Utils::checkDisplayInterval();
        Utils::checkBeaconInterval();
        
        APRS_IS_Utils::checkStatus(); // Need that to update display, maybe split this and send APRSIS status to display func?

        String packet = "";
        if (Config.loramodule.rxActive) {
            packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
        }

        if (packet != "") {
            if (Config.aprs_is.active) { // If APRSIS enabled
                APRS_IS_Utils::processLoRaPacket(packet); // Send received packet to APRSIS
            }

            if (Config.loramodule.txActive && (Config.digi.mode == 2 || Config.digi.mode == 3 || backUpDigiMode)) { // If Digi enabled
                STATION_Utils::clean25SegBuffer();
                DIGI_Utils::processLoRaPacket(packet); // Send received packet to Digi
            }

            if (Config.tnc.enableServer) { // If TNC server enabled
                TNC_Utils::sendToClients(packet); // Send received packet to TNC KISS
            }
            if (Config.tnc.enableSerial) { // If Serial KISS enabled
                TNC_Utils::sendToSerial(packet); // Send received packet to Serial KISS
            }
        }

        if (Config.aprs_is.active) {
            APRS_IS_Utils::processAPRSISPacket();
            //APRS_IS_Utils::listenAPRSIS(); // listen received packet from APRSIS
        }

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