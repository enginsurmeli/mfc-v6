#include "config.h"

float mfc_sv, mfc_pv, mfc_output, max_setpoint;

// Define the tuning parameters for MFC controls:
float consKp = 0.05, consKi = 0.03, consKd = 0.00;
float aggKp = 0.1, aggKi = 0.24, aggKd = 0.05;

const uint32_t pid_sample_time = 1; // in milliseconds.

// Define variables for setpoint ramp rate
float targetSetpoint = 0.0;
float currentSetpoint = 0.0;
float setpointIncrement = 0.5; // Adjust as needed for the desired ramp rate

// Define PWM frequency
const uint32_t pwm_frequency = 11800; // in Hz

// Specify the links for PID
// QuickPID mfcPID(&mfc_pv, &mfc_output, &mfc_sv, aggKp, aggKi, aggKd,
//                 mfcPID.pMode::pOnError, /* pOnError, pOnMeas, pOnErrorMeas */
//                 mfcPID.dMode::dOnMeas,      /* dOnError, dOnMeas */
//                 mfcPID.iAwMode::iAwCondition,     /* iAwCondition, iAwClamp, iAwOff */
//                 mfcPID.Action::direct);

QuickPID mfcPID(&mfc_pv, &mfc_output, &mfc_sv);

void setupPID()
{
  // // Only run the code block below if you are programming this Arduino for the first time.
  // EEPROM.put(4, aggKp);
  // EEPROM.put(8, aggKi);
  // EEPROM.put(12, aggKd);
  // EEPROM.put(16, consKp);
  // EEPROM.put(20, consKi);
  // EEPROM.put(24, consKd);
  // EEPROM.put(GasListIndexEEPROMAddress, gas_list_index); // Put gas_list_index to EEPROM.
  // EEPROM.put(SetpointEEPROMAddress, mfc_sv);             // Put mfc setpoint to EEPROM.

  EEPROM.get(4, aggKp);
  EEPROM.get(8, aggKi);
  EEPROM.get(12, aggKd);
  EEPROM.get(16, consKp);
  EEPROM.get(20, consKi);
  EEPROM.get(24, consKd);
  EEPROM.get(GasListIndexEEPROMAddress, gas_list_index); // Get and set gas_list_index from EEPROM.
  EEPROM.get(SetpointEEPROMAddress, targetSetpoint);     // Get and set mfc setpoint from EEPROM.

  // if (isnan(float(gas_list_index))) gas_list_index = 0;
  // if (isnan(float(aggKp))) aggKp = 0.15;
  // if (isnan(float(aggKd))) aggKd = 0.20;
  // if (isnan(float(aggKp))) aggKp = 0.00;
  // if (isnan(float(consKp))) consKp = 0.015;
  // if (isnan(float(consKd))) consKd = 0.02;
  // if (isnan(float(consKp))) consKp = 0.00;
  max_setpoint = mfc_flowrate_range * gas_multiplier[gas_list_index];
  if (max_setpoint > 9999.9)
    max_setpoint = 9999.9;

  // Set PID tune parameters:
  // mfcPID.SetTunings(aggKp, aggKi, aggKd);
  mfcPID.SetTunings(consKp, consKi, consKd);
  mfcPID.SetProportionalMode(mfcPID.pMode::pOnError);
  mfcPID.SetAntiWindupMode(mfcPID.iAwMode::iAwCondition);
  mfcPID.SetDerivativeMode(mfcPID.dMode::dOnMeas);
  // mfcPID.SetOutputLimits(100, 255);
  // mfcPID.SetMode(mfcPID.Control::automatic);
  mfcPID.SetSampleTimeUs(pid_sample_time * 1000); // Convert pid_sample_time units from milliseconds to microseconds.

  InitTimersSafe(12000); // initialize all timers except for 0, to save time keeping functions

  const bool pwm_frequency_set = SetPinFrequencySafe(MFCOutPin, pwm_frequency);
  if (!pwm_frequency_set)
  {
    Serial.println("Failed to set PWM frequency!");
  }

  // Set AIN1 and STBY pins of TB6612FNG to HIGH and AIN2 to LOW.
  pinMode(HIGHpin, OUTPUT);
  digitalWrite(HIGHpin, LOW);
  pinMode(LOWpin, OUTPUT);
  digitalWrite(LOWpin, LOW);

  abortFlow(); // Make the output zero on every reset, if the setpoint is > 0 it pid will automatically start the flow.
}

void runPID()
{
  incrementSetpoint();
  // static int agg_cons_cutoff_point = mfc_sv / 10
  if (targetSetpoint == 0)
  {
    abortFlow();
    return;
  }

  // if (mfc_pv < 1)
  // {
  //   mfcPID.SetTunings(aggKp, aggKi, aggKd);
  // }
  // else
  // {
  //   mfcPID.SetTunings(consKp, consKi, consKd);
  // }

  // if (mfc_sv / (mfc_pv + 1) <= 1)
  //   mfcPID.SetTunings(consKp, consKi, consKd);

  // if (abs(mfc_sv - mfc_pv) > 10)
  //   mfcPID.SetTunings(aggKp, aggKi, aggKd);

  // if (mfc_pv <= 1 && mfc_output < 100.0)
  //   mfcPID.SetTunings(10.0 * aggKp, aggKi, aggKd);

  digitalWrite(HIGHpin, HIGH);
  mfcPID.SetMode(mfcPID.Control::automatic);
  mfcPID.Compute();
  pwmWrite(MFCOutPin, mfc_output);
}

void incrementSetpoint()
{
  // Update setpoint gradually
  if (currentSetpoint < targetSetpoint)
  {
    currentSetpoint += setpointIncrement;
    // Ensure currentSetpoint does not exceed targetSetpoint
    currentSetpoint = min(currentSetpoint, targetSetpoint);
  }
  else if (currentSetpoint > targetSetpoint)
  {
    currentSetpoint -= setpointIncrement;
    // Ensure currentSetpoint does not go below targetSetpoint
    currentSetpoint = max(currentSetpoint, targetSetpoint);
  }
  mfc_sv = currentSetpoint;
}

void abortFlow()
{
  mfcPID.SetMode(mfcPID.Control::manual);
  mfc_output = 0;
  digitalWrite(HIGHpin, LOW);
  pwmWrite(MFCOutPin, 0);
}