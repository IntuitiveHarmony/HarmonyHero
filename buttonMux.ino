#include <Wire.h>              // For display
#include <Adafruit_SSD1306.h>  // For display

#include <EEPROM.h>  // To save variables across power cycle

#include <MIDI.h>  // Add Midi Library

// Define OLED parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin not used with Pro Micro

// OLED display address
#define OLED_I2C_ADDRESS 0x3C

// Create instance of display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create an instance of the library with default name, serial port and settings
MIDI_CREATE_DEFAULT_INSTANCE();

// ~~~~~~~~~~~~~~
// MUX Parameters
// ~~~~~~~~~~~~~~
// signal pins
uint8_t signal0 = 4;
uint8_t signal1 = 15;
uint8_t signal2 = 8;
// enable pins
uint8_t enableMux0 = 16;
uint8_t enableMux1 = 14;
uint8_t enableMux2 = 10;
// common signal
uint8_t muxCommon = 7;

// ~~~~~~~~~~~~~~~
// Menu Parameters
// ~~~~~~~~~~~~~~~
uint8_t menuLED = 6;
uint8_t menuStep = 0;      // 0-Home 1-Channel 2-Notes 3-velocity 4-StrumSwitches
uint8_t selectedNote = 0;  // Note to edit, based off index

// ~~~~~~~~~~~~~~~~~~~
// Selector Parameters
// ~~~~~~~~~~~~~~~~~~~
uint8_t selectPot = A1;
int selection = 0;

// Array to store the previous state of each button
uint8_t previousButtonState[24] = { 0 };  // Updated array size

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Tuning class to allow for multiple variations
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Tuning {
private:
  // Default values can be changed by the user
  byte notes[10] = {};
  byte channel = 11;
  byte velocity = 127;

  byte neckCC = 1;  // MOD wheel
  byte neckSwitchRest = 0;
  byte neckSwitchDown = 60;
  byte neckSwitchUp = 127;

  byte bridgeCC = 7;  // Volume
  byte bridgeSwitchRest = 127;
  byte bridgeSwitchDown = 0;
  byte bridgeSwitchUp = 60;

public:
  // constructor
  Tuning(byte initialNotes[10]) {
    // Copy the notes from the provided array to the class's notes array
    memcpy(notes, initialNotes, sizeof(notes));
  }
  byte getNote(uint8_t i) {
    return notes[i];
  }
  void changeNote(uint8_t index, byte change) {
    notes[index] += change;
  }
  byte getChannel() {
    return channel;
  }
  void changeChannel(byte change) {
    channel += change;
  }
  byte getVelocity() {
    return velocity;
  }
  void changeVelocity(byte change) {
    velocity += change;
  }

  // Control Change Methods
  byte getNeckCC() {
    return neckCC;
  }
  void changeNeckCC(byte change) {
    neckCC += change;
  }

  byte getNeckSwitchRest() {
    return neckSwitchRest;
  }
  void changeNeckSwitchRest(byte change) {
    neckSwitchRest += change;
  }

  byte getNeckSwitchDown() {
    return neckSwitchDown;
  }
  void changeNeckSwitchDown(byte change) {
    neckSwitchDown += change;
  }

  byte getNeckSwitchUp() {
    return neckSwitchUp;
  }
  void changeNeckSwitchUp(byte change) {
    neckSwitchUp += change;
  }

  byte getBridgeCC() {
    return bridgeCC;
  }
  void changeBridgeCC(byte change) {
    bridgeCC += change;
  }

  byte getBridgeSwitchRest() {
    return bridgeSwitchRest;
  }
  void changeBridgeSwitchRest(byte change) {
    bridgeSwitchRest += change;
  }

  byte getBridgeSwitchDown() {
    return bridgeSwitchDown;
  }
  void changeBridgeSwitchDown(byte change) {
    bridgeSwitchDown += change;
  }

  byte getBridgeSwitchUp() {
    return bridgeSwitchUp;
  }
  void changeBridgeSwitchUp(byte change) {
    bridgeSwitchUp += change;
  }
};
// End of Tuning Class ~~~~~~~~~~~~~~~~~~~~~~~~~

