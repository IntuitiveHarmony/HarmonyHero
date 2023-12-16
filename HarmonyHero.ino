/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File:       HarmonyHero.ino
Author:     Jason Horst
Company:    Intuitive Harmony - https://github.com/IntuitiveHarmony
Date:       November 6, 2023
Purpose:    Using a Guitar Hero controller and Arduino microcontroller to build
a MIDI Controller License:    MIT - https://opensource.org/license/mit/
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// #include <MemoryFree.h>  // Use to check on memory

#include <Adafruit_SSD1306.h>  // For display
#include <Wire.h>              // For display

#include <MIDI.h>  // Add Midi Library

#include <EEPROM.h>  // To save variables across power cycle

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

// Define a flag address in EEPROM
#define INIT_FLAG_ADDRESS 30

// ~~~~~~~~~~~~~~~~~~~~~
// Instrument Parameters
// ~~~~~~~~~~~~~~~~~~~~~
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 1

// ~~~~~~~~~~~~~~
// MUX Parameters
// ~~~~~~~~~~~~~~
// signal pins
#define signal0 4
#define signal1 15
#define signal2 8
// enable pins
#define enableMux0 16
#define enableMux1 14
#define enableMux2 10
// common signal
#define muxCommon 7

// ~~~~~~~~~~~~~~~~~~~~~~
// Main Screen Parameters
// ~~~~~~~~~~~~~~~~~~~~~~
#define MAX_HELD_NOTES 18  // Maximum number of held notes
uint8_t displayStep = 0;   // 0-Notes 1-Neck  2-bridge

uint8_t heldNotes[MAX_HELD_NOTES] = {0};  // Array to store held notes
int numHeldNotes = 0;                     // Number of currently held notes

// ~~~~~~~~~~~~~~~
// Menu Parameters
// ~~~~~~~~~~~~~~~
#define menuLED 6
uint8_t menuStep = 0;  // 0-Home 1-Channel 2-Notes 3-velocity 4-StrumSwitches
uint8_t selectedNote = 0;  // Note to edit, based off index
uint8_t selectedCC = 0;    // CC to edit 0-5 | up 0-2  down 3-5
uint8_t paramUpdated = 0;  // Keep track of when to save
// Keep track of when to display save changes screen
uint8_t saveChangesFlag = 0;
bool paramHeld =
    false;  // Keep track of directional buttons being held for speedyParams()

// Define constants for LED blinking
#define blinkInterval 450  // Blink interval in milliseconds
// Variable to store the last time LED was updated
unsigned long previousMillisLED = 0;
// Variable to store the start time when a Directional button is held
unsigned long heldParamTime;

// ~~~~~~~~~~~~~~~~~~~
// Selector Parameters
// ~~~~~~~~~~~~~~~~~~~
#define selectPot A1
int selection = 0;
int previousSelection = 0;

// ~~~~~~~~~~~~~~~~~~~
// Whammy Parameters
// ~~~~~~~~~~~~~~~~~~~
#define whammyPot A2
int whammy = 0;
int previousWhammy = 0;

