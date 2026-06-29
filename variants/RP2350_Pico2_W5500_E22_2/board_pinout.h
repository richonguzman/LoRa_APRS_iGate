#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

// Raspberry Pi Pico 2 + external W5500 Ethernet on SPI0 (pins fully RELOCATED)
// + EBYTE E22-400M30S (SX1268) on SPI1. RP2350 iGate "-2" board for 433 MHz.
//
// This board is the Meshtastic `pico2_w5500_e22-2` variant: GP16-20 were
// suspected damaged, so the W5500 SPI0 lines were moved to alternate
// SPI0-capable GPIOs (MISO=GP20, SCK=GP22, MOSI=GP7, CS=GP21, RST=GP6).
// Those W5500 pins are configured via the PIN_ETH_* build flags in
// platformio.ini; this header only carries the radio (SPI1) wiring, which is
// IDENTICAL to the standard E22P variant — only the silicon (SX1268) differs.

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
// E22 single combined RF-enable (RFEN) on GP3, held HIGH while active.
// PA keying (TXEN) is done on-module by the SX1268 DIO2->TXEN bridge.
#define RADIO_RXEN        3
#define RADIO_TXEN       -1

#endif
