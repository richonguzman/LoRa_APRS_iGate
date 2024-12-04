#ifndef WX_UTILS_H_
#define WX_UTILS_H_

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include "Adafruit_Si7021.h"
#include <Arduino.h>


namespace WX_Utils {

    void    getWxModuleAddres();
    void    setup();
    String  generateTempString(const float sensorTemp);
    String  generateHumString(const float sensorHum);
    String  generatePresString(const float sensorPres);
    String  readDataSensor();

}

#endif