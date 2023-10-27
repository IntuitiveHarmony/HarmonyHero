// MUX Parameters
// signal pins
int signal0 = 4;
int signal1 = 7;
int signal2 = 8;
// enable pins
int enableMux0 = 16;
int enableMux1 = 14;
// common signal
int muxCommon = A0;

// Array to store the previous state of each button
int previousButtonState[16] = {0};

void setup() {
  Serial.begin(9600);
  pinMode(muxCommon, INPUT_PULLUP);

  pinMode(signal0, OUTPUT);
  pinMode(signal1, OUTPUT);
  pinMode(signal2, OUTPUT);

  pinMode(enableMux0, OUTPUT);
  pinMode(enableMux1, OUTPUT);
}

void loop() {
  buttonMux();
}

void handleButtonPress(int i) {
  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Pressed!");
}

void handleButtonRelease(int i) {
  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Released!");
}

void buttonMux() {
  const int analogThreshold = 100;

  // Loop through all the button channels on the MUX
  for (int i = 0; i < 16; ++i) {
    // Enable the appropriate MUX
    enableMux(i < 8 ? 0 : 1);

    // Control the selector pins based on the binary representation of i
    digitalWrite(signal0, (i & 0x01) ? HIGH : LOW);
    digitalWrite(signal1, (i & 0x02) ? HIGH : LOW);
    digitalWrite(signal2, (i & 0x04) ? HIGH : LOW);

    // Read the analog value from the selected button
    int buttonValue = analogRead(muxCommon);

    // Check for button press
    if (buttonValue < analogThreshold && previousButtonState[i] == 0) {
      // Button is pressed
      handleButtonPress(i);
      previousButtonState[i] = 1;
    }
    // Check for button release
    else if (buttonValue >= analogThreshold && previousButtonState[i] == 1) {
      // Button is released
      handleButtonRelease(i);
      previousButtonState[i] = 0;
    }
  }
}

void enableMux(int mux) {
  if (mux == 0) {
    digitalWrite(enableMux0, LOW);
    digitalWrite(enableMux1, HIGH);
  } else {
    digitalWrite(enableMux0, HIGH);
    digitalWrite(enableMux1, LOW);
  }
}
