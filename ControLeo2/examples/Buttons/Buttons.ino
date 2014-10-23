/*
 ControLeo Library - Buttons
 
 ControLeo has two buttons.  This example shows how to read the buttons
 (with debounce).  The buttons are conected to D2 and D11.
 
 The buttons rely on the pullup resistor internal to the microcontroller
 to keep the signal level HIGH when the button is not pressed.  Pressing
 the button shorts the pin to ground, and the signal becomes LOW.
 
 Released under WTFPL license.
 
 21 October 2014 by Peter Easton
 
*/

#include <ControLeo2.h>
ControLeo2_LiquidCrystal lcd;
ControLeo2_MAX31855 thermocouple;


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
  lcd.print("Top");
  lcd.setCursor(0, 1);
  lcd.print("Bottom");
}

void loop() {
  static int topCount=0, bottomCount=0;
  
  switch (getButton()) {
    case CONTROLEO_BUTTON_NONE:
      break;
    case CONTROLEO_BUTTON_TOP:
      topCount++;
      lcd.setCursor(13, 0);
      lcd.print(topCount);
      break;
    case CONTROLEO_BUTTON_BOTTOM:
      bottomCount++;
      lcd.setCursor(13, 1);
      lcd.print(bottomCount);
      break;
  }
  delay(10);
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








