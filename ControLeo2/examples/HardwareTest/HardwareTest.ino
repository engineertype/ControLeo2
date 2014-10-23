/*
 ControLeo2 Library - HardwareTest
 
 This is a very simple ControLeo hardware test.  It will display the temperature reading from the
 thermocouple, updated twice a second.  Pressing the top button will toggle the buzzer on and off.
 Pressing the bottom button will turn on the next relay output.
 
 Note:  If you connect the thermocouple probe the wrong way around, the temperature will go up
 instead of down (and vice versa).  No problem, just reverse the terminals.
 
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
  lcd.print("Testing");
  lcd.setCursor(0, 1);
  lcd.print("Press buttons");
}


void loop() {
  static int relay = 7;
  static int buzzer = 0;
  
  // Read the temperature in Farenheit
  float currentTemperature = thermocouple.readThermocouple(FAHRENHEIT);
  // Display the temperature on the left of the screen
  displayTemperature(currentTemperature, "F");
  
  // See if a button has been pressed
  switch (getButton()) {
    case CONTROLEO_BUTTON_TOP:
      // Turn the buzzer on and off when the top button is pressed (PWM rate is 15/255)
      buzzer = 15 - buzzer;
      analogWrite(CONTROLEO_BUZZER_PIN, buzzer);
      lcd.setCursor(0, 1);
      if (buzzer)
        lcd.print("Buzzer ON       ");
      else
        lcd.print("Buzzer OFF      ");
      break;
    case CONTROLEO_BUTTON_BOTTOM:
      // The bottom button turns on the next relay output
      digitalWrite(relay, LOW);
      if (++relay == 8)
        relay = 4;
      digitalWrite(relay, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("Output:");
      lcd.print(relay);
      lcd.print("       ");
      break;
  }

  // Wait a bit before taking the next set of readings
  delay(50);
}


// Display the temperature at the given location on the screen
void displayTemperature(float temperature, char* scale) 
{
  lcd.setCursor(9, 0);
  if (temperature == FAULT_OPEN || temperature == FAULT_SHORT_GND || temperature == FAULT_SHORT_VCC)
    lcd.print("       ");
  else {
    // Print current temperature
    lcd.print(temperature);
    // Print degree symbol
    lcd.write(0);
    lcd.print(scale);
    lcd.print("  ");
  }
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

