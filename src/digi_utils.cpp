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
#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration    Config;
extern uint32_t         lastScreenOn;
extern APRSPacket       lastAprsPacket;
extern String           iGateBeaconPacket;
extern String           firstLine;
extern String           secondLine;
extern String           thirdLine;
extern String           fourthLine;
extern String           fifthLine;
extern String           sixthLine;
extern String           seventhLine;
extern bool             backupDigiMode;


namespace DIGI_Utils {

    struct RegionalHop {
        bool found = false;
        int tokenStart = -1;
        int tokenEnd = -1;
        String alias;
        int total = 0;
        int remaining = 0;
    };

    static bool isAliasCharacter(char character) {
        return (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9');
    }

    static bool isAliasSeparator(char character) {
        return character == ' ' || character == ',' || character == '\t' ||
               character == '\r' || character == '\n';
    }

    static int pathTokenIndex(const String& path, const String& expected) {
        unsigned int start = 0;
        while (start < path.length()) {
            int end = path.indexOf(',', start);
            if (end == -1) end = path.length();
            if (path.substring(start, end) == expected) return start;
            start = end + 1;
        }
        return -1;
    }

    static bool pathContainsCallsign(const String& path, const String& callsign) {
        String expected = callsign;
        expected.toUpperCase();
        unsigned int start = 0;
        while (start < path.length()) {
            int end = path.indexOf(',', start);
            if (end == -1) end = path.length();
            String token = path.substring(start, end);
            if (token.endsWith("*")) token = token.substring(0, token.length() - 1);
            token.toUpperCase();
            if (token == expected) return true;
            start = end + 1;
        }
        return false;
    }

    static bool isDecimal(const String& text) {
        if (text.length() == 0) return false;
        for (unsigned int i = 0; i < text.length(); i++) {
            if (text[i] < '0' || text[i] > '9') return false;
        }
        return true;
    }

    static bool parseRegionalToken(const String& token, const String& alias,
                                   int& total, int& remaining) {
        if (alias.length() == 0 || alias.length() > 5 || token.endsWith("*")) return false;
        for (unsigned int i = 0; i < alias.length(); i++) {
            if (!isAliasCharacter(alias[i])) return false;
        }

        if (!token.startsWith(alias)) return false;
        int dash = token.indexOf('-', alias.length() + 1);
        if (dash == -1 || token.indexOf('-', dash + 1) != -1) return false;

        String totalText = token.substring(alias.length(), dash);
        String remainingText = token.substring(dash + 1);
        if (!isDecimal(totalText) || !isDecimal(remainingText)) return false;
        if (dash > 6) return false;  // AX.25 address field before the SSID.

        total = totalText.toInt();
        remaining = remainingText.toInt();
        return total >= 1 && total <= Config.digi.regionalMaxHops &&
               remaining >= 1 && remaining <= total;
    }

    static bool matchConfiguredRegionalToken(const String& token, RegionalHop& hop) {
        unsigned int start = 0;
        const String& aliases = Config.digi.regionalAliases;
        while (start < aliases.length()) {
            while (start < aliases.length() && isAliasSeparator(aliases[start])) start++;
            if (start >= aliases.length()) break;

            int end = start;
            while (end < static_cast<int>(aliases.length()) &&
                   !isAliasSeparator(aliases[end])) end++;
            String alias = aliases.substring(start, end);
            alias.toUpperCase();

            int total = 0;
            int remaining = 0;
            if (parseRegionalToken(token, alias, total, remaining)) {
                hop.found = true;
                hop.alias = alias;
                hop.total = total;
                hop.remaining = remaining;
                return true;
            }
            start = end + 1;
        }
        return false;
    }

    static RegionalHop findRegionalHop(const String& path) {
        RegionalHop hop;
        unsigned int start = 0;
        while (start < path.length()) {
            int end = path.indexOf(',', start);
            if (end == -1) end = path.length();
            String token = path.substring(start, end);
            if (matchConfiguredRegionalToken(token, hop)) {
                hop.tokenStart = start;
                hop.tokenEnd = end;
                return hop;
            }
            start = end + 1;
        }
        return hop;
    }

    static String consumeRegionalHop(const String& path, const String& stationCallsign) {
        RegionalHop hop = findRegionalHop(path);
        if (!hop.found) return "";

        String replacement = stationCallsign + "*";
        if (hop.remaining > 1) {
            replacement += "," + hop.alias + String(hop.total) + "-" + String(hop.remaining - 1);
        }
        return path.substring(0, hop.tokenStart) + replacement + path.substring(hop.tokenEnd);
    }

    String cleanPath(const String& path) {
        String result;
        unsigned int start = 0;
        while (true) {
            int delim = path.indexOf(',', start);
            int end = (delim == -1) ? path.length() : delim;
            String token = path.substring(start, end);
            if (token != "WIDE1*" && token != "WIDE2*") {
                if (result.length() > 0) result += ",";
                result += token;
            }
            if (delim == -1) break;
            start = delim + 1;
        }
        return result;
    }

    String processMode3Path(const String& path, const String& stationCallsign) {
        unsigned int start = 0;
        bool prevTokensAllStar = true;
        int ownTokenEnd = -1;

        while (start < path.length()) {
            int delim = path.indexOf(',', start);
            if (delim == -1) delim = path.length();         // busca todo hasta lograr encontra una coma o el final del string

            String token = path.substring(start, delim);
            bool tokenIsOwn = (token == stationCallsign) || (token == stationCallsign + "*");
            bool tokenStar = token.endsWith("*");

            if (tokenIsOwn) {
                if (tokenStar) return "";                   // already digipeated
                if (!prevTokensAllStar) return "";          // earlier tokens must be marked
                ownTokenEnd = delim;
                break;
            }

            if (!tokenStar) prevTokensAllStar = false;
            start = delim + 1;
        }

        if (ownTokenEnd == -1) return "";
        String tempPacket = cleanPath(path.substring(0, ownTokenEnd));
        return tempPacket + "*" + path.substring(ownTokenEnd);
    }

    String buildPacket(const String& path, const String& packet, bool crossFreq) {
        String stationCallsign  = (Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign);
        String suffix           = (lastAprsPacket.header != "") ? ":}" : ":";
        int suffixIndex         = packet.indexOf(suffix);
        String packetToRepeat;
        if (!crossFreq) {
            int digiMode        = Config.digi.mode;
            String tempPath     = path;

            // A digi must never consume another alias after its own identity
            // has already appeared in the used path. The time-based duplicate
            // cache is not an anti-loop mechanism: the same frame may return
            // after its window has expired or after a restart.
            if ((digiMode == 1 || digiMode == 2) &&
                pathContainsCallsign(tempPath, stationCallsign)) return "";

            int wide1Index = pathTokenIndex(tempPath, "WIDE1-1");
            if (wide1Index != -1 && (digiMode == 1 || digiMode == 2)) {                     // WIDE1-1
                if (tempPath.indexOf("*") != -1 ) return "";                                // "*" shouldn't be in WIDE1-1 (only) type of packet
                tempPath = tempPath.substring(0, wide1Index) + stationCallsign + "*" +
                           tempPath.substring(wide1Index + 7);
            } else if (digiMode == 2) {                                                     // Configured regional alias
                tempPath = cleanPath(path);
                tempPath = consumeRegionalHop(tempPath, stationCallsign);
                if (tempPath == "") return "";
            } else if (digiMode == 3) {                                                     // Repeat if station callsign is in path (free to repeat).
                tempPath = processMode3Path(tempPath, stationCallsign);
                if (tempPath == "") return "";
            }
            packetToRepeat = packet.substring(0, packet.indexOf(",") + 1);
            packetToRepeat += tempPath;
        } else {   // CrossFreq Digipeater
            packetToRepeat = cleanPath(packet.substring(0, suffixIndex));
            if (packetToRepeat.indexOf(stationCallsign) != -1) return "";                   // stationCallsign shouldn't be in path
            packetToRepeat += ",";
            packetToRepeat += stationCallsign;
            packetToRepeat += "*";
        }
        packetToRepeat += APRSPacketLib::checkForStartingBytes(packet.substring(suffixIndex));
        return packetToRepeat;
    }

    String generateDigipeatedPacket(const String& packet){
        String temp;
        if (lastAprsPacket.header != "") {   // thirdparty : only header is used
            const String& header = packet.substring(0, packet.indexOf(":}"));
            temp = header.substring(header.indexOf(">") + 1);
        } else {
            temp = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        }
        int commaIndex      = temp.indexOf(",");
        int digiMode        = Config.digi.mode;
        bool crossFreq      = abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000;   // CrossFreq Digi

        if (commaIndex > 2) {   // "path" found
            const String& path  = temp.substring(commaIndex + 1);
            if (digiMode == 1 || backupDigiMode) {
                bool hasWide = pathTokenIndex(path, "WIDE1-1") != -1;
                if (hasWide || crossFreq) {
                    return buildPacket(path, packet, !hasWide);
                }
                return "";
            }
            if (digiMode == 2) {
                int wide1Index = pathTokenIndex(path, "WIDE1-1");
                bool hasWide1 = wide1Index != -1;
                RegionalHop regionalHop = findRegionalHop(path);

                if (hasWide1 && regionalHop.found && regionalHop.tokenStart < wide1Index) return ""; // fill-in must come first

                if (hasWide1 || regionalHop.found) return buildPacket(path, packet, false);

                if (crossFreq) return buildPacket(path, packet, true);                  // CrossFreq (without WIDE)

                return "";
            }
            if (digiMode == 3) {
                String stationCallsign  = (Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign);
                bool containsOwnCall    = path.indexOf(stationCallsign) != -1;
                if (containsOwnCall) return buildPacket(path, packet, false);
                return "";
            }
            return "";
        }

        if (commaIndex == -1 && (digiMode == 1 || backupDigiMode || digiMode == 2) && crossFreq) return buildPacket("", packet, true);  // no "path" but is CrossFreq Digi

        return "";
    }

    void processLoRaPacket(const String& packet) {
        if (lastAprsPacket.path.indexOf("NOGATE") >= 0) return;

        String temp = (lastAprsPacket.header != "") ? packet.substring(packet.indexOf(":}") + 2) : packet.substring(3);

        String stationCallsign = Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign;
        if (lastAprsPacket.sender == stationCallsign) return;          // Avoid listening to self packets
        if (lastAprsPacket.header == "" && Config.tacticalCallsign == "" && !Utils::callsignIsValid(lastAprsPacket.sender)) return;  // No thirdParty + no tactical y no valid callsign

        if (STATION_Utils::isIn25SegHashBuffer(lastAprsPacket.sender, temp.substring(temp.indexOf(":") + 2))) return;

        STATION_Utils::updateLastHeard(lastAprsPacket.sender);
        Utils::updateLoRaPacketDisplayInfo(lastAprsPacket, 1);               // Digi
        bool queryMessage = false;
        if (lastAprsPacket.type == 1) {   // MESSAGE
            if (lastAprsPacket.addressee == stationCallsign) {     // it's a message for me!
                String AddresseeAndMessage = lastAprsPacket.addressee + ":" + lastAprsPacket.payload;
                queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(lastAprsPacket.sender, APRSPacketLib::checkForStartingBytes(AddresseeAndMessage));
            }
        }
        if (queryMessage) return;                   // answer should not be repeated.

        String loraPacket = generateDigipeatedPacket(packet.substring(3));
        if (loraPacket != "") {
            STATION_Utils::addToOutputPacketBuffer(loraPacket);
            if (Config.digi.ecoMode != 1) displayToggle(true);
            lastScreenOn = millis();
        }
    }

}
