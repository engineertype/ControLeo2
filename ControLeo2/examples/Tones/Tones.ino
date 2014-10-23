/*
 ControLeo2 Library - Tones
 
 This is a very simple demo of ControLeo's tune playing ability.  It uses the Arduino
 tones library. 
 
 Released under WTFPL license.
 
21 October 2014 by Peter Easton
 
*/

#include <ControLeo2.h>
#include "pitches.h"
ControLeo2_LiquidCrystal lcd;

int winner[] = {NOTE_C5,4,NOTE_G4,8,NOTE_G4,8, NOTE_A4,4,NOTE_G4,4,0,4,NOTE_B4,4,NOTE_C5,4,-1};
int twinkle[] = {NOTE_C6,4,NOTE_C6,4,NOTE_G6,4,NOTE_G6,4,NOTE_A6,4,NOTE_A6,4,NOTE_G6,2,NOTE_F6,4,NOTE_F6,4,NOTE_E6,4,NOTE_E6,4,NOTE_D6,4,NOTE_D6,4,NOTE_C6,2,-1};


void setup() {
  // *********** Start of ControLeo2 initialization ***********
  // Set up the buzzer and buttons
  pinMode(CONTROLEO_BUZZER_PIN, OUTPUT);
  pinMode(CONTROLEO_BUTTON_TOP_PIN, INPUT_PULLUP);
  pinMode(CONTROLEO_BUTTON_BOTTOM_PIN, INPUT_PULLUP);
  // Set the relays as outputs and turn them off
  // The relay outputs are on D4 to D7 (4 outputs)
  for (int i=4; i<8; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  // Set up the LCD's number of rows and columns 
  lcd.begin(16, 2);
  // Create the degree symbol for the LCD
  unsigned char degree[8]  = {12,18,18,12,0,0,0,0};
  lcd.createChar(0, degree);
  // *********** End of ControLeo2 initialization ***********
  
  // Write the initial message on the LCD screen
  lcd.setCursor(0, 0);
  lcd.print(" Press a button");
  lcd.setCursor(0, 1);
  lcd.print(" to hear a tone");
}


void loop() {
  // See if a button has been pressed
  switch (getButton()) {
    case CONTROLEO_BUTTON_TOP:
      playTones(winner);
      break;
    case CONTROLEO_BUTTON_BOTTOM:
      playTones(twinkle);
      break;
  }

  // Wait a bit
  delay(10);
}


// Play a tone
// Parameter: tone - an array containing alternating notes and note duration, terminated by -1
void playTones(int *tones) {
  for (int i=0; tones[i] != -1; i+=2) {
    // Note durations: 4 = quarter note, 8 = eighth note, etc.:   
    int duration = 1000/tones[i+1];
    tone(CONTROLEO_BUZZER_PIN, tones[i], duration);
    delay(duration * 1.1);
  }
  noTone(CONTROLEO_BUZZER_PIN);
}


// Determine if a button was pressed (with debounce)
// A button can only be pressed once every 200ms. If a button is
// pressed and held, a button press will be generated every 200ms.
// Returns:
//   CONTROLEO_BUTTON_NONE if no button are pressed
//   CONTROLEO_BUTTON_TOP if the top button was pressed
//   CONTROLEO_BUTTON_BOTTOM if the bottom button was pressed
// Note: If both buttons are pressed simultaneously, CONTROLEO_BUTTON_TOP will be returned
#define DEBOUNCE_INTERVAL  200

int getButton()
{
  static long lastChangeMillis = 0;
  long nowMillis = millis();
  int buttonValue;
  
  // If insufficient time has passed, just return none pressed
  if (lastChangeMillis + DEBOUNCE_INTERVAL > nowMillis)
    return CONTROLEO_BUTTON_NONE;
  
  // Read the current button status
  buttonValue = CONTROLEO_BUTTON_NONE;
  if (digitalRead(CONTROLEO_BUTTON_TOP_PIN) == LOW)
    buttonValue = CONTROLEO_BUTTON_TOP;
  else if (digitalRead(CONTROLEO_BUTTON_BOTTOM_PIN) == LOW)
    buttonValue = CONTROLEO_BUTTON_BOTTOM;
    
  // Note the time the button was pressed
  if (buttonValue != CONTROLEO_BUTTON_NONE)
   lastChangeMillis = nowMillis;
  
  return buttonValue;
}

