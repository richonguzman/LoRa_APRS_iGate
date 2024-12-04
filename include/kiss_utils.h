#ifndef KISS_UTILS_H_
#define KISS_UTILS_H_

#include <Arduino.h>


String encodeAddressAX25(String tnc2Address);
String decodeAddressAX25(const String& ax25Address, bool& isLast, bool isRelay);

String encapsulateKISS(const String& ax25Frame, uint8_t cmd);
String decapsulateKISS(const String& frame);

String encodeKISS(const String& frame);
String decodeKISS(const String& inputFrame, bool& dataFrame);

#endif