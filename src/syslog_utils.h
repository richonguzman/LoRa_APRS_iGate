#ifndef SYSLOG_H_
#define SYSLOG_H_

#include <Arduino.h>

namespace SYSLOG_Utils {

void log(String type ,String packet, int rssi, float snr, int freqError);
void setup();

}

#endif