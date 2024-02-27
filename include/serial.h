#ifndef SERIAL_H
#define SERIAL_H
#include <EEPROM.h>

void serialListen();
void serialStream();
void serialWrite(char charData[]);

#endif