/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
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

#ifndef STATION_UTILS_H_
#define STATION_UTILS_H_


#include <Arduino.h>


struct LastHeardStation {
    uint32_t    lastHeardTime;
    String      station;
};

namespace STATION_Utils {

    void loadBlacklistAndManagers();
    bool isBlacklisted(const String& callsign);
    bool isManager(const String& callsign);
    bool checkObjectTime(const String& packet);
    void deleteNotHeard();
    void updateLastHeard(const String& station);
    bool wasHeard(const String& station);
    bool isIn25SegHashBuffer(const String& station, const String& textMessage);
    void processOutputPacketBufferUltraEcoMode();
    void processOutputPacketBuffer();
    // eligibleForRxt defaults to false: RXT must only ever be attached to a
    // packet that is genuinely relaying a frame just received by this
    // station's own LoRa receiver. Every other origin (self-beacon,
    // telemetry, APRS-IS-to-RF conversion, local TNC client traffic, query/
    // command responses) is self-generated content with no real RF
    // reception behind it, and must stay false -- the single known-safe
    // call site (digi_utils.cpp's genuine digipeat relay) is the only one
    // that should ever pass true explicitly.
    void addToOutputPacketBuffer(const String& packet, bool flag = false, bool eligibleForRxt = false);

}

#endif
