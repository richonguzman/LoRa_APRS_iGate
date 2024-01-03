/*#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>*/
#include <SPIFFS.h>
#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "syslog_utils.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "bme_utils.h"
#include "display.h"
#include "utils.h"

//AsyncWebServer  server(80);

extern WiFiClient           espClient;
extern Configuration        Config;
extern String               versionDate;
extern bool                 statusAfterBoot;
extern String               firstLine;
extern String               secondLine;
extern String               thirdLine;
extern String               fourthLine;
extern String               fifthLine;
extern String               sixthLine;
extern String               seventhLine;
extern uint32_t             lastBeaconTx;
extern uint32_t             lastScreenOn;
extern bool                 beaconUpdate;
extern int                  stationMode;
extern String               iGateBeaconPacket;
extern std::vector<String>  lastHeardStation;
extern int                  rssi;
extern float                snr;
extern int                  freqError;
extern String               distance;
extern String               versionDate;
extern uint32_t             lastWiFiCheck;
extern bool                 WiFiConnect;

String name;
String email;

unsigned long ota_progress_millis = 0;

namespace Utils {

    /*void notFound(AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    }*/

    void processStatus() {
        String status = Config.callsign + ">APLRG1,WIDE1-1";
        if (stationMode==1 || stationMode==2 || (stationMode==5 && WiFi.status() == WL_CONNECTED)) {
            delay(1000);
            status += ",qAC:>https://github.com/richonguzman/LoRa_APRS_iGate " + versionDate;
            espClient.write((status + "\n").c_str());
            SYSLOG_Utils::log("APRSIS Tx", status,0,0,0);
        } else {
            delay(5000);
            status += ":>https://github.com/richonguzman/LoRa_APRS_iGate " + versionDate;
            if (stationMode==4) {
                LoRa_Utils::changeFreqTx();
            }
            LoRa_Utils::sendNewPacket("APRS", status);
            if (stationMode==4) {
                LoRa_Utils::changeFreqRx();
            }        
        }
        statusAfterBoot = false;
    }

    String getLocalIP() {
        return "IP :  " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
    }

