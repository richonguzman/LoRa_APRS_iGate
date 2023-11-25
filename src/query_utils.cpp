#include "configuration.h"
#include "query_utils.h"

extern Configuration        Config;
extern WiFi_AP              *currentWiFi;
extern std::vector<String>  lastHeardStation;
extern std::vector<String>  lastHeardStation_temp;
extern String               versionDate;
extern int                  stationMode;

namespace QUERY_Utils {

String process(String query, String station, String queryOrigin) {
  String answer;
  if (query=="?APRS?" || query=="?aprs?" || query=="?Aprs?" || query=="H" || query=="h" || query=="HELP" || query=="Help" || query=="help" || query=="?") {
    answer = "?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign";
  } else if (query=="?APRSV" || query=="?aprsv" || query=="?Aprsv") {
    answer = "CA2RXU_LoRa_iGate 1.2 v" + versionDate + " sM" + String(stationMode);
  } else if (query=="?APRSP" || query=="?aprsp" || query=="?Aprsp") {
    answer = "iGate QTH: " + String(currentWiFi->latitude,2) + " " + String(currentWiFi->longitude,2);
  } else if (query=="?APRSL" || query=="?aprsl" || query=="?Aprsl") {
    if (lastHeardStation.size() == 0) {
      answer = "No Station Listened in the last " + String(Config.rememberStationTime) + "min.";
    } else {
      for (int i=0; i<lastHeardStation.size(); i++) {
        answer += lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) + " ";
      }
      answer.trim();
    }
  } else if (query.indexOf("?APRSH") == 0 || query.indexOf("?aprsh") == 0 || query.indexOf("?Aprsh") == 0) {
     // sacar callsign despues de ?APRSH
    Serial.println("escuchaste a X estacion? en las ultimas 24 o 8 horas?");
    answer = "APRSH on development 73!";
  } else if (query.indexOf("?WHERE") == 0) { 
    // agregar callsign para completar donde esta X callsign --> posicion
    Serial.println("estaciones escuchadas directo (ultimos 30 min)");
    answer = "?WHERE on development 73!";
  }
  for(int i = station.length(); i < 9; i++) {
    station += ' ';
  }
  if (queryOrigin == "APRSIS") {
    return Config.callsign + ">APLRG1,TCPIP,qAC::" + station + ":" + answer + "\n";
  } else { //} if (queryOrigin == "LoRa") {
    return Config.callsign + ">APLRG1,RFONLY,WIDE1-1::" + station + ":" + answer;
  }
}

}