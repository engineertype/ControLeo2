// Bake logic
// Called from the main loop 20 times per second
// This where the bake logic is controlled

extern char debugBuffer[];

#define MILLIS_TO_SECONDS    ((long) 1000)

// Return false to exit this mode
boolean Bake() {
  static int bakePhase = BAKING_PHASE_INIT;
  static int outputType[4];
  static int bakeTemperature;
  static uint16_t bakeDuration;
  static int elementDutyCounter[4];
  static int bakeDutyCycle, bakeIntegral, counter, coolingDuration;
  static boolean isHeating;
  static long lastOverTempTime = 0;
  
  double currentTemperature;
  int i;
  boolean isOneSecondInterval = false;

  // Determine if this is on a 1-second interval
  if (++counter >= 20) {
    counter = 0;
    isOneSecondInterval = true;
  }
  
  // Read the temperature
  currentTemperature = getCurrentTemperature();
  if (THERMOCOUPLE_FAULT(currentTemperature)) {
    lcdPrintLine(0, "Thermocouple err");
    Serial.print(F("Thermocouple Error: "));
    switch ((int) currentTemperature) {
      case FAULT_OPEN:
        lcdPrintLine(1, "Fault open");
        Serial.println(F("Fault open"));
        break;
      case FAULT_SHORT_GND:
        lcdPrintLine(1, "Short to GND");
        Serial.println(F("Short to ground"));
        break;
      case FAULT_SHORT_VCC:
        lcdPrintLine(1, "Short to VCC");
        break;
    }
    
    // Abort the bake
    Serial.println(F("Bake aborted because of thermocouple error!"));
    bakePhase = BAKING_PHASE_ABORT;
    delay(3000);
  }
  
  // Abort the bake if a button is pressed
  if (getButton() != CONTROLEO_BUTTON_NONE) {
    bakePhase = BAKING_PHASE_ABORT;
    lcdPrintLine(0, "Aborting bake");
    lcdPrintLine(1, "Button pressed");
    Serial.println(F("Button pressed.  Aborting bake ..."));
    delay(2000);
  }
  
  switch (bakePhase) {
    case PHASE_INIT: // User has requested to start a bake
      // Start the bake, regardless of the starting temperature
      // Get the types for the outputs (elements, fan or unused)
      for (i=0; i<4; i++)
        outputType[i] = getSetting(SETTING_D4_TYPE + i);
      // Get the bake temperature
      bakeTemperature = getSetting(SETTING_BAKE_TEMPERATURE);
      // Get the bake duration
      bakeDuration = getBakeSeconds(getSetting(SETTING_BAKE_DURATION));
      Serial.print(F("Baking temperature = "));
      Serial.println(bakeTemperature);
      Serial.print(F("Baking duration = "));
      Serial.println(bakeDuration);
      
      // Don't allow bake if the outputs are not configured
      for (i=0; i<4; i++)
        if (isHeatingElement(outputType[i]))
          break;
      if (i == 4) {
        lcdPrintLine(0, "Please configure");
        lcdPrintLine(1, " outputs first! ");
        Serial.println(F("Outputs must be configured before baking"));
        
        // Abort the baking
        bakePhase = BAKING_PHASE_ABORT;
        delay(3000);
        break;
      }

      // If there is a convection fan then turn it on now
      for (i=0; i< 4; i++) {
        if (outputType[i] == TYPE_CONVECTION_FAN)
          digitalWrite(4 + i, HIGH);
      }
      
      // Move to the next phase
      bakePhase = BAKING_PHASE_HEATUP;
      lcdPrintLine(0, bakingPhaseDescription[bakePhase]);
      lcdPrintLine(1, "");

      // Start with a duty cycle proportional to the desired temperature
      bakeDutyCycle = map(bakeTemperature, 0, 250, 0, 100);
      
      isHeating = true;
      bakeIntegral = 0;
      counter = 0;
      
      // Stagger the element start cycle to avoid abrupt changes in current draw
      // Simple method: there are 4 outputs so space them apart equally
      for (i=0; i< 4; i++)
        elementDutyCounter[i] = 25 * i;
      break;

    case BAKING_PHASE_HEATUP:
      if (isOneSecondInterval) {
        // Display the remaining time
        DisplayBakeTime(bakeDuration, currentTemperature, bakeDutyCycle, bakeIntegral);

        // Don't start decrementing bakeDuration until close to baking temperature
      }
        
      // Is the oven close to the desired temperature?
      if (bakeTemperature - currentTemperature < 15.0) {
        bakePhase = BAKING_PHASE_BAKE;
        lcdPrintLine(0, bakingPhaseDescription[bakePhase]);
        // Reduce the duty cycle for the last 10 degrees
        bakeDutyCycle = bakeDutyCycle / 3;
        Serial.println(F("Move to bake phase"));
       }
       break;
       
    case BAKING_PHASE_BAKE:
      // Make changes every second
      if (isOneSecondInterval) {
        // Display the remaining time
        DisplayBakeTime(bakeDuration, currentTemperature, bakeDutyCycle, bakeIntegral);
        
        // Has the bake duration been reached?
        if (--bakeDuration == 0) {
          bakePhase = BAKING_PHASE_START_COOLING;
          break;
        }

        // Is the oven too hot?
        if (currentTemperature > bakeTemperature) {
          if (isHeating) {
            isHeating = false;
            // Turn all heating elements off
            for (i=0; i< 4; i++) {
              if (isHeatingElement(outputType[i]))
                digitalWrite(4 + i, LOW);
            }

            // The duty cycle caused the temperature to exceed the bake temperature, so decrease it
            // (but not more than once every 30 seconds)
            if (millis() - lastOverTempTime > (30 * MILLIS_TO_SECONDS)) {
              lastOverTempTime = millis();
              if (bakeDutyCycle > 0)
                bakeDutyCycle--;
            }

            // Reset the bake integral, so it will be slow to increase the duty cycle again
            bakeIntegral = 0;
            Serial.println(F("Over-temp. Elements off"));
          }
          // No more to do here
          break;
        }

        // The oven is heating up
        isHeating = true;

        // Increase the bake integral if not close to temperature
        if (bakeTemperature - currentTemperature > 1.0)
          bakeIntegral++;
          
        // Has the oven been under-temperature for a while?
        if (bakeIntegral > 30) {
          bakeIntegral = 0;
          // Increase duty cycles
          if (bakeDutyCycle < 100)
            bakeDutyCycle++;
            Serial.println(F("Under-temp. Increasing duty cycle"));
        }
      }
      break;

    case BAKING_PHASE_START_COOLING:
      Serial.println(F("Starting cooling"));
      isHeating = false;
      
      // Turn off all elements and turn on the fans
      for (i=0; i< 4; i++) {
        switch (outputType[i]) {
           case TYPE_CONVECTION_FAN:
           case TYPE_COOLING_FAN:
             digitalWrite(4 + i, HIGH);
             break;
           default:
             digitalWrite(4 + i, LOW);
             break;
        }
      }
      
      // Move to the next phase
      bakePhase = BAKING_PHASE_COOLING;
      lcdPrintLine(0, bakingPhaseDescription[bakePhase]);

      // If a servo is attached, use it to open the door over 10 seconds
      setServoPosition(getSetting(SETTING_SERVO_OPEN_DEGREES), 10000);
      // Play a tune to let the user know the door should be opened
      playTones(TUNE_REFLOW_DONE);

      // Cooling should be at least 1 minute (60 seconds) in duration
      coolingDuration = 60;
      break;

    case BAKING_PHASE_COOLING:
      if (isOneSecondInterval) {
        // Display the remaining time
        DisplayBakeTime(bakeDuration, currentTemperature, bakeDutyCycle, bakeIntegral);

        // Wait in this phase until the oven has cooled
        if (coolingDuration > 0)
          coolingDuration--;      
        if (currentTemperature < 50.0 && coolingDuration == 0)
          bakePhase = BAKING_PHASE_ABORT;
      }
      break;

     case BAKING_PHASE_ABORT:
      Serial.println(F("Bake is done!"));
      isHeating = false;
      // Turn all elements and fans off
      for (i = 4; i < 8; i++)
        digitalWrite(i, LOW);
      // Close the oven door now, over 3 seconds
      setServoPosition(getSetting(SETTING_SERVO_CLOSED_DEGREES), 3000);
      // Start next time with initialization
      bakePhase = BAKING_PHASE_INIT;
      // Return to the main menu
      return false;
  }
 
  // Turn the outputs on or off based on the duty cycle
  if (isHeating) {
    for (i=0; i< 4; i++) {
      switch (outputType[i]) {
        case TYPE_TOP_ELEMENT:
        case TYPE_BOTTOM_ELEMENT:
          // Turn the output on at 0, and off at the duty cycle value
          if (elementDutyCounter[i] == 0)
            digitalWrite(4 + i, HIGH);
          if (elementDutyCounter[i] == bakeDutyCycle)
            digitalWrite(4 + i, LOW);
          break;
          
        case TYPE_BOOST_ELEMENT: // Give it half the duty cycle of the other elements
          // Turn the output on at 0, and off at the duty cycle value
          if (elementDutyCounter[i] == 0)
            digitalWrite(4 + i, HIGH);
          if (elementDutyCounter[i] == bakeDutyCycle/2)
            digitalWrite(4 + i, LOW);
          break;

        default:
          // Skip unused elements and fans
          break;
      }
      
      // Increment the duty counter
      elementDutyCounter[i] = (elementDutyCounter[i] + 1) % 100;
    }
  }
 
  return true;
}


// Display the current temperature to the LCD screen and print it to the serial port so it can be plotted
void DisplayBakeTime(uint16_t duration, double temperature, int duty, int integral) {
  // Display the temperature on the LCD screen
  displayTemperature(temperature);

  // Write the time and temperature to the serial port, for graphing or analysis on a PC
  sprintf(debugBuffer, "%u, %i, %i, ", duration, duty, integral);
  Serial.print(debugBuffer);
  Serial.println(temperature);

  displayDuration(10, duration);
}


