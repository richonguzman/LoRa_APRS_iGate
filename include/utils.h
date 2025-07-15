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

#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>


class ReceivedPacket {
public:
    String  rxTime;
    String  packet;
    int     RSSI;
    float   SNR;
};

namespace Utils {

    void    processStatus();
    String  getLocalIP();
    void    setupDisplay();
    void    activeStations();
    void    checkBeaconInterval();
    void    checkDisplayInterval();
    void    validateFreqs();
    void    typeOfPacket(const String& packet, const uint8_t packetType);
    void    print(const String& text);
    void    println(const String& text);
    void    checkRebootMode();
    void    checkRebootTime();
    void    checkSleepByLowBatteryVoltage(uint8_t mode);
    bool    checkValidCallsign(const String& callsign);

}

#endif