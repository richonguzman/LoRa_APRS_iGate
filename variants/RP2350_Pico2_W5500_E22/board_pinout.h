#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

// Raspberry Pi Pico 2 (RP2350) + external WIZnet W5500 Ethernet on SPI0
// + EBYTE E22-400M30S (SX1268) on SPI1. RP2350 iGate carrier board for the
// 433 MHz LoRa-APRS network.
//
// The W5500 SPI0 lines (MISO=GP16, CS=GP17, SCK=GP18, MOSI=GP19, RST=GP20) are
// configured via the PIN_ETH_* build flags in platformio.ini; this header only
// carries the radio (SPI1) wiring, which is identical to the W5500-EVB-Pico2
// variant — only the radio silicon (SX1268) differs.

// NOTE: the E22-400M30S is marketed as SX1268, but its die reports "SX1262" in
// the version-string register (REG 0x0320). RadioLib's findChip() does a
// strncmp() of that string against the class name, so the SX1268 class rejects
// this module with RADIOLIB_ERR_CHIP_NOT_FOUND (-2) even though it is alive.
// Use the SX1262 class — it drives the same SX126x command set and its
// setFrequency() range (150-960 MHz) happily covers 433 MHz.
#define HAS_SX1262
#define HAS_1W_LORA
#define HAS_TCXO
#define SX126X_DIO3_TCXO_VOLTAGE 1.8   // E22 TCXO fed from DIO3

// ---- LoRa (E22-400M30S, reports as SX1262) on SPI1 ----
#define RADIO_SCLK_PIN   10
#define RADIO_MISO_PIN   12
#define RADIO_MOSI_PIN   11
#define RADIO_CS_PIN     13
#define RADIO_RST_PIN    15
#define RADIO_DIO1_PIN   14
#define RADIO_BUSY_PIN    2
// Canonical E22-400M30S front-end (datasheet §4.1): TXEN and RXEN are separate
// MCU-driven lines, mutually exclusive (LNA off during TX, PA off during RX).
// RadioLib toggles both; DIO2 is left FLOATING (the on-module DIO2->TXEN bridge
// must be removed and TXEN wired to GP28). GP28 is otherwise free on this carrier
// (not an ETH/W5500 pin, not the VSYS ADC on GP29).
#define RADIO_RXEN        3
#define RADIO_TXEN       28

#endif
