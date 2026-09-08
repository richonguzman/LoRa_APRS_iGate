/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
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

#include <APRSPacketLib.h>
#include <RadioLib.h>
#include <vector>
#include "configuration.h"
#include "network_manager.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "map_utils.h"
#include "ntp_utils.h"
#include "display.h"
#include "utils.h"
#include "lora_utils.h"

extern Configuration    Config;
extern NetworkManager   *networkManager;
extern bool             packetIsBeacon;
// Set true only around the single genuine digipeat-relay call site
// (digi_utils.cpp, via STATION_Utils::addToOutputPacketBuffer's
// eligibleForRxt parameter). Defaults false everywhere else -- see
// sendNewPacket()'s allowRxt logic below.
extern bool             packetEligibleForRxt;

extern std::vector<ReceivedPacket> receivedPackets;

//=================================================================
//Declare and initialize RXT Base-89 conversion parameters
//=================================================================
const int ASCII_OFFSET = 33; // Printable ASCII starting point ('!')
const int RXT_MAX_ALPHABET = 89; // total number of symbols available for RXT compression scheme

// --- RSSI Parameters (Linear) ---
const float RSSI_MIN = -130.0f;
const float RSSI_RES = 1.0f;

// SNR Parameters (Linear)
const float SNR_MIN = -9.0f;
const float SNR_MAX = 12.0f;
const float SNR_RES = 0.25f; // 1 / 4.0

// --- FO Parameters (Quadratic / Non-linear Symmetrical) ---
// Maps a base-89 char to a signed frequency offset in Hz (-2500 to +2500)
const float FO_MAX_Hz = 2500.0f;
const int   FO_CENTER_INDEX = 45; // Center of base -89 range

// --- TTH Parameters (Exponential) ---
// Maps to milliseconds using an exponential growth curve.
// TTH_SCALE is derived at runtime from the active modem's symbol time
// (see currentTthScale()), not fixed, so resolution stays proportional
// to symbol time regardless of the SF/BW an operator runs.
const float TTH_BASE = 1.08f;
const float TTH_SCALE_FACTOR = 10.0f; // tthScale = symbolTimeMs * TTH_SCALE_FACTOR

// --- RXT Whitelist ---
// Crutch until every digi on the network is RXT-enabled: callsigns known to
// measure and append RXT tuples. Used by isRxtWhitelisted()/findAllRxtHops()
// to distinguish RXT-capable digis (which contribute a hop + tuple) from
// legacy digis (which only pass the trailer through unmodified).
// As of this build, only these two nodes are actually running RXT-capable
// firmware in the field. KEYSTN and SOMTNP appeared in this list during
// earlier development as illustrative examples and must not be added back
// until those stations are genuinely upgraded -- including a station here
// that hasn't actually appended a tuple causes every real tuple after it
// in the same trailer to misattribute to the wrong hop.
const char* const RXT_WHITELIST[] = {"TSRXAX", "TSRXBX"};
const size_t RXT_WHITELIST_COUNT = sizeof(RXT_WHITELIST) / sizeof(RXT_WHITELIST[0]);
//=================================================================
//=================================================================

bool operationDone      = true;
bool transmitFlag       = true;
String lastRxtField     = "";

#define DIFS_SLOTS      2       // Number of secuential CAD slots to consider a free channel to Tx
int  backoffMax         = 4;    // Max Backoff value (number of CAD slots to wait before Tx)

// Define RadioLib interrupt handler and web sanitizer locally since they are exclusive to lora_utils.cpp
#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif
void setFlag(void) {
    operationDone = true;
}

String sanitizeForWeb(const String& input) {
    String output = "";
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        switch (c) {
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '&': output += "&amp;"; break;
            case '\"': output += "&quot;"; break;
            case '\'': output += "&#39;"; break;
            default: output += c; break;
        }
    }
    return output;
}

#ifdef HAS_SX1262
    SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
#ifdef HAS_SX1268
    #if defined(LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
        SPIClass loraSPI(FSPI);
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, loraSPI);
    #else
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
    #endif
#endif
#ifdef HAS_SX1278
    SX1278 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#ifdef HAS_SX1276
    SX1276 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#if defined(HAS_LLCC68)        //LLCC68 supports spreading factor only in range of 5-11!
    LLCC68 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

