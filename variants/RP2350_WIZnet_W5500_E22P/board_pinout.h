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
// E22P RF front-end. Default (bridged): single combined RF-enable (RFEN) on GP3
// held HIGH, and PA keying (TXEN) done on-module by the SX1262 DIO2->TXEN bridge
// (RADIO_TXEN = -1). A board that cuts the DIO2->TXEN bridge and wires TXEN to
// its own GPIO can override RADIO_TXEN with a -D build flag (e.g. the rewired
// W5100S carrier passes -D RADIO_TXEN=28) to use the canonical separate TXEN/RXEN
// RF switch (RadioLib setRfSwitchPins: LNA off in TX, PA off in RX).
#define RADIO_RXEN        3
#ifndef RADIO_TXEN
#define RADIO_TXEN       -1
#endif

#endif
