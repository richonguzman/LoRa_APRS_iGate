#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "configuration.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"


AsyncWebServer  server(80);

extern WiFiClient       espClient;
extern Configuration    Config;
extern String           versionDate;
extern bool             statusAfterBoot;
extern String           firstLine;
extern String           secondLine;
extern String           thirdLine;
extern String           fourthLine;
extern uint32_t         lastBeaconTx;
extern uint32_t         lastScreenOn;
extern bool             beacon_update;
extern int              stationMode;
extern String           iGateBeaconPacket;

namespace utils {

void processStatus() {
    String status = Config.callsign + ">APLRG1";
    if (stationMode==1 || stationMode==2) {
        delay(1000);
        status += ",qAC:>https://github.com/richonguzman/LoRa_APRS_iGate";
        espClient.write((status + "\n").c_str()); 
    } else {
        delay(5000);
        status += ":>https://github.com/richonguzman/LoRa_APRS_iGate";
        if (stationMode == 4) {
            LoRa_Utils::changeFreqTx();
        }
        LoRa_Utils::sendNewPacket("APRS", status);
        if (stationMode == 4) {
            LoRa_Utils::changeFreqRx();
        }        
    }
    statusAfterBoot = false;
}

void setupDiplay() {
    setup_display();
    digitalWrite(greenLed,HIGH);
    Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + versionDate);
    show_display("   LoRa APRS iGate", "    Richonguzman", "    -- CD2RXU --", "     " + versionDate, 4000);
    digitalWrite(greenLed,LOW);
    firstLine   = "LoRa iGate: " + Config.callsign;
    if (stationMode==3 || stationMode==4) {
        secondLine = "<DigiRepeater Active>";
    } else {
        secondLine  = "";
    }    
    thirdLine   = "  LoRa Module Ready";
    fourthLine  = "     listening...";
}

String getLocalIP() {
    return "IP : " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
}

void checkBeaconInterval() {
    uint32_t lastTx = millis() - lastBeaconTx;
    if (lastTx >= Config.beaconInterval*60*1000) {
        beacon_update = true;    
    }
    if (beacon_update) {
        display_toggle(true);
        thirdLine = getLocalIP();
        Serial.println("---- Sending iGate Beacon ----");
        if (stationMode==1 || stationMode==2) {
            show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 1000);
            fourthLine = "     listening...";
            espClient.write((iGateBeaconPacket + "\n").c_str());
            show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
        } else if (stationMode==3 || stationMode==4) {
            show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 0);
            fourthLine = "     listening...";
            if (stationMode == 4) {
                LoRa_Utils::changeFreqTx();
            }
            LoRa_Utils::sendNewPacket("APRS",iGateBeaconPacket);
            if (stationMode == 4) {
                LoRa_Utils::changeFreqRx();
            }           
        }
        lastBeaconTx = millis();
        lastScreenOn = millis();
        beacon_update = false;
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

void validateDigiFreqs() {
    if (stationMode == 4) {
        if (abs(Config.loramodule.digirepeaterTxFreq - Config.loramodule.digirepeaterRxFreq) < 125000) {
            Serial.println("Tx Freq less than 125kHz from Rx Freq ---> NOT VALID, check 'data/igate_conf.json'");
            show_display("Tx Freq is less than ", "125kHz from Rx Freq", "change it on : /data/", "igate_conf.json", 0);
            while (1);
        }
    }
}

void typeOfPacket(String packet) {
    if (stationMode==1 || stationMode==2) {
        thirdLine = "Callsign = " + packet.substring(0,packet.indexOf(">"));
    } else {
        thirdLine = "Callsign = " + packet.substring(3,packet.indexOf(">"));
    }
    if (packet.indexOf("::") >= 10) {
        fourthLine = "TYPE ----> MESSAGE";
    } else if (packet.indexOf(":>") >= 10) {
        fourthLine = "TYPE ----> NEW STATUS";
    } else if (packet.indexOf(":!") >= 10 || packet.indexOf(":=") >= 10) {
        fourthLine = "TYPE ----> GPS BEACON";
    } else {
        fourthLine = "TYPE ----> ??????????";
    }
}

void startOTAServer() {
    if (stationMode==1 || stationMode==2) {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi This is your Richonguzman / CD2RXU LoRa iGate.\nIf you want tu update your firmware please go to:  {iGate-IP-Adress}/update");
        });
        AsyncElegantOTA.begin(&server);
        server.begin();
        Serial.println("HTTP server started (OTA Firmware Updates)!\n");
    }
}

}