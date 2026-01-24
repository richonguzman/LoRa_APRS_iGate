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

#include "serial_ports.h"

// Debug Serial Object Definition (single translation unit to avoid multiple definition)
#if DEBUG_SERIAL_INDEX == 0
    HardwareSerial& debugSerial = Serial;
#elif DEBUG_SERIAL_INDEX == 1
    HardwareSerial debugSerial(1);
#endif

// GPS Serial Object Definition
#ifdef HAS_GPS
    #if GPS_SERIAL_INDEX == 0
        HardwareSerial& gpsSerial = Serial;
    #else
        HardwareSerial gpsSerial(1);
    #endif
#endif

// A7670 Modem Serial Definition
#ifdef HAS_A7670
    #if A7670_SERIAL_INDEX == 0
        HardwareSerial& SerialAT = Serial;
    #else
        HardwareSerial SerialAT(1);
    #endif
#endif
