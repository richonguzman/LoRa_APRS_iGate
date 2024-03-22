#ifndef TNC_UTILS_H_
#define TNC_UTILS_H_

#include <Arduino.h>

namespace TNC_Utils {

    void setup();
    void loop();
    
    void sendToClients(String packet);
    void sendToSerial(String packet);

}

#endif