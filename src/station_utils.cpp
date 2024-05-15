#include "station_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include "lora_utils.h"
#include "utils.h"
#include <vector>

extern Configuration            Config;
extern uint32_t                 lastRxTime;
extern String                   fourthLine;

uint32_t lastTxTime             = millis();
std::vector<String>             lastHeardStation;
std::vector<String>             outputPacketBuffer;
std::vector<String>             packet25SegBuffer;


namespace STATION_Utils {

    void deleteNotHeard() {
        std::vector<String>  lastHeardStation_temp;
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

    void updateLastHeard(const String& station) {
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

    bool wasHeard(const String& station) {
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

    void clean25SegBuffer() {
        if (!packet25SegBuffer.empty()) {
            String deltaTimeString = packet25SegBuffer[0].substring(0, packet25SegBuffer[0].indexOf(","));
            uint32_t deltaTime = deltaTimeString.toInt();
            if ((millis() - deltaTime) >  25 * 1000) {
                packet25SegBuffer.erase(packet25SegBuffer.begin());
            }
        }
    }

    bool check25SegBuffer(const String& station, const String& textMessage) {
        if (!packet25SegBuffer.empty()) {
            bool shouldBeIgnored = false;
            for (int i = 0; i < packet25SegBuffer.size(); i++) {
                String temp = packet25SegBuffer[i].substring(packet25SegBuffer[i].indexOf(",") + 1);
                String bufferStation = temp.substring(0, temp.indexOf(","));
                String bufferMessage = temp.substring(temp.indexOf(",") + 1);
                if (bufferStation == station && bufferMessage == textMessage) {
                    shouldBeIgnored = true;
                }
            }
            if (shouldBeIgnored) {
                return false;
            } else {
                packet25SegBuffer.push_back(String(millis()) + "," + station + "," + textMessage);
                return true;
            }
        } else {
            packet25SegBuffer.push_back(String(millis()) + "," + station + "," + textMessage);
            return true;
        }    
    }

    void processOutputPacketBuffer() {
        int timeToWait = 3 * 1000;      // 3 segs between packet Tx and also Rx ???
        uint32_t lastRx = millis() - lastRxTime;
        uint32_t lastTx = millis() - lastTxTime;
        if (outputPacketBuffer.size() > 0 && lastTx > timeToWait && lastRx > timeToWait) {
            LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
            outputPacketBuffer.erase(outputPacketBuffer.begin());
            lastTxTime = millis();
        }
    }

    void addToOutputPacketBuffer(const String& packet) {
        outputPacketBuffer.push_back(packet);
    }

}