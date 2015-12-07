// Baking
// Called from the main loop 20 times per second
// This mode allows to simply keep the oven at a specified
// temperature for an also specified time/duration. This
// can get used for incubating cell cultures, baking "Fimo"
// modelling clay or simply to make a pizza oven out of your
// SMD solder reflow oven again.
//
// But take care not to mix up eatables with your hobby stuff !!!

// This sketch is a copy from "Reflow.ino" with many simplifications


#define MILLIS_TO_SECONDS(x)    ((long)((x)/1000))
#define SECONDS_TO_MILLIS(x)    (((long)x)*1000)
#define OUTPUT_COUNT 4

#define DUTY_PERIOD  1024
#define REGULATOR_PERIOD  ((double)(DUTY_PERIOD/10))    // The regulator can't get called during each iteration this would diminish the amplification resolutionx
#define T_FULL_POS  500.0  // The border temperature which should cause a full-duty-cycle-up or full-duty-cycle-down during one regulator period
#define T_FULL_NEG  10.0  // The border temperature which should cause a full-duty-cycle-up or full-duty-cycle-down during one regulator period
#define P_REGULATOR_AMPLIFICATION_POS (REGULATOR_PERIOD / T_FULL_POS);
#define P_REGULATOR_AMPLIFICATION_NEG (REGULATOR_PERIOD / T_FULL_NEG);


class BakeImplementation {

  /************************************************
   * CLASS VARIABLES
   *
   * Those will be valid for a whole baking run
   ***********************************************/

protected:
  /**
   * The current phase of the baking process
   *
   * @var int
   */
  byte bakingPhase = BAKING_PHASE_INIT;

  /**
   * The target temperature for baking
   *
   * @var int
   */
  int bakeTemperature;

  /**
   * The duration for the baking process in seconds
   *
   * @var int
   */
  int bakeDuration;

  /**
   * The type (UNUSED, TOP, BOTTOM, BOOST, CONVECTION) for each element
   *
   * @var int[]
   */
  byte outputType[4];

  /**
   * Duty cycle (0 .. DUTY_PERIOD) for each element
   *
   * @var int[]
   */
  short elementDutyCycle[4];    // Duty cycle for each output

  /**
   * Cycle counter (0 .. DUTY_PERIOD) for each element
   *
   * @var int[]
   */
  short elementDutyCounter[4];

  /**
   * The point of time when the current phase begun
   *
   * @var unsigned long
   */
  unsigned long phaseStartTime;

  /**
   * The point of time when baking started
   *
   * @var unsigned long
   */
  unsigned long bakingStartTime;

  /**
   * The call counter. Will get used to determine when to switch
   * to the next duty cycle. Rolls over from DUTY_PERIOD to 0.
   *
   * @var unsigned long
   */
  int counter = 0;

  /**
   * The call counter. Will get used to determine when to cal
   * the regulator function. Rolls over from REGULATOR_PERIOD to 0.
   *
   * @var unsigned long
   */
  int regulatorCounter = 0;

  /**
   * Set TRUE every time when switching to the next phase. Can get
   * used to let the handler for each phase make adjustments when
   * being called the first time.
   *
   * @var boolean
   */
  boolean firstTimeInPhase = true;

  /**
   * Buffer used for Serial.print
   *
   * @var char[100]
   */
  char debugBuffer[100];


