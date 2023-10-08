#include "station_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include <vector>

extern Configuration        Config;
extern std::vector<String>  lastHeardStation;
extern std::vector<String>  lastHeardStation_temp;
extern std::vector<String>  packetBuffer;
extern std::vector<String>  packetBuffer_temp;
extern String               fourthLine;

namespace STATION_Utils {

void deleteNotHeard() {
  for (int i=0; i<lastHeardStation.size(); i++) {
    String deltaTimeString = lastHeardStation[i].substring(lastHeardStation[i].indexOf(",")+1);
    uint32_t deltaTime = deltaTimeString.toInt();
    if ((millis() - deltaTime) < Config.rememberStationTime*60*1000) {
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

  fourthLine = "Stations (" + String(Config.rememberStationTime) + "min) = ";
  if (lastHeardStation.size() < 10) {
    fourthLine += " ";
  }
  fourthLine += String(lastHeardStation.size());

  #ifndef PinPointApp  ////// This is just for debugging
  Serial.print("Stations Near (last " + String(Config.rememberStationTime) + " minutes): ");
  for (int k=0; k<lastHeardStation.size(); k++) {
    Serial.print(lastHeardStation[k].substring(0,lastHeardStation[k].indexOf(","))); Serial.print(" ");
  }
  Serial.println("");
  #endif  
}

bool wasHeard(String station) {
  deleteNotHeard();
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      Serial.println(" ---> Listened Station");
      return true;
    } 
  }
  Serial.println(" ---> Station not Heard for last 30 min (Not Tx)\n");
  return false;
}

void checkBuffer() {
  for (int i=0; i<packetBuffer.size(); i++) {
    String deltaTimeString = packetBuffer[i].substring(0,packetBuffer[i].indexOf(","));
    uint32_t deltaTime = deltaTimeString.toInt();
    if ((millis() - deltaTime) < 60*1000) { // cambiar a 15 segundos?
      packetBuffer_temp.push_back(packetBuffer[i]);
    }
  }
  packetBuffer.clear();
  for (int j=0; j<packetBuffer_temp.size(); j++) {
    packetBuffer.push_back(packetBuffer_temp[j]);
  }
  packetBuffer_temp.clear();

  // BORRAR ESTO !!
  for (int i=0; i<packetBuffer.size(); i++) {
    Serial.println(packetBuffer[i]);
  }
  //
}

void updatePacketBuffer(String packet) {
  if ((packet.indexOf(":!") == -1) && (packet.indexOf(":=") == -1) && (packet.indexOf(":>") == -1)) {
    String sender = packet.substring(3,packet.indexOf(">"));
    String tempAddressee = packet.substring(packet.indexOf("::") + 2);
    String addressee = tempAddressee.substring(0,tempAddressee.indexOf(":"));
    addressee.trim();
    String message = tempAddressee.substring(tempAddressee.indexOf(":")+1);
    //Serial.println(String(millis()) + "," + sender + "," + addressee + "," + message);
    packetBuffer.push_back(String(millis()) + "," + sender + "," + addressee + "," + message);
    checkBuffer();
  }
}


}