// Major Pentatonic
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
byte notes1[10] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76 };

// C Minor Pentatonic
byte notes2[10] = { 60, 63, 65, 67, 69, 72, 74, 75, 77, 79 };

// Blues Scale
// MIDI notes: C4, E4, G4, Ab4, A4, C5, D5, F5, G5, Ab5
byte notes3[10] = { 60, 63, 65, 66, 67, 70, 72, 75, 77, 78 };

// Dorian Scale
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
byte notes4[10] = { 60, 62, 63, 65, 67, 69, 71, 72, 74, 76 };

// Mixolydian Scale
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
// byte notes5[10] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76 };
byte notes5[10] = { 120, 127, 124, 125, 67, 115, 111, 123, 120, 116 };

// Array of all the tuning instances available
Tuning tuningSelection[5] = {
  Tuning(notes1),
  Tuning(notes2),
  Tuning(notes3),
  Tuning(notes4),
  Tuning(notes5)
};


// ~~~~~~~~~~~~~
// Arduino Setup
// ~~~~~~~~~~~~~
void setup() {

  Serial.begin(9600);
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  // Set display text size and color
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();  // Clear the display

  readPots();
  displayTuningHeader();

  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.

  // Set up the pinz
  pinMode(muxCommon, INPUT_PULLUP);

  pinMode(signal0, OUTPUT);  // For MUX
  pinMode(signal1, OUTPUT);  // For MUX
  pinMode(signal2, OUTPUT);  // For MUX

  pinMode(enableMux0, OUTPUT);
  pinMode(enableMux1, OUTPUT);
  pinMode(enableMux2, OUTPUT);

  pinMode(selectPot, INPUT_PULLUP);

  pinMode(menuLED, OUTPUT);
}

// ~~~~~~~~~~~~
// Arduino Loop
// ~~~~~~~~~~~~
void loop() {
  readPots();
  buttonMux();
  // Normally the Notes and Velocity will display
  if (menuStep < 4) {
    displayTuningHeader();
    displayNotes();
    displayVelocity();
  }
  // The edit menu is active on steps 4 and 5
  else {
    displayEditStrums();
  }
  lightMenuLED();

  // Update the display at the end of the loop
  display.display();
}