  /************************************************
   * PUBLIC METHOD
   *
   * Used to interact with this class
   ***********************************************/

public:
  /**
   * The handler for this mode/task
   *
   * @return boolean Return false to exit this mode
   */
  boolean handler() {
    unsigned long currentTime = millis();
    int i, j;
    int deltaDuty;
    int absDuty;

    double currentTemperature = getCurrentTemperature();

    // Check for thermocouple fault
    checkThermocoupleFault(currentTemperature);

    // Check for abort condition (button pressed)
    checkAbortCondition();

    switch (this->bakingPhase) {
    case BAKING_PHASE_INIT: // User has requested to start baking

      // Maybe move this sequence of initialization steps to
      // a method of its own.

      // Read settings from EEPROM:
      // 1. Output types        
      initializeOutputTypes();
      // 2. Maximum temperature
      bakeTemperature = getSetting(SETTING_BAKE_TEMPERATURE);
      // 3. Baking duration
      bakeDuration = getSetting(SETTING_BAKE_DURATION);

      // Perform check for valid start temperature
      checkStartTemperature(currentTemperature);
      if (this->bakingPhase == BAKING_PHASE_ABORT) {
        break;
      }

      counter = 0;
      regulatorCounter = 0;

      // Switch to HEATUP phase
      advanceToNextPhase();

      // Output status information
      displayPhaseDescription();
      serialDisplayBakingData();

      // Initialize duty cycles by setting them to a large negativ value.
      // This will reset the duty cycles of heating elements to 0.
      // Unused element will get turned off anyways.
      // Fans will get set to 100% duty cycle.
      alterDutyCycles(-(DUTY_PERIOD*2));

      // Stagger the element start cycle to avoid abrupt changes in current draw
      for (i=0; i < 4; i++) {
        elementDutyCounter[i] = (short)((DUTY_PERIOD >> 2) * i);
      }

      // Start the reflow and phase timers
      phaseStartTime = bakingStartTime = millis();

      break; 

    case BAKING_PHASE_HEATUP:
    case BAKING_PHASE_BAKE:

      // STOPPED --- begin
      // Create PT regulation

      // Here we implement a regulator. For this purpose we have a target
      // value "W" and a regulation value "X". The target value W represents
      // the temperature we want to approach. The regulation value "X"
      // represents the current situation (feedback). The difference between
      // both values determines how the output (dutyCycle) has to get changed
      // by the steering value Y.
      //
      // For reference see: U. Tietze, C. Schenk, Halbleiter Schaltungstechnik
      // 10. Auflage, Kapitel 27 "Elektronische Regler".
      deltaDuty = 0;

      // A normal P-regulator will only take the current signal difference
      // into account and generate an according output value. The amplification
      // determines how strong the regulator reacts on a too low/high
      // input value and thus the value of duty cycle.
      // This code will get executed 200 times per second. A duty period is 1024
      // cycles. So one duty period will be roughly 5 seconds in time. Even if the
      // duty cycle gets only changed by +1/-1 during each iteration the whole duty
      // cycle would get changed to MAX/MIN during the cycle itself.
      // So we should only update the duty cycle every 100 iterations (0.5s).
      // The regulator amplicifaction has to get choosen wisely
      if (++regulatorCounter > REGULATOR_PERIOD) {
        absDuty = RegulatorP(currentTemperature);
        setDutyCycles(absDuty);
        regulatorCounter = 0;
      }

      if (this->bakingPhase == BAKING_PHASE_HEATUP) {
        if ((currentTemperature+10) > bakeTemperature) {
          // The temperature is high enough to move to the next phase
          this->bakingPhase++;
          firstTimeInPhase = true;
          lcdPrintLine(0, bakingPhaseDescription[this->bakingPhase]);
          phaseStartTime = millis();
        }
      } 
      else {
        /*
    if (counter % 40 == 0) {
      sprintf(debugBuffer, "currentTime = %ld, phaseStartTime = %ld, diff = %ld", currentTime, phaseStartTime, currentTime-phaseStartTime);
      Serial.println(debugBuffer);
      sprintf(debugBuffer, "bakeDuration = %d s = %ld ms", bakeDuration, SECONDS_TO_MILLIS(bakeDuration));
      Serial.println(debugBuffer);
    }
    */
        if ( (currentTime-phaseStartTime) > SECONDS_TO_MILLIS(bakeDuration)) {
          // Long enough in baking phase. Move to the next phase.
          this->bakingPhase++;
          firstTimeInPhase = true;
          lcdPrintLine(0, bakingPhaseDescription[this->bakingPhase]);
          phaseStartTime = millis();
        }
      }

      this->setOutputStates();


      // Update the displayed temperature roughly once per second
      if (counter++ % 200 == 0) {
        displayBakingTemperature(currentTime, currentTemperature);
      }
      break;


    case BAKING_PHASE_COOLDOWN:   // Wait until the oven reaches a temperature low enough
      if (firstTimeInPhase) {
        firstTimeInPhase = false;
        // Update the display
        lcdPrintLine(0, "Cool - open door");
        lcdPrintLine(1, " ");
        Serial.println(F("******* Baking phase: Cooling down *******"));
        Serial.println(F("Turning all heating elements off ..."));
        Serial.println(F("Open the oven door ..."));

        // Make sure all the elements are off (keep convection fans on)
        for (int i=0; i<4; i++) {
          if (outputType[i] != TYPE_CONVECTION_FAN)
            digitalWrite(i+4, LOW);
        }

        playTones(TUNE_REFLOW_DONE);
      }

      // Update the displayed temperature roughly once per second
      if (counter++ % 200 == 0) {
        displayBakingTemperature(currentTime, currentTemperature);
      }

      // Baking is finished when temperature drops below 50C
      // This should be save enough to remove whatever is inside (and possibly eat it :)
      if (currentTemperature < 50.0) {
        playTones(TUNE_REMOVE_BOARDS);
        bakingPhase = BAKING_PHASE_ABORT;
        lcdPrintLine(0, "Baking complete!");
        lcdPrintLine(1, " ");
        firstTimeInPhase = true;
      }

      break;

    case BAKING_PHASE_ABORT: // The baking is finished
      Serial.println(F("Reflow is done!"));
      // Turn all elements and fans off
      for (i = 4; i < 8; i++)
        digitalWrite(i, LOW);

      // Start next time with initialization
      bakingPhase = BAKING_PHASE_INIT;
      // Wait for a bit to allow the user to read the last message
      delay(3000);
      // Return to the main menu
      return false;
    }  
    return true;
  }  