int rssi, freqOffset;
float snr;
unsigned long rxCompletedMillis = 0;  // High-resolution timestamp marking exact packet read completion


namespace LoRa_Utils {

    // === RXT HELPERS ===
    // --- RSSI ENCODER/DECODER HELPER ---
    // RSSI Encoder (Physical dBm -> ASCII char)
    char encodeRSSI(float RSSI_dB) {
        float clamped = constrain(RSSI_dB, RSSI_MIN, RSSI_MIN + (RXT_MAX_ALPHABET * RSSI_RES));
        // y = mx + b for index: index = (rssi - min) / resolution
        int index = (int)round((clamped - RSSI_MIN) / RSSI_RES);
        return (char)(index + ASCII_OFFSET);
    }

    // RSSI Decoder (ASCII char -> Physical dBm)
    float decodeRSSI(char cRSSI) {
        int index = cRSSI - ASCII_OFFSET;
        return (index * RSSI_RES) + RSSI_MIN;
    }
   // --- SNR ENCODER/DECODER HELPER ---
    // SNR Encoder (Physical dB -> ASCII char)
    char encodeSNR(float SNR_dB) {
        float clamped = constrain(SNR_dB, SNR_MIN, SNR_MAX);
        // y = mx + b for index: index = (snr - min) / resolution
        int index = (int)round((clamped - SNR_MIN) / SNR_RES);
        return (char)(index + ASCII_OFFSET);
    }

    // SNR Decoder (ASCII char -> Physical dB)
    float decodeSNR(char cSNR) {
        int index = cSNR - ASCII_OFFSET;
        // y = mx + b: snr = (index * resolution) + min
        return (index * SNR_RES) + SNR_MIN;
    }

    // --- FREQUENCY OFFSET ENCODER/DECODER HELPER ---
    // FO Encoder: Hz -> ASCII. Carried as int end-to-end (matches the chip's
    // native integer FreqError reading; no fractional-Hz resolution to gain).
    char encodeFO(int FO_Hz) {
        float clamped = constrain((float)FO_Hz, -FO_MAX_Hz, FO_MAX_Hz);
        float normalized = clamped / FO_MAX_Hz;
        // Inverse of y = u * |u| * max is u = sign(y) * sqrt(|y| / max)
        float sign = (normalized >= 0) ? 1.0f : -1.0f;
        float u = sign * std::sqrt(std::fabs(normalized));
        int index = (int)round((u * FO_CENTER_INDEX) + FO_CENTER_INDEX);
        return (char)(constrain(index, 0, RXT_MAX_ALPHABET) + ASCII_OFFSET);
    }

    // FO Decoder: ASCII -> Hz
    int decodeFO(char cFO) {
        float normalized = ((float)(cFO - ASCII_OFFSET) - FO_CENTER_INDEX) / FO_CENTER_INDEX; // Range: -1.0 to +1.0
        return (int)((normalized * std::fabs(normalized)) * FO_MAX_Hz);
    }

    // --- TIME TO HOP ENCODER/DECODER HELPER ---
    // tthScale is passed in, derived per-call from the active modem's symbol
    // time (see currentTthScale() below), not a fixed constant.

    // Decoder: ASCII -> Milliseconds
    unsigned long decodeTTH(char cTTH, float tthScale) {
        float v_d = (float)(cTTH - ASCII_OFFSET);
        return (unsigned long)((std::pow(TTH_BASE, v_d) - 1.0f) * tthScale);
    }

    // Encoder: Milliseconds -> ASCII
    char encodeTTH(unsigned long TTH_ms, float tthScale) {
        float target = ((float)TTH_ms / tthScale) + 1.0f;
        if (target < 1.0f) target = 1.0f;
        // Inverse of y = (base^x - 1) * scale is x = log(y/scale + 1) / log(base) -> simplified via log base conversion
        float v_d = std::log(target) / std::log(TTH_BASE);
        int index = (int)round(v_d);
        return (char)(constrain(index, 0, RXT_MAX_ALPHABET) + ASCII_OFFSET);
    }

