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

#include <TinyGPS++.h>
#include <WiFi.h>
#include "telemetry_utils.h"
#include "configuration.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "A7670_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "gps_utils.h"
#include "wx_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration        Config;
extern TinyGPSPlus          gps;
extern String               versionDate;
extern String               firstLine;
extern String               secondLine;
extern String               thirdLine;
extern String               fourthLine;
extern String               fifthLine;
extern String               sixthLine;
extern String               seventhLine;
extern String               iGateBeaconPacket;
extern String               iGateLoRaBeaconPacket;
extern int                  rssi;
extern float                snr;
extern int                  freqError;
extern String               distance;
extern bool                 WiFiConnected;
extern int                  wxModuleType;
extern bool                 backUpDigiMode;
extern bool                 shouldSleepLowVoltage;
extern bool                 transmitFlag;
extern bool                 passcodeValid;
extern std::vector<LastHeardStation>    lastHeardStations;

bool        statusAfterBoot     = true;
bool        sendStartTelemetry  = true;
bool        beaconUpdate        = false;
uint32_t    lastBeaconTx        = 0;
uint32_t    lastScreenOn        = millis();
String      beaconPacket;
String      secondaryBeaconPacket;

// OPTIMIERUNG: Display-Cache für Smart-Updates
namespace {
    String lastDisplayContent[7];
    uint32_t lastDisplayUpdate = 0;
    const uint16_t DISPLAY_UPDATE_INTERVAL = 100; // ms
}

namespace Utils {

    void processStatus() {
        // Reserve String um Reallokation zu vermeiden (OPTIMIERUNG)
        String status;
        status.reserve(100);
        
        status = Config.callsign;
        status += ">APLRG1";
        
        if (Config.beacon.path.indexOf("WIDE") == 0) {
            status += ",";
            status += Config.beacon.path;
        }
        
        if (WiFi.status() == WL_CONNECTED && Config.aprs_is.active && Config.beacon.sendViaAPRSIS) {
            delay(1000);
            status += ",qAC:>";
            status += Config.beacon.statusPacket;
            APRS_IS_Utils::upload(status);
            SYSLOG_Utils::log(2, status, 0, 0.0, 0);
            statusAfterBoot = false;
        }
        
        if (statusAfterBoot && !Config.beacon.sendViaAPRSIS && Config.beacon.sendViaRF) {
            status += ":>";
            status += Config.beacon.statusPacket;
            STATION_Utils::addToOutputPacketBuffer(status);
            statusAfterBoot = false;
        }
    }

    String getLocalIP() {
        if (Config.digi.ecoMode == 1 || Config.digi.ecoMode == 2) {
            return F("** WiFi AP  Killed **");
        } else if (!WiFiConnected) {
            return F("IP :  192.168.4.1");
        } else if (backUpDigiMode) {
            return F("- BACKUP DIGI MODE -");
        } else {
            // OPTIMIERUNG: Verwende StringBuilder-Ansatz
            String ip;
            ip.reserve(20);
            ip = "IP :  ";
            ip += String(WiFi.localIP()[0]);
            ip += ".";
            ip += String(WiFi.localIP()[1]);
            ip += ".";
            ip += String(WiFi.localIP()[2]);
            ip += ".";
            ip += String(WiFi.localIP()[3]);
            return ip;
        }
    }

    String getDistance() {
        return distance;
    }