// Array to store the previous state of each button
uint8_t previousButtonState[24] = {0};  // Updated array size

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Tuning class to allow for multiple variations
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Tuning {
 private:
  // Default values can be changed by the user
  byte notes[10] = {};
  byte channel = 1;
  byte velocity = 127;

  // 4 user assignable CC params tied to the strum switches
  // Default Values provided, may not work as anticipated depending on
  // individual synth's local CC routing
  byte neckUpCC = 60;  // MOD wheel
  byte neckUpOff = 60;
  byte neckUpOn = 60;

  byte neckDownCC = 60;  // Portamento
  byte neckDownOffValue = 60;
  byte neckDownOnValue = 60;

  byte bridgeUpCC = 60;  // LFO Depth
  byte bridgeUpOffValue = 60;
  byte bridgeUpOnValue = 60;

  byte bridgeDownCC = 7;  // Volume
  byte bridgeDownOffValue = 127;
  byte bridgeDownOnValue = 0;

 public:
  // constructor
  Tuning(byte initialNotes[10]) {
    // Copy the notes from the provided array to the class's notes array
    memcpy(notes, initialNotes, sizeof(notes));
  }
  // Note Methods
  byte getNote(uint8_t i) { return notes[i]; }
  void changeNote(uint8_t index, byte change) { notes[index] += change; }
  byte getLowestNote() {
    byte minNote = notes[0];
    for (int i = 1; i < sizeof(notes); ++i) {
      if (notes[i] < minNote) {
        minNote = notes[i];
      }
    }
    return minNote;
  }
  byte getHighestNote() {
    byte maxNote = notes[0];
    for (int i = 1; i < sizeof(notes); ++i) {
      if (notes[i] > maxNote) {
        maxNote = notes[i];
      }
    }
    return maxNote;
  }
  void transposeAllNotes(byte semitone) {
    for (int i = 0; i < sizeof(notes); ++i) {
      notes[i] += semitone;
    }
  }

  // Channel Methods
  byte getChannel() { return channel; }
  void changeChannel(byte change) { channel += change; }
  // Velocity methods
  byte getVelocity() { return velocity; }
  void changeVelocity(byte change) { velocity += change; }
  // Control Change Methods ~~~~~~~~~~~~~
  // Neck Up CC Methods
  byte getNeckUpCC() { return neckUpCC; }
  void setNeckUpCC(byte change) { neckUpCC += change; }
  byte getNeckUpOffValue() { return neckUpOff; }
  void setNeckUpOffValue(byte change) { neckUpOff += change; }
  byte getNeckUpOnValue() { return neckUpOn; }
  void setNeckUpOnValue(byte change) { neckUpOn += change; }

  // Neck Down CC Methods
  byte getNeckDownCC() { return neckDownCC; }
  void setNeckDownCC(byte change) { neckDownCC += change; }
  byte getNeckDownOffValue() { return neckDownOffValue; }
  void setNeckDownOffValue(byte change) { neckDownOffValue += change; }
  byte getNeckDownOnValue() { return neckDownOnValue; }
  void setNeckDownOnValue(byte change) { neckDownOnValue += change; }

  // Bridge Up CC Methods
  byte getBridgeUpCC() { return bridgeUpCC; }
  void setBridgeUpCC(byte change) { bridgeUpCC += change; }
  byte getBridgeUpOffValue() { return bridgeUpOffValue; }
  void setBridgeUpOffValue(byte change) { bridgeUpOffValue += change; }
  byte getBridgeUpOnValue() { return bridgeUpOnValue; }
  void setBridgeUpOnValue(byte change) { bridgeUpOnValue += change; }

  // Bridge Down CC Methods
  byte getBridgeDownCC() { return bridgeDownCC; }
  void setBridgeDownCC(byte change) { bridgeDownCC += change; }
  byte getBridgeDownOffValue() { return bridgeDownOffValue; }
  void setBridgeDownOffValue(byte change) { bridgeDownOffValue += change; }
  byte getBridgeDownOnValue() { return bridgeDownOnValue; }
  void setBridgeDownOnValue(byte change) { bridgeDownOnValue += change; }
};
// End of Tuning Class ~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~
// Initial Tunings
// ~~~~~~~~~~~~~~~
// Major Pentatonic
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
byte notes1[10] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76};

// C Minor Pentatonic
byte notes2[10] = {60, 63, 65, 67, 69, 72, 74, 75, 77, 79};

// Blues Scale
// MIDI notes: C4, E4, G4, Ab4, A4, C5, D5, F5, G5, Ab5
byte notes3[10] = {60, 63, 65, 66, 67, 70, 72, 75, 77, 78};

// Dorian Scale
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
byte notes4[10] = {60, 62, 63, 65, 67, 69, 71, 72, 74, 76};

// Mixolydian Scale
// MIDI notes: C4, D4, E4, F4, G4, A4, B4, C5, D5, E5
byte notes5[10] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76};
// byte notes5[10] = { 120, 127, 124, 125, 67, 115, 111, 123, 120, 116 };

// Array of all the tuning instances available
Tuning tuningSelection[5] = {Tuning(notes1), Tuning(notes2), Tuning(notes3),
                             Tuning(notes4), Tuning(notes5)};

// ~~~~~~~~~~~~~
// Arduino Setup
// ~~~~~~~~~~~~~
void setup() {
  Serial.begin(38400);
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

  readSelectPot();

  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.

  // Set up the pins
  pinMode(muxCommon, INPUT_PULLUP);

  pinMode(signal0, OUTPUT);  // For MUX
  pinMode(signal1, OUTPUT);  // For MUX
  pinMode(signal2, OUTPUT);  // For MUX

  pinMode(enableMux0, OUTPUT);
  pinMode(enableMux1, OUTPUT);
  pinMode(enableMux2, OUTPUT);

  pinMode(selectPot, INPUT_PULLUP);
  pinMode(whammyPot, INPUT_PULLUP);

  pinMode(menuLED, OUTPUT);

  // Uncomment to initialize the EEPROM Data
  // EEPROM.put(INIT_FLAG_ADDRESS, 0);

  // Check the initialization flag in EEPROM
  byte initFlag;
  EEPROM.get(INIT_FLAG_ADDRESS, initFlag);
  // If the flag is not set, perform the initialization
  if (initFlag != 1) {
    // Save the initial tunings in EEPROM
    for (int i = 0; i < 5; ++i) {
      saveTuningToEEPROM(i);
      delay(10);  // Add a delay between EEPROM writes
    }
    // Set the flag to indicate initialization has been done
    initFlag = 1;
    EEPROM.put(INIT_FLAG_ADDRESS, initFlag);
  }
  // Load tuning data from EEPROM
  for (int i = 0; i < 5; ++i) {
    loadTuningFromEEPROM(i);
  }
  displayStartUp(3000);  // Start screen displays name and version info
}

