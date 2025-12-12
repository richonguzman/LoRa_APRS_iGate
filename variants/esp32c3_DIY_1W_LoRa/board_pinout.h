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
    #define HAS_SX1268
    #define HAS_1W_LORA
    #define HAS_TCXO
    #define RADIO_SCLK_PIN          8
    #define RADIO_MISO_PIN          9
    #define RADIO_MOSI_PIN          10
    #define RADIO_CS_PIN            5
    #define RADIO_RST_PIN           4
    #define RADIO_DIO1_PIN          2
    #define RADIO_BUSY_PIN          3
    #define RADIO_RXEN              6
    #define RADIO_TXEN              7
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_NUM_2

#endif