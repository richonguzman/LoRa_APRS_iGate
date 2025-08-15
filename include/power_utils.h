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

#ifndef POWER_UTILS_H_
#define POWER_UTILS_H_

#include <Arduino.h>
#if defined(HAS_AXP192) || defined(HAS_AXP2101)
    #include "XPowersLib.h"
#else
    #include <Wire.h>
#endif


namespace POWER_Utils {

    #ifdef VEXT_CTRL
        void vext_ctrl_ON();
        void vext_ctrl_OFF();
    #endif

    #ifdef ADC_CTRL
        void adc_ctrl_ON();
        void adc_ctrl_OFF();
    #endif

    double  getBatteryVoltage();
    bool    isBatteryConnected();
    void    activateMeasurement();
    void    activateGPS();
    void    deactivateGPS();
    void    activateLoRa();
    void    deactivateLoRa();
    bool    begin(TwoWire &port);
    void    setup();

}

#endif