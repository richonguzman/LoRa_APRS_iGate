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

#ifndef SERIAL_PORTS_H_
#define SERIAL_PORTS_H_

#include <Arduino.h>
#include "board_pinout.h"

// ============================================================================
// SERIAL CONFIGURATION MACROS
// ============================================================================
// Configure which serial port to use for debug and GPS
// 
// DEBUG_SERIAL_INDEX: 
//   0  = Use Serial (hardware serial 0) for debug
//   1  = Use Serial1 (hardware serial 1) for debug
//  -1  = Debug completely disabled (all debug macros compile to nothing)
//
// GPS_SERIAL_INDEX:
//   0  = Use Serial (hardware serial 0) for GPS
//   1  = Use Serial1 (hardware serial 1) for GPS
//
// ============================================================================

// Default values
#ifndef DEBUG_SERIAL_INDEX
    #define DEBUG_SERIAL_INDEX 0   // Default: debug on Serial (hardware serial 0)
#endif

#ifdef HAS_GPS
    #ifndef GPS_SERIAL_INDEX
        #define GPS_SERIAL_INDEX 1     // Default: GPS on Serial1 (hardware serial 1)
    #endif
#endif

#ifdef HAS_A7670
    #ifndef A7670_SERIAL_INDEX
        #define A7670_SERIAL_INDEX 1     // Default: A7670 on Serial1 (hardware serial 1)
    #endif
#endif

// ============================================================================
// Validation: Ensure debug, GPS and A7670 don't use the same serial port
// ============================================================================
#ifdef HAS_GPS
    #if DEBUG_SERIAL_INDEX == GPS_SERIAL_INDEX
        #error "DEBUG_SERIAL_INDEX and GPS_SERIAL_INDEX cannot be the same index"
    #endif
#endif
#ifdef HAS_A7670
    #if DEBUG_SERIAL_INDEX == A7670_SERIAL_INDEX
        #error "DEBUG_SERIAL_INDEX and A7670_SERIAL_INDEX cannot be the same index"
    #endif
#endif
#if defined(HAS_GPS) && defined(HAS_A7670)
    #if GPS_SERIAL_INDEX == A7670_SERIAL_INDEX
        #error "GPS_SERIAL_INDEX and A7670_SERIAL_INDEX cannot be the same index"
    #endif
#endif

// ============================================================================
// Debug Serial Object Selection
// ============================================================================
#if DEBUG_SERIAL_INDEX >= 0 && (DEBUG_SERIAL_INDEX == 0 || DEBUG_SERIAL_INDEX == 1)
    #define DEBUG_SERIAL debugSerial
    #define DEBUG_SERIAL_OBJ debugSerial
#else
    // Debug disabled - create dummy defines that do nothing
    #define DEBUG_SERIAL Serial  // Won't be used, but prevents compilation errors
#endif

// ============================================================================
// GPS Serial Object Selection
// ============================================================================
#ifdef HAS_GPS
    #if GPS_SERIAL_INDEX == 0 || GPS_SERIAL_INDEX == 1
        #define GPS_SERIAL gpsSerial
        #define GPS_SERIAL_OBJ gpsSerial
    #else
        #error "GPS_SERIAL_INDEX must be 0 or 1"
    #endif
#endif

// ============================================================================
// A7670 Modem Serial Object Selection
// ============================================================================
#ifdef HAS_A7670
    #if A7670_SERIAL_INDEX == 0 || A7670_SERIAL_INDEX == 1
        #define A7670_SERIAL SerialAT
        #define A7670_SERIAL_OBJ SerialAT
    #else
        #error "A7670_SERIAL_INDEX must be 0 or 1"
    #endif
#endif

// ============================================================================
// Debug Macros
// ============================================================================
#if DEBUG_SERIAL_INDEX >= 0
    // Debug enabled - all macros forward to the selected serial port
    #define DEBUG_BEGIN(baud) DEBUG_SERIAL.begin(baud)
    #define DEBUG_PRINT(x) DEBUG_SERIAL.print(x)
    #define DEBUG_PRINTLN(x) DEBUG_SERIAL.println(x)
    #define DEBUG_PRINTF(format, ...) DEBUG_SERIAL.printf(format, ##__VA_ARGS__)
    #define DEBUG_AVAILABLE() DEBUG_SERIAL.available()
    #define DEBUG_READ() DEBUG_SERIAL.read()
    #define DEBUG_FLUSH() DEBUG_SERIAL.flush()
    #define DEBUG_WRITE(x) DEBUG_SERIAL.write(x)
    #define DEBUG_END() DEBUG_SERIAL.end()
#else
    // Debug disabled - all macros compile to nothing (zero overhead)
    #define DEBUG_BEGIN(baud)
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(format, ...)
    #define DEBUG_AVAILABLE() 0
    #define DEBUG_READ() -1
    #define DEBUG_FLUSH()
    #define DEBUG_WRITE(x) 0
    #define DEBUG_END()
#endif

// ============================================================================
// Serial Object Declarations (definitions in serial_ports.cpp)
// ============================================================================

#if DEBUG_SERIAL_INDEX == 0
    extern HardwareSerial& debugSerial;
#elif DEBUG_SERIAL_INDEX == 1
    extern HardwareSerial debugSerial;
#endif

#ifdef HAS_GPS
    #if GPS_SERIAL_INDEX == 0
        extern HardwareSerial& gpsSerial;
    #else
        extern HardwareSerial gpsSerial;
    #endif
#endif

#ifdef HAS_A7670
    #if A7670_SERIAL_INDEX == 0
        extern HardwareSerial& SerialAT;
    #else
        extern HardwareSerial SerialAT;
    #endif
#endif

#endif // SERIAL_PORTS_H_
