// The ATMEGA32U4 has 1024 bytes of EEPROM.  We use some of it to store settings so that
// the oven doesn't need to be reconfigured every time it is turned on.  Uninitialzed 
// EEPROM is set to 0xFF (255).  One of the first things done when powering up is to
// see if the EEPROM is uninitialized - and initialize it if that is the case.
//
// All settings are stored as bytes (unsigned 8-bit values).  This presents a problem
// for the maximum temperature which can be as high as 280C - which doesn't fit in a
// 8-bit variable.  With the goal of making getSetting/setSetting as simple as possible,
// we could've saved all values as 16-bit values, using consecutive EEPROM locations. We
// instead chose to just offset the temperature by 150C, making the range 25 to 130 instead
// of 175 to 280.

#include <EEPROM.h>


// Get the setting from EEPROM
int getSetting(int settingNum) {
  int val = EEPROM.read(settingNum);
  
  // The maximum temperature has an offset to allow it to be saved in 8-bits (0 - 255)
  if (settingNum == SETTING_MAX_TEMPERATURE)
    return val + TEMPERATURE_OFFSET;

  // Bake temperature is modified before saving
  if (settingNum == SETTING_BAKE_TEMPERATURE)
    return val * BAKE_TEMPERATURE_STEP;

  return val;
}


// Save the setting to EEPROM.  EEPROM has limited write cycles, so don't save it if the
// value hasn't changed.
void setSetting(int settingNum, int value) {
  // Do nothing if the value hasn't changed (don't wear out the EEPROM)
  if (getSetting(settingNum) == value)
    return;
  
  switch (settingNum) {
    case SETTING_D4_TYPE:
    case SETTING_D5_TYPE:
    case SETTING_D6_TYPE:
    case SETTING_D7_TYPE:
      // The element has been reconfigured so reset the duty cycles and restart learning
      EEPROM.write(SETTING_SETTINGS_CHANGED, true);
      EEPROM.write(SETTING_LEARNING_MODE, true);
      EEPROM.write(settingNum, value);
      Serial.println(F("Settings changed!  Duty cycles have been reset and learning mode has been enabled"));
      break;
      
    case SETTING_MAX_TEMPERATURE:
      // Enable learning mode if the maximum temperature has changed a lot
      if (abs(getSetting(settingNum) - value) > 5)
        EEPROM.write(SETTING_LEARNING_MODE, true);
      // Write the new maximum temperature
      EEPROM.write(settingNum, value - TEMPERATURE_OFFSET);
      break;
      
    case SETTING_BAKE_TEMPERATURE:
      EEPROM.write(settingNum, value / BAKE_TEMPERATURE_STEP);
      break;

    default:
      EEPROM.write(settingNum, value);
      break;
  }
}


void InitializeSettingsIfNeccessary() {
  // Does the EEPROM need to be initialized?
  if (getSetting(SETTING_EEPROM_NEEDS_INIT)) {
    // Initialize all the settings to 0 (false)
    for (int i=0; i<1024; i++)
      EEPROM.write(i, 0);
    // Set a reasonable max temperature
    setSetting(SETTING_MAX_TEMPERATURE, 240);
    // Set the servos to neutral positions (90 degrees)
    setSetting(SETTING_SERVO_CLOSED_DEGREES, 90);
    setSetting(SETTING_SERVO_OPEN_DEGREES, 90);
    // Set default baking temperature
    setSetting(SETTING_BAKE_TEMPERATURE, BAKE_MIN_TEMPERATURE);
  }
  
  // Legacy support - Initialize the rest of EEPROM for upgrade from 1.x to 1.4
  if (getSetting(SETTING_SERVO_OPEN_DEGREES) > 180) {
    for (int i=SETTING_SERVO_OPEN_DEGREES; i<1024; i++)
      EEPROM.write(i, 0);
    setSetting(SETTING_SERVO_CLOSED_DEGREES, 90);
    setSetting(SETTING_SERVO_OPEN_DEGREES, 90);
    setSetting(SETTING_BAKE_TEMPERATURE, BAKE_MIN_TEMPERATURE);
  }
}


// Returns the bake duration, in seconds (max 65536 = 18 hours)
uint16_t getBakeSeconds(int duration)
{
  uint16_t minutes;

  // Sanity check on the parameter
  if (duration >= BAKE_MAX_DURATION)
    return 5;
  
  // 5 to 60 minutes, at 1 minute increments
  if (duration <= 55)
    minutes = duration + 5;

  // 1 hour to 4 hours at 5 minute increments
  else if (duration <= 91)
    minutes = (duration - 55) * 5 + 60;

  // 4+ hours in 10 minute increments
  else
    minutes = (duration - 91) * 10 + 240;

  return minutes * 60;
}