    // Derives tthScale (ms) from the live modem config: symbol time * TTH_SCALE_FACTOR.
    float currentTthScale() {
        float symbolTimeMs = pow(2, Config.loramodule.rxSpreadingFactor) / (Config.loramodule.rxSignalBandwidth / 1000.0f);
        return symbolTimeMs * TTH_SCALE_FACTOR;
    }

    // --- Base-91 Math & RXT Conversion Helpers ---

    int clamp(int val, int minVal, int maxVal) {
        if (val < minVal) return minVal;
        if (val > maxVal) return maxVal;
        return val;
    }

    char encodeRSSI(int rssi_dBm) {
        int v_rssi = clamp((int)round((rssi_dBm - RSSI_MIN) / RSSI_RES), 0, RXT_MAX_ALPHABET);
        return (char)(v_rssi + ASCII_OFFSET);
    }

    String buildRxtTuple(int rssi_val, float snr_val, int fo_val, unsigned long tth_val) {
        float tthScale = currentTthScale();

        String tuple = "";
        tuple += encodeRSSI(rssi_val);
        tuple += encodeSNR(snr_val);
        tuple += encodeFO(fo_val);
        tuple += encodeTTH(tth_val, tthScale);
        return tuple;
    }

    String attachRxtTrailer(const String& packet, const String& newTuple) {
        String cleanPacket = packet;
        int len = cleanPacket.length();
        
        // Check if the packet ends with '}'
        if (len > 6 && cleanPacket.charAt(len - 1) == '}') {
            int trailerIdx = -1;
            for (int i = len - 2; i >= 0 && i > len - 20; i--) {
                if (cleanPacket.charAt(i) == '{') {
                    trailerIdx = i;
                    break;
                }
            }
            
            if (trailerIdx != -1) {
                int innerLen = (len - 1) - (trailerIdx + 1);
                String innerContent = cleanPacket.substring(trailerIdx + 1, len - 1);
                bool validRxt = true;
                
                // Cap matches stripRxtTrailer's own <=12 (3 tuples) exactly.
                // A mismatched, looser cap here would let a 4th tuple attach
                // successfully on the wire while stripRxtTrailer downstream
                // (on every receiving node, including this one) rejects the
                // resulting 16-char blob as invalid -- silently destroying
                // the 3 legitimate tuples too, not just failing to record a
                // 4th hop.
                if (innerLen > 0 && (innerLen % 4 == 0) && innerLen <= 12) {
                    for (int i = 0; i < innerContent.length(); i++) {
                        char c = innerContent.charAt(i);
                        if (c < 33 || c > 122) {
                            validRxt = false;
                            break;
                        }
                    }
                } else {
                    validRxt = false;
                }
                
                String prefix = cleanPacket.substring(0, trailerIdx);
                
                if (validRxt) {
                    if (innerLen >= 12) {
                        // Already at the 3-hop cap. Preserve the 3 existing,
                        // valid, decodable tuples unchanged rather than
                        // appending a 4th (which stripRxtTrailer would then
                        // reject wholesale) or discarding them in favor of
                        // just this new one. The 4th hop's measurement is
                        // simply not recorded -- the packet and its first
                        // three hops of RXT history stay fully intact.
                        return cleanPacket;
                    }
                    return prefix + "{" + innerContent + newTuple + "}";
                } else {
                    return prefix + "{" + newTuple + "}";
                }
            }
        }
        
        return cleanPacket + "{" + newTuple + "}";
    }        

    String stripRxtTrailer(const String& packet, String* outTuple) {
        int len = packet.length();
        if (len > 6 && packet.endsWith("}")) {
            int trailerIdx = -1;
            for (int i = len - 2; i >= 0 && i > len - 20; i--) {
                if (packet.charAt(i) == '{') {
                    trailerIdx = i;
                    break;
                }
            }
            
            if (trailerIdx != -1) {
                int innerLen = (len - 1) - (trailerIdx + 1);
                String innerContent = packet.substring(trailerIdx + 1, len - 1);
                bool validRxt = true;
                
                if (innerLen > 0 && (innerLen % 4 == 0) && innerLen <= 12) {
                    for (size_t i = 0; i < innerContent.length(); i++) {
                        char c = innerContent.charAt(i);
                        if (c < 33 || c > 122) {
                            validRxt = false;
                            break;
                        }
                    }
                } else {
                    validRxt = false;
                }
                
                if (validRxt) {
                    if (outTuple != nullptr) {
                        *outTuple = innerContent;
                    }
                    return packet.substring(0, trailerIdx);
                }
            }
        }
        if (outTuple != nullptr) {
            *outTuple = "";
        }
        return packet;
    }

