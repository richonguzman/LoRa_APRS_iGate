#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <Arduino.h>

#define SCREEN_WIDTH    128     // OLED display width, in pixels
#define SCREEN_HEIGHT   64      // OLED display height, in pixels
#define OLED_RESET      -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void setup_display();
void display_toggle(bool toggle);

void show_display(String line1, int wait = 0);
void show_display(String line1, String line2, int wait = 0);
void show_display(String line1, String line2, String line3, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, String line6, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, String line6, String line7, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, String line6, String line7, String line8, int wait = 0);

#endif