// ~~~~~~~~~~~~
// Arduino Loop
// ~~~~~~~~~~~~
void loop() {
  // Serial.print("menuStep: ");
  // Serial.print(menuStep);
  // Serial.print(" Selection: ");
  // Serial.print(selection);
  // Serial.print(" prevSel: ");
  // Serial.println(previousSelection);
  // Serial.print("Display: ");
  // Serial.println(displayStep);
  // Serial.print("Menu: ");
  // Serial.println(menuStep);
  // Serial.print("Free Memory: ");  // Use to check on memory
  // Serial.println(freeMemory());   // Use to check on memory

  readSelectPot();
  readWhammyPot();
  buttonMux();
  updateHeldNotes();
  speedyParams();
  lightMenuLED();
  if (whammy != previousWhammy) {
    handleWhammy();
  }
  // Edit menu and display menus have different lengths
  if (menuStep > 0) {
    syncDisplayMenuStep();
  }
  // Normally the Notes and Velocity will display
  if (displayStep == 0) {
    displayTuningHeader();
    displayNotes();
    displayVelocity();
  } else if (displayStep > 0) {
    displayEditStrums();
  }
  if (saveChangesFlag == 1) {
    displaySaveChanges();
  }
  // Update the display at the end of the loop
  display.display();
}

