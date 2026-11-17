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
volatile size_t   activeStations = 0;
volatile uint32_t rxCounter      = 0;   // RX packets since last telemetry (benign cross-task read-reset)

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

// Verbatim copy of Utils::callsignIsValid (src/utils.cpp), which is not built for
// the RP2350. Keep the two in step.
bool callsignIsValid(const String& callsign) {
    if (callsign == "WLNK-1") return true;
    int totalCallsignLength = callsign.length();
    if (totalCallsignLength < 4) return false;

    int hyphenIndex         = callsign.indexOf("-");
    int baseCallsignLength  = (hyphenIndex > 0) ? hyphenIndex : totalCallsignLength;

    if (hyphenIndex > 0) {    // SSID Validation
        if (hyphenIndex < 4) return false;                                  // base Callsign must have at least 4 characters
        int ssidStart   = hyphenIndex + 1;
        int ssidLength  = totalCallsignLength - ssidStart;
        if (ssidLength == 0 || ssidLength > 2) return false;
        if (callsign.indexOf('-', ssidStart) != -1) return false;           // avoid another "-" in ssid
        if (ssidLength == 2 && callsign[ssidStart] == '0') return false;    // ssid can't start with "0"
        for (int i = ssidStart; i < totalCallsignLength; i++) {
            if (!isDigit(callsign[i])) return false;
        }
    }

    if (baseCallsignLength < 4 || baseCallsignLength > 6) return false;

    bool padded = false;    // Callsigns with 4 characters like A0AA are padded into 5 characters for further analisis : A0AA --> _A0AA
    if (baseCallsignLength == 4 && isAlpha(callsign[0]) && isDigit(callsign[1]) && isAlpha(callsign[2]) && isAlpha(callsign[3])) padded = true;
    char c0, c1, c2, c3;
    if (padded) {
        c0 = ' ';
        c1 = callsign[0];
        c2 = callsign[1];
        c3 = callsign[2];
    } else {
        c0 = callsign[0];
        c1 = callsign[1];
        c2 = callsign[2];
        c3 = callsign[3];
    }
    if (!isDigit(c2) || !isAlpha(c3)) {                                     // __0A__ must be validated
        if (c0 != 'R' && !isDigit(c1) && !isAlpha(c2)) return false;        // to accepto R0A___
    }

    bool isValid =
        ((isAlphaNumeric(c0) || c0 == ' ') && isAlpha(c1)) ||               //  AA0A (+A+A) + _A0AA (+A) + 0A0A (+A+A)
        (isAlpha(c0) && isDigit(c1)) ||                                     //  A00A (+A+A)
        (c0 == 'R' && baseCallsignLength == 6 && isDigit(c1) && isAlpha(c2) && isAlpha(c3) && isAlpha(callsign[4]));  //  R0AA (+A+A)
    if (!isValid) return false;                                             // also 00__ avoided

    if (baseCallsignLength > 4 ) {                                          // to validate ____AA
        for (int i = 4; i < baseCallsignLength; i++) {
            if (!isAlpha(callsign[i])) return false;
        }
    }
    return true;
}

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

String heardListString() {
    deleteNotHeard();
    String out;
    for (const auto& s : lastHeard) { out += s.station; out += ' '; }
    out.trim();
    return out;
}

bool isDuplicate(const String& station, const String& payload) {
    cleanDedup();
    uint32_t h = djb2(station, payload);
    for (const auto& d : dedup)
        if (d.hash == h) return true;
    dedup.push_back({millis(), h});
    return false;
}

void enqueueTx(const String& packet, bool isBeacon) {
    if (packet.length() == 0) return;                       // never queue an empty packet
    outBuffer.push_back({packet, isBeacon});
}

void noteRx() { lastRxTime = millis(); rxCounter++; }

uint32_t takeRxCount() { uint32_t n = rxCounter; rxCounter = 0; return n; }

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
