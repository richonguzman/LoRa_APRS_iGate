/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

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
std::vector<String>             blacklist;
std::vector<String>             managers;
std::vector<LastHeardStation>   lastHeardObjects;

struct OutputPacketBuffer {
    String      packet;
    bool        isBeacon;
};
std::vector<OutputPacketBuffer> outputPacketBuffer;

struct Packet25SegBuffer {
    uint32_t    receivedTime;
    String      station;
    String      payload;
};
std::vector<Packet25SegBuffer>  packet25SegBuffer;


bool saveNewDigiEcoModeConfig   = false;
bool packetIsBeacon             = false;


namespace STATION_Utils {

    std::vector<String> loadCallSignList(const String& list) {
        std::vector<String> loadedList;

        String callsigns = list;
        callsigns.trim();

        while (callsigns.length() > 0) {    // != ""
            int spaceIndex = callsigns.indexOf(" ");
            if (spaceIndex == -1) {         // No more spaces, add the last part
                loadedList.push_back(callsigns);
                break;
            }
            loadedList.push_back(callsigns.substring(0, spaceIndex));
            callsigns = callsigns.substring(spaceIndex + 1);
            callsigns.trim();               // Trim in case of multiple spaces
        }
        return loadedList;
    }

    void loadBlacklistAndManagers() {
        blacklist   = loadCallSignList(Config.blacklist);
        managers    = loadCallSignList(Config.remoteManagement.managers);
    }

    bool checkCallsignList(const std::vector<String>& list, const String& callsign) {
        for (int i = 0; i < list.size(); i++) {
            int wildcardIndex = list[i].indexOf("*");
            if (wildcardIndex >= 0) {
                String wildcard = list[i].substring(0, wildcardIndex);
                if (callsign.startsWith(wildcard)) return true;
            } else {
                if (list[i] == callsign) return true;
            }
        }
        return false;
    }

    bool isBlacklisted(const String& callsign) {
        return checkCallsignList(blacklist, callsign);
    }

    bool isManager(const String& callsign) {
        return checkCallsignList(managers, callsign);
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
        Utils::showActiveStations();
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

    void processOutputPacketBufferUltraEcoMode() {
        size_t currentIndex = 0;
        while (currentIndex < outputPacketBuffer.size()) {                  // this sends all packets from output buffer
            delay(3000);                                                    // and cleans buffer to avoid sending packets with time offset
            if (outputPacketBuffer[currentIndex].isBeacon) packetIsBeacon = true;
            LoRa_Utils::sendNewPacket(outputPacketBuffer[currentIndex].packet);    // next time it wakes up
            if (outputPacketBuffer[currentIndex].isBeacon) packetIsBeacon = false;
            currentIndex++;
        }
        outputPacketBuffer.clear();
        //
        if (saveNewDigiEcoModeConfig) {
            Config.writeFile();
            delay(1000);
            displayToggle(false);
            ESP.restart();
        }
        //
    }

    void processOutputPacketBuffer() {
        int timeToWait                  = 3 * 1000;      // 3 segs between packet Tx and also Rx ???
        uint32_t lastRx                 = millis() - lastRxTime;
        uint32_t lastTx                 = millis() - lastTxTime;
        if (outputPacketBuffer.size() > 0 && lastTx > timeToWait && lastRx > timeToWait) {
            if (outputPacketBuffer[0].isBeacon) packetIsBeacon = true;
            LoRa_Utils::sendNewPacket(outputPacketBuffer[0].packet);
            if (outputPacketBuffer[0].isBeacon) packetIsBeacon = false;
            outputPacketBuffer.erase(outputPacketBuffer.begin());
            lastTxTime = millis();
        }
        if (shouldSleepLowVoltage) {
            while (outputPacketBuffer.size() > 0) {
                if (outputPacketBuffer[0].isBeacon) packetIsBeacon = true;
                LoRa_Utils::sendNewPacket(outputPacketBuffer[0].packet);
                if (outputPacketBuffer[0].isBeacon) packetIsBeacon = false;
                outputPacketBuffer.erase(outputPacketBuffer.begin());
                delay(4000);
            }
        }
        if (saveNewDigiEcoModeConfig) {
            Config.writeFile();
            delay(1000);
            displayToggle(false);
            ESP.restart();
        }
    }

    void addToOutputPacketBuffer(const String& packet, bool flag) {
        OutputPacketBuffer entry;
        entry.packet    = packet;
        entry.isBeacon  = flag;

        outputPacketBuffer.push_back(entry);
    }

}