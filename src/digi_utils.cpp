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
extern bool             backUpDigiMode;

namespace DIGI_Utils {

    // OPTIMIERUNG: String Reserve und weniger Substring-Calls
    String buildPacket(const String& path, const String& packet, bool thirdParty, bool crossFreq) {
        String packetToRepeat;
        packetToRepeat.reserve(packet.length() + 20);
        
        if (!crossFreq) {
            int commaPos = packet.indexOf(",");
            if (commaPos == -1) return "";
            
            packetToRepeat = packet.substring(0, commaPos + 1);
            String tempPath = path;

            if (path.indexOf("WIDE1-1") != -1 && (Config.digi.mode == 2 || Config.digi.mode == 3)) {
                tempPath.replace("WIDE1-1", Config.callsign + "*");
            } else if (path.indexOf("WIDE2-") != -1 && Config.digi.mode == 3) {
                if (path.indexOf(",WIDE1*") != -1) {
                    tempPath.remove(path.indexOf(",WIDE1*"), 7);
                }
                if (path.indexOf("*") != -1) {
                    tempPath.remove(path.indexOf("*"), 1);
                }
                if (path.indexOf("WIDE2-1") != -1) {
                    tempPath.replace("WIDE2-1", Config.callsign + "*");
                } else if (path.indexOf("WIDE2-2") != -1) {
                    tempPath.replace("WIDE2-2", Config.callsign + "*,WIDE2-1");
                } else {
                    return "";
                }
            }
            
            packetToRepeat += tempPath;
            
            if (thirdParty) {
                int colonPos = packet.indexOf(":}");
                if (colonPos != -1) {
                    packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(colonPos));
                }
            } else {
                int colonPos = packet.indexOf(":");
                if (colonPos != -1) {
                    packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(colonPos));
                }
            }
            
            return packetToRepeat;
            
        } else {
            // CrossFreq Digipeater
            String suffix = thirdParty ? ":}" : ":";
            int suffixPos = packet.indexOf(suffix);
            if (suffixPos == -1) return "";
            
            packetToRepeat = packet.substring(0, suffixPos);
            
            // OPTIMIERUNG: Array-basierter String-Remove
            const char* terms[] = {",WIDE1*", ",WIDE2*", "*"};
            for (const char* term : terms) {
                int index = packetToRepeat.indexOf(term);
                if (index != -1) {
                    packetToRepeat.remove(index, strlen(term));
                }
            }
            
            packetToRepeat += ",";
            packetToRepeat += Config.callsign;
            packetToRepeat += "*";
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(suffixPos));
            
            return packetToRepeat;
        }
    }

    String generateDigipeatedPacket(const String& packet, bool thirdParty) {
        String temp;
        temp.reserve(packet.length());
        
        if (thirdParty) {
            int colonPos = packet.indexOf(":}");
            if (colonPos == -1) return "";
            
            const String& header = packet.substring(0, colonPos);
            int gtPos = header.indexOf(">");
            if (gtPos == -1) return "";
            
            temp = header.substring(gtPos + 1);
        } else {
            int gtPos = packet.indexOf(">");
            int colonPos = packet.indexOf(":");
            if (gtPos == -1 || colonPos == -1) return "";
            
            temp = packet.substring(gtPos + 1, colonPos);
        }
        
        int commaPos = temp.indexOf(",");
        if (commaPos <= 2) return "";
        
        const String& path = temp.substring(commaPos + 1);
        
        if (Config.digi.mode == 2 || backUpDigiMode) {
            if (path.indexOf("WIDE1-1") != -1) {
                return buildPacket(path, packet, thirdParty, false);
            } else if (path.indexOf("WIDE1-1") == -1 && 
                      (abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000)) {
                return buildPacket(path, packet, thirdParty, true);
            }
        } else if (Config.digi.mode == 3) {
            if (path.indexOf("WIDE1-1") != -1 || path.indexOf("WIDE2-") != -1) {
                return buildPacket(path, packet, thirdParty, false);
            } else if (path.indexOf("WIDE1-1") == -1 && path.indexOf("WIDE2-") == -1 && 
                      (abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000)) {
                return buildPacket(path, packet, thirdParty, true);
            }
        }
        
        return "";
    }

    void processLoRaPacket(const String& packet) {
        if (packet.indexOf("RFONLY") == -1 && packet.indexOf("NOGATE") == -1) {
            bool thirdPartyPacket = false;
            String temp, Sender;
            
            int firstColonIndex = packet.indexOf(":");
            
            if (firstColonIndex > 5 && firstColonIndex < (packet.length() - 1) && 
                packet[firstColonIndex + 1] == '}' && packet.indexOf("TCPIP") > 0) {
                // 3rd Party
                thirdPartyPacket = true;
                int thirdPartyStart = packet.indexOf(":}");
                if (thirdPartyStart == -1) return;
                
                temp = packet.substring(thirdPartyStart + 2);
                int gtPos = temp.indexOf(">");
                if (gtPos == -1) return;
                
                Sender = temp.substring(0, gtPos);
            } else {
                temp = packet.substring(3);
                int gtPos = packet.indexOf(">");
                if (gtPos < 3) return;
                
                Sender = packet.substring(3, gtPos);
            }
            
            if (Sender != Config.callsign) {
                if (!thirdPartyPacket && !Utils::checkValidCallsign(Sender)) {
                    return;
                }
                
                int colonPos = temp.indexOf(":");
                if (colonPos == -1) return;
                
                String payload = temp.substring(colonPos + 2);
                
                if (STATION_Utils::check25SegBuffer(Sender, payload)) {
                    STATION_Utils::updateLastHeard(Sender);
                    Utils::typeOfPacket(temp, 2);
                    
                    bool queryMessage = false;
                    
                    if (temp.indexOf("::") > 10) {
                        int doubleColonPos = temp.indexOf("::");
                        String AddresseeAndMessage = temp.substring(doubleColonPos + 2);
                        int colonPos = AddresseeAndMessage.indexOf(":");
                        if (colonPos == -1) return;
                        
                        String Addressee = AddresseeAndMessage.substring(0, colonPos);
                        Addressee.trim();
                        
                        if (Addressee == Config.callsign) {
                            queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage, thirdPartyPacket);
                        }
                    }
                    
                    if (!queryMessage) {
                        String loraPacket = generateDigipeatedPacket(packet.substring(3), thirdPartyPacket);
                        if (loraPacket != "") {
                            STATION_Utils::addToOutputPacketBuffer(loraPacket);
                            if (Config.digi.ecoMode != 1) {
                                displayToggle(true);
                            }
                            lastScreenOn = millis();
                        }
                    }
                }
            }
        }
    }

    void checkEcoMode() {
        // EcoMode check logic
    }

}
