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

#ifndef MAP_UTILS_H_
#define MAP_UTILS_H_

#include <Arduino.h>
#include <vector>


struct MapStation {
    char     callsign[10];      // "XX9XXX-NN" + null  -> clave para deduplicar
    float    latitude;          // grados decimales (+N / -S)
    float    longitude;         // grados decimales (+E / -W)
    char     symbol[3];         // tabla + codigo APRS (ej "/>"), "" si no hay
    int16_t  rssi;              // ultimo RSSI
    float    snr;               // ultimo SNR
    uint16_t count;             // nro de paquetes oidos de esa estacion
    char     lastHeard[10];     // "HH:MM:SS" si hay NTP valido, "" si no
    uint32_t lastHeardMillis;   // millis() -> ordenar / expirar (no se muestra)
};

namespace MAP_Utils {

    void    upsert(const String& callsign, float latitude, float longitude, const String& symbol, int rssi, float snr);
    String  getStationsJson();

}

#endif
