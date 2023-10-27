# Arduino MUX Button Interface for Guitar Hero MIDI Controller

## Overview

This Arduino project demonstrates how to interface with multiple buttons using Multiplexer (MUX) modules. The code reads button presses and releases from a matrix of buttons connected through MUX modules. The ultimate goal is to create a MIDI controller by hacking into a Guitar Hero controller.

## Hardware 

- Arduino pro micro
- Multiplexer (MUX) modules (e.g., CD74HC4051)
- Buttons (from guitar hero controller)
- Jumper wires
- Guitar Hero controller (for MIDI hacking)

## Circuit Setup

1. Connect the common signal wire of the buttons to A0 pin on the Arduino.
2. Connect the signal pins 4, 7, 8 to the MUX modules.
3. Connect the enable pins 10, 14, 16 to the MUX module.
4. Connect buttons to the Input channels of the MUX based on the desired index

## Wiring Diagram
*This Diagram Has a 4052 IC but the actual circuit uses a 4051*

![Wiring Diagram](buttonMux%20Diagram.jpg)


## Contributors

- Jason Horst
- Chat GPT

## License

This project is licensed under the [MIT License](LICENSE).
