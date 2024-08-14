#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <Arduino.h>

#define SCREEN_ADDRESS  0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

void cleanTFT();
void displaySetup();
void displayToggle(bool toggle);

bool shouldCleanTFT(const String& header, const String& line1, const String& line2, const String& line3);
bool shouldCleanTFT(const String& header, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5, const String& line6);

void displayShow(const String& header, const String& line1, const String& line2, const String& line3, int wait = 0);
void displayShow(const String& header, const String& line1, const String& line2, const String& line3, const String& line4, const String& line5, const String& line6, int wait = 0);

#endif