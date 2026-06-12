#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

// WIZnet W5500-EVB-Pico2 (RP2350, on-board W5500 on SPI0) + EBYTE E22P-433M30S
// (SX1262) on SPI1. RP2350 port of the LoRa APRS iGate.

#define HAS_SX1262
#define HAS_1W_LORA
#define HAS_TCXO
#define SX126X_DIO3_TCXO_VOLTAGE 1.8   // E22P TCXO fed from DIO3

// ---- LoRa (E22P / SX1262) on SPI1 ----
#define RADIO_SCLK_PIN   10
#define RADIO_MISO_PIN   12
#define RADIO_MOSI_PIN   11
#define RADIO_CS_PIN     13
#define RADIO_RST_PIN    15
#define RADIO_DIO1_PIN   14
#define RADIO_BUSY_PIN    2
// E22P has a single combined RF-enable (RFEN) on GP3, held HIGH while active.
// PA keying (TXEN) is done on-module by the SX1262 DIO2->TXEN bridge.
#define RADIO_RXEN        3
#define RADIO_TXEN       -1

#endif
