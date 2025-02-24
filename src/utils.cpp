#include <TinyGPS++.h>
#include <WiFi.h>
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
extern WiFiClient           espClient;
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


namespace Utils {

    void processStatus() {
        String status = Config.callsign;
        status.concat(">APLRG1");
        if (Config.beacon.path.indexOf("WIDE") == 0) {
            status.concat(",");
            status.concat(Config.beacon.path);
        }
        if (WiFi.status() == WL_CONNECTED && Config.aprs_is.active && Config.beacon.sendViaAPRSIS) {
            delay(1000);
            status.concat(",qAC:>https://github.com/richonguzman/LoRa_APRS_iGate ");
            status.concat(versionDate);
            APRS_IS_Utils::upload(status);
            SYSLOG_Utils::log(2, status, 0, 0.0, 0);   // APRSIS TX
            statusAfterBoot = false;
        }
        if (statusAfterBoot && !Config.beacon.sendViaAPRSIS && Config.beacon.sendViaRF) {
            status.concat(":>https://github.com/richonguzman/LoRa_APRS_iGate ");
            status.concat(versionDate);
            STATION_Utils::addToOutputPacketBuffer(status);
            statusAfterBoot = false;
        }
    }

    String getLocalIP() {
        if (Config.digi.ecoMode) {
            return "** WiFi AP  Killed **";
        } else if (!WiFiConnected) {
            return "IP :  192.168.4.1";
        } else if (backUpDigiMode) {
            return "- BACKUP DIGI MODE -";
        } else {
            return "IP :  " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
        }
    }

