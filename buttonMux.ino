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

void setup() {
  Serial.begin(9600);
  pinMode(muxCommon, INPUT_PULLUP);  // Enable internal pull-up resistor (makes buttons more stable)

  // set up signal pins as outputs
  pinMode(signal0, OUTPUT);
  pinMode(signal1, OUTPUT);
  pinMode(signal2, OUTPUT);

  // set up enable pins as outputs
  pinMode(enableMux0, OUTPUT);
  pinMode(enableMux1, OUTPUT);
}

void loop() {
  // enableMux(0);
  // buttonMux();
  // enableMux(1);
  buttonMux();
}


void handleButton(int i) {
  Serial.print("Button ");
  Serial.print(i);
  Serial.println(" Pressed!");
}

void buttonMux() {
  const int analogThreshold = 100;  // Adjust as needed
  // Loop through all the button channels on the mux
  for (int i = 0; i < 16; ++i) {
    // Switch between the two mux and enable and disable accordingly
    // Enable the appropriate MUX
    enableMux(i < 8 ? 0 : 1);
    // if (i % 2) {
    //   digitalWrite(enableMux0, HIGH);
    //   digitalWrite(enableMux1, LOW);
    // } else {
    //   digitalWrite(enableMux1, HIGH);
    //   digitalWrite(enableMux0, LOW);
    // }

    // Control the selector pins based on the binary representation of i
    digitalWrite(signal0, (i & 0x01) ? HIGH : LOW);
    digitalWrite(signal1, (i & 0x02) ? HIGH : LOW);
    digitalWrite(signal2, (i & 0x04) ? HIGH : LOW);


    // Read the analog value from the selected button
    int buttonValue = analogRead(muxCommon);

    if (buttonValue < analogThreshold) {
      // Handle the button press
      handleButton(i);
    }
    // Print the button number and its corresponding analog value
    // Serial.print("Button ");
    // Serial.print(i);
    // Serial.print(": ");
    // Serial.println(buttonValue);

    // Delay for stability, adjust as needed
    // delay(100);
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
