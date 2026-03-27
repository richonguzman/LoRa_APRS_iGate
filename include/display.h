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

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <Arduino.h>


#define SCREEN_ADDRESS  0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void displaySetup();
void displayToggle(bool toggle);

void displayShow(const String& header, const String& line1, const String& line2, const String& line3, int wait = 0);
void displayShow(const String& header, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5, const String& line6, int wait = 0);

#endif