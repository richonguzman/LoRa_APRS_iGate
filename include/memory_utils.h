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

#ifndef MEMORY_UTILS_H_
#define MEMORY_UTILS_H_

#include <Arduino.h>

// String-Pool für häufig verwendete APRS-Strings
// Spart Heap-Speicher durch PROGMEM
namespace StringPool {
    // APRS Path Strings
    extern const char WIDE1_1[] PROGMEM;
    extern const char WIDE2_1[] PROGMEM;
    extern const char WIDE2_2[] PROGMEM;
    extern const char TCPIP[] PROGMEM;
    extern const char RFONLY[] PROGMEM;
    extern const char NOGATE[] PROGMEM;
    
    // APRS Packet Types
    extern const char TYPE_MESSAGE[] PROGMEM;
    extern const char TYPE_BEACON[] PROGMEM;
    extern const char TYPE_TELEMETRY[] PROGMEM;
    extern const char TYPE_WX[] PROGMEM;
    extern const char TYPE_OBJECT[] PROGMEM;
    extern const char TYPE_STATUS[] PROGMEM;
    extern const char TYPE_MICE[] PROGMEM;
    
    // Helper functions
    inline String getWIDE1_1() { return FPSTR(WIDE1_1); }
    inline String getWIDE2_1() { return FPSTR(WIDE2_1); }
    inline String getWIDE2_2() { return FPSTR(WIDE2_2); }
    inline String getTCPIP() { return FPSTR(TCPIP); }
    inline String getRFONLY() { return FPSTR(RFONLY); }
    inline String getNOGATE() { return FPSTR(NOGATE); }
}

// Memory monitoring utilities
namespace MemoryUtils {
    void printHeapStats();
    size_t getFreeHeap();
    size_t getMinFreeHeap();
    size_t getMaxAllocHeap();
    void logMemoryUsage(const char* location);
}

#endif
