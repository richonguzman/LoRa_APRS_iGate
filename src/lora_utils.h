#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>

namespace LoRa_Utils {

void setup();
void sendNewPacket(const String &typeOfMessage, const String &newPacket);
String generatePacketMessage(String aprsisPacket);
String generatePacketSameContent(String aprsisPacket);
String receivePacket();
void changeFreqTx();
void changeFreqRx();

}

#endif