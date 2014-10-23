// Play the selected tone
// This is a synchronous (blocking) call.  The function doesn't return until the
// tone has been played.  This works fine for this application, but it could be
// done using a timer interrupt and a lot more (complicated) code.

#include "pitches.h"

const int tones[MAX_TUNES][17] = {
      {NOTE_C5,8,NOTE_G4,8,-1},   // TUNE_STARTUP
      {NOTE_F5,30,-1},            // TUNE_TOP_BUTTON_PRESS
      {NOTE_B5,20,-1},            // TUNE_BOTTOM_BUTTON_PRESS
      {NOTE_C5,4,NOTE_G4,8,NOTE_G4,8, NOTE_A4,4,NOTE_G4,4,0,4,NOTE_B4,4,NOTE_C5,4,-1}, // TUNE_REFLOW_DONE
      {NOTE_C5,4,NOTE_B4,4,NOTE_E4,2,-1}    // TUNE_REMOVE_BOARDS
};

// Play a tone
// Parameter: tone - an array containing alternating notes and note duration, terminated by -1
void playTones(int tune) {
  if (tune >= MAX_TUNES)
    return;
  const int *tonesToPlay = tones[tune];
  for (int i=0; tonesToPlay[i] != -1; i+=2) {
    // Note durations: 4 = quarter note, 8 = eighth note, etc.:   
    int duration = 1000/tonesToPlay[i+1];
    tone(CONTROLEO_BUZZER_PIN, tonesToPlay[i], duration);
    delay(duration * 1.1);
  }
  noTone(CONTROLEO_BUZZER_PIN);
}

