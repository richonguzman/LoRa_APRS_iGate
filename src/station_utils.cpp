#include "station_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include "lora_utils.h"
#include "utils.h"
#include <vector>

extern Configuration        Config;
extern std::vector<String>  lastHeardStation;
extern std::vector<String>  lastHeardStation_temp;
extern std::vector<String>  outputPacketBuffer;
extern uint32_t             lastTxTime;
extern uint32_t             lastRxTime;
extern String               fourthLine;


namespace STATION_Utils {

    void deleteNotHeard() {
        for (int i = 0; i < lastHeardStation.size(); i++) {
            String deltaTimeString = lastHeardStation[i].substring(lastHeardStation[i].indexOf(",") + 1);
            uint32_t deltaTime = deltaTimeString.toInt();
            if ((millis() - deltaTime) < Config.rememberStationTime * 60 * 1000) {
                lastHeardStation_temp.push_back(lastHeardStation[i]);
            }
        }
        lastHeardStation.clear();
        for (int j = 0; j < lastHeardStation_temp.size(); j++) {
            lastHeardStation.push_back(lastHeardStation_temp[j]);
        }
        lastHeardStation_temp.clear();
    }

    void updateLastHeard(String station) {
        deleteNotHeard();
        bool stationHeard = false;
        for (int i = 0; i < lastHeardStation.size(); i++) {
            if (lastHeardStation[i].substring(0, lastHeardStation[i].indexOf(",")) == station) {
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
    }

    bool wasHeard(String station) {
        deleteNotHeard();
        for (int i = 0; i < lastHeardStation.size(); i++) {
            if (lastHeardStation[i].substring(0, lastHeardStation[i].indexOf(",")) == station) {
                Utils::println(" ---> Listened Station");
                return true;
            }
        }
        Utils::println(" ---> Station not Heard for last 30 min (Not Tx)\n");
        return false;
    }

    void processOutputPacketBuffer() {
        int timeToWait = 3 * 1000;      // 3 segs between packet Tx and also Rx ???
        uint32_t lastRx = millis() - lastRxTime;
        uint32_t lastTx = millis() - lastTxTime;
        if (outputPacketBuffer.size() == 0 || lastTx < timeToWait || lastRx < timeToWait) {
            return;
        } else {
            LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
            outputPacketBuffer.erase(outputPacketBuffer.begin());
            lastTxTime = millis();
        }
    }

    void addToOutputPacketBuffer(String packet) {
        outputPacketBuffer.push_back(packet);
    }

}