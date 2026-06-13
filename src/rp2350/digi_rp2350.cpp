/*
 * RP2350 digipeater — lean port of DIGI_Utils (src/digi_utils.cpp).
 * Computes the WIDEn-N-rewritten packet to re-transmit over RF. The actual TX
 * is done by the caller (loraTask, which owns the radio) via
 * LoRa_Utils::sendNewPacket(), so this module never touches the radio or W5500.
 */
#include "digi_rp2350.h"
#include "configuration.h"

extern Configuration Config;

namespace {

// --- lean port of Utils::callsignIsValid (src/utils.cpp) ---
bool callsignIsValid(const String& callsign) {
    if (callsign == "WLNK-1") return true;
    int totalLen = callsign.length();
    if (totalLen < 4) return false;

    int hyphen   = callsign.indexOf('-');
    int baseLen  = (hyphen > 0) ? hyphen : totalLen;

    if (hyphen > 0) {                                   // SSID validation
        if (hyphen < 4) return false;                   // base call >= 4 chars
        int ssidStart  = hyphen + 1;
        int ssidLen    = totalLen - ssidStart;
        if (ssidLen == 0 || ssidLen > 2) return false;
        if (callsign.indexOf('-', ssidStart) != -1) return false;
        if (ssidLen == 2 && callsign[ssidStart] == '0') return false;
        for (int i = ssidStart; i < totalLen; i++)
            if (!isDigit(callsign[i])) return false;
    }
    if (baseLen < 4 || baseLen > 6) return false;
    for (int i = 0; i < baseLen; i++)
        if (!isAlphaNumeric(callsign[i])) return false;
    return true;
}

// --- dedup ring buffer: break repeater loops / multi-path echoes ---
constexpr int      DEDUP_SLOTS  = 24;
constexpr uint32_t DEDUP_WINDOW = 30000;   // ms (the original "25-seg" buffer)
struct Seen { uint32_t hash; uint32_t when; };
Seen dedup[DEDUP_SLOTS];

uint32_t fnv1a(const String& s) {
    uint32_t h = 2166136261UL;
    for (size_t i = 0; i < s.length(); i++) { h ^= (uint8_t)s[i]; h *= 16777619UL; }
    return h;
}

// Returns true if key was seen within the window (a duplicate). Otherwise records
// it (reusing an empty or the oldest slot) and returns false. when==0 marks an
// empty slot, so a real timestamp of 0 is bumped to 1.
bool seenRecently(const String& key) {
    uint32_t now = millis();
    uint32_t h   = fnv1a(key);

    int      victim   = 0;       // slot to overwrite if not a duplicate
    uint32_t maxAge   = 0;
    bool     haveEmpty = false;

    for (int i = 0; i < DEDUP_SLOTS; i++) {
        if (dedup[i].when != 0 && dedup[i].hash == h && (now - dedup[i].when) < DEDUP_WINDOW)
            return true;                                  // duplicate within window
        if (dedup[i].when == 0) {                         // empty slot — prefer it
            if (!haveEmpty) { victim = i; haveEmpty = true; }
        } else if (!haveEmpty) {                          // else track the oldest
            uint32_t age = now - dedup[i].when;
            if (age >= maxAge) { maxAge = age; victim = i; }
        }
    }
    dedup[victim].hash = h;
    dedup[victim].when = now ? now : 1;
    return false;
}

String stationCall() {
    return Config.tacticalCallsign.length() ? Config.tacticalCallsign : Config.callsign;
}

// strip any residual on-air header bytes ('<' 0xFF 0x01) appended to the payload
String checkForStartingBytes(const String& packet) {
    int idx = packet.indexOf("\x3c\xff\x01");
    return (idx != -1) ? packet.substring(0, idx) : packet;
}

String cleanPathAsterisks(String path) {
    const char* terms[] = {",WIDE1*", ",WIDE2*", "*"};
    for (const char* term : terms) {
        int idx = path.indexOf(term);
        if (idx != -1) path.remove(idx, strlen(term));
    }
    return path;
}

// Port of DIGI_Utils::buildPacket, non-crossFreq / non-thirdParty branch only.
// `packet` is the headerless "SENDER>PATH:payload"; `path` is the part after the
// first comma in PATH (e.g. "WIDE1-1" or "WIDE2-2").
String buildPacket(const String& path, const String& packet) {
    String call      = stationCall();
    int    colon     = packet.indexOf(':');
    int    digiMode  = Config.digi.mode;
    String tempPath  = path;

    if (tempPath.indexOf("WIDE1-1") != -1 && (digiMode == 2 || digiMode == 3 || Config.digi.backupDigiMode)) {
        if (tempPath.indexOf('*') != -1) return "";          // '*' shouldn't precede WIDE1-1
        tempPath.replace("WIDE1-1", call + "*");
    } else if (tempPath.indexOf("WIDE2-") != -1 && digiMode == 3) {
        tempPath = cleanPathAsterisks(path);
        if (tempPath.indexOf("WIDE2-1") != -1) {
            tempPath.replace("WIDE2-1", call + "*");
        } else if (tempPath.indexOf("WIDE2-2") != -1) {
            tempPath.replace("WIDE2-2", call + "*,WIDE2-1");
        } else {
            return "";                                       // WIDE2-3+ unsupported
        }
    } else {
        return "";
    }

    String out = packet.substring(0, packet.indexOf(',') + 1);   // "SENDER>APxxxx,"
    out += tempPath;
    out += checkForStartingBytes(packet.substring(colon));       // ":payload"
    return out;
}

// Port of DIGI_Utils::generateDigipeatedPacket, non-thirdParty / non-crossFreq.
String generateDigipeatedPacket(const String& packet) {
    int gt    = packet.indexOf('>');
    int colon = packet.indexOf(':');
    if (gt < 0 || colon <= gt) return "";
    String hdr = packet.substring(gt + 1, colon);    // "APxxxx,WIDE1-1,..."
    int comma  = hdr.indexOf(',');
    int digiMode = Config.digi.mode;
    bool backup  = Config.digi.backupDigiMode;

    if (comma <= 2) return "";                        // no path -> nothing to consume
    String path = hdr.substring(comma + 1);

    if (digiMode == 2 || backup) {
        return (path.indexOf("WIDE1-1") != -1) ? buildPacket(path, packet) : "";
    }
    if (digiMode == 3) {
        int w1 = path.indexOf("WIDE1-1");
        int w2 = path.indexOf("WIDE2-");
        bool hasW1 = w1 != -1, hasW2 = w2 != -1;
        if (hasW1 && hasW2 && w2 < w1) return "";     // WIDE1 must precede WIDE2
        if (hasW1 || hasW2) return buildPacket(path, packet);
    }
    return "";
}

}  // namespace

namespace Digi {

bool enabled() {
    return Config.digi.mode == 2 || Config.digi.mode == 3 || Config.digi.backupDigiMode;
}

String process(const String& rawPacket) {
    if (!enabled()) return "";
    if (rawPacket.length() < 5) return "";

    String packet = rawPacket.substring(3);                 // drop 3-byte LoRa-APRS header
    int gt = packet.indexOf('>');
    if (gt < 3) return "";
    String sender = packet.substring(0, gt);

    // exclusions handled like the iGate path
    if (packet.indexOf("NOGATE") >= 0 || packet.indexOf("RFONLY") >= 0) return "";

    String call = stationCall();
    if (sender == call) return "";                          // never repeat ourselves
    if (Config.tacticalCallsign.length() == 0 && !callsignIsValid(sender)) return "";

    // dedup on sender + payload (after the first ':')
    int colon = packet.indexOf(':');
    if (colon < 0) return "";
    if (seenRecently(sender + packet.substring(colon))) return "";

    return generateDigipeatedPacket(packet);
}

}  // namespace Digi
