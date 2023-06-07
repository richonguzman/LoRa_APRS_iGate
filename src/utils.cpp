#include <WiFi.h>
#include <Arduino.h>
#include "configuration.h"
#include "wifi_utils.h"
#include "utils.h"

extern WiFiClient espClient;
extern Configuration Config;
extern bool statusAfterBoot;

namespace utils {

void validateMode(int mode) {
    if (mode == 1 || mode == 2 || mode == 5) {
        if (mode==1) {
            Serial.println("stationMode ---> iGate (only Rx)");
        } else {
            Serial.println("stationMode ---> iGate (Rx + Tx)");
        }
        WIFI_Utils::setup();
        btStop();
    } else if (mode == 3) {
        Serial.println("stationMode ---> DigiRepeater Rx freq = Tx freq");
    } else if (mode == 4) {
        Serial.println("stationMode ---> DigiRepeater Rx freq = Tx freq");
    } else { 
        Serial.println("stationMode ---> NOT VALID, check 'data/igate_conf.json'");
        while (1);
    }
}


void processStatus() {
    delay(1000);
    String startupStatus = Config.callsign + ">APLRG1,qAC:>" + Config.defaultStatus;
    espClient.write((startupStatus + "\n").c_str()); 
    statusAfterBoot = false;
}

}