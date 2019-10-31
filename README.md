# DualShock 2 emulator for Arduino Uno

Emulate a DualShock 2 controller with Arduino Uno and control It throught I2C bus.

## Hardware

To be able to communicate with the PlayStation 2, the circuit connected
to **pin 13** needs to be removed somehow.
The circuit consists in a led and a resistor:
you can desolder one component, both, or just cut the connection between them.

## Software

Install [this](https://github.com/NicksonYap/digitalWriteFast) library.

## Wiring

| Function | Arduino | Wire   |
| -------- | ------- | ------ |
| GND      | GND     | Black  |
| ACK      | 9       | Green  |
| SS       | 10      | Yellow |
| MOSI     | 11      | Orange |
| MISO     | 12      | Brown  |
| SCK      | 13      | Blue   |
| SDA      | A4      | -      |
| SCL      | A5      | -      |

## I2C

Default address: **8**

| Byte | Target             | Details               |
| ---- | ------------------ | --------------------- |
| 0    | Buttons            | see below             |
| 1    | Buttons            | see below             |
| 2    | Right stick X-axis | 0x00=Left, 0xFF=Right |
| 3    | Right stick Y-axis | 0x00=Up, 0xFF=Down    |
| 4    | Left stick X-axis  | 0x00=Left, 0xFF=Right |
| 5    | Left stick Y-axis  | 0x00=Up, 0xFF=Down    |

### Byte 0

**LSB, active low.**

- Select
- L3
- R3
- Start
- Up
- Right
- Down
- Left

### Byte 1

**LSB, active low.**

- L2
- R2
- L1
- R1
- Triangle
- Circle
- Cross
- Square

## Resources

- http://avrbeginners.net/architecture/spi/spi.html
- https://gist.github.com/scanlime/5042071
- http://store.curiousinventor.com/guides/PS2/
- http://www.lynxmotion.com/images/files/ps2cmd01.txt
- http://blog.nearfuturelaboratory.com/category/psx/
- http://procrastineering.blogspot.com/2010/12/simulated-ps2-controller-for.html
