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

#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>


namespace APRS_IS_Utils {

    void    upload(const String& line);
    void    connect();

    void    checkStatus();
    String  checkForStartingBytes(const String& packet);

    String  buildPacketToUpload(const String& packet);
    bool    processReceivedLoRaMessage(const String& sender, const String& packet, bool thirdParty);
    void    processLoRaPacket(const String& packet);

    String  buildPacketToTx(const String& aprsisPacket, uint8_t packetType);
    void    processAPRSISPacket(const String& packet);
    void    listenAPRSIS();

    void    firstConnection();

}

#endif