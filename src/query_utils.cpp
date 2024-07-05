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

    String process(const String& query, const String& station, bool queryFromAPRSIS, bool thirdParty) {
        String answer;
        String queryQuestion = query;
        queryQuestion.toUpperCase();
        if (queryQuestion == "?APRS?" || queryQuestion == "H" || queryQuestion == "HELP" || queryQuestion=="?") {
            answer.concat("?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign");
        } else if (queryQuestion == "?APRSV") {
            answer.concat("CA2RXU_LoRa_iGate 1.4 v");
            answer.concat(versionDate);
        } else if (queryQuestion == "?APRSP") {
            answer.concat("iGate QTH: ");
            answer.concat(String(Config.beacon.latitude,3));
            answer.concat(" ");
            answer.concat(String(Config.beacon.longitude,3));
        } else if (queryQuestion == "?APRSL") {
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
        } else if (queryQuestion == "?APRSSR") {
            char signalData[35];
            snprintf(signalData, sizeof(signalData), " %ddBm / %.2fdB / %dHz", rssi, snr, freqError);
            answer.concat(signalData);        
        } else if (queryQuestion.indexOf("?APRSH") == 0) {
            // sacar callsign despues de ?APRSH
            Serial.println("escuchaste a X estacion? en las ultimas 24 o 8 horas?");
            answer.concat("?APRSH on development 73!");
        } else if (queryQuestion.indexOf("?WHERE") == 0) { 
            // agregar callsign para completar donde esta X callsign --> posicion
            Serial.println("estaciones escuchadas directo (ultimos 30 min)");
            answer.concat("?WHERE on development 73!");
        }

        String queryAnswer = Config.callsign;
        queryAnswer += ">APLRG1";
        if (queryFromAPRSIS) {
            queryAnswer += ",TCPIP,qAC";
        } else {
            if (!thirdParty) queryAnswer += ",RFONLY";
            if (Config.beacon.path != "") {
                queryAnswer += ",";
                queryAnswer += Config.beacon.path;
            }
        }
        queryAnswer += "::";

        String processedStation = station;
        for (int i = station.length(); i < 9; i++) {
            processedStation += ' ';
        }
        queryAnswer += processedStation;
        queryAnswer += ":";
        queryAnswer += answer;
        return queryAnswer;
    }

}