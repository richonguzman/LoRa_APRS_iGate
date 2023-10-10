#ifndef BME_UTILS_H_
#define BME_UTILS_H_

#include <Arduino.h>
#include <Adafruit_Sensor.h>

//#define BMPSensor // uncoment this line if BMP280 Module is connected instead of BME280

#ifndef BMPSensor
#include <Adafruit_BME280.h>
#else
#include <Adafruit_BMP280.h>
#endif

namespace BME_Utils {

void setup();
String generateTempString(float bmeTemp);
String generateHumString(float bmeHum);
String generatePresString(float bmePress);
String readDataSensor();

}

#endif