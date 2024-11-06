#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>


namespace LoRa_Utils {

    void    setup();
    void    sendNewPacket(const String& newPacket);
    //String  packetSanitization(const String& packet);
    String  receivePacket();
    void    changeFreqTx();
    void    changeFreqRx();
    void    startReceive();
    void    sleepRadio();

}

#endif