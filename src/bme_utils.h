#ifndef BME_UTILS_H_
#define BME_UTILS_H_

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include "Adafruit_Si7021.h"
#include <Arduino.h>


namespace BME_Utils {

    void  getWxModuleAddres();
    void  setup();
    String generateTempString(const float bmeTemp);
    String generateHumString(const float bmeHum);
    String generatePresString(const float bmePress);
    String readDataSensor();

}

#endif