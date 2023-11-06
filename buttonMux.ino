#include <Wire.h>              // For display
#include <Adafruit_SSD1306.h>  // For display

#include <EEPROM.h>  // To save variables across power cycle
// Define a flag address in EEPROM
#define INIT_FLAG_ADDRESS 0

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


// ~~~~~~~~~~~~~~~~~~~~~~
// Main Screen Parameters
// ~~~~~~~~~~~~~~~~~~~~~~
uint8_t displayStep = 0;  // 0-Notes 1-Neck  2-bridge

// ~~~~~~~~~~~~~~~
// Menu Parameters
// ~~~~~~~~~~~~~~~
uint8_t menuLED = 6;
uint8_t menuStep = 0;         // 0-Home 1-Channel 2-Notes 3-velocity 4-StrumSwitches
uint8_t selectedNote = 0;     // Note to edit, based off index
uint8_t selectedCC = 0;       // CC to edit 0-5 | up 0-2  down 3-5
uint8_t paramUpdated = 0;     // Keep track of when to save
uint8_t saveChangesFlag = 0;  // Keep track of when to display save changes screen

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
  byte channel = 12;
  byte velocity = 127;

  // 4 user assignable CC params tied to the strum switches
  // Default Values provided, may not work as anticipated depending on individual synth's local CC routing
  byte neckUpCC = 60;  // MOD wheel
  byte neckUpOff = 60;
  byte neckUpOn = 60;

  byte neckDownCC = 60;  // Portamento
  byte neckDownOffValue = 60;
  byte neckDownOnValue = 60;

  byte bridgeUpCC = 60;  // LFO Depth
  byte bridgeUpOffValue = 60;
  byte bridgeUpOnValue = 60;

  byte bridgeDownCC = 60;  // Volume
  byte bridgeDownOffValue = 60;
  byte bridgeDownOnValue = 60;

public:
  // constructor
  Tuning(byte initialNotes[10]) {
    // Copy the notes from the provided array to the class's notes array
    memcpy(notes, initialNotes, sizeof(notes));
  }
  // Note Methods
  byte getNote(uint8_t i) {
    return notes[i];
  }
  void changeNote(uint8_t index, byte change) {
    notes[index] += change;
  }
  // Channel Methods
  byte getChannel() {
    return channel;
  }
  void changeChannel(byte change) {
    channel += change;
  }
  // Velocity methods
  byte getVelocity() {
    return velocity;
  }
  void changeVelocity(byte change) {
    velocity += change;
  }
  // Control Change Methods ~~~~~~~~~~~~~
  // Neck Up CC Methods
  byte getNeckUpCC() {
    return neckUpCC;
  }
  void setNeckUpCC(byte change) {
    neckUpCC += change;
  }
  byte getNeckUpOffValue() {
    return neckUpOff;
  }
  void setNeckUpOffValue(byte change) {
    neckUpOff += change;
  }
  byte getNeckUpOnValue() {
    return neckUpOn;
  }
  void setNeckUpOnValue(byte change) {
    neckUpOn += change;
  }

  // Neck Down CC Methods
  byte getNeckDownCC() {
    return neckDownCC;
  }
  void setNeckDownCC(byte change) {
    neckDownCC += change;
  }
  byte getNeckDownOffValue() {
    return neckDownOffValue;
  }
  void setNeckDownOffValue(byte change) {
    neckDownOffValue += change;
  }
  byte getNeckDownOnValue() {
    return neckDownOnValue;
  }
  void setNeckDownOnValue(byte change) {
    neckDownOnValue += change;
  }

  // Bridge Up CC Methods
  byte getBridgeUpCC() {
    return bridgeUpCC;
  }
  void setBridgeUpCC(byte change) {
    bridgeUpCC += change;
  }
  byte getBridgeUpOffValue() {
    return bridgeUpOffValue;
  }
  void setBridgeUpOffValue(byte change) {
    bridgeUpOffValue += change;
  }
  byte getBridgeUpOnValue() {
    return bridgeUpOnValue;
  }
  void setBridgeUpOnValue(byte change) {
    bridgeUpOnValue += change;
  }

  // Bridge Down CC Methods
  byte getBridgeDownCC() {
    return bridgeDownCC;
  }
  void setBridgeDownCC(byte change) {
    bridgeDownCC += change;
  }
  byte getBridgeDownOffValue() {
    return bridgeDownOffValue;
  }
  void setBridgeDownOffValue(byte change) {
    bridgeDownOffValue += change;
  }
  byte getBridgeDownOnValue() {
    return bridgeDownOnValue;
  }
  void setBridgeDownOnValue(byte change) {
    bridgeDownOnValue += change;
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
byte notes5[10] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76 };
// byte notes5[10] = { 120, 127, 124, 125, 67, 115, 111, 123, 120, 116 };

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
}

