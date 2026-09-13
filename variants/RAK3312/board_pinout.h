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

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define HAS_TCXO
    #define RADIO_SCLK_PIN      5
    #define RADIO_MISO_PIN      3
    #define RADIO_MOSI_PIN      6
    #define RADIO_CS_PIN        7
    #define RADIO_RST_PIN       8
    #define RADIO_DIO1_PIN      47
    #define RADIO_BUSY_PIN      48
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_47

    #define RADIO_HAS_RF_SWITCH //  ANT_SW - antenna switch/PA power enable
    #define RADIO_RF_SWITCH     4

    //  Base Board Power
    #define VEXT_CTRL_PIN       14  //  3V3_EN - powers GPS on Base Board
    #define VEXT_CTRL_ON_STATE  HIGH

    //  GPS
    #define HAS_GPS
    #define GPS_BAUDRATE        9600
    #define GPS_RX              43
    #define GPS_TX              44

    //  Aditional Config
    #define INTERNAL_LED_PIN    46
    #define BATTERY_PIN         1

#endif
