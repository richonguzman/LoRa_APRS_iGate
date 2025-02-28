#include "station_utils.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"
#include <vector>

extern Configuration            Config;
extern uint32_t                 lastRxTime;
extern String                   fourthLine;
extern bool                     shouldSleepLowVoltage;

uint32_t lastTxTime             = millis();
std::vector<LastHeardStation>   lastHeardStations;
std::vector<String>             outputPacketBuffer;
std::vector<Packet25SegBuffer>  packet25SegBuffer;
std::vector<String>             blackList;
std::vector<String>             managers;
std::vector<LastHeardStation>   lastHeardObjects;

bool saveNewDigiEcoModeConfig   = false;


namespace STATION_Utils {

    std::vector<String> loadCallSignList(String list) {
        std::vector<String> result;

        if (list != "") {
            String callsigns    = list;
            int spaceIndex      = callsigns.indexOf(" ");

            while (spaceIndex >= 0) {
                result.push_back(callsigns.substring(0, spaceIndex));
                callsigns   = callsigns.substring(spaceIndex + 1);
                spaceIndex  = callsigns.indexOf(" ");
            }

            callsigns.trim();

            if (callsigns.length() > 0)
                result.push_back(callsigns);
        }

        return result;
    }

    bool checkCallSignList(const std::vector<String> list, const String& callsign) {
        for (int i = 0; i < list.size(); i++) {
            if (list[i].indexOf("*") >= 0) {
                String wildCard = list[i].substring(0, list[i].indexOf("*"));
                if (callsign.startsWith(wildCard))
                    return true;
            } else {
                if (list[i] == callsign)
                    return true;
            }                
        }
        return false;
    }


    void loadBlackList() {
        blackList = loadCallSignList(Config.blackList);
    }

    void loadManagers() {
        managers = loadCallSignList(Config.aprsRemote.managers);
    }

    bool checkBlackList(const String& callsign) {
        return checkCallSignList(blackList, callsign);
    }

    bool isManager(const String& callsign) {
        return checkCallSignList(managers, callsign);
    }

    void cleanObjectsHeard() {
        for (auto it = lastHeardObjects.begin(); it != lastHeardObjects.end(); ) {
            if (millis() - it->lastHeardTime >= 9.75 * 60 * 1000) { // 9.75 = 9min 45secs
                it = lastHeardObjects.erase(it);    // erase() returns the next valid iterator
            } else {
                ++it;                               // Only increment if not erasing
            }
        }
    }

    bool checkObjectTime(const String& packet) {
        cleanObjectsHeard();

        int objectIDIndex = packet.indexOf(":;");
        String object = packet.substring(objectIDIndex + 2, objectIDIndex + 11);
        object.trim();

        for (int i = 0; i < lastHeardObjects.size(); i++) {                 // Check if i should Tx object
            if (lastHeardObjects[i].station == object) return false;
        }
        lastHeardObjects.emplace_back(LastHeardStation{millis(), object});  // Add new object and Tx
        return true;
    }
    
    void deleteNotHeard() {
        std::vector<LastHeardStation>  lastHeardStation_temp;
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (millis() - lastHeardStations[i].lastHeardTime < Config.rememberStationTime * 60 * 1000) {
                lastHeardStation_temp.push_back(lastHeardStations[i]);
            }
        }
        lastHeardStations.clear();
        for (int j = 0; j < lastHeardStation_temp.size(); j++) {
            lastHeardStations.push_back(lastHeardStation_temp[j]);
        }
        lastHeardStation_temp.clear();
    }

    void updateLastHeard(const String& station) {
        deleteNotHeard();
        bool stationHeard = false;
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                lastHeardStations[i].lastHeardTime = millis();
                stationHeard = true;
                break;
            }
        }
        if (!stationHeard) lastHeardStations.emplace_back(LastHeardStation{millis(), station});
        Utils::activeStations();
    }

    bool wasHeard(const String& station) {
        deleteNotHeard();
        for (int i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                Utils::println(" ---> Listened Station");
                return true;
            }
        }
        Utils::println(" ---> Station not Heard in " + String(Config.rememberStationTime) + " min: No Tx");
        return false;
    }

    void clean25SegBuffer() {
        if (!packet25SegBuffer.empty() && (millis() - packet25SegBuffer[0].receivedTime) >  25 * 1000) packet25SegBuffer.erase(packet25SegBuffer.begin());
    }

    bool check25SegBuffer(const String& station, const String& textMessage) {
        if (!packet25SegBuffer.empty()) {
            for (int i = 0; i < packet25SegBuffer.size(); i++) {
                if (packet25SegBuffer[i].station == station && packet25SegBuffer[i].payload == textMessage) return false;
            }
        }
        packet25SegBuffer.emplace_back(Packet25SegBuffer{millis(), station, textMessage});
        return true;
    }

    void processOutputPacketBuffer() {
        int timeToWait                  = 3 * 1000;      // 3 segs between packet Tx and also Rx ???
        uint32_t lastRx                 = millis() - lastRxTime;
        uint32_t lastTx                 = millis() - lastTxTime;
        if (outputPacketBuffer.size() > 0 && lastTx > timeToWait && lastRx > timeToWait) {
            LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
            outputPacketBuffer.erase(outputPacketBuffer.begin());
            lastTxTime = millis();
        }
        if (shouldSleepLowVoltage) {
            while (outputPacketBuffer.size() > 0) {
                LoRa_Utils::sendNewPacket(outputPacketBuffer[0]);
                outputPacketBuffer.erase(outputPacketBuffer.begin());
                delay(4000);
            }
        }
        if (saveNewDigiEcoModeConfig) {
            setCpuFrequencyMhz(80);
            Config.writeFile();
            delay(1000);
            displayToggle(false);
            ESP.restart();
        }
    }

    void addToOutputPacketBuffer(const String& packet) {
        outputPacketBuffer.push_back(packet);
    }

}