    void setupDisplay() {
        displaySetup();
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,HIGH);
        #endif
        Serial.println("\nStarting Station: " + Config.callsign + "   Version: " + versionDate);
        Serial.println((Config.digi.ecoMode) ? "(DigiEcoMode: ON)" : "(DigiEcoMode: OFF)");
        displayShow(" LoRa APRS", "", "", "   ( iGATE & DIGI )", "", "" , "  CA2RXU  " + versionDate, 4000);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        firstLine   = Config.callsign;
        seventhLine = "     listening...";
    }

    void activeStations() {
        char buffer[30]; // Adjust size as needed
        sprintf(buffer, "Stations (%dmin) = %2d", Config.rememberStationTime, lastHeardStations.size());
        fourthLine = buffer;
    }

    void sendInitialTelemetryPackets() {
        char sender[10];                                                    // 9 characters + null terminator
        snprintf(sender, sizeof(sender), "%-9s", Config.callsign.c_str());  // Left-align with spaces

        String baseAPRSISTelemetryPacket = Config.callsign;
        baseAPRSISTelemetryPacket += ">APLRG1,TCPIP,qAC::";
        baseAPRSISTelemetryPacket += sender;
        baseAPRSISTelemetryPacket += ":";

        String baseRFTelemetryPacket = Config.callsign;
        baseRFTelemetryPacket += ">APLRG1";
        if (Config.beacon.path.indexOf("WIDE") != -1) {
            baseRFTelemetryPacket += ",";
            baseRFTelemetryPacket += Config.beacon.path;
        }
        baseRFTelemetryPacket += "::";
        baseRFTelemetryPacket += sender;
        baseRFTelemetryPacket += ":";

        String telemetryPacket1 = "EQNS.";
        if (Config.battery.sendInternalVoltage) {
            telemetryPacket1 += "0,0.01,0";
        }
        if (Config.battery.sendExternalVoltage) {
            telemetryPacket1 += String(Config.battery.sendInternalVoltage ? ",0,0.02,0" : "0,0.02,0");
        }

        String telemetryPacket2 = "UNIT.";
        if (Config.battery.sendInternalVoltage) {
            telemetryPacket2 += "VDC";
        }
        if (Config.battery.sendExternalVoltage) {
            telemetryPacket2 += String(Config.battery.sendInternalVoltage ? ",VDC" : "VDC");
        }

        String telemetryPacket3 = "PARM.";
        if (Config.battery.sendInternalVoltage) {
            telemetryPacket3 += "V_Batt";
        }
        if (Config.battery.sendExternalVoltage) {
            telemetryPacket3 += String(Config.battery.sendInternalVoltage ? ",V_Ext" : "V_Ext");
        }

        if (Config.beacon.sendViaAPRSIS) {
            #ifdef HAS_A7670
                A7670_Utils::uploadToAPRSIS(baseAPRSISTelemetryPacket + telemetryPacket1);
                delay(300);
                A7670_Utils::uploadToAPRSIS(baseAPRSISTelemetryPacket + telemetryPacket2);
                delay(300);
                A7670_Utils::uploadToAPRSIS(baseAPRSISTelemetryPacket + telemetryPacket3);
                delay(300);
            #else
                APRS_IS_Utils::upload(baseAPRSISTelemetryPacket + telemetryPacket1);
                delay(300);
                APRS_IS_Utils::upload(baseAPRSISTelemetryPacket + telemetryPacket2);
                delay(300);
                APRS_IS_Utils::upload(baseAPRSISTelemetryPacket + telemetryPacket3);
                delay(300);
            #endif
            delay(300);
        } else if (Config.beacon.sendViaRF) {
            LoRa_Utils::sendNewPacket(baseRFTelemetryPacket + telemetryPacket1);
            delay(3000);
            LoRa_Utils::sendNewPacket(baseRFTelemetryPacket + telemetryPacket2);
            delay(3000);
            LoRa_Utils::sendNewPacket(baseRFTelemetryPacket + telemetryPacket3);
            delay(3000);
        }
        sendStartTelemetry = false;
    }

    void checkBeaconInterval() {
        uint32_t lastTx = millis() - lastBeaconTx;
        if (lastBeaconTx == 0 || lastTx >= Config.beacon.interval * 60 * 1000) {
            beaconUpdate = true;    
        }

        #ifdef HAS_GPS
            if (Config.beacon.gpsActive && gps.location.lat() == 0.0 && gps.location.lng() == 0.0 && Config.beacon.latitude == 0.0 && Config.beacon.longitude == 0.0) {
                GPS_Utils::getData();
                beaconUpdate = false;
            }
        #endif

        if (beaconUpdate) {
            if (!Config.display.alwaysOn && Config.display.timeout != 0) displayToggle(true);

            if (sendStartTelemetry && 
                Config.battery.sendVoltageAsTelemetry &&
                !Config.wxsensor.active && 
                (Config.battery.sendInternalVoltage || Config.battery.sendExternalVoltage) &&
                (lastBeaconTx > 0)) {
                //(!Config.digi.ecoMode || lastBeaconTx > 0)) {
                sendInitialTelemetryPackets();
            }

            STATION_Utils::deleteNotHeard();

            activeStations();

            beaconPacket            = iGateBeaconPacket;
            secondaryBeaconPacket   = iGateLoRaBeaconPacket;
            #ifdef HAS_GPS
                if (Config.beacon.gpsActive && !Config.digi.ecoMode) {
                    GPS_Utils::getData();
                    if (gps.location.isUpdated() && gps.location.lat() != 0.0 && gps.location.lng() != 0.0) {
                        GPS_Utils::generateBeaconFirstPart();
                        String encodedGPS = GPS_Utils::encodeGPS(gps.location.lat(), gps.location.lng(), Config.beacon.overlay, Config.beacon.symbol);
                        beaconPacket = iGateBeaconPacket + encodedGPS;
                        secondaryBeaconPacket = iGateLoRaBeaconPacket + encodedGPS;
                    }
                }
            #endif

            if (Config.wxsensor.active) {
                const char* sensorData = (wxModuleType == 0) ? ".../...g...t..." : WX_Utils::readDataSensor().c_str();
                beaconPacket            += sensorData;
                secondaryBeaconPacket   += sensorData;
            }
            beaconPacket            += Config.beacon.comment;
            secondaryBeaconPacket   += Config.beacon.comment;

            #if defined(BATTERY_PIN) || defined(HAS_AXP192) || defined(HAS_AXP2101)
                if (Config.battery.sendInternalVoltage || Config.battery.monitorInternalVoltage) {
                    float internalVoltage       = BATTERY_Utils::checkInternalVoltage();
                    if (Config.battery.monitorInternalVoltage && internalVoltage < Config.battery.internalSleepVoltage) {
                        beaconPacket            += " **IntBatWarning:SLEEP**";
                        secondaryBeaconPacket   += " **IntBatWarning:SLEEP**";
                        shouldSleepLowVoltage   = true;
                    }

                    if (Config.battery.sendInternalVoltage) {
                        char internalVoltageInfo[10];   // Enough to hold "xx.xxV\0"
                        snprintf(internalVoltageInfo, sizeof(internalVoltageInfo), "%.2fV", internalVoltage);

                        char sixthLineBuffer[25];       // Enough to hold "    (Batt=xx.xxV)"
                        snprintf(sixthLineBuffer, sizeof(sixthLineBuffer), "    (Batt=%s)", internalVoltageInfo);
                        sixthLine = sixthLineBuffer;

                        if (!Config.battery.sendVoltageAsTelemetry) {
                            beaconPacket            += " Batt=";
                            beaconPacket            += internalVoltageInfo;
                            secondaryBeaconPacket   += " Batt=";
                            secondaryBeaconPacket   += internalVoltageInfo;
                        }
                    }
                }
            #endif

            #ifndef HELTEC_WP
                if (Config.battery.sendExternalVoltage || Config.battery.monitorExternalVoltage) {
                    float externalVoltage       = BATTERY_Utils::checkExternalVoltage();
                    if (Config.battery.monitorExternalVoltage && externalVoltage < Config.battery.externalSleepVoltage) {
                        beaconPacket            += " **ExtBatWarning:SLEEP**";
                        secondaryBeaconPacket   += " **ExtBatWarning:SLEEP**";
                        shouldSleepLowVoltage   = true;
                    }

                    if (Config.battery.sendExternalVoltage) {
                        char externalVoltageInfo[10];  // "xx.xxV\0" (max 7 chars)
                        snprintf(externalVoltageInfo, sizeof(externalVoltageInfo), "%.2fV", externalVoltage);
                    
                        char sixthLineBuffer[25];  // Ensure enough space
                        snprintf(sixthLineBuffer, sizeof(sixthLineBuffer), "    (Ext V=%s)", externalVoltageInfo);
                        sixthLine = sixthLineBuffer;

                        if (!Config.battery.sendVoltageAsTelemetry) {
                            beaconPacket            += " Ext=";
                            beaconPacket            += externalVoltageInfo;
                            secondaryBeaconPacket   += " Ext=";
                            secondaryBeaconPacket   += externalVoltageInfo;
                        }
                    }
                }
            #endif

            if (Config.battery.sendVoltageAsTelemetry && !Config.wxsensor.active && (Config.battery.sendInternalVoltage || Config.battery.sendExternalVoltage)){
                String encodedTelemetry = BATTERY_Utils::generateEncodedTelemetry();
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

        if (statusAfterBoot) {
            processStatus();
        }
    }

    void checkDisplayInterval() {
        uint32_t lastDisplayTime = millis() - lastScreenOn;
        if (!Config.display.alwaysOn && lastDisplayTime >= Config.display.timeout * 1000) {
            displayToggle(false);
        }
    }

    void validateFreqs() {
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq && abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) < 125000) {
            Serial.println("Tx Freq less than 125kHz from Rx Freq ---> NOT VALID");
            displayShow("Tx Freq is less than ", "125kHz from Rx Freq", "device will autofix", "and then reboot", 1000);
            Config.loramodule.txFreq = Config.loramodule.rxFreq; // Inform about that but then change the TX QRG to RX QRG and reset the device
            Config.writeFile();
            ESP.restart();
        }
    }

    void typeOfPacket(const String& packet, const uint8_t packetType) {
        String sender = packet.substring(0,packet.indexOf(">"));
        switch (packetType) {
            case 0: // LoRa-APRS
                fifthLine = "LoRa Rx ----> APRS-IS";
                break;
            case 1: // APRS-LoRa
                fifthLine = "APRS-IS ----> LoRa Tx";
                break;
            case 2: // Digipeater
                fifthLine = "LoRa Rx ----> LoRa Tx";
                break;
        }

        int firstColonIndex = packet.indexOf(":");
        char nextChar       = packet[firstColonIndex + 1];

        for (int i = sender.length(); i < 9; i++) {
            sender += " ";
        }
        sixthLine = sender;

        if (nextChar == ':') {
            sixthLine += "> MESSAGE";
        } else if (nextChar == '>') {
            sixthLine += "> NEW STATUS";
        } else if (nextChar == '!' || nextChar == '=' || nextChar == '@') {
            sixthLine += "> GPS BEACON";
            if (!Config.syslog.active) GPS_Utils::getDistanceAndComment(packet);       // to be checked!!!
            seventhLine = "RSSI:";
            seventhLine += String(rssi);
            seventhLine += "dBm";
            seventhLine += (rssi <= -100) ? " " : "  ";
            if (distance.indexOf(".") == 1) seventhLine += " ";
            seventhLine += "D:";
            seventhLine += distance;
            seventhLine += "km";
        } else if (nextChar == '`' || nextChar == '\'') {
            sixthLine += ">  MIC-E";
        } else if (nextChar == ';') {
            sixthLine += ">  OBJECT";
        } else if (packet.indexOf(":T#") >= 10 && packet.indexOf(":=/") == -1) {
            sixthLine += "> TELEMETRY";
        } else {
            sixthLine += "> ??????????";
        }
        if (nextChar != '!' && nextChar != '=' && nextChar != '@') {    // Common assignment for non-GPS cases
            seventhLine = "RSSI:";
            seventhLine += String(rssi);
            seventhLine += "dBm SNR: ";
            seventhLine += String(snr);
            seventhLine += "dBm";
        }
    }

    void print(const String& text) {
        if (!Config.tnc.enableSerial) {
            Serial.print(text);
        }
    }

    void println(const String& text) {
        if (!Config.tnc.enableSerial) {
            Serial.println(text);
        }
    }

    void checkRebootMode() {
        if (Config.rebootMode && Config.rebootModeTime > 0) {
            Serial.println("(Reboot Time Set to " + String(Config.rebootModeTime) + " hours)");
        }
    }

    void checkRebootTime() {
        if (Config.rebootMode && Config.rebootModeTime > 0) {
            if (millis() > Config.rebootModeTime * 60 * 60 * 1000) {
                Serial.println("\n*** Automatic Reboot Time Restart! ****\n");
                ESP.restart();
            }
        }
    }

    void checkSleepByLowBatteryVoltage(uint8_t mode) {
        if (shouldSleepLowVoltage) {
            if (mode == 0) {    // at startup
                delay(3000);
            }
            Serial.println("\n\n*** Sleeping Low Battey Voltage ***\n\n");
            esp_sleep_enable_timer_wakeup(30 * 60 * 1000000); // sleep 30 min
            if (mode == 1) {    // low voltage detected after a while
                displayToggle(false);
            }
            #ifdef VEXT_CTRL
                #ifndef HELTEC_WSL_V3
                    digitalWrite(VEXT_CTRL, LOW);
                #endif
            #endif
            LoRa_Utils::sleepRadio();
            transmitFlag = true;
            delay(100);
            esp_deep_sleep_start();
        }
    }

    bool checkValidCallsign(const String& callsign) {
        if (callsign == "WLNK-1") return true;
        
        String cleanCallsign;
        if (callsign.indexOf("-") > 0) {    // SSID Validation
            cleanCallsign = callsign.substring(0, callsign.indexOf("-"));
            String ssid = callsign.substring(callsign.indexOf("-") + 1);
            if (ssid.indexOf("-") != -1 || ssid.length() > 2) return false;
            for (int i = 0; i < ssid.length(); i++) {
                if (!isAlphaNumeric(ssid[i])) return false;
            }
        } else {
            cleanCallsign = callsign;
        }

        if (cleanCallsign.length() < 4 || cleanCallsign.length() > 6) return false;

        if (cleanCallsign.length() < 6 && isAlpha(cleanCallsign[0]) && isDigit(cleanCallsign[1]) && isAlpha(cleanCallsign[2]) && isAlpha(cleanCallsign[3]) ) {
            cleanCallsign = " " + cleanCallsign;    // A0AA --> _A0AA
        }

        if (!isDigit(cleanCallsign[2]) || !isAlpha(cleanCallsign[3])) {     // __0A__ must be validated
            if (cleanCallsign[0] != 'R' && !isDigit(cleanCallsign[1]) && !isAlpha(cleanCallsign[2])) return false;    // to accepto R0A___
        }

        bool isValid = false;
        if ((isAlphaNumeric(cleanCallsign[0]) || cleanCallsign[0] == ' ') && isAlpha(cleanCallsign[1])) {
            isValid = true;     //  AA0A (+A+A) + _A0AA (+A) + 0A0A (+A+A)
        } else if (isAlpha(cleanCallsign[0]) && isDigit(cleanCallsign[1])) {
            isValid = true;     //  A00A (+A+A)
        } else if (cleanCallsign[0] == 'R' && cleanCallsign.length() == 6 && isDigit(cleanCallsign[1]) && isAlpha(cleanCallsign[2]) && isAlpha(cleanCallsign[3]) && isAlpha(cleanCallsign[4])) {
            isValid = true;     //  R0AA (+A+A)
        }
        if (!isValid) return false;   // also 00__ avoided

        if (cleanCallsign.length() > 4) {   // to validate ____AA
            for (int i = 5; i <= cleanCallsign.length(); i++) {
                if (!isAlpha(cleanCallsign[i - 1])) return false;
            }
        }
        return true;
    }

}