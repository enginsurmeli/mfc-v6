#include "config.h"

const uint8_t menu_x_points[] = {9, 113, 9, 113, 3}, menu_y_points[] = {9, 9, 98, 98, 45};
int8_t current_menu_item = 0;
int8_t current_page, last_page;
bool change_setpoint_active, select_gas_active;

float prev_mfc_sv, prev_mfc_pv, temp_mfc_sv;

unsigned long previousMillis_lcd; // previousMillis: will store last time lcd was updated
long lcd_refresh_interval = 250;  // interval at which to refresh lcd (milliseconds)

// create an instance of the library
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// color definitions
const uint16_t Display_Color_Black = 0x0000;
const uint16_t Display_Color_Blue = 0x001F;
const uint16_t Display_Color_Red = 0xF800;
const uint16_t Display_Color_Green = 0x07E0;
const uint16_t Display_Color_Cyan = 0x07FF;
const uint16_t Display_Color_Magenta = 0xF81F;
const uint16_t Display_Color_Yellow = 0xFFE0;
const uint16_t Display_Color_White = 0xFFFF;

// The colors we actually want to use
const uint16_t Display_Text_Color = Display_Color_White;
const uint16_t Display_Background_Color = Display_Color_Black;

// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(PIN_B, PIN_A, RotaryEncoder::LatchMode::TWO03);

// Define some constants for rotary encoder reading:
// the maximum acceleration is 10 times.
constexpr float m = 10;
// at 200ms or slower, there should be no acceleration. (factor 1)
constexpr float longCutoff = 50;
// at 5 ms, we want to have maximum acceleration (factor m)
constexpr float shortCutoff = 5;

// To derive the calc. constants, compute as follows:
// On an x(ms) - y(factor) plane resolve a linear formular factor(ms) = a * ms + b;
// where  f(4)=10 and f(200)=1
constexpr float a = (m - 1) / (shortCutoff - longCutoff);
constexpr float b = 1 - longCutoff * a;
// a global variable to hold the last position of rotary encoder.
int encoderNewPos;
static int encoderLastPos = 0;
long deltaTicks;
int8_t encoder_diff;

int8_t temp_gas_list_index, prev_gas_list_index;

void setupDisplay()
{
  tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
  tft.setRotation(3);
  tft.setFont();
  tft.fillScreen(Display_Background_Color);
  tft.setTextColor(Display_Text_Color);
  tft.setTextSize(1);

  current_page = 0;
  last_page = -1;
  prev_mfc_pv = -1;
  prev_mfc_sv = -1;
  prev_gas_list_index = -1;

  pinMode(PUSH_BTN, INPUT_PULLUP);
}

void updateDisplay()
{
  updateOnButtonPush();
  updateOnEncoderRotation();
  updateOnPageChange();
  updateOnEachIteration();
}

void updateOnButtonPush()
{
  if (digitalRead(PUSH_BTN) == LOW)
  { // Check if the button is pressed.
    switch (current_page)
    {
    case 0: // home page
      current_page = current_menu_item + 1;
      break;
    case 1: // setpoint page
      if (change_setpoint_active)
      {
        change_setpoint_active = 0;
        drawMenuRect();
      }
      else
      {
        switch (current_menu_item)
        {
        case 2:
          targetSetpoint = temp_mfc_sv;
          EEPROM.put(SetpointEEPROMAddress, targetSetpoint); // Put mfc setpoint to EEPROM.
          current_page = 0;
          break;
        case 3:
          temp_mfc_sv = targetSetpoint;
          current_page = 0;
          break;
        default:
          break;
        }
      }
      break;
    case 2: // gas select page
      if (select_gas_active)
      {
        select_gas_active = 0;
        drawMenuRect();
      }
      else
      {
        switch (current_menu_item)
        {
        case 2:
          gas_list_index = temp_gas_list_index;
          EEPROM.put(GasListIndexEEPROMAddress, gas_list_index % 9); // Put gas_list_index to EEPROM.
          current_page = 0;
          break;
        case 3:
          temp_gas_list_index = gas_list_index;
          current_page = 0;
          break;
        default:
          break;
        }
      }
      break;
    case 3: // totalizer page
      current_page = 0;
      break;
    default:
      break;
    }
    // Wait until button released (demo only! Blocking call!)
    while (digitalRead(PUSH_BTN) == LOW)
    {
      delay(1);
    }
  }
}

