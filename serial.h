#ifndef SERIAL_H
#define SERIAL_H
#include <Arduino.h>
#include <EEPROM.h>
#include "pid.h"
#include "config.h"

void serialListen();
void serialStream();
void serialWrite(char charData[]);

#endif