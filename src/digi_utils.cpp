#include "configuration.h"
#include "station_utils.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern int              stationMode;
extern uint32_t         lastScreenOn;

namespace DIGI_Utils {

void processPacket(String packet) {
    String loraPacket;
    if (packet != "") {
        Serial.print("Received Lora Packet   : " + String(packet));
        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
            Serial.println("   ---> APRS LoRa Packet");
            String sender = packet.substring(3,packet.indexOf(">"));
            STATION_Utils::updateLastHeard(sender);
            Utils::typeOfPacket(packet, "Digi");
            if ((stationMode==3 || stationMode==5) && (packet.indexOf("WIDE1-1") > 10)) {
                loraPacket = packet.substring(3);
                loraPacket.replace("WIDE1-1", Config.callsign + "*");
                delay(500);
                LoRa_Utils::sendNewPacket("APRS", loraPacket);
                display_toggle(true);
                lastScreenOn = millis();
            } else if (stationMode ==4){
                if (packet.indexOf("WIDE1-1") == -1) {
                    loraPacket = packet.substring(3,packet.indexOf(":")) + "," + Config.callsign + "*" + packet.substring(packet.indexOf(":"));
                } else {
                    loraPacket = packet.substring(3,packet.indexOf(",")+1) + Config.callsign + "*" + packet.substring(packet.indexOf(","));
                }
                delay(500);
                if (stationMode == 4) {
                    LoRa_Utils::changeFreqTx();
                }
                LoRa_Utils::sendNewPacket("APRS", loraPacket);
                if (stationMode == 4) {
                    LoRa_Utils::changeFreqRx();
                }
                display_toggle(true);
                lastScreenOn = millis();
            }
        } else {
            Serial.println("   ---> LoRa Packet Ignored (first 3 bytes or NOGATE)\n");
        }
    }
}

}