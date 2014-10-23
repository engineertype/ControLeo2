  /*
  ControLeo Library - TextDirection
 
 Demonstrates the use a 16x2 LCD display.  
 
 This sketch demonstrates how to use leftToRight() and rightToLeft()
 to move the cursor.
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe 
 modified 25 July 2009
 by David A. Mellis
 modified 21 October 2014 to work with ControLeo2
 by Peter Easton
 
 http://www.arduino.cc/en/Tutorial/LiquidCrystal
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
  Serial.begin(9600);
}

void loop() {
  static char thisChar = 'a';
  // reverse directions at 'm':
  if (thisChar == 'm') {
    // go right for the next letter
    lcd.rightToLeft(); 
  }
  // reverse again at 's':
  if (thisChar == 's') {
    // go left for the next letter
    lcd.leftToRight(); 
  }
  // reset at 'z':
  if (thisChar > 'z') {
    // go to (0,0):
    lcd.home(); 
    // start again at 0
    thisChar = 'a';
  }
  // print the character
  lcd.write(thisChar);
  // wait a second:
  delay(500);
  // increment the letter:
  thisChar++;
}








