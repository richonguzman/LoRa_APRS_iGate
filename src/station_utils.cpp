#include "station_utils.h"
#include <vector>
#include "configuration.h"
/*#include <WiFi.h>
#include "configuration.h"
#include "display.h"


extern WiFiClient     espClient;
extern int            internalLedPin;
extern uint32_t       lastRxTxTime;*/
extern Configuration        Config;
extern std::vector<String>  lastHeardStation;
extern std::vector<String>  lastHeardStation_temp;

namespace STATION_Utils {

void deleteNotHeard() {
  uint32_t minReportingTime = Config.rememberStationTime*60*1000;
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

void updateLastHeard(String station) {
  deleteNotHeard();
  bool stationHeard = false;
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      lastHeardStation[i] = station + "," + String(millis());
      stationHeard = true;
    }
  }
  if (!stationHeard) {
    lastHeardStation.push_back(station + "," + String(millis()));
  }

  //////
  Serial.print("Stations Near (last 30 minutes): ");
  for (int k=0; k<lastHeardStation.size(); k++) {
    Serial.print(lastHeardStation[k].substring(0,lastHeardStation[k].indexOf(","))); Serial.print(" ");
  }
  Serial.println("");
}

bool wasHeard(String station) {
  deleteNotHeard();
  bool validStation = false;
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      validStation = true;
      Serial.println(" ---> Listened Station");
    } 
  }
  if (!validStation) {
    Serial.println("   ---> Station not Heard for last 30 min (Not Tx)\n");
  }
  return validStation;
}

}