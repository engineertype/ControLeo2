/*******************************************************************************
* ControLeo Reflow Oven Controller
* Author: Peter Easton
* Website: whizoo.com
* Thanks to Rocketscream for the original code using the PID library.  The code has
* been heavily modified (removing PID) to give finer control over individual heating
* elements.
* 
* This is an example of a reflow oven controller. The reflow curve below is for a
* lead-free profile, but this code supports both leaded and lead-free profiles.
*
* Temperature (Degree Celcius)                 Magic Happens Here!
* 245-|                                               x  x  
*     |                                            x        x
*     |                                         x              x
*     |                                      x                    x
* 200-|                                   x                          x
*     |                              x    |                          |   x   
*     |                         x         |                          |       x
*     |                    x              |                          |
* 150-|               x                   |                          |
*     |             x |                   |                          |
*     |           x   |                   |                          | 
*     |         x     |                   |                          | 
*     |       x       |                   |                          | 
*     |     x         |                   |                          |
*     |   x           |                   |                          |
* 30 -| x             |                   |                          |
*     |<  60 - 90 s  >|<    90 - 120 s   >|<       90 - 120 s       >|
*     | Preheat Stage |   Soaking Stage   |       Reflow Stage       | Cool
*  0  |_ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _ _ _|_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
*                                                                Time (Seconds)
*
* This firmware builds on the work of other talented individuals:
* ==========================================
* Rocketscream (www.rocketscream.com)
* Produced the Arduino reflow oven shield ands code that inspired this project.
*
* ==========================================
* Limor Fried of Adafruit (www.adafruit.com)
* Author of Arduino MAX6675 library. Adafruit has been the source of tonnes of
* tutorials, examples, and libraries for everyone to learn.
*
* Disclaimer
* ==========
* Dealing with high voltage is a very dangerous act! Please make sure you know
* what you are dealing with and have proper knowledge before hand. Your use of 
* any information or materials on this reflow oven controller is entirely at 
* your own risk, for which we shall not be liable. 
*
* Released under WTFPL license.
*
* Revision  Description
* ========  ===========
* 1.00      Initial public release.
* 1.1       Minor bug fixes
* 1.2       Modified to work on ControLeo2 (21 October 2014)
*******************************************************************************/



// ***** INCLUDES *****
#include <ControLeo2.h>

// ***** TYPE DEFINITIONS *****
typedef enum
{
  REFLOW_STATUS_OFF,
  REFLOW_STATUS_ON
} reflowStatus_t;

typedef enum
{
  REFLOW_TYPE_LEAD_FREE,
  REFLOW_TYPE_LEADED
} reflowType_t;


// ***** CONSTANTS *****
#define SENSOR_SAMPLING_TIME 1000  // Frequency of reading temperature, in milliseconds
#define ROOM_TEMP            50    // Temperature above which reflow process cannot be started
#define TEMPERATURE_POINTS   5     // Number of phases in reflow process
#define NO_OF_ELEMENTS       3     // NUmber of heating elements in the reflow oven

// This array defines the temperature transition points during the relow process
int tempPoints[2][TEMPERATURE_POINTS] = {
      {50, 150, 200, 230, 235},  // Lead-free
      {50, 130, 170, 185, 190},  // Leaded
    };
    
// Each element of this array is a 8-second window for an element.  For example, if the value is 0b11001111 then
// the element will be on for 2 seconds, off for 2 seconds then on for 4 seconds.  This pattern will keep 
// repeating itself until the temperature rises through the temperate transition point given in tempPoints.  This
// gives fine control over each element and has the following benefits:
// 1. Prevents individual elements from getting too hot, perhaps burning insulation.
// 2. Ensures heat comes from the right part of the oven at the right time
// 3. Helps overall current draw by being able to turn off some elements while turning others on
// ==================== YOU SHOULD TUNE THESE VALUES TO YOUR REFLOW OVEN!!! ====================
int elementCycle[2][NO_OF_ELEMENTS][TEMPERATURE_POINTS] = {
      {    // Lead-free
        {0b11011101, 0b11001101, 0b00110011, 0b11111101, 0b11111100},  // Upper element
        {0b10111111, 0b10111110, 0b11101110, 0b11101111, 0b11100111},  // Lower element
        {0b11101110, 0b01010011, 0b00010001, 0b10111010, 0b00011000},  // Boost Element
      },
      {    // Leaded
        {0b11011101, 0b11001101, 0b01000100, 0b11111110, 0b00010001},  // Upper element
        {0b11111111, 0b11111110, 0b10101011, 0b11111111, 0b01000100},  // Lower element
        {0b11101111, 0b01110011, 0b00010000, 0b11101101, 0b00000000},  // Boost Element
      }
    };
    
const char* reflowPhases[TEMPERATURE_POINTS] = {"Pre-heat", "Pre-heat", "Soak", "Reflow", "Waiting"};

// Default reflow mode is lead-free solder
reflowType_t reflowType = REFLOW_TYPE_LEAD_FREE;
int reflowPhase;

// Pin assignments
int elementPins[NO_OF_ELEMENTS] = {4, 5, 6};

unsigned long nextCheck = 0;
unsigned long nextRead = 0;
unsigned long timerSoak;
unsigned long buzzerPeriod;

// Reflow oven controller status
reflowStatus_t reflowStatus = REFLOW_STATUS_OFF;

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
  lcd.print("ControLeo2");
  lcd.setCursor(0, 1);
  lcd.print("Reflow Oven 1.2");
  delay(3000);
  lcd.clear();
}


