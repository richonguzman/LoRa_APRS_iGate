#include <Wire.h>
#include "configuration.h"
#include "board_pinout.h"
#include "display.h"


#ifdef HAS_DISPLAY
    #ifdef HAS_TFT
        #include <TFT_eSPI.h>

        TFT_eSPI tft = TFT_eSPI(); 

        #ifdef HELTEC_WIRELESS_TRACKER
            #define bigSizeFont     2.5
            #define smallSizeFont   1.5
            #define lineSpacing     12
        #endif
    #else
        #ifdef HAS_EPAPER
            //
        #else
            #include <Adafruit_GFX.h>
            #include <Adafruit_SSD1306.h>
            #if defined(HELTEC_V3) || defined(HELTEC_WS)
                #define OLED_DISPLAY_HAS_RST_PIN
            #endif
            #ifdef HELTEC_WSL_V3_DISPLAY
                Adafruit_SSD1306 display(128, 64, &Wire1, OLED_RST);
            #else
                Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);
            #endif
        #endif
    #endif
#endif
    
extern  Configuration   Config;

String  oldHeader, oldFirstLine, oldSecondLine, oldThirdLine, oldFourthLine, oldFifthLine, oldSixthLine;
bool    displayFound    = false;


void cleanTFT() {
    #ifdef HAS_TFT
        tft.fillScreen(TFT_BLACK);
    #endif
}

void displaySetup() {
    #ifdef HAS_DISPLAY
        delay(500);
        #ifdef HAS_TFT
            tft.init();
            tft.begin();
            if (Config.display.turn180) {
                    tft.setRotation(3);
            } else {
                tft.setRotation(1);
            }
            tft.setTextFont(0);
            tft.fillScreen(TFT_BLACK);
        #else
            #ifdef HAS_EPAPER
                //
            #else
                #ifdef OLED_DISPLAY_HAS_RST_PIN
                    pinMode(OLED_RST, OUTPUT);
                    digitalWrite(OLED_RST, LOW);
                    delay(20);
                    digitalWrite(OLED_RST, HIGH);
                #endif

                if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
                    displayFound = true;
                    if (Config.display.turn180) display.setRotation(2);
                    display.clearDisplay();
                    display.setTextColor(WHITE);
                    display.setTextSize(1);
                    display.setCursor(0, 0);
                    display.ssd1306_command(SSD1306_SETCONTRAST);
                    display.ssd1306_command(1);
                    display.display();
                }                
            #endif
        #endif
        delay(1000);
    #endif
}

void displayToggle(bool toggle) {
    #ifdef HAS_DISPLAY
        if (toggle) {
            #ifdef HAS_TFT
                digitalWrite(TFT_BL, HIGH);
            #else
                #ifdef HAS_EPAPER
                    // ... to be continued
                #else
                    if (displayFound) display.ssd1306_command(SSD1306_DISPLAYON);
                #endif
            #endif
        } else {
            #ifdef HAS_TFT
                digitalWrite(TFT_BL, LOW);
            #else
                #ifdef HAS_EPAPER
                    // ... to be continued
                #else
                    if (displayFound) display.ssd1306_command(SSD1306_DISPLAYOFF);
                #endif
            #endif
        }
    #endif
}

bool shouldCleanTFT(const String& header, const String& line1, const String& line2, const String& line3) {
    if (oldHeader != header || oldFirstLine != line1 || oldSecondLine != line2 || oldThirdLine != line3) {
        oldHeader       = header;
        oldFirstLine    = line1;
        oldSecondLine   = line2;
        oldThirdLine    = line3;
        return true;
    } else {
        return false;
    }
}

bool shouldCleanTFT(const String& header, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5, const String& line6) {
    if (oldHeader != header || oldFirstLine != line1 || oldSecondLine != line2 || oldThirdLine != line3 || oldFourthLine != line4 || oldFifthLine != line5 || oldSixthLine != line6) {
        oldHeader       = header;
        oldFirstLine    = line1;
        oldSecondLine   = line2;
        oldThirdLine    = line3;
        oldFourthLine   = line4;
        oldFifthLine    = line5;
        oldSixthLine    = line6;
        return true;
    } else {
        return false;
    }
}

void displayShow(const String& header, const String& line1, const String& line2, const String& line3, int wait) {
    #ifdef HAS_DISPLAY
        const String* const lines[] = {&line1, &line2, &line3};
        #ifdef HAS_TFT
            if (shouldCleanTFT(header, line1, line2, line3)) {
                cleanTFT();
            }
            tft.setTextColor(TFT_WHITE,TFT_BLACK);
            tft.setTextSize(bigSizeFont);
            tft.setCursor(0, 0);
            tft.print(header);
            tft.setTextSize(smallSizeFont);
            for (int i = 0; i < 3; i++) {
                tft.setCursor(0, ((lineSpacing * (2 + i)) - 2));
                tft.print(*lines[i]);
            }
        #else
            #ifdef HAS_EPAPER
                // ... to be continued
            #else
                if (displayFound) {
                    display.clearDisplay();
                    display.setTextColor(WHITE);
                    display.setTextSize(1);
                    display.setCursor(0, 0);
                    display.println(header);
                    for (int i = 0; i < 3; i++) {
                        display.setCursor(0, 8 + (8 * i));
                        display.println(*lines[i]);
                    }
                    display.ssd1306_command(SSD1306_SETCONTRAST);
                    display.ssd1306_command(1);
                    display.display();
                }
            #endif
        #endif
        delay(wait);
    #endif
}

void displayShow(const String& header, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5, const String& line6, int wait) {
    #ifdef HAS_DISPLAY
        const String* const lines[] = {&line1, &line2, &line3, &line4, &line5, &line6};
        #ifdef HAS_TFT
            if (shouldCleanTFT(header, line1, line2, line3, line4, line5, line6)) {
                cleanTFT();
            }
            tft.setTextColor(TFT_WHITE,TFT_BLACK);
            tft.setTextSize(bigSizeFont);
            tft.setCursor(0, 0);
            tft.print(header);
            tft.setTextSize(smallSizeFont);
            for (int i = 0; i < 6; i++) {
                tft.setCursor(0, ((lineSpacing * (2 + i)) - 2));
                tft.print(*lines[i]);
            }
        #else
            #ifdef HAS_EPAPER
                // ... to be continued
            #else
                if (displayFound) {
                    display.clearDisplay();
                    display.setTextColor(WHITE);
                    display.setTextSize(2);
                    display.setCursor(0, 0);
                    display.println(header);
                    display.setTextSize(1);
                    for (int i = 0; i < 6; i++) {
                        display.setCursor(0, 16 + (8 * i));
                        display.println(*lines[i]);
                    }
                    display.ssd1306_command(SSD1306_SETCONTRAST);
                    display.ssd1306_command(1);
                    display.display();
                }
            #endif
        #endif
        delay(wait);
    #endif
}