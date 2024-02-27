#include "serial.h"
#include "config.h"

bool data_stream_on;
unsigned long previousMillis_serial; // previousMillis: will store last time serial stream was updated
long serial_print_interval = 500;    // interval at which to stream serial data (milliseconds)

const uint8_t SERIAL_MAX_MESSAGE_LENGTH = 30;
char *token;
const char *delimiter = "+";

char read_string[SERIAL_MAX_MESSAGE_LENGTH], write_string[SERIAL_MAX_MESSAGE_LENGTH];
char buffer1[8];
char buffer2[8];
char buffer3[8];

void (*resetFunc)(void) = 0; // declare reset function @ address 0

void serialListen()
{
  // TODO: Implement serial command to change gas type.
  while (Serial.available() > 0)
  {
    // Create a place to hold the incoming message
    static char message[SERIAL_MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    char inByte = Serial.read(); // Read the next available byte in the serial receive buffer

    if (inByte != '\n' && (message_pos < SERIAL_MAX_MESSAGE_LENGTH - 1)) // Message coming in (check not terminating character) and guard for over message size
    {
      message[message_pos] = inByte; // Add the incoming byte to our message
      message_pos++;
    } // Full message received...
    else
    {
      message[message_pos] = '\0'; // Add null character to string

      token = strtok(message, delimiter);

      static char *message_array[4];
      int i = 0;
      while (token != NULL)
      {
        message_array[i] = token;
        token = strtok(NULL, delimiter);
        i++;
      }

      // read_string = message_array[0];
      if (!strcmp(message_array[0], "A"))
      {
        dtostrf(mfc_pv, 4, 2, buffer1);
        sprintf(write_string, "A+%s", buffer1);
      }
      else if (!strcmp(message_array[0], "AS?"))
      {
        dtostrf(targetSetpoint, 4, 2, buffer1);
        sprintf(write_string, "AS+%s", buffer1);
      }
      else if (!strcmp(message_array[0], "AS"))
      {
        targetSetpoint = atof(message_array[1]);
        targetSetpoint = constrain(targetSetpoint, 0, max_setpoint);
        EEPROM.put(SetpointEEPROMAddress, targetSetpoint);
        dtostrf(targetSetpoint, 4, 2, buffer1);
        sprintf(write_string, "AS+%s", buffer1);
      }
      else if (!strcmp(message_array[0], "APA?"))
      {
        dtostrf(aggKp, 5, 3, buffer1);
        dtostrf(aggKi, 5, 3, buffer2);
        dtostrf(aggKd, 5, 3, buffer3);
        sprintf(write_string, "APA+%s+%s+%s", buffer1, buffer2, buffer3);
      }
      else if (!strcmp(message_array[0], "APA"))
      {
        aggKp = atof(message_array[1]);
        aggKi = atof(message_array[2]);
        aggKd = atof(message_array[3]);
        EEPROM.put(4, aggKp);
        EEPROM.put(8, aggKi);
        EEPROM.put(12, aggKd);
        mfcPID.SetTunings(aggKp, aggKi, aggKd);
        dtostrf(aggKp, 5, 3, buffer1);
        dtostrf(aggKi, 5, 3, buffer2);
        dtostrf(aggKd, 5, 3, buffer3);
        sprintf(write_string, "APA+%s+%s+%s", buffer1, buffer2, buffer3);
      }
      else if (!strcmp(message_array[0], "APC?"))
      {
        dtostrf(consKp, 5, 3, buffer1);
        dtostrf(consKi, 5, 3, buffer2);
        dtostrf(consKd, 5, 3, buffer3);
        sprintf(write_string, "APC+%s+%s+%s", buffer1, buffer2, buffer3);
      }
      else if (!strcmp(message_array[0], "APC"))
      {
        consKp = atof(message_array[1]);
        consKi = atof(message_array[2]);
        consKd = atof(message_array[3]);
        EEPROM.put(16, consKp);
        EEPROM.put(20, consKi);
        EEPROM.put(24, consKd);
        mfcPID.SetTunings(consKp, consKi, consKd);
        dtostrf(consKp, 5, 3, buffer1);
        dtostrf(consKi, 5, 3, buffer2);
        dtostrf(consKd, 5, 3, buffer3);
        sprintf(write_string, "APC+%s+%s+%s", buffer1, buffer2, buffer3);
      }
      else if (!strcmp(message_array[0], "AHC"))
      {
        // abortFlow();
        targetSetpoint = 0;
        sprintf(write_string, "AHC");
      }
      else if (!strcmp(message_array[0], "AHO"))
      {
        targetSetpoint = max_setpoint;
        mfcPID.SetMode(mfcPID.Control::manual);
        analogWrite(MFCOutPin, 255);
        sprintf(write_string, "AHO");
      }
      else if (!strcmp(message_array[0], "AR"))
      {
        resetFunc();
        delay(1);
        sprintf(write_string, "Reset failed!");
      }
      else if (!strcmp(message_array[0], "A@=@"))
        data_stream_on = 1;
      else if (!strcmp(message_array[0], "@@=A"))
        data_stream_on = 0;
      else if (!strcmp(message_array[0], "AW91"))
      {
        serial_print_interval = atof(message_array[1]);
        if (serial_print_interval <= 0)
          serial_print_interval = 1;
      }
      else if (!strcmp(message_array[0], "pMode"))
      {
        switch (atoi(message_array[1]))
        {
        case 0:
          mfcPID.SetProportionalMode(mfcPID.pMode::pOnError);
          break;
        case 1:
          mfcPID.SetProportionalMode(mfcPID.pMode::pOnErrorMeas);
          break;
        case 2:
          mfcPID.SetProportionalMode(mfcPID.pMode::pOnMeas);
          break;
        default:
          break;
        }
        sprintf(write_string, "pMode+%i", mfcPID.GetPmode());
      }
      else if (!strcmp(message_array[0], "pMode?"))
      {
        sprintf(write_string, "pMode+%i", mfcPID.GetPmode());
      }
      else if (!strcmp(message_array[0], "iAwMode"))
      {
        switch (atoi(message_array[1]))
        {
        case 0:
          mfcPID.SetAntiWindupMode(mfcPID.iAwMode::iAwCondition);
          break;
        case 1:
          mfcPID.SetAntiWindupMode(mfcPID.iAwMode::iAwClamp);
          break;
        case 2:
          mfcPID.SetAntiWindupMode(mfcPID.iAwMode::iAwOff);
          break;
        default:
          break;
        }
        sprintf(write_string, "iAwMode+%i", mfcPID.GetAwMode());
      }
      else if (!strcmp(message_array[0], "iAwMode?"))
      {
        sprintf(write_string, "iAwMode+%i", mfcPID.GetAwMode());
      } // TODO: -add change and read gas option over serial.
        //       -add change and read pid sample time.
        //       -add change and read pid Dmode.
        //       -add change and read serial device id.
        //       -add change and read totalizer time.
        //       -add change and read screen refresh frequency.
      else
      {
        strncpy(write_string, "error", SERIAL_MAX_MESSAGE_LENGTH - 1);
      }

      Serial.println(write_string);
      message_pos = 0; // Reset for the next message
    }
  }
}

void serialStream()
{
  unsigned long currentMillis_print = millis();
  if (data_stream_on && currentMillis_print - previousMillis_serial >= serial_print_interval)
  {
    previousMillis_serial = currentMillis_print;

    dtostrf(mfc_pv, 4, 2, buffer1);
    dtostrf(mfc_sv, 4, 2, buffer2);
    dtostrf(mfc_output, 4, 2, buffer3);
    // sprintf(write_string, "A+%s", buffer1);
    sprintf(write_string, "A+%s, AS+%s, PID+%s", buffer1, buffer2, buffer3);
    Serial.println(write_string);
  }
}

void serialWrite(char charData[SERIAL_MAX_MESSAGE_LENGTH])
{
  Serial.write(charData);
  Serial.write(10);
  Serial.flush();
}