#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>


namespace APRS_IS_Utils {

    void upload(String line);
    void connect();
    void checkStatus();
    String buildPacketToUpload(String unprocessedPacket);
    String buildPacketToTx(String aprsisPacket);
    bool processReceivedLoRaMessage(String sender, String packet);
    void processLoRaPacket(String packet);
    void processAPRSISPacket(String packet);
    void listenAPRSIS();

}

#endif