// ~~~~~~~~~~~~
// Button Logic
// ~~~~~~~~~~~~
// Neck buttons..........0-9
// Strum buttons.........10-13 || 10-NeckDown   11-NeckUp     12-BridgeDown
// 13-BridgeUp Directional buttons...14-17 || 14-Up    15-Right    16-Down
// 17-Left Start Button..........18 Select Button.........19 Save / Menu
// Button....20
void handleButtonPress(uint8_t i) {
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI note on based on the current tuning selection (note, velocity,
  // channel)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i <= 9) {
    MIDI.sendNoteOn(tuningSelection[selection].getNote(i),
                    tuningSelection[selection].getVelocity(),
                    tuningSelection[selection].getChannel());
  }
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI CC messages from the strum buttons (control, value, channel)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  else if (i >= 10 && i <= 13) {
    // Neck Down
    if (i == 10) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckDownCC(),
                             tuningSelection[selection].getNeckDownOnValue(),
                             tuningSelection[selection].getChannel());
    }
    // Neck Up
    else if (i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(),
                             tuningSelection[selection].getNeckUpOnValue(),
                             tuningSelection[selection].getChannel());
    }
    // Bridge Down
    else if (i == 12) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeDownCC(),
                             tuningSelection[selection].getBridgeDownOnValue(),
                             tuningSelection[selection].getChannel());
    }
    // Bridge Up
    else if (i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeUpCC(),
                             tuningSelection[selection].getBridgeUpOnValue(),
                             tuningSelection[selection].getChannel());
    }
  }
  // ~~~~~~~~~~~~~~~~~~~
  // Directional buttons
  // ~~~~~~~~~~~~~~~~~~~
  else if (i >= 14 && i <= 17) {
    // Up Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Keep track of when to save
    if (i == 14 && menuStep > 0) {
      paramUpdated = 1;
    }
    // change the MIDI channel up
    if (i == 14 && menuStep == 1) {
      // limit 1-16
      if (tuningSelection[selection].getChannel() < 16) {
        tuningSelection[selection].changeChannel(1);
      }
    }
    // Change the velocity up
    else if (i == 14 && menuStep == 3) {
      // limit 0-127
      if (tuningSelection[selection].getVelocity() < 127) {
        tuningSelection[selection].changeVelocity(1);
      }
    }
    // Change the selected note up
    else if (i == 14 && menuStep == 2) {
      // limit 0-127
      if (tuningSelection[selection].getNote(selectedNote) < 127) {
        // Change note on the fly if it being held down, will take care of MIDI
        // note off errors
        handleHeldNotesWhileTransposing(1);
      }
    }
    // ~~~~~~~~~~~~~~~~~~~~~~
    // Up Arrow CC Monster 👹
    // ~~~~~~~~~~~~~~~~~~~~~~
    // Change Neck Up CC
    else if (i == 14 && menuStep == 4) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpCC() < 127) {
          tuningSelection[selection].setNeckUpCC(1);
        }
      }
      // Change Neck Up Off Value
      else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOffValue() < 127) {
          tuningSelection[selection].setNeckUpOffValue(1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(),
                                 tuningSelection[selection].getNeckUpOffValue(),
                                 tuningSelection[selection].getChannel());
        }
      }
      // Change Neck Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOnValue() < 127) {
          tuningSelection[selection].setNeckUpOnValue(1);
        }
      }
      // Change Neck Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownCC() < 127) {
          tuningSelection[selection].setNeckDownCC(1);
        }
      }
      // Change Neck Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOffValue() < 127) {
          tuningSelection[selection].setNeckDownOffValue(1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getNeckDownCC(),
              tuningSelection[selection].getNeckDownOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Neck Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOnValue() < 127) {
          tuningSelection[selection].setNeckDownOnValue(1);
        }
      }
    }

    // Change Bridge Up CC
    else if (i == 14 && menuStep == 5) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpCC() < 127) {
          tuningSelection[selection].setBridgeUpCC(1);
        }
      }
      // Change Bridge Up CC
      else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOffValue() < 127) {
          tuningSelection[selection].setBridgeUpOffValue(1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getBridgeUpCC(),
              tuningSelection[selection].getBridgeUpOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Bridge Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOnValue() < 127) {
          tuningSelection[selection].setBridgeUpOnValue(1);
        }
      }
      // Change Bridge Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownCC() < 127) {
          tuningSelection[selection].setBridgeDownCC(1);
        }
      }
      // Change Bridge Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOffValue() < 127) {
          tuningSelection[selection].setBridgeDownOffValue(1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getBridgeDownCC(),
              tuningSelection[selection].getBridgeDownOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Bridge Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOnValue() < 127) {
          tuningSelection[selection].setBridgeDownOnValue(1);
        }
      }
    }
    // ~~~~~~ End of the UP Arrow CC 👹 ~~~~~~~~~~~~~~~~
    // Transpose all notes one semitone up
    if (i == 14 && menuStep == 0) {
      // check to make sure notes don't go above 127
      if (tuningSelection[selection].getHighestNote() <= 126) {
        handleHeldNotesWhileTransposing(1);
        // paramUpdated = 1;
      }
    }
    // Up Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Right Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Scroll right through notes
    else if (i == 15 && menuStep == 2) {
      // Limit to 0-9
      if (selectedNote < 9) {
        selectedNote++;
      } else {
        selectedNote = 0;
      }
    }
    // Scroll right through CC values
    // for both neck and bridge
    else if (i == 15 && menuStep >= 4) {
      // Limit to 0-5
      if (selectedCC < 5) {
        selectedCC++;
      } else {
        selectedCC = 0;
      }
    }
    // Transpose all notes one Octave up
    if (i == 15 && menuStep == 0) {
      // check to make sure notes don't go above 127
      if (tuningSelection[selection].getHighestNote() <= 115) {
        handleHeldNotesWhileTransposing(12);
      }
    }
    // Right Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Down Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // change the MIDI channel down
    if (i == 16 && menuStep == 1) {
      // limit 1-16
      if (tuningSelection[selection].getChannel() > 1) {
        tuningSelection[selection].changeChannel(-1);
      }
    }
    // Change the selected note down
    else if (i == 16 && menuStep == 2) {
      // limit 0-127
      if (tuningSelection[selection].getNote(selectedNote) > 0) {
        // Change note on the fly if it being held down, will take care of MIDI
        // note off errors
        handleHeldNotesWhileTransposing(-1);
      }
    }
    // Change the velocity down
    else if (i == 16 && menuStep == 3) {
      // limit 0-127
      if (tuningSelection[selection].getVelocity() > 0) {
        tuningSelection[selection].changeVelocity(-1);
      }
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Down Arrow CC Monster 👹
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Change Neck Up CC
    else if (i == 16 && menuStep == 4) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpCC() > 0) {
          tuningSelection[selection].setNeckUpCC(-1);
        }
      }
      // Change Neck Up Off
      else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOffValue() > 0) {
          tuningSelection[selection].setNeckUpOffValue(-1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(),
                                 tuningSelection[selection].getNeckUpOffValue(),
                                 tuningSelection[selection].getChannel());
        }
      }
      // Change Neck Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOnValue() > 0) {
          tuningSelection[selection].setNeckUpOnValue(-1);
        }
      }
      // Change Neck Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownCC() > 0) {
          tuningSelection[selection].setNeckDownCC(-1);
        }
      }
      // Change Neck Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOffValue() > 0) {
          tuningSelection[selection].setNeckDownOffValue(-1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getNeckDownCC(),
              tuningSelection[selection].getNeckDownOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Neck Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOnValue() > 0) {
          tuningSelection[selection].setNeckDownOnValue(-1);
        }
      }
    }

    // Change Bridge Up CC
    else if (i == 16 && menuStep == 5) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpCC() > 0) {
          tuningSelection[selection].setBridgeUpCC(-1);
        }
      }
      // Change Bridge Up Off
      else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOffValue() > 0) {
          tuningSelection[selection].setBridgeUpOffValue(-1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getBridgeUpCC(),
              tuningSelection[selection].getBridgeUpOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Bridge Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOnValue() > 0) {
          tuningSelection[selection].setBridgeUpOnValue(-1);
        }
      }
      // Change Bridge Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownCC() > 0) {
          tuningSelection[selection].setBridgeDownCC(-1);
        }
      }
      // Change Bridge Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOffValue() > 0) {
          tuningSelection[selection].setBridgeDownOffValue(-1);
          // Send the off CC as it is updated
          MIDI.sendControlChange(
              tuningSelection[selection].getBridgeDownCC(),
              tuningSelection[selection].getBridgeDownOffValue(),
              tuningSelection[selection].getChannel());
        }
      }
      // Change Bridge Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOnValue() > 0) {
          tuningSelection[selection].setBridgeDownOnValue(-1);
        }
      }
    }
    // ~~~~~~ End of the DOWN Arrow CC 👹 ~~~~~~~~~~~~~~~~
    // Transpose all notes one semitone down
    if (i == 16 && menuStep == 0) {
      // check to make sure notes don't go below 0
      if (tuningSelection[selection].getLowestNote() >= 1) {
        handleHeldNotesWhileTransposing(-1);
      }
    }

    // Keep track of when to save
    if (i == 16 && menuStep > 0) {
      paramUpdated = 1;
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
    // Scroll left through CC values
    // for both neck and bridge
    if (i == 17 && menuStep >= 4) {
      // Limit to 0-5
      if (selectedCC > 0) {
        selectedCC--;
      } else {
        selectedCC = 5;
      }
    }
    // Transpose all notes one octave down
    if (i == 17 && menuStep == 0) {
      // check to make sure notes don't go below 0
      if (tuningSelection[selection].getLowestNote() >= 12) {
        handleHeldNotesWhileTransposing(-12);
      }
    }
    // Left Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }
  // ~~~~~~~~~~~~~~~~~~~~~~
  // Menu / Display Buttons
  // ~~~~~~~~~~~~~~~~~~~~~~
  // Start Button ~~~~~~~~~~~~~~~~~~~~~~~~
  if (i == 18) {
    // To confirm Save
    if (saveChangesFlag == 1) {
      confirmSave();
    }
    // Make sure menu is toggled on and not in the save screen
    if (menuStep > 0) {  // display main screen after save BUG hunt
      // Limit to 1-5
      if (menuStep < 5) {
        menuStep++;
      } else if (menuStep > 0) {
        menuStep = 1;
      }
    }
    // Switch What Info to Display in main screen
    else {  //  BUG hunt
      // Limit to 0-2
      if (displayStep < 2) {
        displayStep++;
      } else if (displayStep > 0) {
        displayStep = 0;
      }
    }
  }
  // End Start Button ~~~~~~~~~~~~~~~~~~
  // Select Button ~~~~~~~~~~~~~~~~~~~~~
  if (i == 19) {
    // for cancel save function
    if (saveChangesFlag == 1) {
      cancelSave();
    }
    // Make sure menu is toggled on and not in the save screen
    if (menuStep > 0) {  // display main screen after save BUG hunt
      // Limit to 1-5
      if (menuStep > 1) {
        menuStep--;
      } else if (menuStep > 0) {
        menuStep = 5;
      }
    }
    // Switch what info to display in main screen
    else {  // BUG hunt
      // Limit to 0-2
      if (displayStep > 0) {
        displayStep--;
      } else {
        displayStep = 2;
      }
    }
  }
  // End Select Button ~~~~~~~~~~~~~~~~
  // Save / Menu Button ~~~~~~~~
  if (i == 20) {
    // Exit the save menu without discarding changes
    if (saveChangesFlag == 1 && menuStep >= 0) {
      menuStep = 0;
      saveChangesFlag = 0;
    }
    // Turn on the edit menu
    else if (menuStep == 0) {
      menuStep++;
    } else {
      // There have been changes to params go to save screen
      if (paramUpdated == 1 && saveChangesFlag == 0) {
        saveChangesFlag = 1;
      }

      // Close the edit menu if no changes
      if (paramUpdated == 0) {
        menuStep = 0;
      }
    }
  }
  // End Save / Menu Button ~~~~

  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Pressed!");
}

