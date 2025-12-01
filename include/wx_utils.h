/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WX_UTILS_H_
#define WX_UTILS_H_

#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h> 
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