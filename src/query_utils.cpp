#include "configuration.h"
#include "query_utils.h"
#include "lora_utils.h"

extern Configuration        Config;
extern std::vector<String>  lastHeardStation;
extern String               versionDate;
extern int                  rssi;
extern float                snr;
extern int                  freqError;


namespace QUERY_Utils {

    String process(const String& query, const String& station, const uint8_t queryOrigin) {
        String answer;
        if (query=="?APRS?" || query=="?aprs?" || query=="?Aprs?" || query=="H" || query=="h" || query=="HELP" || query=="Help" || query=="help" || query=="?") {
            answer = "?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign";
        } else if (query=="?APRSV" || query=="?aprsv" || query=="?Aprsv") {
            answer = "CA2RXU_LoRa_iGate 1.3 v";
            answer.concat(versionDate);
        } else if (query=="?APRSP" || query=="?aprsp" || query=="?Aprsp") {
            answer = "iGate QTH: ";
            answer.concat(String(Config.beacon.latitude,2));
            answer.concat(" ");
            answer.concat(String(Config.beacon.longitude,2));
        } else if (query=="?APRSL" || query=="?aprsl" || query=="?Aprsl") {
            if (lastHeardStation.size() == 0) {
                char answerArray[50];
                snprintf(answerArray, sizeof(answerArray), "No Station Listened in the last %d min.", Config.rememberStationTime);
                answer.concat(answerArray);
            } else {
                for (int i=0; i<lastHeardStation.size(); i++) {
                    answer += lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) + " ";
                }
                answer.trim();
            }
        } else if (query=="?APRSSR" || query=="?aprssr" || query=="?Aprssr") {
            char signalData[35];
            snprintf(signalData, sizeof(signalData), " %ddBm / %.2fdB / %dHz", rssi, snr, freqError);
            answer.concat(signalData);        
        } else if (query.indexOf("?APRSH") == 0 || query.indexOf("?aprsh") == 0 || query.indexOf("?Aprsh") == 0) {
            // sacar callsign despues de ?APRSH
            Serial.println("escuchaste a X estacion? en las ultimas 24 o 8 horas?");
            answer = "?APRSH on development 73!";
        } else if (query.indexOf("?WHERE") == 0) { 
            // agregar callsign para completar donde esta X callsign --> posicion
            Serial.println("estaciones escuchadas directo (ultimos 30 min)");
            answer = "?WHERE on development 73!";
        }
        String processedStation = station;
        for (int i = station.length(); i < 9; i++) {
            processedStation += ' ';
        }
        if (queryOrigin == 1) { // from APRS-IS
            return Config.callsign + ">APLRG1,TCPIP,qAC::" + processedStation + ":" + answer;
        } else { // else == 0 , from LoRa
            if (Config.beacon.path == "") {
                return Config.callsign + ">APLRG1,RFONLY::" + processedStation + ":" + answer;
            } else {
                return Config.callsign + ">APLRG1,RFONLY," + Config.beacon.path + "::" + processedStation + ":" + answer;
            }
        }
    }

}