void updateOnEncoderRotation()
{
  encoder.tick();
  encoderNewPos = encoder.getPosition();
  if (encoderLastPos != encoderNewPos)
  { // Check if the rotary encoder state has changed.

    // accelerate when there was a previous rotation in the same direction.
    unsigned long ms = encoder.getMillisBetweenRotations();

    if (ms < longCutoff)
    {
      // do some acceleration using factors a and b

      // limit to maximum acceleration
      if (ms < shortCutoff)
      {
        ms = shortCutoff;
      }

      float ticksActual_float = a * ms + b;
      deltaTicks = (long)ticksActual_float * (encoderNewPos - encoderLastPos);

      encoderNewPos = encoderNewPos + deltaTicks;
      encoder.setPosition(encoderNewPos);
    }
    encoder_diff = encoderNewPos - encoderLastPos;

    switch (current_page)
    {
    case 0: // home page
      drawMenuRect(TOP_LEFT, BOTTOM_LEFT);
      break;

    case 1: // setpoint page
      if (change_setpoint_active)
      {
        temp_mfc_sv += encoder_diff;
        temp_mfc_sv = constrain(temp_mfc_sv, 0, max_setpoint);
      }
      else
      {
        drawMenuRect(BOTTOM_LEFT, BOTTOM_RIGHT);
      }

      break;
    case 2: // gas select page
      if (select_gas_active)
      {
        temp_gas_list_index += encoder_diff;
        temp_gas_list_index = constrain(temp_gas_list_index, 0, gas_list_length - 1);
      }
      else
      {
        drawMenuRect(BOTTOM_LEFT, BOTTOM_RIGHT);
      }
      break;
    case 3: // totalizer page
      break;
    default:
      break;
    }
    encoderLastPos = encoderNewPos;
  }
}

void updateOnPageChange()
{
  if (last_page != current_page)
  { // Update these elements once only if the page is changed.
    last_page = current_page;
    tft.fillScreen(Display_Background_Color);
    switch (current_page)
    {

    case 0: // home page
      current_menu_item = 0;
      prev_mfc_pv = -1; // Force the display to update.

      printTextOnScreen(1, menu_x_points[0], menu_y_points[0], "", "SETPNT");

      printTextOnScreen(1, menu_x_points[1], menu_y_points[1], "", "SELECT");
      printTextOnScreen(1, menu_x_points[1] + 8, menu_y_points[1] + 12, "", "GAS");

      printTextOnScreen(1, menu_x_points[2] + 3, menu_y_points[2], "", "mm:ss");
      printTextOnScreen(1, menu_x_points[2], menu_y_points[2] + 12, "", "TOTLZR");

      printTextOnScreen(1, 128, 50, "", "SCCM");
      printTextOnScreen(1, 128, 62, "", gas_list[gas_list_index]);

      drawMenuRect();

      break;
    case 1: // setpoint page
      change_setpoint_active = 1;
      current_menu_item = 2;
      prev_mfc_sv = -1; // Force the display to update.
      // temp_mfc_sv = targetSetpoint;

      printTextOnScreen(2, 30, 15, "", "Setpoint:");
      printTextOnScreen(1, menu_x_points[2] + 8, menu_y_points[2] + 7, "", "SET");
      printTextOnScreen(1, menu_x_points[3], menu_y_points[3] + 7, "", "CANCEL");

      break;

    case 2: // gas select page
      select_gas_active = 1;
      current_menu_item = 2;
      prev_gas_list_index = -1; // Force the display to update.
      // temp_gas_list_index = gas_list_index;

      printTextOnScreen(2, 15, 15, "", "Select Gas:");
      printTextOnScreen(1, menu_x_points[2] + 8, menu_y_points[2] + 7, "", "SET");
      printTextOnScreen(1, menu_x_points[3], menu_y_points[3] + 7, "", "CANCEL");

      break;

    case 3: // totalizer page
      // set_totalizer_active = 1;
      current_menu_item = 2;

      printTextOnScreen(2, 25, 15, "", "Totalizer:");
      printTextOnScreen(1, menu_x_points[2] + 8, menu_y_points[2] + 7, "", "SET");
      printTextOnScreen(1, menu_x_points[3], menu_y_points[3] + 7, "", "CANCEL");

      break;

    default:
      break;
    }
  }
}

