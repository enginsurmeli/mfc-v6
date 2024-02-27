#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "display.h"
#include "pid.h"
#include "serial.h"
#include "flow_sensor.h"

// Flow sensor ADC input pin.
#define FS1012_INPUT_PIN A0

// TB6612FNG pins.
#define MFCOutPin 9
#define HIGHpin 8
#define LOWpin 7

// ST7735 LCD pins.
#define TFT_CS 10
#define TFT_RST -1
#define TFT_DC 6

// Rotary encoder pins.
#define PIN_A 5
#define PIN_B 4
#define PUSH_BTN 2

extern const char *gas_list[];
extern int8_t gas_list_index;
extern const float gas_multiplier[];
extern const uint8_t gas_list_length;

extern const float mfc_flowrate_range;

extern float mfc_pv, mfc_sv, mfc_output;
extern float consKp, consKi, consKd;
extern float aggKp, aggKi, aggKd;
extern QuickPID mfcPID;

extern float targetSetpoint, currentSetpoint, setpointIncrement;
extern float max_setpoint;

const int GasListIndexEEPROMAddress = 28, SetpointEEPROMAddress = 0;
const char serial_device_id = 'A'; // TODO: Implement device ID for multi-device serial communication.

#endif