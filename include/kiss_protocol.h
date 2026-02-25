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

#define DCD_ON            0x03

#define FEND              0xC0
#define FESC              0xDB
#define TFEND             0xDC
#define TFESC             0xDD

#define CMD_UNKNOWN       0xFE
#define CMD_DATA          0x00
#define CMD_HARDWARE      0x06

#define HW_RSSI           0x21

#define CMD_ERROR         0x90
#define ERROR_INITRADIO   0x01
#define ERROR_TXFAILED    0x02
#define ERROR_QUEUE_FULL  0x04

#define APRS_CONTROL_FIELD 0x03
#define APRS_INFORMATION_FIELD 0xf0

#define HAS_BEEN_DIGIPITED_MASK 0b10000000
#define IS_LAST_ADDRESS_POSITION_MASK 0b1