// ~~~~~~~~~~~~
// Button Logic
// ~~~~~~~~~~~~
// Neck buttons....0-9
// Strum buttons...10-13       || 10-NeckDown   11-NeckUp     12-BridgeDown     13-BridgeUp
// Directional buttons...14-17 || 14-Up    15-Right    16-Down    17-Left
// Save Button...18
void handleButtonPress(uint8_t i) {
  byte velocity = 80;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI note on based on the current tuning selection (note, velocity, channel)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i <= 9) {
    MIDI.sendNoteOn(tuningSelection[selection].getNote(i), tuningSelection[selection].getVelocity(), tuningSelection[selection].getChannel());
  }
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI CC messages from the strum buttons
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  else if (i >= 10 && i <= 13) {
    // Neck Down
    if (i == 10) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckCC(), tuningSelection[selection].getNeckSwitchDown(), tuningSelection[selection].getChannel());
    }
    // Neck Up
    else if (i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckCC(), tuningSelection[selection].getNeckSwitchUp(), tuningSelection[selection].getChannel());
    }
    // Bridge Down
    if (i == 12) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeCC(), tuningSelection[selection].getBridgeSwitchDown(), tuningSelection[selection].getChannel());
    }
    // Bridge Up
    else if (i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeCC(), tuningSelection[selection].getBridgeSwitchUp(), tuningSelection[selection].getChannel());
    }
  }
  // ~~~~~~~~~~~~~~~~~~~
  // Directional buttons
  // ~~~~~~~~~~~~~~~~~~~
  else if (i >= 14 && i <= 17) {
    // Up Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // change the MIDI channel up
    if (i == 14 && menuStep == 1) {
      // limit 1-16
      if (tuningSelection[selection].getChannel() < 16) {
        tuningSelection[selection].changeChannel(1);
      } else {
        // Reset to 1 because max of 16 reached
        tuningSelection[selection].changeChannel(-15);
      }
    }
    // Change the velocity up
    else if (i == 14 && menuStep == 3) {
      // limit 0-127
      if (tuningSelection[selection].getVelocity() < 127) {
        tuningSelection[selection].changeVelocity(1);
      } else {
        // Do noting because max of 127 reached
      }
    }
    // Change the selected note up
    else if (i == 14 && menuStep == 2) {
      // limit 0-127
      if (tuningSelection[selection].getNote(selectedNote) < 127) {
        tuningSelection[selection].changeNote(selectedNote, 1);
      } else {
        // Do noting because max of 127 reached
      }
    }
    // Up Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Right Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if (i == 15 && menuStep == 2) {
      // Scroll right through notes
      // Limit to 0-9
      if (selectedNote < 9) {
        selectedNote++;
      } else {
        selectedNote = 0;
      }
    }
    // Right Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Down Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // change the MIDI channel down
    else if (i == 16 && menuStep == 1) {
      // limit 1-16
      if (tuningSelection[selection].getChannel() > 1) {
        tuningSelection[selection].changeChannel(-1);
      } else {
        // Reset to 16 because min of 1 reached
        tuningSelection[selection].changeChannel(15);
      }
    }
    // Change the selected note down
    else if (i == 16 && menuStep == 2) {
      // limit 0-127
      if (tuningSelection[selection].getNote(selectedNote) > 0) {
        tuningSelection[selection].changeNote(selectedNote, -1);
      } else {
        // Do nothing because min of 0 reached
      }
    }
    // Change the velocity down
    else if (i == 16 && menuStep == 3) {
      // limit 0-127
      if (tuningSelection[selection].getVelocity() > 0) {
        tuningSelection[selection].changeVelocity(-1);
      } else {
        // Do nothing because min of 0 reached
      }
    }
    // Down Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Left Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Scroll left through notes
    if (i == 17 && menuStep == 2) {
      // Limit to 0-9
      if (selectedNote > 0) {
        selectedNote--;
      } else {
        selectedNote = 9;
      }
    }
    // Left Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }
  // ~~~~~~~~~~~~
  // Menu Buttons
  // ~~~~~~~~~~~~
  // Start Button ~~~~~~~~~~~~~~~~~~~~~~~~
  else if (i == 18) {
    // Limit to 0-5
    if (menuStep < 5) {
      menuStep++;
    } else {
      menuStep = 0;
    }
  }
  // Select Button ~~~~~~~~~~~~~~~~~~~~~~~
  else if (i == 19) {
    // Limit to 0-5
    if (menuStep > 0) {
      menuStep--;
    } else {
      menuStep = 5;
    }
  }
  // Save Button ~~~~~~~~~~~~~~~~~~~~~~~~~~
  else if (i == 20) {
    // eeprom logic
  }

  // display.setCursor(0, 16);
  // display.print(F("Button "));
  // display.setCursor(39, 16);
  // display.print(i);
  // display.setCursor(44, 16);
  // display.print(F(" Pressed!"));
  // display.display();  // Update the display

  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Pressed!");
}

void handleButtonRelease(uint8_t i) {
  // Trun off MIDI notes that have been played (note, velocity, channel)
  if (i <= 9) {
    MIDI.sendNoteOff(tuningSelection[selection].getNote(i), 0, tuningSelection[selection].getChannel());
  }
  // Reset the strum switches

  else if (i >= 10 && i <= 13) {
    // Reset neck Switch
    if (i == 10 || i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckCC(), tuningSelection[selection].getNeckSwitchRest(), tuningSelection[selection].getChannel());
    }
    // Reset neck Switch
    else if (i == 12 || i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeCC(), tuningSelection[selection].getBridgeSwitchRest(), tuningSelection[selection].getChannel());
    }
  }
  // display.clearDisplay();  // clear the display


  // Serial.print("Button ");
  // Serial.print(i);
  // Serial.println(" Released!");
}



