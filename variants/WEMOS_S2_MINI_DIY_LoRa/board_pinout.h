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
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          36
    #define RADIO_MISO_PIN          37
    #define RADIO_MOSI_PIN          35
    #define RADIO_CS_PIN            34
    #define RADIO_RST_PIN           33
    #define RADIO_BUSY_PIN          38
    #define RADIO_WAKEUP_PIN        RADIO_BUSY_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_38

    //  I2C
    #define USE_WIRE_WITH_BOARD_I2C_PINS
    #define BOARD_I2C_SDA           11
    #define BOARD_I2C_SCL           12

    //  Aditional Config
    #define INTERNAL_LED_PIN        15

#endif