void handleButtonRelease(uint8_t i) {
  // Turn off MIDI notes that have been played (note, velocity, channel)
  if (i <= 9) {
    MIDI.sendNoteOff(tuningSelection[selection].getNote(i), 0,
                     tuningSelection[selection].getChannel());
  }
  // Reset the strum switches
  else if (i >= 10 && i <= 13) {
    // Reset neck down Switch
    if (i == 10) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckDownCC(),
                             tuningSelection[selection].getNeckDownOffValue(),
                             tuningSelection[selection].getChannel());
    }
    // Reset neck up Switch
    else if (i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(),
                             tuningSelection[selection].getNeckUpOffValue(),
                             tuningSelection[selection].getChannel());
    }
    // Reset bridge down Switch
    else if (i == 12) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeDownCC(),
                             tuningSelection[selection].getBridgeDownOffValue(),
                             tuningSelection[selection].getChannel());
    }
    // Reset bridge up Switch
    else if (i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeUpCC(),
                             tuningSelection[selection].getBridgeUpOffValue(),
                             tuningSelection[selection].getChannel());
    }
  }

  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Released!");
}

void buttonMux() {
  // Loop through all the button channels on the MUX
  for (uint8_t i = 0; i < 24; ++i) {
    // Enable the appropriate MUX
    enableMux(i < 8 ? 0 : (i < 16 ? 1 : 2));

    // Control the selector pins based on the binary representation of i (this
    // all chatGPT 😂) Checks the least significant bit (LSB) of the variable i.
    // The & operator performs a bitwise AND operation
    digitalWrite(signal0, (i & 0x01) ? HIGH : LOW);
    digitalWrite(signal1, (i & 0x02) ? HIGH : LOW);
    digitalWrite(signal2, (i & 0x04) ? HIGH : LOW);

    // Read the value from the selected button
    uint8_t buttonValue = digitalRead(muxCommon);

    // Check for button press
    if (buttonValue == 0 && previousButtonState[i] == 0) {
      // Button is pressed
      previousButtonState[i] = 1;
      handleButtonPress(i);
    }
    // Check for button release
    else if (buttonValue > 0 && previousButtonState[i] == 1) {
      // Button is released
      previousButtonState[i] = 0;
      handleButtonRelease(i);
    }
  }
}

