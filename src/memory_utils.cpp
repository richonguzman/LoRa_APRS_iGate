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

#include "memory_utils.h"
#include <esp_heap_caps.h>

// String Pool Definitionen
namespace StringPool {
    const char WIDE1_1[] PROGMEM = "WIDE1-1";
    const char WIDE2_1[] PROGMEM = "WIDE2-1";
    const char WIDE2_2[] PROGMEM = "WIDE2-2";
    const char TCPIP[] PROGMEM = "TCPIP";
    const char RFONLY[] PROGMEM = "RFONLY";
    const char NOGATE[] PROGMEM = "NOGATE";
    
    const char TYPE_MESSAGE[] PROGMEM = "MESSAGE";
    const char TYPE_BEACON[] PROGMEM = "GPS";
    const char TYPE_TELEMETRY[] PROGMEM = "TELEMETRY";
    const char TYPE_WX[] PROGMEM = "WX";
    const char TYPE_OBJECT[] PROGMEM = "OBJECT";
    const char TYPE_STATUS[] PROGMEM = "STATUS";
    const char TYPE_MICE[] PROGMEM = "MIC-E";
}

namespace MemoryUtils {

    void printHeapStats() {
        Serial.println("\n=== Heap Statistics ===");
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        
        Serial.print("Min Free Heap: ");
        Serial.print(ESP.getMinFreeHeap());
        Serial.println(" bytes");
        
        Serial.print("Max Alloc Heap: ");
        Serial.print(ESP.getMaxAllocHeap());
        Serial.println(" bytes");
        
        Serial.print("Heap Size: ");
        Serial.print(ESP.getHeapSize());
        Serial.println(" bytes");
        
        Serial.print("PSRAM Free: ");
        Serial.print(ESP.getFreePsram());
        Serial.println(" bytes");
        
        Serial.println("=====================\n");
    }

    size_t getFreeHeap() {
        return ESP.getFreeHeap();
    }

    size_t getMinFreeHeap() {
        return ESP.getMinFreeHeap();
    }

    size_t getMaxAllocHeap() {
        return ESP.getMaxAllocHeap();
    }

    void logMemoryUsage(const char* location) {
        Serial.print("[");
        Serial.print(location);
        Serial.print("] Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(" bytes | Min: ");
        Serial.print(ESP.getMinFreeHeap());
        Serial.println(" bytes");
    }

}
