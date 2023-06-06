#include "station_utils.h"
/*#include <WiFi.h>
#include "configuration.h"
#include "display.h"

extern Configuration  Config;
extern WiFiClient     espClient;
extern int            internalLedPin;
extern uint32_t       lastRxTxTime;*/

namespace APRS_IS_Utils {


void deleteNotHeardStation() {
  uint32_t minReportingTime = 30*60*1000; // 30 minutes // from .json and CONFIGURATION?????
  for (int i=0; i<lastHeardStation.size(); i++) {
    String deltaTimeString = lastHeardStation[i].substring(lastHeardStation[i].indexOf(",")+1);
    uint32_t deltaTime = deltaTimeString.toInt();
    if ((millis() - deltaTime) < minReportingTime) {
      lastHeardStation_temp.push_back(lastHeardStation[i]);
    }
  }
  lastHeardStation.clear();
  for (int j=0; j<lastHeardStation_temp.size(); j++) {
    lastHeardStation.push_back(lastHeardStation_temp[j]);
  }
  lastHeardStation_temp.clear();
}

}