// Use the enable pin on the Multiplexer to turn on only the MUX we want to read
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

// Change note on the fly if it being held down, will take care of MIDI note off
// for any held notes and MIDI on for the updated note value
void handleHeldNotesWhileTransposing(byte semitones) {
  // Check if any notes are being held
  if (numHeldNotes > 0) {
    turnOffHeldNotes();

    // This changes the entire array of notes
    if (menuStep == 0) {              // check if on main screen
      for (int i = 0; i < 10; ++i) {  // loop through all notes
        tuningSelection[selection].changeNote(i, semitones);
      }
    }
    // Single Note change
    else if (menuStep == 2) {  // Edit single notes menu selection
      tuningSelection[selection].changeNote(
          selectedNote, semitones);  // update single note in array
      return;  // Keeps the note change from skipping to the end when up or down
               // is held
    }
    // Play new notes
    for (int i = 0; i < numHeldNotes; ++i) {
      handleButtonPress(heldNotes[i]);
    }
  }
  // No held notes, simply update
  else if (numHeldNotes == 0) {
    // This targets changing the entire array at once
    if (menuStep == 0) {  // Main display screen
      tuningSelection[selection].transposeAllNotes(semitones);
    }
    // Single Note change
    else {
      tuningSelection[selection].changeNote(selectedNote, semitones);
    }
  }
}

void turnOffHeldNotes() {
  for (int i = 0; i < numHeldNotes; ++i) {
    // Limit button release to fret board
    if (heldNotes[i] >= 0 && heldNotes[i] <= 9) {
      handleButtonRelease(heldNotes[i]);  // Turn off held note
    }
  }
}

