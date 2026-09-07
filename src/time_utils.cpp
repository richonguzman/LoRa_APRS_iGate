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

#include "time_utils.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace TIME_Utils {

    String getFormattedTime(unsigned long timeMillis) {
        // Split into total seconds and remaining milliseconds
        std::time_t seconds = timeMillis / 1000;

        // Convert seconds to UTC calendar structure safely
        std::tm time_info;
        gmtime_r(&seconds, &time_info); // POSIX safe variant

        // Format the base date/time structure
        char buffer[30];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &time_info);

        // Append fractional milliseconds and UTC anchor 'Z'
        std::ostringstream oss;
        oss << buffer;
        return String(oss.str().c_str());
    }

    String getFormattedDateTime(unsigned long timeMillis) {
        if (!isValid(timeMillis)) {
            return "Not set";
        }
        // Split into total seconds and remaining milliseconds
        std::time_t seconds = timeMillis / 1000;

        // Convert seconds to UTC calendar structure safely
        std::tm time_info;
        gmtime_r(&seconds, &time_info); // POSIX safe variant

        // Format the base date/time structure
        char buffer[30];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &time_info);

        // Append fractional milliseconds and UTC anchor 'Z'
        std::ostringstream oss;
        oss << buffer;
        return String(String(oss.str().c_str())+" "+String(timeMillis));
    }

    boolean isValid(unsigned long timeMillis) {
        if (timeMillis < 1767225600) {  // 2026-01-01T00:00:00
            return false;
        }
        return true;
    }

    String  generateDuration(unsigned long startMillis, unsigned long endMillis) {
        unsigned long diff = endMillis - startMillis;
        diff /= 1000;  // convert to seconds

        unsigned long seconds = diff - ((diff/60) * 60);
        diff -= seconds;
        diff /= 60; // convert to minutes
        unsigned long minutes = diff - ((diff/60) * 60);
        diff -= minutes;
        diff /= 60; // convert to hours
        unsigned long hours = diff - ((diff/24) * 24);
        diff -= hours;
        diff /= 24; // convert to days
        unsigned long days = diff;

        return String(days) + " d " + String(hours) + " h " + String(minutes) + " m " + String(seconds) + " s" ;
    }

}
