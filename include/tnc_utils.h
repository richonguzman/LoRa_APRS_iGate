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

#ifndef TNC_UTILS_H_
#define TNC_UTILS_H_

#include <Arduino.h>
#include <vector>
#include "lora_utils.h" // for LoRa_Utils::RxtHopMetric


namespace TNC_Utils {

    void setup();
    void loop();

    void sendToClients(const String& packet, bool stripBytes, const std::vector<LoRa_Utils::RxtHopMetric>& hopMetrics);
    void sendToSerial(const String& packet, bool stripBytes, const std::vector<LoRa_Utils::RxtHopMetric>& hopMetrics);
    void sendCrcErrorToClients();
    void sendCrcErrorToSerial();
    // Non-APRS diagnostic text (boot banner, WiFi/NTP/mDNS status, CAD/DIFS
    // channel-contention retries) -- material to a client's understanding of
    // TNC connection/performance, per explicit design decision, so exposed
    // on both serial and IP in TNC2 mode. Never sent in KISS mode on either
    // interface -- no AX.25 representation exists, and it would corrupt a
    // legacy client's binary frame stream. This is the single point either
    // interface is reached from for this category of text; Utils::print()/
    // println() delegate here rather than touching Serial or client sockets
    // themselves, keeping tnc_utils.cpp the sole owner of client comms.
    void broadcastDiagnostic(const String& text, bool newline);

}

#endif
