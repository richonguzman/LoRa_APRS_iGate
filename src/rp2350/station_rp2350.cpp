/*
 * RP2350 station services — lean port of STATION_Utils (src/station_utils.cpp).
 * Blacklist/managers, last-heard tracking, 25 s dedup, and an anti-collision RF
 * output buffer. All called from loraTask (single-threaded -> no locks).
 */
#include "station_rp2350.h"
#include "configuration.h"
#include "lora_utils.h"
#include <vector>

extern Configuration Config;

namespace {

constexpr uint32_t SECS_TO_WAIT  = 3;          // s between TX, and after an RX, before TXing
constexpr uint32_t DEDUP_WINDOW  = 25UL * 1000;

struct LastHeard { uint32_t when; String station; };
struct OutPacket { String packet; bool isBeacon; };
struct DedupEntry { uint32_t when; uint32_t hash; };

std::vector<String>     blacklist;
std::vector<String>     managers;
std::vector<LastHeard>  lastHeard;
std::vector<OutPacket>  outBuffer;
std::vector<DedupEntry> dedup;

uint32_t lastRxTime = 0;
uint32_t lastTxTime = 0;
volatile size_t activeStations = 0;

std::vector<String> loadCallsignList(const String& list) {
    std::vector<String> out;
    int start = 0, n = list.length();
    while (start < n) {
        while (start < n && list[start] == ' ') start++;
        if (start >= n) break;
        int end = start;
        while (end < n && list[end] != ' ') end++;
        out.emplace_back(list.substring(start, end));
        start = end + 1;
    }
    return out;
}

// matches a callsign against a list with trailing-wildcard support ("EA4*")
bool inList(const std::vector<String>& list, const String& callsign) {
    for (const String& entry : list) {
        int star = entry.indexOf('*');
        if (star >= 0) {
            if (star >= 2 && (int)callsign.length() >= star &&
                strncmp(callsign.c_str(), entry.c_str(), star) == 0)
                return true;
        } else if (entry == callsign) {
            return true;
        }
    }
    return false;
}

uint32_t djb2(const String& station, const String& payload) {
    uint32_t h = 5381;
    for (size_t i = 0; i < station.length(); i++) h = ((h << 5) + h) + (uint8_t)station[i];
    for (size_t i = 0; i < payload.length(); i++) h = ((h << 5) + h) + (uint8_t)payload[i];
    return h;
}

void deleteNotHeard() {
    uint32_t now     = millis();
    uint32_t timeout = (uint32_t)Config.rememberStationTime * 60UL * 1000UL;
    for (int i = (int)lastHeard.size() - 1; i >= 0; i--)
        if (now - lastHeard[i].when >= timeout) lastHeard.erase(lastHeard.begin() + i);
    activeStations = lastHeard.size();
}

void cleanDedup() {
    uint32_t now = millis();
    for (int i = (int)dedup.size() - 1; i >= 0; i--)
        if (now - dedup[i].when > DEDUP_WINDOW) dedup.erase(dedup.begin() + i);
}

}  // namespace

namespace Station {

void setup() {
    blacklist = loadCallsignList(Config.blacklist);
    managers  = loadCallsignList(Config.remoteManagement.managers);
    Serial.printf("[station] blacklist=%u managers=%u\n",
                  (unsigned)blacklist.size(), (unsigned)managers.size());
}

bool isBlacklisted(const String& callsign) { return inList(blacklist, callsign); }
bool isManager(const String& callsign)     { return inList(managers, callsign); }

void updateLastHeard(const String& station) {
    deleteNotHeard();
    uint32_t now = millis();
    for (auto& s : lastHeard) {
        if (s.station == station) { s.when = now; return; }
    }
    lastHeard.push_back({now, station});
    activeStations = lastHeard.size();
}

bool wasHeard(const String& station) {
    deleteNotHeard();
    for (const auto& s : lastHeard)
        if (s.station == station) return true;
    return false;
}

size_t activeCount() { return activeStations; }

bool isDuplicate(const String& station, const String& payload) {
    cleanDedup();
    uint32_t h = djb2(station, payload);
    for (const auto& d : dedup)
        if (d.hash == h) return true;
    dedup.push_back({millis(), h});
    return false;
}

void enqueueTx(const String& packet, bool isBeacon) {
    outBuffer.push_back({packet, isBeacon});
}

void noteRx() { lastRxTime = millis(); }

size_t txPending() { return outBuffer.size(); }

void processTxBuffer() {
    if (outBuffer.empty()) return;
    uint32_t now    = millis();
    uint32_t window = SECS_TO_WAIT * 1000;
    if ((now - lastTxTime) <= window || (now - lastRxTime) <= window) return;  // let the channel settle
    LoRa_Utils::sendNewPacket(outBuffer.front().packet);
    outBuffer.erase(outBuffer.begin());
    lastTxTime = millis();
}

}  // namespace Station
