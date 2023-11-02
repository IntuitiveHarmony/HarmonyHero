#include <Wire.h>              // For display
#include <Adafruit_SSD1306.h>  // For display

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

// Array to store the previous state of each button
uint8_t previousButtonState[24] = { 0 };  // Updated array size


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


  MIDI.begin(MIDI_CHANNEL_OMNI);  // Initialize the Midi Library.
  pinMode(muxCommon, INPUT_PULLUP);

  pinMode(signal0, OUTPUT);
  pinMode(signal1, OUTPUT);
  pinMode(signal2, OUTPUT);

  pinMode(enableMux0, OUTPUT);
  pinMode(enableMux1, OUTPUT);
  pinMode(enableMux2, OUTPUT);
}

// ~~~~~~~~~~~~
// Arduino Loop
// ~~~~~~~~~~~~
void loop() {
  buttonMux();
}

// ~~~~~~~~~~~~
// Button Logic
// ~~~~~~~~~~~~
void handleButtonPress(uint8_t i) {
  byte note = i + 60;
  byte velocity = 80;
  byte channel = 14;

  MIDI.sendNoteOn(note, velocity, channel);

  display.setCursor(0, 0);
  display.print(F("Button "));
  display.setCursor(39, 0);
  display.print(i);
  display.setCursor(44, 0);
  display.print(F(" Pressed!"));
  display.display();  // Update the display

  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Pressed!");
}

void handleButtonRelease(uint8_t i) {
  MIDI.sendNoteOff(i + 60, 0, 14);

  display.clearDisplay();  // clear the display
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
