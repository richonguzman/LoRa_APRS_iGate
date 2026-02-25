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

#ifndef GPS_UTILS_H_
#define GPS_UTILS_H_

#include <Arduino.h>


namespace GPS_Utils {

    String  getiGateLoRaBeaconPacket();
    char    *ax25_base91enc(char *s, uint8_t n, uint32_t v);
    String  encodeGPS(float latitude, float longitude, const String& overlay, const String& symbol);
    void    generateBeaconFirstPart();
    void    generateBeacons();
    String  decodeEncodedGPS(const String& packet);
    String  getReceivedGPS(const String& packet);
    String  getDistanceAndComment(const String& packet);

    void    setup();
    void    getData();

}

#endif