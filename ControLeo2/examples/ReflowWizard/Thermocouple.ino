// Thermocouple
// Instead of getting instantaneous readings from the thermocouple, get an average
// Also, some convection ovens have noisy fans that generate spurious short-to-ground and 
// short-to-vcc errors.  This will help to eliminate those.
//
// With a 16MHz clock, we set the prescaler to 1024.  This gives a timer speed of 16,000,000 / 1024 = 15,625. This means
// the timer counts from 0 to 15,625 in one second.  We'd like to sample the MAX31855 every 125ms (0.125s) or 8 times per
// second so we set the compare register OCR1A to 15,625 * 0.125 = 1953.

#define NUM_READINGS           4  // Number of readings to average the temperature over (4 readings = 1/2 second)
#define ERROR_THRESHOLD        5  // Number of consecutive short-to-xxx faults before a fault is returned


// Store the temperatures as they are read
volatile float recentTemperatures[NUM_READINGS];
volatile int temperatureErrorCount = 0;
volatile float temperatureError;
ControLeo2_MAX31855 thermocouple;


// Initialize Timer1
void initializeThermocoupleTimer(void) {
  cli();                                // Disable global interrupts
  ADMUX  = 0;                           // Initialize the MUX and enable ADC and set frequency
  ADCSRA = (1<<ADEN) | (1<<ADPS2); 
  TCCR1A = 0;                           // Initialize Timer1 TCCR1A register to 0
  TCCR1B = 0;                           // Same for TCCR1B
  OCR1A = 1953;                         // Set compare match register to desired timer count
  TCCR1B |= (1 << WGM12);               // Turn on CTC mode
  TCCR1B |= (1 << CS10) + (1 << CS12);  // Set CS10-CS12 for 1024 prescaler
  TIMSK1 |= (1 << OCIE1A);              // Enable timer compare interrupt
  sei();                                // Enable global interrupts
  
  // The array of temperatures should be initialized here, but the reality is that 3*8 = 24 readings
  // will be taken before the splash screen finishes displaying.
}


// Timer ISR
ISR(TIMER1_COMPA_vect)
{
  volatile static int readingNum = 0;
    
  // The timer has fired.  It has been 0.125 seconds since the previous reading was taken
  // Take a thermocouple reading
  float temperature = thermocouple.readThermocouple(CELSIUS);
  
  // Is there an error?
  if (THERMOCOUPLE_FAULT(temperature)) {
    if (temperature == FAULT_SHORT_GND || temperature == FAULT_SHORT_VCC) {
      // Noise can cause spurious short faults.  These are typically caused by the convection fan
      if (temperatureErrorCount < ERROR_THRESHOLD)
        temperatureErrorCount++;
      temperatureError = temperature;
    }
    else {
      // Must be an "open" fault.  Report it immediately
      temperatureErrorCount = ERROR_THRESHOLD;
      temperatureError = temperature; // FAULT_OPEN
    }
  }
  else {
    // There is no error.  Save the temperature
    recentTemperatures[readingNum] = temperature;
    readingNum = (readingNum + 1) % NUM_READINGS;
    // Clear any previous error
    temperatureErrorCount = 0;
  }
}


// Routine used by the main app to get temperatures
// This routine disables and then re-enables interrupts so that data corruption isn't caused
// by the ISR writing data at the same time it is read here.
float getCurrentTemperature() {
  float temperature = 0;
  
  // Disable interrupts while reading values
  noInterrupts();

  // Is there an error?
  if (temperatureErrorCount >= ERROR_THRESHOLD)
    temperature = temperatureError;
  else {
    // Return the average of the last NUM_READINGS readings
    for (int i=0; i< NUM_READINGS; i++)
      temperature += recentTemperatures[i];
    temperature = temperature / NUM_READINGS;
  }
  
  // Reenable interrupts
  interrupts();
  
  // Return the temperature
  return temperature;
}