// Update the array of held notes to help with handleHeldNotesWhileTransposing()
void updateHeldNotes() {
  numHeldNotes = 0;

  // Loop through all notes to see if they are held down
  for (int i = 0; i < MAX_HELD_NOTES; ++i) {
    if (previousButtonState[i] == 1) {
      // Index of held button and update array and count
      heldNotes[numHeldNotes++] = i;
    }
  }

  // Check if buttons 14 or 16 are pressed
  if ((previousButtonState[14] == 1 || previousButtonState[16] == 1) &&
      !paramHeld) {
    // Set heldParamTime and paramHeld only once
    heldParamTime = millis();
    paramHeld = true;
  }

  // Reset paramHeld only if buttons 14 and 16 are released
  if (previousButtonState[14] == 0 && previousButtonState[16] == 0) {
    paramHeld = false;
  }
}

// When Directional buttons are held down they will update the parameters
// quickly
void speedyParams() {
  // Use the loop only when directional button is held and in a menu screen
  if (paramHeld && menuStep > 0) {
    // Loop through held notes
    for (int i = 0; i < numHeldNotes; i++) {
      // Wait for .4 sec before going fast
      if (millis() - heldParamTime >= 400) {
        // Up button
        if (heldNotes[i] == 14) {
          handleButtonPress(14);
        }
        // Down button
        if (heldNotes[i] == 16) {
          handleButtonPress(16);
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~
// Potentiometer Functions
// ~~~~~~~~~~~~~~~~~~~~~~~
void readSelectPot() {
  // Read the "5 way" selection Pot, map it and assign it. -1 to sync with index
  // of tuningSelection array
  uint8_t selectVoltage = analogRead(selectPot);
  int newSelection = map(selectVoltage, 15, 215, 1, 5) - 1;
  // Turn off any held notes and cancel any changes when changing tunings
  if (previousSelection != newSelection) {
    turnOffHeldNotes();
    if (paramUpdated) {
      cancelSave();
    }
    previousSelection = newSelection;
  }
  selection = newSelection;

  // Serial.print("Selection voltage: ");
  // Serial.println(selectVoltage);
  // Serial.print("Selection: ");
  // Serial.println(selection);
}

void readWhammyPot() {
  previousWhammy = whammy;
  // Read The Whammy Pot and use it to adjust the pitch
  uint8_t whammyVoltage = analogRead(whammyPot);

  // Try to limit the whammy pot's response to voltage spikes
  if (whammyVoltage >= 20 && whammyVoltage <= 220) {
    int newWhammy = map(whammyVoltage, 20, 195, 0, 8000);

    // Limit to 8000 to keep within MIDI bounds
    if (newWhammy >= 0 && newWhammy <= 8000) {
      if (newWhammy != previousWhammy) {
        whammy = newWhammy;
      }
    }
  } else {
    whammy = 0;
  }

  // Print debug information
  // Serial.print(" Whammy voltage: ");
  // Serial.print(whammyVoltage);
  // Serial.print(" whammy: ");
  // Serial.print(whammy);
  // Serial.print(" Previous Whammy: ");
  // Serial.println(previousWhammy);
}

void handleWhammy() {
  Serial.print(" --handle Whammy--");
  MIDI.sendPitchBend(whammy, tuningSelection[selection].getChannel());
}

// ~~~~~~~~~~~~~~
// Save Functions
// ~~~~~~~~~~~~~~
void confirmSave() {
  display.clearDisplay();
  display.setCursor(0, 26);
  display.print(F(" Changes Saved! "));
  display.display();
  saveTuningToEEPROM(selection);
  delay(1000);

  // Reset flags
  menuStep = 0;
  saveChangesFlag = 0;
  paramUpdated = 0;
  displayStep = 0;
}

void cancelSave() {
  display.clearDisplay();
  display.setCursor(0, 26);
  display.print(F(" Changes Discarded! "));
  display.display();
  loadTuningFromEEPROM(selection);
  delay(1000);

  // Reset flags
  menuStep = 0;
  saveChangesFlag = 0;
  paramUpdated = 0;
  displayStep = 0;
}

// Store tuning in EEPROM
void saveTuningToEEPROM(int selection) {
  EEPROM.put(selection * sizeof(Tuning), tuningSelection[selection]);
  delay(10);  // Add a delay between EEPROM writes
}
// Load tuning from EEPROM
void loadTuningFromEEPROM(int selection) {
  EEPROM.get(selection * sizeof(Tuning), tuningSelection[selection]);
}

void lightMenuLED() {
  unsigned long currentMillis = millis();  // Keep track of time
  if (paramUpdated == 1) {
    // Blink the LED
    if (currentMillis - previousMillisLED >= blinkInterval) {
      // Save the current time
      previousMillisLED = currentMillis;

      // Toggle the LED state
      if (digitalRead(menuLED) == HIGH) {
        digitalWrite(menuLED, LOW);
      } else {
        digitalWrite(menuLED, HIGH);
      }
    }
  } else if (menuStep > 0) {
    // Turn on the LED
    digitalWrite(menuLED, HIGH);
  } else {
    // Turn off the LED
    digitalWrite(menuLED, LOW);
  }
}

// This keeps track of the different lengths for the display and edit menu
void syncDisplayMenuStep() {
  if (menuStep == 4) {
    displayStep = 1;
  } else if (menuStep == 5) {
    displayStep = 2;
  } else if (menuStep < 4) {
    displayStep = 0;
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
  display.setCursor(0, 15);
  if (displayStep == 1) {
    // Highlight if selected
    if (selectedCC <= 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" Neck Up: "));
    display.setTextColor(WHITE, BLACK);  // Reset text color
    display.setCursor(0, 28);
    // Highlight if selected
    if (selectedCC == 0 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F("C: "));
    display.print(tuningSelection[selection].getNeckUpCC());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 1) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" X: "));
    display.print(tuningSelection[selection].getNeckUpOffValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" O: "));
    display.print(tuningSelection[selection].getNeckUpOnValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    display.setCursor(0, 42);
    // Highlight if selected
    if (selectedCC > 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" Neck Down: "));
    display.setTextColor(WHITE, BLACK);  // Reset text color
    display.setCursor(0, 55);
    // Highlight if selected
    if (selectedCC == 3 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F("C: "));
    display.print(tuningSelection[selection].getNeckDownCC());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 4 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" X: "));
    display.print(tuningSelection[selection].getNeckDownOffValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 5 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" O: "));
    display.print(tuningSelection[selection].getNeckDownOnValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color
  } else if (displayStep == 2) {
    // Highlight if selected
    if (selectedCC <= 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" Bridge Up: "));
    display.setTextColor(WHITE, BLACK);  // Reset text color
    display.setCursor(0, 28);
    // Highlight if selected
    if (selectedCC == 0 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F("C: "));
    display.print(tuningSelection[selection].getBridgeUpCC());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 1 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" X: "));
    display.print(tuningSelection[selection].getBridgeUpOffValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" O: "));
    display.print(tuningSelection[selection].getBridgeUpOnValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC > 2 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.setCursor(0, 42);
    display.print(F(" Bridge Down: "));
    display.setTextColor(WHITE, BLACK);  // Reset text color

    display.setCursor(0, 55);
    // Highlight if selected
    if (selectedCC == 3 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F("C: "));
    display.print(tuningSelection[selection].getBridgeDownCC());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 4 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" X: "));
    display.print(tuningSelection[selection].getBridgeDownOffValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color

    // Highlight if selected
    if (selectedCC == 5 && menuStep >= 4) {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(F(" O: "));
    display.print(tuningSelection[selection].getBridgeDownOnValue());
    display.setTextColor(WHITE, BLACK);  // Reset text color
  }
}

