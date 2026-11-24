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

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LilyGO T-Internet-POE V2 (ESP32-WROOM-32E, LAN8720) + Ai-Thinker RA-02 (SX1278, 433 MHz)
    //  Headless: RMII Ethernet uses GPIO 19/21/22/25/26/27

    //  LoRa Radio
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          33
    #define RADIO_MISO_PIN          34      // input-only GPIO, fine for MISO
    #define RADIO_MOSI_PIN          32
    #define RADIO_CS_PIN            4       // RA-02 NSS
    #define RADIO_RST_PIN           16
    #define RADIO_BUSY_PIN          35      // SX127x has no BUSY line: this is DIO0 (RX interrupt), input-only GPIO
    #define RADIO_WAKEUP_PIN        RADIO_BUSY_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_35

    //  I2C for an external sensor (e.g. BME280), also gives the firmware's sensor probing a valid bus. GPIO 14/15 are shared with the microSD slot
    #define SENSOR_I2C_BUS          Wire
    #define BOARD_I2C_SDA           14
    #define BOARD_I2C_SCL           15

    //  Ethernet (LAN8720, RMII)
    #define HAS_ETH
    #define ETH_CFG_PHY_TYPE        ETH_PHY_LAN8720
    #define ETH_CFG_PHY_ADDR        0
    #define ETH_CFG_MDC_PIN         23
    #define ETH_CFG_MDIO_PIN        18
    #define ETH_CFG_PHY_POWER_PIN   5       // PHY power/reset line
    #define ETH_CFG_CLK_MODE        ETH_CLOCK_GPIO17_OUT

#endif