// ~~~~~~~~~~~~
// Arduino Loop
// ~~~~~~~~~~~~
void loop() {

  Serial.print("Selection ");
  Serial.println(selection);


  readPots();
  buttonMux();
  lightMenuLED();
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
// Neck buttons....0-9
// Strum buttons...10-13       || 10-NeckDown   11-NeckUp     12-BridgeDown     13-BridgeUp
// Directional buttons...14-17 || 14-Up    15-Right    16-Down    17-Left
// Save Button...18
void handleButtonPress(uint8_t i) {
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI note on based on the current tuning selection (note, velocity, channel)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i <= 9) {
    MIDI.sendNoteOn(tuningSelection[selection].getNote(i), tuningSelection[selection].getVelocity(), tuningSelection[selection].getChannel());
  }
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Send MIDI CC messages from the strum buttons (control, value, channel)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  else if (i >= 10 && i <= 13) {
    // Neck Down
    if (i == 10) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckDownCC(), tuningSelection[selection].getNeckDownOnValue(), tuningSelection[selection].getChannel());
    }
    // Neck Up
    else if (i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(), tuningSelection[selection].getNeckUpOnValue(), tuningSelection[selection].getChannel());
    }
    // Bridge Down
    else if (i == 12) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeDownCC(), tuningSelection[selection].getBridgeDownOnValue(), tuningSelection[selection].getChannel());
    }
    // Bridge Up
    else if (i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeUpCC(), tuningSelection[selection].getBridgeUpOnValue(), tuningSelection[selection].getChannel());
    }
  }
  // ~~~~~~~~~~~~~~~~~~~
  // Directional buttons
  // ~~~~~~~~~~~~~~~~~~~
  else if (i >= 14 && i <= 17) {
    // Up Button ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Keep track of when to save
    if (menuStep > 0) {
      paramUpdated = 1;
    }
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
        // Change note on the fly if it being held down, will take care of MIDI note off errors
        if (buttonBeingHeld() > 0 && buttonBeingHeld() <= 9) {  // restrict to the note triggers
          uint8_t heldNote = buttonBeingHeld();
          handleButtonRelease(heldNote);                           // Turn off held note
          tuningSelection[selection].changeNote(selectedNote, 1);  // update the note in array
          handleButtonPress(heldNote);                             // Play new note
        }
        // No held notes, simply update
        else {
          tuningSelection[selection].changeNote(selectedNote, 1);
        }
      } else {
        // Do noting because max of 127 reached
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
        } else {
          // Do nothing because max of 127 reached
        }
      } else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOffValue() < 127) {
          tuningSelection[selection].setNeckUpOffValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Neck Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOnValue() < 127) {
          tuningSelection[selection].setNeckUpOnValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Neck Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownCC() < 127) {
          tuningSelection[selection].setNeckDownCC(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Neck Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOffValue() < 127) {
          tuningSelection[selection].setNeckDownOffValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Neck Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOnValue() < 127) {
          tuningSelection[selection].setNeckDownOnValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
    }


    // Change Bridge Up CC
    else if (i == 14 && menuStep == 5) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpCC() < 127) {
          tuningSelection[selection].setBridgeUpCC(1);
        } else {
          // Do nothing because max of 127 reached
        }
      } else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOffValue() < 127) {
          tuningSelection[selection].setBridgeUpOffValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Bridge Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOnValue() < 127) {
          tuningSelection[selection].setBridgeUpOnValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Bridge Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownCC() < 127) {
          tuningSelection[selection].setBridgeDownCC(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Bridge Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOffValue() < 127) {
          tuningSelection[selection].setBridgeDownOffValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
      }
      // Change Bridge Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOnValue() < 127) {
          tuningSelection[selection].setBridgeDownOnValue(1);
        } else {
          // Do nothing because max of 127 reached
        }
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
        // Change note on the fly if it being held down, will take care of MIDI note off errors
        if (buttonBeingHeld() > 0 && buttonBeingHeld() <= 9) {  // restrict to the note triggers
          uint8_t heldNote = buttonBeingHeld();
          handleButtonRelease(heldNote);                            // Turn off held note
          tuningSelection[selection].changeNote(selectedNote, -1);  // update the note in array
          handleButtonPress(heldNote);                              // Play new note
        }
        // No held notes, simply update
        else {
          tuningSelection[selection].changeNote(selectedNote, -1);
        }
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
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Down Arrow CC Monster 👹
    // ~~~~~~~~~~~~~~~~~~~~~~~~
    // Change Neck Up CC
    else if (i == 16 && menuStep == 4) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpCC() > 0) {
          tuningSelection[selection].setNeckUpCC(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      } else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOffValue() > 0) {
          tuningSelection[selection].setNeckUpOffValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Neck Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getNeckUpOnValue() > 0) {
          tuningSelection[selection].setNeckUpOnValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Neck Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownCC() > 0) {
          tuningSelection[selection].setNeckDownCC(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Neck Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOffValue() > 0) {
          tuningSelection[selection].setNeckDownOffValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Neck Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getNeckDownOnValue() > 0) {
          tuningSelection[selection].setNeckDownOnValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
    }


    // Change Bridge Up CC
    else if (i == 16 && menuStep == 5) {
      if (selectedCC == 0) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpCC() > 0) {
          tuningSelection[selection].setBridgeUpCC(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      } else if (selectedCC == 1) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOffValue() > 0) {
          tuningSelection[selection].setBridgeUpOffValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Bridge Up On Value
      else if (selectedCC == 2) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeUpOnValue() > 0) {
          tuningSelection[selection].setBridgeUpOnValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Bridge Down CC Value
      else if (selectedCC == 3) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownCC() > 0) {
          tuningSelection[selection].setBridgeDownCC(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Bridge Down Off Value
      else if (selectedCC == 4) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOffValue() > 0) {
          tuningSelection[selection].setBridgeDownOffValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
      // Change Bridge Down On Value
      else if (selectedCC == 5) {
        // limit 0-127
        if (tuningSelection[selection].getBridgeDownOnValue() > 0) {
          tuningSelection[selection].setBridgeDownOnValue(-1);
        } else {
          // Do nothing because min of 0 reached
        }
      }
    }
    // Keep track of when to save
    if (menuStep > 0) {
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
    // Scroll right through CC values
    // for both neck and bridge
    else if (i == 17 && menuStep >= 4) {
      // Limit to 0-5
      if (selectedCC > 0) {
        selectedCC--;
      } else {
        selectedCC = 5;
      }
    }
    // Left Button End ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  }
  // ~~~~~~~~~~~~
  // Menu / Display Buttons
  // ~~~~~~~~~~~~
  // Start Button ~~~~~~~~~~~~~~~~~~~~~~~~
  if (i == 18) {
    // To confirm Save
    if (saveChangesFlag == 1) {
      confirmSave();
    }
    // Make sure menu is toggled on
    if (menuStep > 0) {
      // Limit to 1-5
      if (menuStep < 5) {
        menuStep++;
      } else if (menuStep > 0) {
        menuStep = 1;
      }
    }
    // Switch What Info to Display in main screen
    else {
      // Limit to 1-2
      if (displayStep < 2) {
        displayStep++;
      } else if (displayStep > 0) {
        displayStep = 0;
      }
    }
  }
  // Select Button ~~~~~~~~~~~~~~~~~~
  if (i == 19) {
    // for cancel save function
    if (saveChangesFlag == 1) {
      cancelSave();
    }
    // Make sure menu is toggled on
    if (menuStep > 0) {
      // Limit to 1-5
      if (menuStep > 1) {
        menuStep--;
      } else if (menuStep > 0) {
        menuStep = 5;
      }
    }
    // Switch what info to display in main screen
    else {
      // Limit to 0-2
      if (displayStep > 0) {
        displayStep--;
      } else {
        displayStep = 2;
      }
    }
  }
  // Menu Toggle / Save Button ~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (i == 20) {
    // Turn on the edit menu
    if (menuStep == 0) {
      menuStep++;
    } else {
      // There have been changes to params
      if (paramUpdated == 1) {
        saveChangesFlag = 1;
      }

      // Close the edit menu if no changes
      if (paramUpdated == 0) {
        menuStep = 0;
      }
    }
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
    // Reset neck down Switch
    if (i == 10) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckDownCC(), tuningSelection[selection].getNeckDownOffValue(), tuningSelection[selection].getChannel());
    }
    // Reset neck up Switch
    else if (i == 11) {
      MIDI.sendControlChange(tuningSelection[selection].getNeckUpCC(), tuningSelection[selection].getNeckUpOffValue(), tuningSelection[selection].getChannel());
    }
    // Reset bridge down Switch
    else if (i == 12) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeDownCC(), tuningSelection[selection].getBridgeDownOffValue(), tuningSelection[selection].getChannel());
    }
    // Reset bridge up Switch
    else if (i == 13) {
      MIDI.sendControlChange(tuningSelection[selection].getBridgeUpCC(), tuningSelection[selection].getBridgeUpOffValue(), tuningSelection[selection].getChannel());
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

uint8_t buttonBeingHeld() {
  // only loop through the note triggers
  for (int i = 0; i <= 9; ++i) {
    Serial.print("From the held function: ");
    Serial.println(i);
    if (previousButtonState[i] == 1) {
      return i;
    }
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

  display.setCursor(0, 0);
  display.print(F(" Save Changes? "));
}

void readPots() {
  // Read the "5 way" selection Pot, map it and assign it. -1 to sync with index of tuningSelection array
  uint8_t selectVoltage = analogRead(selectPot);
  selection = map(selectVoltage, 15, 215, 1, 5) - 1;
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

// Save Functions
void confirmSave() {
  display.clearDisplay();
  display.setCursor(0, 26);
  display.print(F(" Changes Saved! "));
  display.display();
  saveTuningToEEPROM(selection);

  Tuning tempTuning(notes1);
  loadTuningFromEEPROM(selection);
  tempTuning = tuningSelection[selection];
  Serial.print("Read from EEPROM after save - Tuning ");
  Serial.print(selection);
  Serial.print(": ");
  for (int j = 0; j < 10; ++j) {
    Serial.print(tempTuning.getNote(j));
    Serial.print(" ");
  }
  Serial.print("Channel: ");
  Serial.print(tempTuning.getChannel());
  Serial.println();

  delay(1000);
  // Reset flags
  saveChangesFlag = 0;
  paramUpdated = 0;
  menuStep = 0;
}
void cancelSave() {
  display.clearDisplay();
  display.setCursor(0, 26);
  display.print(F(" Changes Canceled! "));
  display.display();
  loadTuningFromEEPROM(selection);
  delay(1000);

  // Reset flags
  saveChangesFlag = 0;
  paramUpdated = 0;
  menuStep = 0;
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
  if (menuStep > 0) {
    digitalWrite(menuLED, HIGH);
  } else {
    digitalWrite(menuLED, LOW);
  }
}