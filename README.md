# Arduino OLED Bitmap Manipulation and BLE Communication System

Arduino Uno R3 prototype that renders, moves, rotates, transmits, receives, and displays compact bitmap graphics through an OLED interface and Bluetooth Low Energy communication.

The system uses a two-axis analogue joystick for local image control, an AT-09 BLE module for wireless packet exchange with an Android device, and a red LED to indicate communication activity.

![Integrated hardware prototype](assets/integrated-hardware-prototype.jpg)

## Features

* Custom 5×7 bitmap letters stored in program memory
* Local bitmap translation using analogue joystick input
* Directional hold gestures for letter selection, rotation, and transmission
* 90° bitmap rotation at 0°, 90°, 180°, and 270°
* Event-based Bluetooth Low Energy communication
* Packet transmission and reception using a compact text format
* Input packet validation and error feedback
* Split-screen OLED display for local and received bitmap states
* LED activity indication for two seconds after successful send or receive events

## Hardware

* Arduino Uno R3
* 0.96-inch 128×64 SSD1306 I²C OLED display
* AT-09 Bluetooth Low Energy module
* Two-axis analogue thumb joystick
* Red LED and 220 Ω current-limiting resistor
* Android device running Serial Bluetooth Terminal for communication testing

## Hardware connections

| Component    | Arduino Uno connection                          |
| ------------ | ----------------------------------------------- |
| OLED SDA     | A4                                              |
| OLED SCL     | A5                                              |
| OLED VCC     | 5V                                              |
| OLED GND     | GND                                             |
| Joystick VRx | A0                                              |
| Joystick VRy | A1                                              |
| Joystick VCC | 5V                                              |
| Joystick GND | GND                                             |
| AT-09 TXD    | D2 — SoftwareSerial RX                          |
| Arduino D3   | AT-09 RXD through a logic-level voltage divider |
| LED anode    | D6 through 220 Ω resistor                       |
| LED cathode  | GND                                             |

> Ensure that the BLE module supply voltage and RX logic level match the module specification. Do not connect a 5V Arduino TX signal directly to a 3.3V RX input.

## Control scheme

| Joystick action               | Function                             |
| ----------------------------- | ------------------------------------ |
| Moderate directional movement | Move the local bitmap                |
| Hold right for about 800 ms   | Select the next bitmap letter        |
| Hold left for about 800 ms    | Rotate the local bitmap by 90°       |
| Hold down for about 900 ms    | Send the local bitmap state over BLE |

## Bluetooth packet format

The project transfers image state rather than complete bitmap arrays.

```text
(Letter,X,Y,Angle)
```

Example:

```text
(F,30,20,180)
```

The receiver validates the packet structure, stores the received letter, position, and orientation, then renders the received bitmap on the right side of the OLED.

## Software dependencies

Install these libraries through the Arduino IDE Library Manager:

* Adafruit GFX Library
* Adafruit SSD1306

The sketch also uses the Arduino core libraries:

* Wire
* SoftwareSerial

## Running the project

1. Connect the OLED, joystick, BLE module, and LED using the table above.
2. Open `src/image_manipulation_ble.ino` in the Arduino IDE.
3. Install the required Adafruit libraries.
4. Select **Arduino Uno** under **Tools → Board**.
5. Select the correct serial port.
6. Upload the sketch.
7. Connect an Android device to the AT-09 BLE module using Serial Bluetooth Terminal.
8. Use the joystick to move, rotate, select, transmit, and receive bitmap states.

## Demonstration evidence

### Bluetooth packet testing

![Bluetooth packet testing](assets/bluetooth-packet-test.jpg)

### OLED split-screen result

![OLED split-screen result](assets/oled-split-screen-result.jpg)

### Prototype side view

![Prototype side view](assets/prototype-side-view.jpg)

## Repository structure

```text
arduino-image-manipulation-ble/
├── src/
│   └── image_manipulation_ble.ino
├── assets/
│   ├── integrated-hardware-prototype.jpg
│   ├── prototype-side-view.jpg
│   ├── bluetooth-packet-test.jpg
│   └── oled-split-screen-result.jpg
└── README.md
```

## Limitations and future improvements

* Replace dynamic `String` packet handling with fixed-size character buffers for improved memory predictability on the Arduino Uno.
* Add packet checksums, field-range validation, and timeouts.
* Use a second embedded device for fully automated two-way testing.
* Add non-volatile storage for saved bitmap states.
* Replace the breadboard prototype with a compact PCB implementation.

## Scope

This repository contains the original embedded source code and selected hardware/test evidence from the completed prototype. It does not include the full assessed coursework report, assignment materials, or university submission documents.