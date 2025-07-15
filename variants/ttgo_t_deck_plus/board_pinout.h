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
    #define RADIO_SCLK_PIN          40
    #define RADIO_MISO_PIN          38
    #define RADIO_MOSI_PIN          41
    #define RADIO_CS_PIN            9
    #define RADIO_RST_PIN           17
    #define RADIO_DIO1_PIN          45
    #define RADIO_BUSY_PIN          13
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_45

    //  Display
    #define HAS_DISPLAY
    #define HAS_TFT

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST 

    //  GPS
    #define GPS_RX                  43
    #define GPS_TX                  44
    #define GPS_BAUDRATE            38400

    //  Aditional Config
    #define BATTERY_PIN             4

    #define BOARD_POWERON           10
    #define BOARD_SDCARD_CS         39
    #define BOARD_BL_PIN            42

    #define BOARD_I2C_SDA           18
    #define BOARD_I2C_SCL           8

#endif