    void setupDisplay() {
        setup_display();
        digitalWrite(internalLedPin,HIGH);
        Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + versionDate);
        show_display(" LoRa APRS", "", "      ( iGATE )", "", "", "Richonguzman / CA2RXU", "      " + versionDate, 4000);
        digitalWrite(internalLedPin,LOW);
        firstLine   = Config.callsign;
        seventhLine = "     listening...";
    }

    void activeStations() {
        fourthLine = "Stations (" + String(Config.rememberStationTime) + "min) = ";
        if (lastHeardStation.size() < 10) {
            fourthLine += " ";
        }
        fourthLine += String(lastHeardStation.size());            
    }

    void checkBeaconInterval() {
        uint32_t lastTx = millis() - lastBeaconTx;
        String beaconPacket;
        if (lastTx >= Config.beaconInterval*60*1000) {
            beaconUpdate = true;    
        }
        if (beaconUpdate) {
            display_toggle(true);
            Serial.println("---- Sending iGate Beacon ----");
            STATION_Utils::deleteNotHeard();
            activeStations();
            if (Config.bme.active) {
                beaconPacket = iGateBeaconPacket.substring(0,iGateBeaconPacket.indexOf(":=")+20) + "_" + BME_Utils::readDataSensor() + iGateBeaconPacket.substring(iGateBeaconPacket.indexOf(":=")+21) + " + WX";
            } else {
                beaconPacket = iGateBeaconPacket;
            }
            #ifndef HELTEC_V3
            if (Config.sendBatteryVoltage) {
                beaconPacket += " (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
            }
            #endif
            if (Config.externalVoltageMeasurement) { 
                beaconPacket += " (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
            }
            if (stationMode==1 || stationMode==2) {
                thirdLine = getLocalIP();
                if (!Config.bme.active) {
                    fifthLine = "";
                }
                sixthLine = "";
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING iGate BEACON", 1000);         
                #ifndef HELTEC_V3
                if (Config.sendBatteryVoltage) { 
                    sixthLine = "     (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
                }
                #endif
                if (Config.externalVoltageMeasurement) { 
                    sixthLine = "    (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
                }
                seventhLine = "     listening...";
                espClient.write((beaconPacket + "\n").c_str());
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
            } else if (stationMode==3 || stationMode==4) {
                String Rx = String(Config.loramodule.digirepeaterRxFreq);
                String Tx = String(Config.loramodule.digirepeaterTxFreq);
                if (stationMode==3) {
                    secondLine = "Rx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
                } else {
                    secondLine = "Rx:" + String(Rx.substring(0,3)) + "." + String(Rx.substring(3,6));
                }
                secondLine += " Tx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
                thirdLine = "<<   DigiRepeater  >>";
                fifthLine = "";
                sixthLine = "";
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING iGate BEACON", 0);
                #ifndef HELTEC_V3
                if (Config.sendBatteryVoltage) { 
                    sixthLine = "     (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
                }
                #endif
                if (Config.externalVoltageMeasurement) { 
                    sixthLine = "    (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
                }
                seventhLine = "     listening...";
                if (stationMode==4) {
                    LoRa_Utils::changeFreqTx();
                }
                LoRa_Utils::sendNewPacket("APRS", beaconPacket);
                if (stationMode==4) {
                    LoRa_Utils::changeFreqRx();
                }
            } else if (stationMode==5) {
                if (!Config.bme.active) {
                    fifthLine = "";
                }
                sixthLine = "";
                if (WiFi.status() == WL_CONNECTED && espClient.connected()) {
                    APRS_IS_Utils::checkStatus();
                    thirdLine = getLocalIP();
                    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING iGate BEACON", 1000);         
                    #ifndef HELTEC_V3
                    if (Config.sendBatteryVoltage) { 
                        sixthLine = "     (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
                    }
                    #endif
                    if (Config.externalVoltageMeasurement) { 
                        sixthLine = "    (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
                    }
                    seventhLine = "     listening...";
                    espClient.write((beaconPacket + "\n").c_str());
                    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
                } else {
                    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING iGate BEACON", 0);
                    #ifndef HELTEC_V3
                    if (Config.sendBatteryVoltage) { 
                        sixthLine = "     (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
                    }
                    #endif
                    if (Config.externalVoltageMeasurement) { 
                        sixthLine = "    (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
                    }
                    seventhLine = "     listening...";
                    LoRa_Utils::sendNewPacket("APRS", beaconPacket);
                }
            } else if (stationMode==6) {
                /* si hay wifi 
                secondLine muestra wifistatus
                else
                secondLine = freq digi*/
                thirdLine = "<<   Digi + iGate  >>";
                fifthLine = "";
                sixthLine = "";
                
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, "SENDING iGate BEACON", 0);
                #ifndef HELTEC_V3
                if (Config.sendBatteryVoltage) { 
                    sixthLine = "     (Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V)";
                }
                #endif
                if (Config.externalVoltageMeasurement) { 
                    sixthLine = "    (Ext V=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V)";
                }
                seventhLine = "     listening...";

                if (stationMode==6 && ((WiFi.status()==WL_CONNECTED) && espClient.connected())) {
                    espClient.write((beaconPacket + "\n").c_str());
                    Serial.println("---> Uploaded to APRS-IS");
                }
                LoRa_Utils::sendNewPacket("APRS", beaconPacket);
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
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
        if (!Config.display.alwaysOn) {
            if (lastDisplayTime >= Config.display.timeout*1000) {
                display_toggle(false);
            }
        }
    }

    void checkWiFiInterval() {
        uint32_t WiFiCheck = millis() - lastWiFiCheck;
        if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15*60*1000) {
        WiFiConnect = true;
        }
        if (WiFiConnect) {
        Serial.println("\nConnecting to WiFi ...");
        WIFI_Utils::startWiFi();
        lastWiFiCheck = millis();
        WiFiConnect = false;
        }
    }

    void validateDigiFreqs() {
        if (stationMode==4) {
            if (abs(Config.loramodule.digirepeaterTxFreq - Config.loramodule.digirepeaterRxFreq) < 125000) {
                Serial.println("Tx Freq less than 125kHz from Rx Freq ---> NOT VALID, check 'data/igate_conf.json'");
                show_display("Tx Freq is less than ", "125kHz from Rx Freq", "change it on : /data/", "igate_conf.json", 0);
                while (1);
            }
        }
    }

    void typeOfPacket(String packet, String packetType) {
        String sender;
        if (stationMode==1 || stationMode==2 || (stationMode==5 && WiFi.status()==WL_CONNECTED)) {
            if (packetType == "LoRa-APRS") {
                fifthLine = "LoRa Rx ----> APRS-IS";
            } else if (packetType == "APRS-LoRa") {
                fifthLine = "APRS-IS ----> LoRa Tx";
            }
            sender = packet.substring(0,packet.indexOf(">"));
        } else {
            fifthLine = "LoRa Rx ----> LoRa Tx";
            sender = packet.substring(3,packet.indexOf(">"));
        }
        for (int i=sender.length();i<9;i++) {
            sender += " ";
        }
        if (packet.indexOf("::") >= 10) {
            if (packetType == "APRS-LoRa") {
                String addresseeAndMessage = packet.substring(packet.indexOf("::")+2);
                String addressee = addresseeAndMessage.substring(0, addresseeAndMessage.indexOf(":"));
                addressee.trim();
                sixthLine = sender + " > " + addressee;
            } else {
                sixthLine = sender + "> MESSAGE";
            }
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        } else if (packet.indexOf(":>") >= 10) {
            sixthLine = sender + "> NEW STATUS";
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        } else if (packet.indexOf(":!") >= 10 || packet.indexOf(":=") >= 10) {
            sixthLine = sender + "> GPS BEACON";
            GPS_Utils::getDistance(packet);
            seventhLine = "RSSI:" + String(rssi) + "dBm";
            if (rssi <= -100) {
                seventhLine += " ";
            } else {
                seventhLine += "  ";
            }
            if (distance.indexOf(".") == 1) {
                seventhLine += " ";
            }
            seventhLine += "D:" + distance + "km";
        } else if (packet.indexOf(":T#") >= 10 && packet.indexOf(":=/") == -1) {
            sixthLine = sender + "> TELEMETRY";
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        } else if (packet.indexOf(":`") >= 10) {
            sixthLine = sender + ">  MIC-E";
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        } else if (packet.indexOf(":;") >= 10) {
            sixthLine = sender + ">  OBJECT";
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        } else {
            sixthLine = sender + "> ??????????";
            seventhLine = "RSSI:" + String(rssi) + "dBm SNR: " + String(snr) + "dBm";
        }
    }

    /*void onOTAStart() {
        Serial.println("OTA update started!");
        display_toggle(true);
        lastScreenOn = millis();
        show_display("", "", "", " OTA update started!", "", "", "", 1000);
    }

    void onOTAProgress(size_t current, size_t final) {
        if (millis() - ota_progress_millis > 1000) {
            display_toggle(true);
            lastScreenOn = millis();
            ota_progress_millis = millis();
            Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
            show_display("", "", "  OTA Progress : " + String((current*100)/final) + "%", "", "", "", "", 100);
        }
    }

    void onOTAEnd(bool success) {
        display_toggle(true);
        lastScreenOn = millis();
        if (success) {
            Serial.println("OTA update finished successfully!");
            show_display("", "", " OTA update success!", "", "    Rebooting ...", "", "", 4000);
        } else {
            Serial.println("There was an error during OTA update!");
            show_display("", "", " OTA update fail!", "", "", "", "", 4000);
        }
    }
    */
    /*void startServer() {
        if (stationMode==1 || stationMode==2 || (stationMode==5 && WiFi.status()==WL_CONNECTED)) {
            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(200, "text/plain", "Hi " + Config.callsign + ", \n\nthis is your (Richonguzman/CA2RXU) LoRa APRS iGate , version " + versionDate + "\n\nTo update your firmware or filesystem go to: http://" + getLocalIP().substring(getLocalIP().indexOf(":")+3) + "/update\n\n\n73!");
            });

            server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(SPIFFS, "/test_info_1.html", "text/html");//"application/json");
            });

            server.on("/test2", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(SPIFFS, "/test1.html", "text/html");
            });

            if (Config.ota.username != ""  && Config.ota.password != "") {
                ElegantOTA.begin(&server, Config.ota.username.c_str(), Config.ota.password.c_str());
            } else {
                ElegantOTA.begin(&server);
            }
            ElegantOTA.setAutoReboot(true);
            ElegantOTA.onStart(onOTAStart);
            ElegantOTA.onProgress(onOTAProgress);
            ElegantOTA.onEnd(onOTAEnd);

            server.on("/process_form.php", HTTP_POST, [](AsyncWebServerRequest *request){
                String message;

                if (request->hasParam("email", true) && request->hasParam("name", true)) {
                    email = request->getParam("email", true)->value();
                    name = request->getParam("name", true)->value();
                    
                    String responseMessage = "Received EMAIL: " + email + ", NAME: " + name;

                    // Assuming you're sending an HTTP response, for example, in an HTTP server context
                    request->send(200, "text/plain", responseMessage);
                } else {
                    // Handle the case where one or both parameters are missing
                    request->send(400, "text/plain", "Both EMAIL and NAME parameters are required.");
                }
            });

            server.onNotFound(notFound);

            server.serveStatic("/", SPIFFS, "/");

            server.begin();
            Serial.println("init : OTA Server     ...     done!");            
        }
    }*/

}