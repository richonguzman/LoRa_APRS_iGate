#ifndef BME_UTILS_H_
#define BME_UTILS_H_

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include <Arduino.h>


namespace BME_Utils {

    void  getWxModuleAddres();
    void  setup();
    const String generateTempString(const float bmeTemp);
    const String generateHumString(const float bmeHum);
    const String generatePresString(const float bmePress);
    const String readDataSensor();

}

#endif