void buttonMux() {
  // Loop through all the button channels on the MUX
  for (uint8_t i = 0; i < 24; ++i) {
    // Enable the appropriate MUX
    enableMux(i < 8 ? 0 : (i < 16 ? 1 : 2));

    // Control the selector pins based on the binary representation of i
    digitalWrite(signal0, (i & 0x01) ? HIGH : LOW);
    digitalWrite(signal1, (i & 0x02) ? HIGH : LOW);
    digitalWrite(signal2, (i & 0x04) ? HIGH : LOW);

    // Read the value from the selected button
    uint8_t buttonValue = digitalRead(muxCommon);
    // Serial.println(buttonValue);

    // Check for button press
    if (buttonValue == 0 && previousButtonState[i] == 0) {
      // Button is pressed
      handleButtonPress(i);
      previousButtonState[i] = 1;
    }
    // Check for button release
    else if (buttonValue > 0 && previousButtonState[i] == 1) {
      // Button is released
      handleButtonRelease(i);
      previousButtonState[i] = 0;
    }
  }
}

void enableMux(uint8_t mux) {
  switch (mux) {
    case 0:
      digitalWrite(enableMux0, LOW);
      digitalWrite(enableMux1, HIGH);
      digitalWrite(enableMux2, HIGH);
      break;
    case 1:
      digitalWrite(enableMux0, HIGH);
      digitalWrite(enableMux1, LOW);
      digitalWrite(enableMux2, HIGH);
      break;
    case 2:
      digitalWrite(enableMux0, HIGH);
      digitalWrite(enableMux1, HIGH);
      digitalWrite(enableMux2, LOW);
      break;
  }
}


// ~~~~~~~~~~~~~~~~~~~~
// Display Functions 👀
// ~~~~~~~~~~~~~~~~~~~~
void displayTuningHeader() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("Tuning: "));
  display.print(selection + 1);

  // Highlight the channel if in the Channel step of the menu
  if (menuStep == 1) {
    display.setTextColor(BLACK, WHITE);
  }
  display.print(F(" Channel: "));
  display.print(tuningSelection[selection].getChannel());
  display.setTextColor(WHITE, BLACK);  // Reset text color
}

void displayNotes() {
  // Highlight the notes if in the Notes step of the menu
  if (menuStep == 2) {
    display.setTextColor(BLACK, WHITE);
  }
  // Display first set of notes
  display.setCursor(0, 15);
  display.print(F(" Notes: "));
  display.setTextColor(WHITE, BLACK);  // Reset text color
  display.setCursor(2, 28);
  for (int i = 0; i < 5; ++i) {
    // Highlight selected note to edit
    if (selectedNote == i && menuStep == 2) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(tuningSelection[selection].getNote(i));
    display.setTextColor(WHITE, BLACK);  // Reset text color
    display.print(F(" "));
  }
  // Display second set of notes on next line
  display.setCursor(2, 42);
  for (int i = 5; i < 10; ++i) {
    // Highlight selected note to edit
    if (selectedNote == i && menuStep == 2) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(tuningSelection[selection].getNote(i));
    display.setTextColor(WHITE, BLACK);  // Reset text color
    display.print(F(" "));
  }
}

void displayVelocity() {
  // Highlight the velocity if in the Velocity step of the menu
  if (menuStep == 3) {
    display.setTextColor(BLACK, WHITE);
  }
  display.setCursor(35, 56);
  display.print(F(" Velocity: "));
  display.print(tuningSelection[selection].getVelocity());
  display.setTextColor(WHITE, BLACK);  // Reset text color
}

void displayEditStrums() {
  display.clearDisplay();
  displayTuningHeader();
}

void readPots() {
  // Read the "5 way" selection Pot, map it and assign it. -1 to sync with index of tuningSelection array
  uint8_t selectVoltage = analogRead(selectPot);
  selection = map(selectVoltage, 15, 215, 1, 5) - 1;
}

void lightMenuLED() {
  if (menuStep > 0) {
    digitalWrite(menuLED, HIGH);
  } else {
    digitalWrite(menuLED, LOW);
  }
}