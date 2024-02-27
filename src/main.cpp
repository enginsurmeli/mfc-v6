#include "config.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");
  setupDisplay();
  setupPID();
  Serial.println("Device is ready.");
}

void loop()
{
  serialListen();
  readFlowRate();
  runPID();
  serialStream();
  updateDisplay();
  delay(5);
}