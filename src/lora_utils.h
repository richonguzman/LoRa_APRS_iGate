#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

namespace LoRaUtils {

void setup();
void sendNewPacket(const String &typeOfMessage, const String &newPacket);
//String receivePacket();

}
#endif