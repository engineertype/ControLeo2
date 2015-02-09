// Keep some strings in Flash instead of RAM.  RAM space is limited to 2.5Kb

#include <avr/pgmspace.h>

PROGMEM const char  s0[] = "ControLeo2 Reflow Oven controller v1.4";
PROGMEM const char  s1[] = "Reflow aborted because of thermocouple error!";
PROGMEM const char  s2[] = "Button pressed.  Aborting reflow ...";
PROGMEM const char  s3[] = "Oven too hot to start reflow.  Please wait ...";
PROGMEM const char  s4[] = "Settings changed by user.  Reinitializing element duty cycles and enabling learning mode ...";
PROGMEM const char  s5[] = "Learning mode is enabled.  Duty cycles may be adjusted automatically if necessary";
PROGMEM const char  s6[] = "Adjustments have been made to duty cycles for this phase.  Continuing ...";
PROGMEM const char  s7[] = "Adjustments have been made to duty cycles for this phase.  Aborting ...";
PROGMEM const char  s8[] = "Duty cycles lowered slightly for future runs";
PROGMEM const char  s9[] = "Duty cycles increased slightly for future runs";
PROGMEM const char s10[] = "Aborting reflow.  Oven cannot reach required temperature!";
PROGMEM const char s11[] = "******* Phase: Waiting *******";
PROGMEM const char s12[] = "Turning all heating elements off ...";
PROGMEM const char s13[] = "******* Phase: Cooling *******";
PROGMEM const char s14[] = "Open the oven door ...";
PROGMEM const char s15[] = "Reflow is done!";

PROGMEM const char * const phrases[FLASH_LAST_STRING+1] = {s0,s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15};


// Prints the given string number to the USB port
void printToSerial(int msgNum) {
  if (msgNum > FLASH_LAST_STRING)
    return;
  // Copy the word group from flash into RAM
  strcpy_P(debugBuffer, (char*) pgm_read_word(&(phrases[msgNum])));
  Serial.println(debugBuffer);
}
