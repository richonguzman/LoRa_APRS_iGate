#include <WiFi.h>
#include <Arduino.h>
//#include <igate_config.h>
#include "configuration.h"
#include "utils.h"

extern WiFiClient espClient;
extern Configuration Config;
extern bool statusAfterBoot;

namespace utils {

void processStatus() {
    delay(1000);
    String startupStatus = Config.callsign + ">APLR10,qAC:>" + Config.defaultStatus;
    espClient.write((startupStatus + "\n").c_str()); 
    statusAfterBoot = false;
}

}