void displaySaveChanges() {
  display.clearDisplay();

  display.setCursor(5, 0);
  display.print(F(" Save Changes? "));
  display.setCursor(3, 20);
  display.print(F("Select"));
  display.setCursor(0, 30);
  display.print(F("DISCARD"));
  display.setCursor(72, 20);
  display.print(F("Start"));
  display.setCursor(79, 30);
  display.print(F("YES"));
  display.setCursor(49, 40);
  display.print(F("Menu"));
  display.setCursor(44, 50);
  display.print(F("CANCEL"));
}

void displayStartUp(int milliSec) {
  // Display splash screen for a few seconds
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Intuitive"));
  display.setCursor(0, 20);
  display.print(F("Harmony"));
  display.drawCircle(108, 33, 13, SSD1306_WHITE);
  display.drawCircle(113, 27, 10, SSD1306_WHITE);
  display.setTextSize(1);  // reset Text Size
  display.setCursor(0, 42);
  display.print(F("HarmonyHero"));
  display.setCursor(0, 55);
  display.print(F("Version: "));
  display.print(MAJOR_VERSION);
  display.print(F("."));
  display.print(MINOR_VERSION);
  display.print(F("."));
  display.print(PATCH_VERSION);
  display.display();

  delay(milliSec);
  // Clear the display
  display.clearDisplay();
  display.display();
}