void loop()
{
  unsigned long timeNow = millis();
  static double currentTemperature;
  boolean updateThings = false;
  static int reflowToggleCounter;

  // Time to read thermocouple?
  if (timeNow > nextRead)
  {
    // Read current temperature
    currentTemperature = thermocouple.readThermocouple(CELSIUS);

    // If thermocouple problem detected
    if (currentTemperature == FAULT_OPEN || currentTemperature == FAULT_SHORT_GND || currentTemperature == FAULT_SHORT_VCC) {
      // There is a problem with the thermocouple
      displayMessage("Error reading", 0);
      
      // Abort the reflow
      stopReflow();
        
     // Done for now
      return;
    }
   // Set the next thermocouple read
    nextRead += SENSOR_SAMPLING_TIME;
  }
  
  // Should things be updated?
  if (timeNow > nextCheck) {
    updateThings = true;
    nextCheck = timeNow + 1000;
  }

  // See if any buttons are pressed
  int buttonPressed = getButton();
    
  // If reflow process is on going
  if (reflowStatus == REFLOW_STATUS_ON)
  {
    // Should the reflow be aborted?
    if (buttonPressed == CONTROLEO_BUTTON_BOTTOM) {
      // Abort the reflow
      stopReflow();
      return;
    }
      
    // Should elements be turned on or off?
    if (!updateThings)
      return;
      
    // Determine which reflow state we're in
    reflowPhase = 0;
    while (reflowPhase < TEMPERATURE_POINTS && currentTemperature > tempPoints[reflowType][reflowPhase])
      reflowPhase++;
      
    // Is the reflow done?
    if (reflowPhase == TEMPERATURE_POINTS) {
      stopReflow();
      
      // Turn on buzzer to indicate completion
      analogWrite(CONTROLEO_BUZZER_PIN, 20);
      // Turn off the buzzer after 1 second
      buzzerPeriod = timeNow + 1000;
      return;
    }
    
    // The reflow is underway.  Turn the elements on or off as necessary
    for (int i = 0; i < NO_OF_ELEMENTS; i++) {
      if (elementCycle[reflowType][i][reflowPhase] & reflowToggleCounter)
        digitalWrite(elementPins[i], HIGH);    // On
      else
        digitalWrite(elementPins[i], LOW);     // Off
    }
    
    // Advance the reflow toggle counter
    if (reflowToggleCounter == 1)
      reflowToggleCounter = 0x80;
    else 
      reflowToggleCounter = reflowToggleCounter >> 1;
  }
  else
  {
    // Reflow is currently off
    // Ignore button presses if too hot
    if (currentTemperature < ROOM_TEMP) {
      // Was the lead/lead-free button pressed?
      if (buttonPressed == CONTROLEO_BUTTON_TOP) {
        if (reflowType == REFLOW_TYPE_LEAD_FREE)
          reflowType = REFLOW_TYPE_LEADED;
        else
          reflowType = REFLOW_TYPE_LEAD_FREE;
        updateThings = true;
      }
        
      // Was the Start button pressed?
      if (buttonPressed == CONTROLEO_BUTTON_BOTTOM) {
        // Start the reflow
        reflowStatus = REFLOW_STATUS_ON;
        reflowToggleCounter = 0x80;
        reflowPhase = 0;
        updateThings = true;
      }
    }

    // Beep to indicate the boards can be removed at 100C
    if (currentTemperature >= 99 && currentTemperature <= 100) {
      // Turn on buzzer to indicate boards can be removed
      analogWrite(CONTROLEO_BUZZER_PIN, 20);
      // Turn off the buzzer after 100ms (plus the time spent in the 99-100 degree range)
      buzzerPeriod = timeNow + 100;
    }
  }
    
  // Display the current status
  if (updateThings) {
    if (reflowStatus == REFLOW_STATUS_OFF) {
      if (currentTemperature > ROOM_TEMP)
        displayMessage("Cool - open door", currentTemperature);
      else {
        if (reflowType == REFLOW_TYPE_LEAD_FREE)
          displayMessage("Lead-free solder", currentTemperature);
        else
          displayMessage("Leaded solder", currentTemperature);
      }
    }
    else {
      // Display the reflow status
      displayMessage(reflowPhases[reflowPhase], currentTemperature);
    }
  }
  
  // Turn off the buzzer?
  if (timeNow > buzzerPeriod)
    analogWrite(CONTROLEO_BUZZER_PIN, 0);
  
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


// Display a message and the temperature
// The existing display is overwritten rather than the screen cleared
// to avoid refresh flickering
void displayMessage(const char* msg, double temperature) 
{
  // Display the message and clear to the end of the line
  lcd.setCursor(0, 0);
  lcd.print(msg);
  for (int i = strlen(msg); i < 16; i++)
    lcd.print(" ");
  
  // Display the temperature on the 2nd line
  lcd.setCursor(0, 1);
  if (!temperature)
    lcd.print("thermocouple");
  else {
    // Print current temperature
    lcd.print(temperature);
    // Print degree Celsius symbol
    lcd.write((uint8_t)0);
    lcd.print("C         ");
  }
}


// Stop the reflow process
void stopReflow()
{
  // Turn all elements off
  for (int i = 0; i < NO_OF_ELEMENTS; i++)
    digitalWrite(elementPins[i], LOW);
        
  // Turn the reflow off
  reflowStatus = REFLOW_STATUS_OFF;
  
  // Force the display to be updated
  nextCheck = 0;
}