void updateOnEachIteration()
{
  static char buffer1[6];
  static char buffer2[6];

  if (millis() - previousMillis_lcd >= lcd_refresh_interval)
  { // Continously update these items according to page number. Like 'mfc_pv' etc...
    previousMillis_lcd = millis();
    switch (current_page)
    {
    case 0: // home page
      printNumOnScreen(4, 24, 48, prev_mfc_pv, mfc_pv, buffer1);
      printNumOnScreen(1, menu_x_points[0] + 8, menu_y_points[0] + 12, prev_mfc_sv, targetSetpoint, buffer2);
      break;

    case 1: // setpoint page
      printNumOnScreen(4, 34, 48, prev_mfc_sv, temp_mfc_sv, buffer2);
      break;

    case 2: // gas select page
      printTextOnScreen(4, 34, 48, gas_list[prev_gas_list_index], gas_list[temp_gas_list_index]);
      prev_gas_list_index = temp_gas_list_index;
      break;

    case 3: // totalizer page
      break;
    default:
      break;
    }
  }
}

void drawMenuRect(Corner firstCorner, Corner lastCorner)
{
  if (firstCorner != Corner::NONE && lastCorner != Corner::NONE)
  { // If the function is called without arguments (so they are assigned to default values of "NONE" both), just draw a rectangle in the current menu item's position.
    tft.drawRect(menu_x_points[current_menu_item] - 3, menu_y_points[current_menu_item] - 3, 40, 26, Display_Background_Color);
    current_menu_item += encoder_diff;
    current_menu_item = constrain(current_menu_item, firstCorner, lastCorner);
  }
  tft.drawRect(menu_x_points[current_menu_item] - 3, menu_y_points[current_menu_item] - 3, 40, 26, Display_Text_Color);
}

void printNumOnScreen(int textSize, int cursorX, int cursorY, float &prevValue, float newValue, char *buffer)
{
  tft.setTextSize(textSize);
  if (prevValue != newValue)
  {
    tft.setCursor(cursorX, cursorY);
    tft.setTextColor(Display_Background_Color);
    tft.print(buffer); // delete previous value
    prevValue = newValue;
  }
  tft.setCursor(cursorX, cursorY);
  tft.setTextColor(Display_Text_Color);
  dtostrf(newValue, 4, 0, buffer);
  tft.print(buffer); // print new value
}

void printTextOnScreen(int textSize, int cursorX, int cursorY, const char *prevText, const char *newText)
{
  tft.setTextSize(textSize);
  if (strcmp(prevText, newText))
  { // If two strings are equal, strcmp will return 0.
    tft.setCursor(cursorX, cursorY);
    tft.setTextColor(Display_Background_Color);
    tft.print(prevText); // delete previous text.
  }
  tft.setCursor(cursorX, cursorY);
  tft.setTextColor(Display_Text_Color);
  tft.print(newText); // print new text
}

// Note: checkEncoderButton() and checkEncoderRotation() were written for future implementation to make maintaining the code easier.
bool checkEncoderButton()
{
  if (digitalRead(PUSH_BTN) == LOW)
    return true;
  else
    return false;
}

int8_t checkEncoderRotation()
{
  encoder.tick();
  encoderNewPos = encoder.getPosition();
  if (encoderLastPos != encoderNewPos)
  { // Check if the rotary encoder state has changed.

    // accelerate when there was a previous rotation in the same direction.
    unsigned long ms = encoder.getMillisBetweenRotations();

    if (ms < longCutoff)
    {
      // do some acceleration using factors a and b

      // limit to maximum acceleration
      if (ms < shortCutoff)
      {
        ms = shortCutoff;
      }

      float ticksActual_float = a * ms + b;
      deltaTicks = (long)ticksActual_float * (encoderNewPos - encoderLastPos);

      encoderNewPos = encoderNewPos + deltaTicks;
      encoder.setPosition(encoderNewPos);
    }
    encoder_diff = encoderNewPos - encoderLastPos;
  }
  encoderLastPos = encoderNewPos;
  return encoder_diff;
}