  /************************************************
   * PROTECTED METHODS
   *
   * Used internally by this class
   ***********************************************/

protected:
  /**
   * Checks for a faulty thermocouple. Will switch to ABORT phase when fault is detected
   *
   * @param double currentTemperatur The temperature currently read by the handler
   * @return void
   */
  void checkThermocoupleFault(double currentTemperature) {
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

      // Abort the reflow
      Serial.println(F("Baking aborted because of thermocouple error!"));
      this->bakingPhase = BAKING_PHASE_ABORT;
    }
  }


  /**
   * Checks for an abort condition.
   *
   * @return void
   */
  void checkAbortCondition() {
    // Abort the baking if a button is pressed
    if (getButton() != CONTROLEO_BUTTON_NONE) {
      lcdPrintLine(0, "Aborting baking");
      lcdPrintLine(1, "Button pressed");
      Serial.println(F("Button pressed.  Aborting baking ..."));
      this->bakingPhase = BAKING_PHASE_ABORT;
    }
  }


  /**
   * Initialize the outputType class variable by reading the appropriate
   * settings from the EEPROM
   *
   * @return void
   */
  void initializeOutputTypes() {
    byte i;
    // Retrieve type of each output
    for (i=0; i<OUTPUT_COUNT; i++) {
      this->outputType[i] = getSetting(SETTING_D4_TYPE + i);
    }
  }


  /**
   * Checks if the start temperature is valid. If the oven is still
   * too hot then ABORT baking.
   *
   * @return void
   */
  void checkStartTemperature(double currentTemperature) {
    // Make sure the oven is cooler than the target temperature.
    if (currentTemperature > bakeTemperature) {
      lcdPrintLine(0, "T_now > T_baking");
      lcdPrintLine(1, "Please wait...");
      Serial.println(F("Oven too hot to start baking.  Please wait ..."));  

      // Abort the reflow
      this->bakingPhase = BAKING_PHASE_ABORT;
    } else {
      Serial.println(F("checkStartTemperature"));
      Serial.println(currentTemperature);
      Serial.println(bakeTemperature);  
    }
  }


  /**
   * Advance state machine to next phase
   *
   * @return void
   */
  void advanceToNextPhase() {
    this->firstTimeInPhase = true;
    switch (this->bakingPhase) {
    case BAKING_PHASE_INIT:
      this->bakingPhase = BAKING_PHASE_HEATUP;
      break;
    case BAKING_PHASE_HEATUP:
      this->bakingPhase = BAKING_PHASE_BAKE;
      break;
    case BAKING_PHASE_BAKE:
      this->bakingPhase = BAKING_PHASE_COOLDOWN;
      break;
    case BAKING_PHASE_COOLDOWN:
      this->bakingPhase = BAKING_PHASE_ABORT;
      break;
    case BAKING_PHASE_ABORT:
      // Stay in this phase
      break;
    }
  }


  /**
   * Show description of current phase on LCD
   *
   * @return void
   */
  void displayPhaseDescription() {
    lcdPrintLine(0, bakingPhaseDescription[this->bakingPhase]);
    lcdPrintLine(1, "");

    // Display information about this phase
    serialDisplayBakingData();
  }


  /**
   * Show information about the current baking process on the
   * serial line.
   *
   * @return void
   */
  void serialDisplayBakingData() {
    sprintf(debugBuffer, "******* Baking phase: %s *******", bakingPhaseDescription[this->bakingPhase]);
    Serial.println(debugBuffer);
    sprintf(debugBuffer, "Baking duration = %d seconds", this->bakeDuration);
    Serial.println(debugBuffer);
    sprintf(debugBuffer, "Baking temperature = %d Celsius", this->bakeTemperature);
    Serial.println(debugBuffer);
    Serial.println(F("Duty cycles: "));
    for (int i=0; i<4; i++) {
      sprintf(debugBuffer, "  D%d = %d  (%s)", i+4, this->elementDutyCycle[i], outputDescription[this->outputType[i]]);
      Serial.println(debugBuffer);
    }
  }


  /**
   * Change duty cycles according to "diff" parameter. 
   *
   * @param int diff: The amount by which to change duty cycle
   * @return void
   */
  void alterDutyCycles(short diff) {
    byte i;
    for (i=0; i<OUTPUT_COUNT; i++) {
      switch (this->outputType[i]) {
      case TYPE_UNUSED:
        this->elementDutyCycle[i] = 0;
        break;

      case TYPE_TOP_ELEMENT:
        this->elementDutyCycle[i] += diff;
        break;

      case TYPE_BOTTOM_ELEMENT:
        this->elementDutyCycle[i] += diff;
        break;

      case TYPE_BOOST_ELEMENT:
        this->elementDutyCycle[i] += diff;
        break;

      case TYPE_CONVECTION_FAN:
        this->elementDutyCycle[i] = DUTY_PERIOD;
        break;
      }
      if (this->elementDutyCycle[i] < 0) {
        this->elementDutyCycle[i] = 0;
      } else if (this->elementDutyCycle[i] > DUTY_PERIOD) {
        this->elementDutyCycle[i] = DUTY_PERIOD;
      }
    }
  }
  
  /**
   * Sets duty cycles to specified value. 
   *
   * @param short value: The value to which to set the duty cycles
   * @return void
   */
  void setDutyCycles(short value) {
    byte i;
    for (i=0; i<OUTPUT_COUNT; i++) {
      switch (this->outputType[i]) {
      case TYPE_UNUSED:
        this->elementDutyCycle[i] = 0;
        break;

      case TYPE_TOP_ELEMENT:
        this->elementDutyCycle[i] = value;
        break;

      case TYPE_BOTTOM_ELEMENT:
        this->elementDutyCycle[i] = value;
        break;

      case TYPE_BOOST_ELEMENT:
        this->elementDutyCycle[i] = value;
        break;

      case TYPE_CONVECTION_FAN:
        this->elementDutyCycle[i] = DUTY_PERIOD;
        break;
      }
      if (this->elementDutyCycle[i] < 0) {
        this->elementDutyCycle[i] = 0;
      } else if (this->elementDutyCycle[i] > DUTY_PERIOD) {
        this->elementDutyCycle[i] = DUTY_PERIOD;
      }
    }
  }


  /**
   * Sets the state of each output according to it's duty cycle
   *
   * @return void
   */
  void setOutputStates() {        
    byte i;
    // Turn the output on or off based on its duty cycle
    for (i=0; i<OUTPUT_COUNT; i++) {
      // Skip unused elements
      if (this->outputType[i] == TYPE_UNUSED) {
        continue;
      }

      // Turn the output on at 0, and off at the duty cycle value
      if (elementDutyCounter[i] == 0) {
        digitalWrite(4 + i, HIGH);
      }
      if (elementDutyCounter[i] >= elementDutyCycle[i]) {
        digitalWrite(4 + i, LOW);
      }

      // Increment the duty counter
      if (++elementDutyCounter[i] >= DUTY_PERIOD) {
        elementDutyCounter[i] = 0;
      }
    }
  }

  /**
   * Change duty cycles according to "diff" parameter. 
   *
   * @param int diff: The amount by which to change duty cycle
   * @return void
   */
  int RegulatorP(double currentTemperature) {
    static int p = 0;
    int absDuty = 0;
    int tDiff = bakeTemperature - currentTemperature;
    if (tDiff > 50) {
      absDuty = DUTY_PERIOD;
    } else if (tDiff <= 0) {
      absDuty = 0;
    } else {
      absDuty = tDiff * 20;
    }

    if (p++ % 5 == 0) {
      sprintf(debugBuffer, "currentTemperature = %ld", (long)currentTemperature);
      Serial.println(debugBuffer);
      sprintf(debugBuffer, "tDiff = %d", tDiff);
      Serial.println(debugBuffer);
      sprintf(debugBuffer, "deltaDuty = %d", absDuty);
      Serial.println(debugBuffer);
    }
    
    return absDuty;
  }


  // Display the current temperature to the LCD screen and print it to the serial port so it can be plotted
  void displayBakingTemperature(unsigned long currentTime, double temperature) {
    // Display the temperature on the LCD screen
    displayTemperature(temperature);

    // Write the time and temperature to the serial port, for graphing or analysis on a PC
    sprintf(debugBuffer, "%ld, %ld, ", MILLIS_TO_SECONDS(currentTime - bakingStartTime), MILLIS_TO_SECONDS(currentTime - phaseStartTime));
    Serial.print(debugBuffer);
    Serial.println(temperature);
  }
};


// Return false to exit this mode
boolean Bake() {
  static BakeImplementation instance = BakeImplementation();
  return instance.handler();
}



