#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>
#include <TimeLib.h>

namespace utils {

char *ax25_base91enc(char *s, uint8_t n, uint32_t v);
String createDateString(time_t t);
String createTimeString(time_t t);

}
#endif