    String getLastRxtField() {
        return lastRxtField;
    }

    // --- MULTI-HOP RXT PATH RESOLUTION ---
    // Crutch until every digi on the network is RXT-enabled: a hardcoded
    // whitelist of callsigns known to append RXT tuples. Used to figure out
    // which path elements actually measured/appended a tuple versus which
    // are plain legacy digis just passing the trailer through unmodified.
    bool isRxtWhitelisted(const String& callsign) {
        String baseCall = callsign;
        int dashIdx = baseCall.indexOf('-');
        if (dashIdx > 0) baseCall = baseCall.substring(0, dashIdx);
        for (size_t i = 0; i < RXT_WHITELIST_COUNT; i++) {
            if (baseCall.equals(RXT_WHITELIST[i])) return true;
        }
        return false;
    }

    struct RxtHopIdentifier {
        String fromNode;
        String toNode;
    };

    // Walks the path in transmission order (source first, most recent digi
    // last). usedPathNodes must already be star-trimmed -- i.e. contain only
    // elements that have actually transmitted this packet, with the '*'
    // stripped -- so trailing unconsumed aliases (WIDE2-1, etc.) are never
    // mistaken for real hops.
    //
    // Every time a whitelisted RXT digi is encountered, its immediate
    // predecessor in the path -- RXT-enabled or not -- is recorded as the
    // fromNode for that hop. Because RXT trailers are appended left-to-right
    // in the order digis actually transmit, the hop list built here is in
    // the same order as the tuples packed into lastRxtField: hops[i] always
    // pairs with tuple i.
    std::vector<RxtHopIdentifier> findAllRxtHops(const String& sourceCall, const std::vector<String>& usedPathNodes) {
        std::vector<RxtHopIdentifier> hops;
        for (size_t i = 0; i < usedPathNodes.size(); i++) {
            if (isRxtWhitelisted(usedPathNodes[i])) {
                RxtHopIdentifier hop;
                hop.toNode   = usedPathNodes[i];
                hop.fromNode = (i == 0) ? sourceCall : usedPathNodes[i - 1];
                hops.push_back(hop);
            }
        }
        return hops;
    }

