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

#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>
#include <vector>


namespace LoRa_Utils {

struct RxtHopMetric {
        String fromNode; // The node that transmitted (e.g., N7AIL-15 or previous digi)
        String toNode;   // The digi that received and reported (e.g., SOMTNX, TSRXAX)
        bool hasData;    // false = this hop exists in the path but no RXT-enabled
                         // digi measured it (caller should print "NA", not the
                         // fields below, which are meaningless when false)
        int rssi;
        float snr;
        int fo;
        unsigned long tth; // ms -- unsigned long (not int) so slow SF/BW configs
                            // (e.g. SF12, ~2000-3000 ms packet frames) can't overflow
    };
    void    setup();
    void    sendNewPacket(const String& newPacket);
    String  receivePacketFromSleep();
    String  receivePacket();
    String  stripRxtTrailer(const String& packet, String* outTuple = nullptr);
    String  getLastRxtField();
    std::vector<RxtHopMetric> getDecodedRxtMetrics(const String& packet);
    void    changeFreqTx();
    void    changeFreqRx();
    void    wakeRadio();
    void    sleepRadio();

}

#endif
