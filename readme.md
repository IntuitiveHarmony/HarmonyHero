# Guitar Hero MIDI Controller

## Overview

This Arduino project chronicles the creation of a MIDI controller by hacking into a Guitar Hero controller.

## Hardware 

- Arduino pro micro
- Multiplexer (MUX) modules (e.g., CD74HC4051)
- OLED SSD Screen (4 pin)
- Guitar Hero controller (for MIDI hacking)
- Buttons (from guitar hero controller)
- TRS jack (from guitar hero controller) 

## Software

- [Adafruit_SSD1306 library](https://github.com/adafruit/Adafruit_SSD1306) for display
- [arduino_midi_library](https://github.com/FortySevenEffects/arduino_midi_library/tree/dev) for MIDI functionality
- [EEPROM library](https://docs.arduino.cc/learn/built-in-libraries/eeprom) to persist data across power cycle
- [Custom Code](https://github.com/IntuitiveHarmony/guitarHeroHack/blob/master/buttonMux.ino) hacked together with 💜 by me

## Circuit Setup

1. Connect the common signal wire of the buttons to A0 pin on the Arduino.
2. Connect the signal pins 4, 7, 8 to the MUX modules.
3. Connect the enable pins 10, 14, 16 to the MUX module.
4. Connect buttons to the Input channels of the MUX based on the desired index

## Wiring Diagram
*This Diagram Has a 4052 IC but the actual circuit uses a 4051*

![Wiring Diagram](buttonMux%20Diagram.jpg)


## A word about MIDI

MIDI stands for Musical Instrument Digital Interface.  Ok,  what does that even mean?  The short answer is that MIDI is a way that electronic instruments communicate with each other.  So a MIDI controller such as the one I am building doesn't produce or send audio information, it rather sends data.  A MIDI controller is just a real-time, data slinger.  One of the pieces of data MIDI is used to control is the note data.  This comes in the form of a pair of MIDI messages, a `note on` and a `note off` message. They look something like this.

```c++
MIDI.sendNoteOn(note, velocity, channel);
MIDI.sendNoteOff(note, velocity, channel);

```

 Each `note on` will be executed the moment a note key is pressed. The `note off` will be executed once the button is released. The note is a value 0-127 and each correlates with a musical note and is what ties the `note off` to the `note on`. (with me?) 

 Let's say a function transposes the note while it is being held.  Then something like below will more than likely happen causing the original note to play until it a `note off` is played for that note.  I call them `sticky notes` and they are pretty annoying. Especially when it is high pitched and I cannot make the right `note off` message happen 😵‍💫. 

 ```c++
MIDI.sendNoteOn(note, velocity, channel);   // Sticky note will play forever...
MIDI.sendNoteOff(newNote, velocity, channel);

```  

There are a few instances in the Guitar Hero Hack where there is the potential for this to happen and was indeed happening so many times during testing.  To help take care of this I implemented the `handleHeldNotesWhileTransposing` function.  It loops through any held notes, triggers their respective `note off`, updates the note by the chosen interval, and then plays the updated note or notes.  This make the instrument the sound of "tuning" it while scrolling through the available semitone and octave interval steps.

```c++
// Change note on the fly if it being held down, will take care of MIDI note off for any held notes and MIDI on for the updated note value
void handleHeldNotesWhileTransposing(byte semitones) {
  // Check if any notes are being held
  if (numHeldNotes > 0) {
    for (int i = 0; i < numHeldNotes; ++i) {
      uint8_t heldNote = heldNotes[i];
      handleButtonRelease(heldNote);  // Turn off held note
    }
    // This changes the entire array of notes
    if (displayStep == 0 && menuStep == 0) {  // check if on main screen
      for (int i = 0; i < 10; ++i) {          // loop through all notes
        tuningSelection[selection].changeNote(i, semitones);
      }
    }
    // Single Note change
    else if (menuStep == 2) {                                          // Edit single notes menu selection
      tuningSelection[selection].changeNote(selectedNote, semitones);  // update single note in array
    }
    // Play new notes
    for (int i = 0; i < numHeldNotes; ++i) {
      uint8_t heldNote = heldNotes[i];
      handleButtonPress(heldNote);
    }
  }
  // No held notes, simply update
  else if (numHeldNotes == 0) {
    // This targets changing the entire array at once
    if (displayStep == 0 && menuStep == 0) { // Main display screen
      tuningSelection[selection].transposeAllNotes(semitones);
    }
    // Single Note change
    else {
      tuningSelection[selection].changeNote(selectedNote, semitones);
    }
  }
}
```


## Contributors

- Jason Horst
- Inspiration from a handful of docs, projects and articles I'm currently looking for the links to.
- Chat GPT (tedious work, some logic)

## License

This project is licensed under the [MIT License](https://opensource.org/license/mit/).
