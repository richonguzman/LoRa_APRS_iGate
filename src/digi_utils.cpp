#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "query_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
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

    String generateDigiRepeatedPacket(const String& packet){
        String sender, temp0, tocall, path;
        sender = packet.substring(0, packet.indexOf(">"));
        temp0 = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        if (temp0.indexOf(",") > 2) {
            tocall = temp0.substring(0, temp0.indexOf(","));
            path = temp0.substring(temp0.indexOf(",") + 1, temp0.indexOf(":"));
            if (path.indexOf("WIDE1-") >= 0) {
                String hop = path.substring(path.indexOf("WIDE1-") + 6, path.indexOf("WIDE1-") + 7);
                if (hop.toInt() >= 1 && hop.toInt() <= 7) {
                    if (hop.toInt() == 1) {
                        path.replace("WIDE1-1", Config.callsign + "*");
                    }
                    else {
                        path.replace("WIDE1-" + hop, Config.callsign + "*,WIDE1-" + String(hop.toInt() - 1));
                    }

                    String repeatedPacket = sender;
                    repeatedPacket += ">";
                    repeatedPacket += tocall;
                    repeatedPacket += ",";
                    repeatedPacket += path;
                    repeatedPacket += packet.substring(packet.indexOf(":"));
                    return repeatedPacket;
                }
                else {
                    return "";
                }
            }
            else {
                return "";
            }
        }
        else {
            return "";
        }
    }

    void processLoRaPacket(const String& packet) {
        bool queryMessage = false;
        String loraPacket, Sender, AddresseeAndMessage, Addressee;
        if (packet != "") {
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
                Sender = packet.substring(3, packet.indexOf(">"));
                if (Sender != Config.callsign) {
                    if (STATION_Utils::check25SegBuffer(Sender, packet.substring(packet.indexOf(":") + 2))) {
                        STATION_Utils::updateLastHeard(Sender);
                        Utils::typeOfPacket(packet.substring(3), 2);    // Digi
                        AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                        Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                        Addressee.trim();
                        if (packet.indexOf("::") > 10 && Addressee == Config.callsign) {      // its a message for me!
                            queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage);
                        }
                        if (!queryMessage && packet.indexOf("WIDE1-") > 10 && (Config.digi.mode == 2 || backUpDigiMode)) { // If should repeat packet (WIDE1 Digi)
                            loraPacket = generateDigiRepeatedPacket(packet.substring(3));
                            if (loraPacket != "") {
                                STATION_Utils::addToOutputPacketBuffer(loraPacket);
                                display_toggle(true);
                                lastScreenOn = millis();
                            }
                        }
                    }
                }
            }
        }
    }

}