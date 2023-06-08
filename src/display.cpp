#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "pins_config.h"
#include "display.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup_display() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
    	Serial.println(F("SSD1306 allocation failed"));
    	for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(1000);
}

void display_toggle(bool toggle) {
  if (toggle) {
    display.ssd1306_command(SSD1306_DISPLAYON);
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
}

void show_display(String line1, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, int wait) {
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
}

void show_display(String line1, String line2, String line3, int wait) {
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
}

void show_display(String line1, String line2, String line3, String line4, int wait) {
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
}