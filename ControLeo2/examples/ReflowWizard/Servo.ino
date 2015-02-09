// Timer 1 is used for 2 things:
// 1. Take thermocouple readings every 200ms (5 times per second)
// 2. Control the servo used to open the oven door
//
// Servo timer interrupt operation
// ===============================
// See http://en.wikipedia.org/wiki/Servo_control for more information regarding servo operation.
// Note: Some (most) servos only rotate around 90°, not the 180° that is possible in theory
// The servo position information should be sent every 20ms, or 50 times per second.  To do this:
//   - Timer 1 is set to CTC mode
//   - Compare A is set to a value to force a timer interrupt every 20ms
// For every 10 times the timer fires, a call is made to get a thermocouple reading.
// If servo movement is enabled (interrupt on Compare B, OCIE1B is set) then the servo pin
// is set high.  It must be lowered somewhere between 1ms and 2ms later, depending on the desired
// position.  To do this, the appropriate value is written to OCR1B.  Keep in mind that unlike
// Compare A, Compare B does not reset Timer 1's counter.  The steps look something like this:
//   a. Counter = 0: Write servo pin HIGH, and set Compare B to correct duration.
//   b. Counter = Compare B: Write servo pin low
//   c. Counter = Compare A: Counter is set back to 0 (go to a.)
// Once the servo has reached the desired position, the Compare B interrupt is disabled
//
// With a 16MHz clock, the prescaler is set to 8.  This gives a timer speed of 16,000,000 / 8 = 2,000,000. This means
// the timer counts from 0 to 2,000,000 in one second.  We'd like the interrupt to fire 50 times per second so we set
// the compare register OCR1A to 2,000,000 / 50 = 40,000.

#define SERVO_PIN               3     // The I/O pin used for the servo
#define MIN_PULSE_WIDTH       544     // The shortest pulse sent to a servo (from Arduino's servo library)
#define MAX_PULSE_WIDTH      2400     // The longest pulse sent to a servo (from Arduino's servo library)


// Variables used to control servo movement
int servoEndValue;                    // The desired pulse width
volatile int servoMovements;          // Number of movements to reach the desired position
int servoIncrement;                   // The amount to increase/decrease the pulse every interrupt


// Initialize Timer 1
// This timer controls both the thermocouple readings and the servo
// It should fire 50 times every second (every 20ms)
void initializeTimer(void) {
  cli();                               // Disable global interrupts
  TCCR1A = 0;                          // Timer 0 is independent of the I/O pins, CTC mode
  TCCR1B = _BV(WGM12) + _BV(CS11);     // Timer 0 CTC mode, prescaler is 64
  TCNT1 = 0;                           // Clear the timer count 
  OCR1A = 40000;                       // Set compare match so the interrupt occurs 50 times per second
  TIMSK1 |= _BV(OCIE1A);               // Enable timer compare interrupt
  sei();                               // Enable global interrupts

  // Set the servo pin as output
  pinMode(SERVO_PIN, OUTPUT);
  
  // Assume the servo is close to the closed position
  OCR1B = degreesToTimerCounter(getSetting(SETTING_SERVO_CLOSED_DEGREES) + 1);
}


// Timer 1 ISR
// This timer fires 50 times per second (every 20ms)
ISR(TIMER1_COMPA_vect)
{
  volatile static int thermocoupleTimer = 0;
  
  // Read the thermocouple 5 times per second (every 0.2 seconds)
  if (++thermocoupleTimer >= 10) {
    thermocoupleTimer = 0;
    takeCurrentThermocoupleReading();
    
    // Don't move the servo - the pulse won't have the correct timing because of the time taken to read the thermocouple
    return;
  }
  
  // Is the servo timer interrupt active?
  if (TIMSK1 & _BV(OCIE1B)) {
    // Has the servo reached the desired position?
    if (!servoMovements)
      // Disable the servo interrupt
      TIMSK1 &= ~_BV(OCIE1B);
    else {
      // Move the servo to the next position
      OCR1B += servoIncrement;
      servoMovements--;
      // Write the servo signal pin high.  This is lowered when Compare B fires
      digitalWrite(SERVO_PIN, HIGH);
    }
  }
}


// Timer 1 Compare B interrrupt
// This interrupt fires once the desired piulse duration has been sent to the servo
ISR(TIMER1_COMPB_vect)
{
  digitalWrite(SERVO_PIN, LOW);
}


// Move the servo to servoDegrees, in timeToTake milliseconds (1/1000 second)
void setServoPosition(unsigned int servoDegrees, int timeToTake) {
  sprintf(debugBuffer, "Servo: move to %d degrees, over %d ms", servoDegrees, timeToTake);
  Serial.println(debugBuffer);
  // Make sure the degrees are 0 - 180
  if (servoDegrees > 180)
    return;
  // Figure out what the end timer value should be
  servoEndValue = degreesToTimerCounter(servoDegrees);
  
  // If the servo is already in this position, then don't do anything
  if (servoEndValue == (int) OCR1B)
    return;
    
  // Figure out how many movements this will take (a movement is made every 20ms)
  servoMovements = timeToTake / 20;
  
  // Is the movement small?
  if (abs(servoEndValue - (int) OCR1B) < servoMovements) {
    // Make the full movement in one go
    OCR1B = servoEndValue;
    servoIncrement = 0;
  }
  else {
    // Figure out the timer increment to achieve the end value
    servoIncrement = (servoEndValue - (int) OCR1B) / servoMovements;
    // There are rounding errors, so calculate the number of movements again
    servoMovements = (servoEndValue - (int) OCR1B) / servoIncrement;
  }

  // Enable the servo compare interrupt to start the servo motion
  TIMSK1 |= _BV(OCIE1B);
} 


// Convert degrees (0-180) to a timer counter value
unsigned int degreesToTimerCounter(unsigned int servoDegrees) {
  // Get the pulse duration in microseconds
  unsigned int duration = map(servoDegrees, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  // Convert this to a timer value.  One microsecond is 2 timer counts
  return duration << 1;
}

