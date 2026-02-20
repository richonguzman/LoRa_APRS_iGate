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

#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration    Config;
extern uint32_t         lastScreenOn;
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

    String buildPacket(const String& path, const String& packet, bool thirdParty, bool crossFreq) {
        String stationCallsign = (Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign);
        if (!crossFreq) {
            String packetToRepeat = packet.substring(0, packet.indexOf(",") + 1);
            String tempPath = path;
            int digiMode = Config.digi.mode;

            if (path.indexOf("WIDE1-1") != -1 && (digiMode == 2 || digiMode == 3)) {
                tempPath.replace("WIDE1-1", stationCallsign + "*");
            } else if (path.indexOf("WIDE2-") != -1 && digiMode == 3) {
                int wide1AsteriskIndex = path.indexOf(",WIDE1*");   // less memory than: tempPath.replace(",WIDE1*", "");
                if (wide1AsteriskIndex != -1) {
                    tempPath.remove(wide1AsteriskIndex, 7);
                }
                int asteriskIndex = path.indexOf("*");              // less memory than: tempPath.replace("*", "");
                if (asteriskIndex != -1) {
                    tempPath.remove(asteriskIndex, 1);
                }
                if (path.indexOf("WIDE2-1") != -1) {
                    tempPath.replace("WIDE2-1", stationCallsign + "*");
                } else if (path.indexOf("WIDE2-2") != -1) {
                    tempPath.replace("WIDE2-2", stationCallsign + "*,WIDE2-1");
                } else {
                    return "";
                }
            }
            packetToRepeat += tempPath;
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(thirdParty ? ":}" : ":")));
            return packetToRepeat;
        } else {   // CrossFreq Digipeater
            String suffix   = thirdParty ? ":}" : ":";
            int suffixIndex = packet.indexOf(suffix);
            String packetToRepeat = packet.substring(0, suffixIndex);

            String terms[] = {",WIDE1*", ",WIDE2*", "*"};
            for (String term : terms) {
                int index = packetToRepeat.indexOf(term);
                if (index != -1) {
                    packetToRepeat.remove(index, term.length());
                }
            }
            packetToRepeat += ",";
            packetToRepeat += stationCallsign;
            packetToRepeat += "*";
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(suffixIndex));
            return packetToRepeat;
        }
    }

    String generateDigipeatedPacket(const String& packet, bool thirdParty){
        String temp;
        if (thirdParty) { // only header is used
            const String& header = packet.substring(0, packet.indexOf(":}"));
            temp = header.substring(header.indexOf(">") + 1);
        } else {
            temp = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        }
        int commaIndex      = temp.indexOf(",");
        int digiMode        = Config.digi.mode;
        bool crossFreq      = abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000;   // CrossFreq Digi

        if (commaIndex > 2) {   // Packet has "path"
            const String& path  = temp.substring(commaIndex + 1);
            if (digiMode == 2 || backupDigiMode) {
                bool hasWide = path.indexOf("WIDE1-1") != -1;
                if (hasWide || crossFreq) {
                    return buildPacket(path, packet, thirdParty, !hasWide);
                }
                return "";
            }
            if (digiMode == 3) {
                int wide1Index = path.indexOf("WIDE1-1");
                int wide2Index = path.indexOf("WIDE2-");
                bool hasWide1 = wide1Index != -1;
                bool hasWide2 = wide2Index != -1;

                if (hasWide1 && hasWide2 && wide2Index < wide1Index) return "";                     // check that WIDE1 before WIDE2

                if (hasWide1 || hasWide2) return buildPacket(path, packet, thirdParty, false);      // regular APRS with WIDEn-N

                if (crossFreq) return buildPacket(path, packet, thirdParty, true);                  // CrossFreq (without WIDE)

                return "";
            }
            return "";
        }

        if (commaIndex == -1 && (digiMode == 2 || backupDigiMode || digiMode == 3) && crossFreq) return buildPacket("", packet, thirdParty, true);  // no "path" but is CrossFreq Digi

        return "";
    }

    void processLoRaPacket(const String& packet) {
        if (packet.indexOf("NOGATE") >= 0) return;

        bool thirdPartyPacket = false;
        String temp, Sender;
        int firstColonIndex = packet.indexOf(":");
        if (firstColonIndex > 5 && firstColonIndex < (packet.length() - 1) && packet[firstColonIndex + 1] == '}' && packet.indexOf("TCPIP") > 0) {   // 3rd Party
            thirdPartyPacket = true;
            temp    = packet.substring(packet.indexOf(":}") + 2);
            Sender  = temp.substring(0, temp.indexOf(">"));
        } else {
            temp    = packet.substring(3);
            Sender  = packet.substring(3, packet.indexOf(">"));
        }

        String stationCallsign = Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign;
        if (Sender == stationCallsign) return;          // Avoid listening to self packets
        if (!thirdPartyPacket && Config.tacticalCallsign == "" && !Utils::callsignIsValid(Sender)) return;  // No thirdParty + no tactical y no valid callsign

        if (STATION_Utils::check25SegBuffer(Sender, temp.substring(temp.indexOf(":") + 2))) {
            STATION_Utils::updateLastHeard(Sender);
            Utils::typeOfPacket(temp, 2);               // Digi
            bool queryMessage = false;
            int doubleColonIndex = temp.indexOf("::");
            if (doubleColonIndex > 10) {                // it's a message
                String AddresseeAndMessage  = temp.substring(doubleColonIndex + 2);
                String Addressee            = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                Addressee.trim();
                if (Addressee == stationCallsign) {     // it's a message for me!
                    queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage, thirdPartyPacket);
                }
            }
            if (!queryMessage) {
                String loraPacket = generateDigipeatedPacket(packet.substring(3), thirdPartyPacket);
                if (loraPacket != "") {
                    STATION_Utils::addToOutputPacketBuffer(loraPacket);
                    if (Config.digi.ecoMode != 1) displayToggle(true);
                    lastScreenOn = millis();
                }
            }
        }
    }

}