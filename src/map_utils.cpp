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

#include <ArduinoJson.h>
#include "map_utils.h"
#include "ntp_utils.h"

#define MAX_MAP_STATIONS    50
#define STATION_TTL_MS      3600000UL   // 1 hora (en milisegundos)


std::vector<MapStation> mapStations;


namespace MAP_Utils {

    // Copia la hora NTP "HH:MM:SS" si es valida; si no hay red (NTP devuelve
    // un texto de fallback) deja el campo vacio para no guardar hora falsa.
    void writeFormatedTime(char* dst, size_t size) {
        String t = NTP_Utils::getFormatedTime();
        if (t.length() == 8 && t.charAt(2) == ':') {
            t.toCharArray(dst, size);
        } else {
            dst[0] = '\0';
        }
    }

    // Inserta una estacion nueva o actualiza la existente (dedup por callsign).
    // Llamar solo cuando el parser ya decodifico una posicion valida.
    void upsert(const String& callsign, float latitude, float longitude, const String& symbol, int rssi, float snr) {
        for (auto &s : mapStations) {                       // 1) ya existe -> actualizar
            if (callsign.equals(s.callsign)) {
                s.latitude  = latitude;
                s.longitude = longitude;
                s.rssi      = rssi;
                s.snr       = snr;
                s.count++;
                symbol.toCharArray(s.symbol, sizeof(s.symbol));
                writeFormatedTime(s.lastHeard, sizeof(s.lastHeard));
                s.lastHeardMillis = millis();
                return;
            }
        }

        if (mapStations.size() >= MAX_MAP_STATIONS) {       // 2) llena -> descartar la mas vieja
            size_t oldest = 0;
            for (size_t i = 1; i < mapStations.size(); i++) {
                if (mapStations[i].lastHeardMillis < mapStations[oldest].lastHeardMillis) {
                    oldest = i;
                }
            }
            mapStations.erase(mapStations.begin() + oldest);
        }

        MapStation st = {};                                 // 3) insertar nueva
        callsign.toCharArray(st.callsign, sizeof(st.callsign));
        symbol.toCharArray(st.symbol, sizeof(st.symbol));
        st.latitude  = latitude;
        st.longitude = longitude;
        st.rssi      = rssi;
        st.snr       = snr;
        st.count     = 1;
        writeFormatedTime(st.lastHeard, sizeof(st.lastHeard));
        st.lastHeardMillis = millis();
        mapStations.push_back(st);
    }

    void purgeOldStations() {
        uint32_t now = millis();
        for (int i = (int)mapStations.size() - 1; i >= 0; i--) {
            if (now - mapStations[i].lastHeardMillis > STATION_TTL_MS) {
                mapStations.erase(mapStations.begin() + i);
            }
        }
    }

    String getStationsJson() {
        purgeOldStations();
        JsonDocument data;

        for (size_t i = 0; i < mapStations.size(); i++) {
            data[i]["callsign"]  = mapStations[i].callsign;
            data[i]["lat"]       = mapStations[i].latitude;
            data[i]["lon"]       = mapStations[i].longitude;
            data[i]["symbol"]    = mapStations[i].symbol;
            data[i]["RSSI"]      = mapStations[i].rssi;
            data[i]["SNR"]       = mapStations[i].snr;
            data[i]["count"]     = mapStations[i].count;
            data[i]["lastHeard"] = mapStations[i].lastHeard;
        }

        String buffer;

        serializeJson(data, buffer);

        return buffer;
    }

}