    std::vector<RxtHopMetric> getDecodedRxtMetrics(const String& packet) {
        std::vector<RxtHopMetric> realHops; // hops with actual RXT-measured data

        String sourceCall = "UNKNOWN";
        int gtIdx = packet.indexOf('>');
        if (gtIdx != -1) {
            sourceCall = packet.substring(0, gtIdx);
        }

        // Build the full path node list, but stop right after the last
        // starred element -- anything beyond it (e.g. a trailing WIDE2-1)
        // hasn't transmitted this packet yet and must not be treated as a hop.
        // This runs regardless of whether a trailer is present: the physical
        // chain of hops is a fact about the path, not about which of those
        // hops happened to be RXT-instrumented.
        std::vector<String> usedPathNodes;
        int commaIdx = packet.indexOf(',');
        if (commaIdx != -1 && gtIdx != -1) {
            int colonIdx = packet.indexOf(':');
            String pathPart = packet.substring(commaIdx + 1, (colonIdx != -1 ? colonIdx : packet.length()));

            int start = 0;
            bool reachedEnd = false;
            bool foundStar = false;
            while (!reachedEnd) {
                int nextComma = pathPart.indexOf(',', start);
                String node = (nextComma != -1) ? pathPart.substring(start, nextComma)
                                                 : pathPart.substring(start);
                reachedEnd = (nextComma == -1);

                bool wasStarred = node.indexOf('*') != -1;
                node.replace("*", "");
                if (node.length() > 0) {
                    usedPathNodes.push_back(node);
                }
                if (wasStarred) {
                    foundStar = true;
                    break; // everything after the star is unconsumed
                }

                start = nextComma + 1;
            }
            if (!foundStar) {
                // No star anywhere in the path means this packet hasn't
                // been digipeated at all yet -- zero hops have actually
                // occurred. Every token we saw was an unconsumed alias
                // (WIDE1-1, WIDE2-2, etc.), not a real or NA-able hop.
                usedPathNodes.clear();
            }
        }

        if (usedPathNodes.empty()) {
            // Zero hops have actually occurred yet -- nothing to report,
            // real or NA.
            return realHops; // empty
        }

        // Decode any real tuples present. If there's no valid trailer yet
        // (no RXT-capable digi has touched this packet), realHops simply
        // stays empty and every hop below will show as NA -- the full
        // chain is still reported either way, for visual consistency.
        if (lastRxtField.length() >= 4 && lastRxtField.length() % 4 == 0) {
            // Decoding node's own live modem config -- valid for this network
            // since all infrastructure nodes run a fixed SF/BW/CR; gear-shifting
            // is out-of-band for these statistics.
            float tthScale = currentTthScale();

            std::vector<RxtHopIdentifier> hops = findAllRxtHops(sourceCall, usedPathNodes);
            int numHops = lastRxtField.length() / 4;
            // Mismatch between tuples present and RXT digis resolved from the
            // path (e.g. a whitelist gap or a corrupted trailer) is handled by
            // pairing what we can, from the start; unresolved trailing tuples
            // get a placeholder node name rather than silently misattributing
            // them to the wrong hop. Callers needing to detect this can compare
            // numHops to hops.size() themselves if that becomes useful.

            for (int i = 0; i < numHops; i++) {
                String hopTuple = lastRxtField.substring(i * 4, (i + 1) * 4);

                char cRssi = hopTuple.charAt(0);
                char cSnr  = hopTuple.charAt(1);
                char cFo   = hopTuple.charAt(2);
                char cTth  = hopTuple.charAt(3);

                RxtHopMetric hop;
                hop.hasData = true;
                hop.rssi = decodeRSSI(cRssi);
                hop.snr  = decodeSNR(cSnr);
                hop.fo   = decodeFO(cFo);
                hop.tth  = decodeTTH(cTth, tthScale);

                if ((size_t)i < hops.size()) {
                    hop.toNode   = hops[i].toNode;
                    hop.fromNode = hops[i].fromNode;
                } else {
                    hop.toNode   = "RXT_NODE_" + String(i + 1);
                    hop.fromNode = "UNKNOWN";
                }

                realHops.push_back(hop);
            }
        }

        // Build the full physical chain -- source through the final digi --
        // as the returned result: real data where an RXT-enabled digi
        // measured a hop, hasData=false ("NA") where the hop exists in the
        // path but nothing instrumented it (including the case where no
        // trailer exists at all yet). Ordered most-recent-hop-first,
        // matching the order the packet was actually relayed toward this
        // receiver.
        std::vector<RxtHopMetric> fullChainMetrics;
        std::vector<String> fullChain;
        fullChain.push_back(sourceCall);
        for (size_t i = 0; i < usedPathNodes.size(); i++) fullChain.push_back(usedPathNodes[i]);

        for (int i = (int)fullChain.size() - 1; i >= 1; i--) {
            String from = fullChain[i - 1];
            String to   = fullChain[i];

            bool found = false;
            for (size_t j = 0; j < realHops.size(); j++) {
                if (realHops[j].fromNode == from && realHops[j].toNode == to) {
                    fullChainMetrics.push_back(realHops[j]);
                    found = true;
                    break;
                }
            }
            if (!found) {
                RxtHopMetric naHop;
                naHop.hasData  = false;
                naHop.fromNode = from;
                naHop.toNode   = to;
                naHop.rssi = 0;
                naHop.snr  = 0.0f;
                naHop.fo   = 0;
                naHop.tth  = 0;
                fullChainMetrics.push_back(naHop);
            }
        }

        return fullChainMetrics;
    }

