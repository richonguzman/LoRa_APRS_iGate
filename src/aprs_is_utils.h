#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>

namespace APRS_IS_Utils {

void connect();
String checkStatus();
//void processSplitedMessage(String addressee, String message1, String message2);

}

#endif