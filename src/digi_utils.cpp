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

    static int pathTokenIndex(const String& path, const String& token) {
        unsigned int start = 0;
        while (start < path.length()) {
            int end = path.indexOf(",", start);
            if (end == -1) end = path.length();
            if (path.substring(start, end) == token) return start;
            start = end + 1;
        }
        return -1;
    }

    String processMode3Path(const String& path, const String& stationCallsign) {
        int start = 0;
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

            if (tempPath.indexOf("WIDE1-1") != -1 && (digiMode == 1 || digiMode == 2)) {    // WIDE1-1
                if (tempPath.indexOf("*") != -1 ) return "";                                // "*" shouldn't be in WIDE1-1 (only) type of packet
                tempPath.replace("WIDE1-1", stationCallsign + "*");
            } else if (tempPath.indexOf("WIDE2-") != -1 && digiMode == 2) {                 // WIDE2-n Digipeater
                tempPath = cleanPath(path);
                int idx = pathTokenIndex(tempPath, "WIDE2-1");
                if (idx != -1) {
                    tempPath = tempPath.substring(0, idx) + stationCallsign + "*" + tempPath.substring(idx + 7);
                } else {
                    idx = pathTokenIndex(tempPath, "WIDE2-2");
                    if (idx == -1) return "";
                    tempPath = tempPath.substring(0, idx) + stationCallsign + "*,WIDE2-1" + tempPath.substring(idx + 7);
                }
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
                bool hasWide = path.indexOf("WIDE1-1") != -1;
                if (hasWide || crossFreq) {
                    return buildPacket(path, packet, !hasWide);
                }
                return "";
            }
            if (digiMode == 2) {
                int wide1Index = path.indexOf("WIDE1-1");
                int wide2Index = path.indexOf("WIDE2-");
                bool hasWide1 = wide1Index != -1;
                bool hasWide2 = wide2Index != -1;

                if (hasWide1 && hasWide2 && wide2Index < wide1Index) return "";                     // check that WIDE1 before WIDE2

                if (hasWide1 || hasWide2) return buildPacket(path, packet, false);      // regular APRS with WIDEn-N

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
        Utils::typeOfPacket(temp, 2);               // Digi
        bool queryMessage = false;
        if (lastAprsPacket.type == 1) {   // MESSAGE
            if (lastAprsPacket.addressee == stationCallsign) {     // it's a message for me!
                String AddresseeAndMessage = lastAprsPacket.addressee + ":" + lastAprsPacket.payload;
                queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(lastAprsPacket.sender, AddresseeAndMessage);
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