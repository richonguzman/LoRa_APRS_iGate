#ifndef SYSLOG_H_
#define SYSLOG_H_

#include <Arduino.h>


namespace SYSLOG_Utils {

    void log(uint8_t type ,const String& packet, int rssi, float snr, int freqError);
    void setup();

}

#endif