    void setup() {
        #if defined (LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
            pinMode(RADIO_VCC_PIN,OUTPUT);
            digitalWrite(RADIO_VCC_PIN,HIGH);
            loraSPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
        #else
            SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        #endif
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        #if defined(RADIO_HAS_XTAL)
            radio.XTAL = true;
        #endif
        int state = radio.begin(freq);
        if (state != RADIOLIB_ERR_NONE) {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setDio1Action(setFlag);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif

        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCRC(true);

        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN))    
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        #ifdef HAS_1W_LORA  
            state = radio.setOutputPower(Config.loramodule.power); 
            radio.setCurrentLimit(140);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            state = radio.setOutputPower(Config.loramodule.power); 
            radio.setCurrentLimit(100);
        #endif
        #if (defined(HAS_SX1268) || defined(HAS_SX1262)) && !defined(HAS_1W_LORA)
            state = radio.setOutputPower(Config.loramodule.power + 2); 
            radio.setCurrentLimit(140);
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setRxBoostedGainMode(true);
        #endif

        #if defined(HAS_TCXO) && !defined(HAS_1W_LORA)
            radio.setDio2AsRfSwitch();
        #endif
        #ifdef HAS_TCXO
            radio.setTCXO(1.8);
        #endif

        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...    done!");
        } else {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
    }

    void changeFreqTx() {
        float freq = (float)Config.loramodule.txFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.txSpreadingFactor);
        radio.setCodingRate(Config.loramodule.txCodingRate4);
        float signalBandwidth = Config.loramodule.txSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    void changeFreqRx() {
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    bool doCAD() {      
        return radio.scanChannel() != RADIOLIB_CHANNEL_FREE;    
    }

    bool doDIFS() {
        for (uint8_t i = DIFS_SLOTS; i > 0; i--) {
            if (doCAD()) return false;
        }
        return true;
    }

    void waitForDIFS() {
        while (!doDIFS()) {
            Serial.println("CAD/DIFS failed, retry...");
        }
    }

    void doBEB() {
        int backoffCounter = random(1, backoffMax + 1);
        while (backoffCounter > 0) {
            if (doCAD()) {
                waitForDIFS();  
            } else {
                backoffCounter--;
            }
        }
    }

    void sendNewPacket(const String& rawPacket) {
        if (!Config.loramodule.txActive) return;

        unsigned long dwellTimeMs = 0;
        if (rxCompletedMillis > 0) {
            dwellTimeMs = millis() - rxCompletedMillis;
        }

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqTx();
            }
        }

        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, HIGH);    
        #endif

        unsigned long cadStart = millis();
        if (Config.loramodule.cadActive) {
            waitForDIFS();  
            doBEB();        
        }
        unsigned long channelWaitMs = millis() - cadStart;
        unsigned long totalDwellAndChannelMs = dwellTimeMs + channelWaitMs;

        // Only ever true for a genuine relay of a frame this station's own
        // LoRa receiver just heard (set via packetEligibleForRxt, mirroring
        // packetIsBeacon's set/reset pattern). Self-originated content --
        // beacons, telemetry, APRS-IS-to-RF conversions, local TNC client
        // traffic, query/command responses -- has no real RF reception
        // behind it and must never carry RXT data, regardless of this
        // packet's payload shape.
        bool allowRxt = packetEligibleForRxt;
        int colonIdx = rawPacket.indexOf(':');
        if (colonIdx != -1) {
            int gtIdx = rawPacket.indexOf('>');
            if (colonIdx > gtIdx) {
                String payload = rawPacket.substring(colonIdx + 1);
                if (payload.length() > 0 && payload.charAt(0) == ':') {
                    allowRxt = false;
                }
            }
        }

        String finalPacket = rawPacket;
        if (allowRxt) {
            String localTuple = buildRxtTuple(rssi, snr, freqOffset, totalDwellAndChannelMs);
            String pendingPacket = attachRxtTrailer(rawPacket, localTuple);
            String fullPayloadWithHeader = "\x3c\xff\x01" + pendingPacket;

            size_t totalBytes = fullPayloadWithHeader.length();
            unsigned long timeOnAirMs = radio.getTimeOnAir(totalBytes) / 1000; 

            unsigned long finalTTH = totalDwellAndChannelMs + timeOnAirMs;
            String finalTuple = buildRxtTuple(rssi, snr, freqOffset, finalTTH);
            finalPacket = attachRxtTrailer(rawPacket, finalTuple);
        }

        int state = radio.transmit("\x3c\xff\x01" + finalPacket);
        transmitFlag = true;

        if (state == RADIOLIB_ERR_NONE) {
            if (Config.syslog.active && networkManager->isConnected()) {
                SYSLOG_Utils::log(3, finalPacket, 0, 0.0, 0);    
            }
            Utils::print("---> LoRa Packet Tx : ");
            Utils::println(finalPacket);
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }

        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, LOW);      
        #endif

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqRx();
            }
        }

        rxCompletedMillis = 0;
    }

    String receivePacket() {
        String packet = "";
        lastRxtField = ""; // State hygiene reset at packet boundary
        
        if (operationDone) {
            operationDone = false;
            if (transmitFlag) {
                radio.startReceive();
                transmitFlag = false;
            } else {
                int state = radio.readData(packet);
                if (state == RADIOLIB_ERR_NONE) {
                    rxCompletedMillis = millis();  
                    if (packet != "") {
                        if (packet.startsWith("\x3c\xff\x01")) {
                            packet = packet.substring(3);
                        }
                        int gtIndex = packet.indexOf(">");
                        if (gtIndex != -1) {
                            String sender = packet.substring(0, gtIndex);
                            if (!STATION_Utils::isBlacklisted(sender)) {
                                rssi        = radio.getRSSI();
                                snr         = radio.getSNR();
                                freqOffset   = radio.getFrequencyError();
                                
                                // Capture the RXT trailer into lastRxtField, but do NOT overwrite
                                // 'packet' with the stripped result -- 'packet' is what gets
                                // returned to the caller and ultimately reaches
                                // DIGI_Utils::generateDigipeatedPacket(). If a second RXT-capable
                                // digi forwards this packet further, its own attachRxtTrailer()
                                // call needs to see the EXISTING trailer intact in order to
                                // concatenate onto it (grow to multiple tuples) rather than
                                // starting fresh -- stripping here would silently replace, not
                                // extend, multi-hop RXT data. cleanPacket is used everywhere a
                                // trailer-free string is actually required.
                                String cleanPacket = stripRxtTrailer(packet, &lastRxtField);
                                #ifdef RXT_RAW_DEBUG
                                Serial.println("[RXT-STRIP] len=" + String(lastRxtField.length()) + " field=\"" + lastRxtField + "\"");
                                #endif

                                if (Config.digi.ecoMode == 0) {
                                    if (receivedPackets.size() >= 10) {
                                        receivedPackets.erase(receivedPackets.begin());
                                    }
                                    ReceivedPacket receivedPacket;
                                    receivedPacket.rxTime   = NTP_Utils::getFormatedTime();
                                    receivedPacket.packet   = sanitizeForWeb(cleanPacket);
                                    receivedPacket.RSSI     = rssi;
                                    receivedPacket.SNR      = snr;
                                    receivedPackets.push_back(receivedPacket);

                                    APRSPacket aprsPacket = APRSPacketLib::processReceivedPacket(cleanPacket, rssi, snr, freqOffset);
                                    if (aprsPacket.type == 0 || aprsPacket.type == 4) {   
                                        MAP_Utils::upsert(aprsPacket.sender, aprsPacket.latitude, aprsPacket.longitude, aprsPacket.overlay + aprsPacket.symbol, aprsPacket.rssi, aprsPacket.snr);
                                    }
                                }

                                if (Config.syslog.active && networkManager->isConnected()) {
                                    SYSLOG_Utils::log(1, cleanPacket, rssi, snr, freqOffset); 
                                }
                            } else {
                                packet = "";
                            }
                        } else {
                            packet = "";
                        }
                        return packet;
                    }
                } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                    rssi        = radio.getRSSI();
                    snr         = radio.getSNR();
                    freqOffset   = radio.getFrequencyError();
                    Utils::println(F("CRC error!"));
                    if (Config.syslog.active && networkManager->isConnected()) {
                        SYSLOG_Utils::log(0, "", rssi, snr, freqOffset); 
                    }
                    packet = "";
                } else {
                    Utils::print(F("failed, code "));
                    Utils::println(String(state));
                    packet = "";
                }
            }
        }
        return packet;
    }

    String receivePacketFromSleep() {
        transmitFlag = false;
        operationDone = true;
        return receivePacket();
    }

    void wakeRadio() {
        radio.startReceive();
    }

    void sleepRadio() {
        radio.sleep();
    }

}
