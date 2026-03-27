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

#define SECS_TO_WAIT            3   // soon to be deleted...


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
    OutputPacketBuffer(const String& p, bool b) : packet(p), isBeacon(b) {}
};
std::vector<OutputPacketBuffer> outputPacketBuffer;

struct Packet25SegBuffer {
    uint32_t    receivedTime;
    uint32_t    hash;
};
std::vector<Packet25SegBuffer>  packet25SegBuffer;

bool saveNewDigiEcoModeConfig   = false;
bool packetIsBeacon             = false;


namespace STATION_Utils {

    std::vector<String> loadCallsignList(const String& list) {
        std::vector<String> loadedList;
        int start       = 0;
        int listLength  = list.length();

        while (start < listLength) {
            while (start < listLength && list[start] == ' ') start++;  // avoid blank spaces
            if (start >= listLength) break;

            int end = start;
            while (end < listLength && list[end] != ' ') end++;         // find another blank space or reach listLength

            loadedList.emplace_back(list.substring(start, end));
            start = end + 1;                                            // keep on searching if listLength not reached
        }
        return loadedList;
    }

    void loadBlacklistAndManagers() {
        blacklist   = loadCallsignList(Config.blacklist);
        managers    = loadCallsignList(Config.remoteManagement.managers);
    }

    bool checkCallsignList(const std::vector<String>& list, const String& callsign) {
        for (size_t i = 0; i < list.size(); i++) {
            int wildcardIndex = list[i].indexOf("*");
            if (wildcardIndex >= 0) {
                if (wildcardIndex >= 2 && callsign.length() >= wildcardIndex && strncmp(callsign.c_str(), list[i].c_str(), wildcardIndex) == 0) {
                    return true;
                }
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
        uint32_t currentTime    = millis();
        uint32_t timeout        = Config.rememberStationTime * 60UL * 1000UL;

        for (int i = lastHeardStations.size() - 1; i >= 0; i--) {
            if (currentTime - lastHeardStations[i].lastHeardTime >= timeout) {
                lastHeardStations.erase(lastHeardStations.begin() + i);
            }
        }
    }

    void updateLastHeard(const String& station) {
        deleteNotHeard();
        uint32_t currentTime = millis();
        for (size_t i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                lastHeardStations[i].lastHeardTime = currentTime;
                Utils::showActiveStations();
                return;
            }
        }
        lastHeardStations.emplace_back(LastHeardStation{currentTime, station});
        Utils::showActiveStations();
    }

    bool wasHeard(const String& station) {
        deleteNotHeard();
        for (size_t i = 0; i < lastHeardStations.size(); i++) {
            if (lastHeardStations[i].station == station) {
                Utils::println(" ---> Listened Station");
                return true;
            }
        }
        Utils::println(" ---> Station not Heard in " + String(Config.rememberStationTime) + " min: No Tx");
        return false;
    }

    void clean25SegHashBuffer() {
        uint32_t currentTime = millis();
        for (int i = packet25SegBuffer.size() - 1; i >= 0; i--) {
            if ((currentTime - packet25SegBuffer[i].receivedTime) > 25 * 1000) {
                packet25SegBuffer.erase(packet25SegBuffer.begin() + i);
            }
        }
    }

    uint32_t makeHash(const String& station, const String& payload) {   // DJB2 Hash
        uint32_t h = 5381;
        for (size_t i = 0; i < station.length(); i++)
            h = ((h << 5) + h) + station[i];
        for (size_t i = 0; i < payload.length(); i++)
            h = ((h << 5) + h) + payload[i];
        return h;
    }

    bool isIn25SegHashBuffer(const String& station, const String& textMessage) {
        clean25SegHashBuffer();
        uint32_t newHash        = makeHash(station, textMessage);
        uint32_t currentTime    = millis();
        for (int i = 0; i < packet25SegBuffer.size(); i++) {
            if (packet25SegBuffer[i].hash == newHash) return true;
        }
        packet25SegBuffer.push_back({currentTime, newHash});
        return false;
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
        int timeToWait                  = SECS_TO_WAIT * 1000;          // 3 segs between packet Tx and also Rx ???
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
        outputPacketBuffer.emplace_back(OutputPacketBuffer{packet, flag});
    }

}