#ifndef BME_UTILS_H_
#define BME_UTILS_H_

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

namespace BME_Utils {

void setup();
String generateTempString(float bmeTemp);
String generateHumString(float bmeHum);
String generatePresString(float bmePress);
String readDataSensor();

}

#endif