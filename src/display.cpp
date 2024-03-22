#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "configuration.h"
#include "pins_config.h"
#include "display.h"

#ifdef HAS_DISPLAY
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
#endif

extern Configuration Config;

void setup_display() {
    #ifdef HAS_DISPLAY
    Wire.begin(OLED_SDA, OLED_SCL);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    if (Config.display.turn180) {
        display.setRotation(2);
    }
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(1000);
    #endif
}

void display_toggle(bool toggle) {
    #ifdef HAS_DISPLAY
    if (toggle) {
        display.ssd1306_command(SSD1306_DISPLAYON);
    } else {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
    #endif
}

void show_display(String line1, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setCursor(0, 8);
    display.println(line2);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, String line3, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setCursor(0, 8);
    display.println(line2);
    display.setCursor(0, 16);
    display.println(line3);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, String line3, String line4, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setCursor(0, 8);
    display.println(line2);
    display.setCursor(0, 16);
    display.println(line3);
    display.setCursor(0, 24);
    display.println(line4);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, String line3, String line4, String line5, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setCursor(0, 8);
    display.println(line2);
    display.setCursor(0, 16);
    display.println(line3);
    display.setCursor(0, 24);
    display.println(line4);
    display.setCursor(0, 32);
    display.println(line5);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, String line3, String line4, String line5, String line6, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.setCursor(0, 8);
    display.println(line2);
    display.setCursor(0, 16);
    display.println(line3);
    display.setCursor(0, 24);
    display.println(line4);
    display.setCursor(0, 32);
    display.println(line5);
    display.setCursor(0, 40);
    display.println(line6);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}

void show_display(String line1, String line2, String line3, String line4, String line5, String line6, String line7, int wait) {
    #ifdef HAS_DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(line1);
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.println(line2);
    display.setCursor(0, 24);
    display.println(line3);
    display.setCursor(0, 32);
    display.println(line4);
    display.setCursor(0, 40);
    display.println(line5);
    display.setCursor(0, 48);
    display.println(line6);
    display.setCursor(0, 56);
    display.println(line7);
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(wait);
    #endif
}