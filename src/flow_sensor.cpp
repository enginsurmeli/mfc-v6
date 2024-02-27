#include "config.h"

// define calibration coefficients for each model (2nd degree poly fit. in the form of a*x^2 + b*x + c)
const float calibration_coeff_a1 = 0.001398;
const float calibration_coeff_b1 = -0.117059876;
const float calibration_coeff_c1 = -12.91843904;
const float calibration_curve_change_point = 258; // above this adc reading value, we switch to another calibration curve.
const float calibration_coeff_a2 = 0.002998;
const float calibration_coeff_b2 = -0.998101989;
const float calibration_coeff_c2 = 108.9509672;

const float gas_multiplier[] = {1.0340, 0.8200, 1.6700, 1.0540, 1.2340, 1.7200, 1.7800, 1.9600, 2.2000, 2.1000, 0.9480, 1.0000, 1.2380, 0.6040, 0.9100, 0.7400, 0.8000, 1.1760};
const float mfc_flowrate_range = 1248.0; // Change this according to MFC's specified max flow rate.
const uint8_t gas_list_length = sizeof(gas_multiplier) / sizeof(gas_multiplier[0]);

const int numreadings = 10; // Reduce this number if you want to gain more RAM.
int readIndex = 0;
float readings[numreadings], total = 0, average = 0;

float flow_rate;

const char *gas_list[] = {" Air", "  Ar", " CH4", "  CO", " CO2", "C2H2", "C2H4", "C2H6", "C3H8", "  H2", "  He", "  N2", " N2O", "  Ne", "  O2", "  Kr", "  Xe", " SF6"};
int8_t gas_list_index = 0;

void readFlowRate()
{
  total -= readings[readIndex];
  // readings[readIndex] = analogRead(FS1012_INPUT_PIN) * (5.0 / 1023.0);  // units in V.
  readings[readIndex] = analogRead(FS1012_INPUT_PIN);
  total += readings[readIndex];
  readIndex += 1;
  if (readIndex >= numreadings)
    readIndex = 0;
  average = total / numreadings;

  if (average < calibration_curve_change_point)
    flow_rate = calibration_coeff_a1 * pow(average, 2) + calibration_coeff_b1 * average + calibration_coeff_c1;
  else
    flow_rate = calibration_coeff_a2 * pow(average, 2) + calibration_coeff_b2 * average + calibration_coeff_c2;
  mfc_pv = flow_rate * gas_multiplier[gas_list_index];
  mfc_pv = constrain(mfc_pv, 0, max_setpoint);
  // if (mfc_pv < 0) mfc_pv = 0;
  // mfc_pv = average;  // For testing purposes, always shows flow rate at 0.
}