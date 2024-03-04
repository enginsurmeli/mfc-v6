#include "config.h"

int main(void)
{
  init();
  Serial.begin(115200);
  Serial.println("Initializing...");
  setupDisplay();
  setupPID();
  Serial.println("Device is ready.");
  while (1)
  {
    serialListen();
    readFlowRate();
    runPID();
    serialStream();
    updateDisplay();
    delay(5);
  }
  return 0;
}