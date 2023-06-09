#include "configuration.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
//extern String           thirdLine;
//extern String           fourthLine;
extern int              stationMode;
extern uint32_t         lastScreenOn;

namespace DIGI_Utils {

/*void typeOfPacket(String packet) {
    String Sender = packet.substring(3,packet.indexOf(">"));
    if (packet.indexOf("::") >= 10) {
        thirdLine = "Callsign = " + Sender;
        fourthLine = "TYPE ----> MESSAGE";
    } else if (packet.indexOf(":>") >= 10) {
        thirdLine = "Callsign = " + Sender;
        fourthLine = "TYPE ----> NEW STATUS";
    } else if (packet.indexOf(":!") >= 10 || packet.indexOf(":=") >= 10) {
        thirdLine = "Callsign = " + Sender;
        fourthLine = "TYPE ----> GPS BEACON";
    } else {
        thirdLine = "Callsign = " + Sender;
        fourthLine = "TYPE ----> ??????????";
    }
}*/

void processPacket(String packet) {
    String firstPart, lastPart, loraPacket;
    if (packet != "") {
        Serial.print("Received Lora Packet   : " + String(packet));
        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
            Serial.println("   ---> APRS LoRa Packet");
            if ((stationMode==3) && (packet.indexOf("WIDE1-1") > 10)) {
                utils::typeOfPacket(packet);
                firstPart = packet.substring(3,packet.indexOf(",")+1);
                lastPart = packet.substring(packet.indexOf(":"));
                loraPacket = firstPart + Config.callsign + "*" + lastPart;
                delay(500);
                LoRa_Utils::sendNewPacket("APRS", loraPacket);
                display_toggle(true);
                lastScreenOn = millis();
            } else { // stationMode = 4
                utils::typeOfPacket(packet);
                firstPart = packet.substring(3,packet.indexOf(",")+1);
                lastPart = packet.substring(packet.indexOf(",")+1);
                loraPacket = firstPart + Config.callsign + lastPart;  // se agrega "*"" ???
                delay(500);
                if (stationMode == 4) {     // Digirepeating with Freq Rx !=  Tx
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