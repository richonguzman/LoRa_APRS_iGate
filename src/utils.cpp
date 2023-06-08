#include <WiFi.h>
#include "configuration.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"

extern WiFiClient       espClient;
extern Configuration    Config;
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
        status += ",qAC:>" + Config.defaultStatus;
        espClient.write((status + "\n").c_str()); 
    } else {
        delay(5000);
        status += ":>" + Config.defaultStatus;
        if (stationMode == 4) {     // Digirepeating with Freq Rx !=  Tx
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
    firstLine   = "LoRa iGate: " + Config.callsign;
    if (stationMode==3 || stationMode==4) {
        secondLine = "<DigiRepeater Active>";
    } else {
        secondLine  = "";
    }    
    thirdLine   = "  LoRa Module Ready";
    fourthLine  = "     listening...";
}

void checkBeaconInterval() {
    uint32_t lastTx = millis() - lastBeaconTx;
    if (lastTx >= Config.beaconInterval*60*1000) {
        beacon_update = true;    
    }
    if (beacon_update) {
        display_toggle(true);
        thirdLine = "";
        Serial.println("---- Sending iGate Beacon ----");
        if (stationMode==1 || stationMode==2) {
            show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 1000);
            fourthLine = "     listening...";
            espClient.write((iGateBeaconPacket + "\n").c_str());
            show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
        } else if (stationMode==3 || stationMode==4) {
            show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 0);
            fourthLine = "     listening...";
            if (stationMode == 4) {     // Digirepeating with Freq Rx !=  Tx
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

}