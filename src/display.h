#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <Arduino.h>

#define SCREEN_ADDRESS  0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void cleanTFT();
bool shouldCleanTFT3(String header, String line1, String line2, String line3);
bool shouldCleanTFT6(String header, String line1, String line2, String line3, String line4, String line5, String line6);
void setup_display();
void display_toggle(bool toggle);

/*void show_display(String header, int wait = 0);
void show_display(String header, String line1, int wait = 0);
void show_display(String header, String line1, String line2, int wait = 0);*/
void show_display(String header, String line1, String line2, String line3, int wait = 0);
/*void show_display(String header, String line1, String line2, String line3, String line4, int wait = 0);
void show_display(String header, String line1, String line2, String line3, String line4, String line5, int wait = 0);*/
void show_display(String header, String line1, String line2, String line3, String line4, String line5, String line6, int wait = 0);

#endif