// Testing menu
// Called from the main loop
// Allows the user to test the outputs
// Buttons: The bottom button moves to the next output
//          The top button toggles the output on and off

// Called when in testing mode
// Return false to exit this mode
boolean Testing() {
  static boolean firstRun = true;
  static boolean channelIsOn = true;
  static int channel = 4; 
  
  // Is this the first time "Testing" has been run?
  if (firstRun) {
    firstRun = false;
    lcdPrintLine(0, "Test Outputs");
    lcdPrintLine(1, "Output 4");
    displayOnState(channelIsOn);
  }
  
  // Turn the currently selected channel on, and the others off
  for (int i=4; i<8; i++) {
    if (i == channel && channelIsOn)
      digitalWrite(i, HIGH);
    else
      digitalWrite(i, LOW);
  }
  
  // Was a button pressed?
  switch (getButton()) {
    case CONTROLEO_BUTTON_TOP:
      // Toggle the output on and off
      channelIsOn = !channelIsOn;
      displayOnState(channelIsOn);
      break;
    case CONTROLEO_BUTTON_BOTTOM:
      // Move to the next output
      channel = channel + 1;
      if (channel == 8) {
        // Turn all the outputs off
        for (int i=4; i<8; i++)
          digitalWrite(i, LOW);
        // Initialize variables for the next time through
        firstRun = true;
        channel = 4;
        channelIsOn = true;
        // Return to the main menu
        return false;
      }
      lcd.setCursor(7, 1);
      lcd.print(channel);
      displayOnState(channelIsOn);
      break;
  }
  
  // Stay in this mode;
  return true;
}


void displayOnState(boolean isOn) {
  lcd.setCursor(9, 1);
  lcd.print(isOn? "is on ": "is off");
}
