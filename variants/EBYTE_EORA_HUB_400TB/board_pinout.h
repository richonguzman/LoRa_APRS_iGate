#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_LR1121
    #define RADIO_CS_PIN            8
    #define RADIO_SCLK_PIN          9
    #define RADIO_MOSI_PIN          10
    #define RADIO_MISO_PIN          11
    #define RADIO_RST_PIN           12
    #define RADIO_BUSY_PIN          13
    #define RADIO_DIO1_PIN          14
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_14

    //  Display
    #define HAS_DISPLAY
    
    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SCL                17
    #define OLED_SDA                18
    #define OLED_RST                21
    #define OLED_DISPLAY_HAS_RST_PIN

    //  Aditional Config
    #define BATTERY_PIN             1
    #define INTERNAL_LED_PIN        35
    #define VEXT_CTRL               36
    #define ADC_CTRL                37

    // Required for LR1121, will very for different manufacturers
    #include <RadioLib.h>

    static const uint32_t rfswitch_dio_pins[] = { 
        RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
        RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC
    };

    static const Module::RfSwitchMode_t rfswitch_table[] = {
        // mode                  DIO5  DIO6 
        { LR11x0::MODE_STBY,   { LOW,  LOW  } },
        { LR11x0::MODE_RX,     { LOW,  LOW  } },
        { LR11x0::MODE_TX,     { LOW,  HIGH } },
        { LR11x0::MODE_TX_HP,  { HIGH, LOW  } },
        { LR11x0::MODE_TX_HF,  { HIGH, HIGH } },
        { LR11x0::MODE_GNSS,   { LOW,  LOW  } },
        { LR11x0::MODE_WIFI,   { LOW,  LOW  } },
        END_OF_MODE_TABLE,
    };

#endif