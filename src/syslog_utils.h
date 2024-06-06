#ifndef SYSLOG_H_
#define SYSLOG_H_

#include <Arduino.h>


namespace SYSLOG_Utils {

    void log(const uint8_t type ,const String& packet, const int rssi, const float snr, const int freqError);
    void setup();

}

#endif