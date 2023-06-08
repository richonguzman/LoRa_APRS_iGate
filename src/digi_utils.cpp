#include "digi_utils.h"
#include "configuration.h"
#include "lora_utils.h"


extern Configuration    Config;
extern String           thirdLine;
extern String           fourthLine;
extern int              stationMode;

namespace DIGI_Utils {

void typeOfPacket(String packet) {
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
}

void process(String packet) {
    String firstPart, lastPart;
    if (packet != "") {
        Serial.print("Received Lora Packet   : " + String(packet));
        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("WIDE1-1") > 10)) { // confirmar lo de WIDE1-1 !!!
            Serial.println("   ---> APRS LoRa Packet");
            typeOfPacket(packet);
            firstPart = packet.substring(3,packet.indexOf(",")+1);
            lastPart = packet.substring(packet.indexOf(":"));
            Serial.println(firstPart + Config.callsign + lastPart);
            delay(500);
            if (stationMode == 4) {     // Digirepeating with Freq Rx !=  Tx
                LoRa_Utils::changeFreqTx();
            }
            LoRa_Utils::sendNewPacket("APRS", firstPart + Config.callsign + lastPart);
            if (stationMode == 4) {
                LoRa_Utils::changeFreqRx();
            }
        } else {
            Serial.println("   ---> LoRa Packet Ignored (first 3 bytes or NOGATE)\n");
        }
    }
}

}