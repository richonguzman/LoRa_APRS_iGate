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

#ifndef KISS_UTILS_H_
#define KISS_UTILS_H_

#include <Arduino.h>


String encodeAddressAX25(String tnc2Address);
String decodeAddressAX25(const String& ax25Address, bool& isLast, bool isRelay);

String encapsulateKISS(const String& ax25Frame, uint8_t cmd);
String decapsulateKISS(const String& frame);

String encodeKISS(const String& frame);
String decodeKISS(const String& inputFrame, bool& dataFrame);

#endif