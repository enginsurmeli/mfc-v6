#ifndef DISPLAY_H
#define DISPLAY_H
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#include <RotaryEncoder.h>
// #include "pid.h"
// #include "config.h"

enum Corner
{ // Indices for drawMenuRect function.
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    NONE
};

void setupDisplay();
void updateDisplay();
void updateOnButtonPush();
void updateOnEncoderRotation();
void updateOnPageChange();
void updateOnEachIteration();
void drawMenuRect(Corner firstCorner = Corner::NONE, Corner lastCorner = Corner::NONE);
void printNumOnScreen(int textSize, int cursorX, int cursorY, float &prevValue, float newValue, char *buffer);
void printTextOnScreen(int textSize, int cursorX, int cursorY, const char *prevText, const char *newText);
bool checkEncoderButton();
int8_t checkEncoderRotation();

#endif