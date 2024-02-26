#ifndef PID_H
#define PID_H
#include <QuickPID.h>
#include "serial.h"
#include "flow_sensor.h"
#include "config.h"

void setupPID();
void runPID();
void incrementSetpoint();
void abortFlow();

#endif