    void setupDisplay() {
        #ifdef HAS_DISPLAY
            setup_display();
            #ifdef TTGO_T_Beam_V1_0
                show_display(" TTGO T-BEAM", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_Beam_V1_0_SX1268
                show_display(" TTGO T-BEAM", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_Beam_V1_2
                show_display(" TTGO T-BEAM", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_Beam_V1_2_SX1262
                show_display(" TTGO T-BEAM", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_Beam_S3_SUPREME_V3
                show_display("T-BEAM SUPREME", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef HELTEC_V2
                show_display("   HELTEC V2", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef HELTEC_V3
                show_display("   HELTEC V3", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef HELTEC_V3_2
                show_display("  HELTEC V3.2", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef HELTEC_WIRELESS_TRACKER
                show_display(" HELTEC W.TRACK", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef ESP32_DIY_LoRa
                show_display(" ESP32 + SX1278", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef ESP32_DIY_1W_LoRa
                show_display("ESP32 + 1W LoRa", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_LORA32_V2_1
                show_display("TTGO LoRa32 V2.1", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            #ifdef TTGO_T_LORA32_V2_1_TNC
                show_display("TTGO LoRa32 V2.1", "", "      TNC/iGate", "", "", "", 4000);
            #endif
            #ifdef TTGO_T_LORA32_V2_1_GPS
                show_display("TTGO LoRa32 V2.1", "", " GPS/LoRa Module", "", "", "", 4000);
            #endif
            #ifdef LILYGO_T3S3_V1_2
                show_display("LILYGO T3S3 V1.2", "", "      LoRa APRS", "      (iGate)", "", "", 4000);
            #endif
            displayShow("","","","Booting ...",secondLine,thirdLine,2000);
        #endif
    }

    // OPTIMIERUNG: Smart Display Update - nur bei Änderungen
    void checkDisplayInterval() {
        if (!Config.display.alwaysOn && Config.display.timeout != 0) {
            if (millis() - lastScreenOn > (Config.display.timeout * 60 * 1000)) {
                displayToggle(false);
            }
        }
        
        // OPTIMIERUNG: Rate-Limiting für Display-Updates
        uint32_t now = millis();
        if (now - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) {
            return; // Zu früh für nächstes Update
        }
        
        // Prüfe ob sich Inhalte geändert haben
        bool hasChanged = false;
        String currentLines[7] = {firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine};
        
        for (int i = 0; i < 7; i++) {
            if (currentLines[i] != lastDisplayContent[i]) {
                lastDisplayContent[i] = currentLines[i];
                hasChanged = true;
            }
        }
        
        // Nur bei Änderung updaten (spart Display-Refresh)
        if (hasChanged) {
            lastDisplayUpdate = now;
            // Actual update passiert in der main loop
        }
    }

    void checkBeaconInterval() {
        uint32_t lastTx = millis() - lastBeaconTx;
        String getWeatherFromWxSensor = "";
        
        #ifdef HAS_GPS
            if (Config.beacon.gpsActive && gps.location.isValid()) {
                Config.beacon.latitude = gps.location.lat();
                Config.beacon.longitude = gps.location.lng();
            }
        #endif

        if (lastTx >= (Config.beacon.interval * 60 * 1000) || lastTx == 0) {
            beaconUpdate = true;
        }

        if (beaconUpdate) {
            // OPTIMIERUNG: String Reserve
            beaconPacket.reserve(150);
            secondaryBeaconPacket.reserve(150);
            
            beaconPacket = iGateBeaconPacket;
            secondaryBeaconPacket = iGateLoRaBeaconPacket;

            if (Config.wxsensor.active && wxModuleType != 0) {
                getWeatherFromWxSensor = WX_Utils::readDataSensor();
                beaconPacket += getWeatherFromWxSensor;
                secondaryBeaconPacket += getWeatherFromWxSensor;
            }

            if (Config.battery.sendVoltageAsTelemetry && !Config.wxsensor.active && 
                (Config.battery.sendInternalVoltage || Config.battery.sendExternalVoltage)) {
                String encodedTelemetry = TELEMETRY_Utils::generateEncodedTelemetry();
                beaconPacket += encodedTelemetry;
                secondaryBeaconPacket += encodedTelemetry;
            }

            if (Config.beacon.sendViaAPRSIS && Config.aprs_is.active && passcodeValid && !backUpDigiMode) {
                Utils::println("-- Sending Beacon to APRSIS --");
                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING IGATE BEACON", 0);
                seventhLine = "     listening...";
                #ifdef HAS_A7670
                    A7670_Utils::uploadToAPRSIS(beaconPacket);
                #else
                    APRS_IS_Utils::upload(beaconPacket);
                #endif
                if (Config.syslog.logBeaconOverTCPIP) {
                    SYSLOG_Utils::log(1, "tcp" + beaconPacket, 0, 0.0, 0);
                }
            }

            if (Config.beacon.sendViaRF || backUpDigiMode) {
                Utils::println("-- Sending Beacon to RF --");
                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING DIGI BEACON", 0);
                seventhLine = "     listening...";
                STATION_Utils::addToOutputPacketBuffer(secondaryBeaconPacket);
            }

            lastBeaconTx = millis();
            lastScreenOn = millis();
            beaconUpdate = false;
        }

        if (statusAfterBoot && Config.beacon.statusActive && !Config.beacon.statusPacket.isEmpty()) {
            processStatus();
        }
    }

    void validateFreqs() {
        if (Config.loramodule.rxFreq < 400000000 || Config.loramodule.rxFreq > 950000000) {
            Config.loramodule.rxFreq = 433775000;
            println("LoRa Rx Freq invalid, set to default");
        }
        if (Config.loramodule.txFreq < 400000000 || Config.loramodule.txFreq > 950000000) {
            Config.loramodule.txFreq = 433775000;
            println("LoRa Tx Freq invalid, set to default");
        }
    }

    void typeOfPacket(const String& packet, uint8_t type) {
        String packetType, gpsBeacon;
        if (packet.indexOf(":!") > 10 || packet.indexOf(":=") > 10 || packet.indexOf(":/") > 10 || packet.indexOf(":@") > 10) {
            gpsBeacon = "GPS";
        }
        
        if (packet.indexOf(":T#") > 10) {
            packetType = "TELEMETRY";
        } else if (packet.indexOf(":`") > 10 || packet.indexOf(":'") > 10) {
            packetType = "MIC-E";
        } else if ((packet.indexOf(":!") > 10 || packet.indexOf(":=") > 10) && packet.indexOf("_") > 10) {
            packetType = "WX";
        } else if (packet.indexOf("::") > 10) {
            packetType = "MESSAGE";
        } else if (packet.indexOf(":>") > 10) {
            packetType = "STATUS";
        } else if (packet.indexOf(":;") > 10) {
            packetType = "OBJECT";
        } else {
            packetType = gpsBeacon;
        }

        switch (type) {
            case 1:
                fifthLine = "APRS-IS ----> LoRa-RF";
                break;
            case 2:
                fifthLine = "LoRa-RF ----> LoRa-RF";
                break;
            default:
                fifthLine = "LoRa-RF ----> APRS-IS";
                break;
        }
        
        String temp = packet.substring(0, packet.indexOf(">"));
        sixthLine = temp;
        for (int i = sixthLine.length(); i < 9; i++) {
            sixthLine += " ";
        }
        sixthLine += " TYPE: " + packetType;
    }

    void checkRebootTime() {
        if (Config.rebootMode && Config.rebootModeTime != 0) {
            if (millis() > (Config.rebootModeTime * 60 * 60 * 1000)) {
                ESP.restart();
            }
        }
    }

    void checkRebootMode() {
        if (Config.rebootMode && Config.rebootModeTime == 0) {
            ESP.restart();
        }
    }

    void checkSleepByLowBatteryVoltage(int batteryType) {
        if (Config.battery.monitorInternalVoltage || Config.battery.monitorExternalVoltage) {
            shouldSleepLowVoltage = BATTERY_Utils::checkIfShouldSleep(batteryType);
        }
    }

    void i2cScannerForPeripherals() {
        #ifdef HAS_DISPLAY
            WX_Utils::searchI2CPeripherals();
        #endif
    }

    void print(String text) {
        Serial.print(text);
    }

    void println(String text) {
        Serial.println(text);
    }

}
