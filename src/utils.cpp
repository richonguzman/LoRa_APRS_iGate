#include <WiFi.h>
#include <Arduino.h>
#include "configuration.h"
#include "wifi_utils.h"
#include "utils.h"

extern WiFiClient espClient;
extern Configuration Config;
extern bool statusAfterBoot;

namespace utils {

void processStatus() {
    delay(1000);
    espClient.write((Config.callsign + ">APLRG1,qAC:>" + Config.defaultStatus + "\n").c_str()); 
    statusAfterBoot = false;
}

}