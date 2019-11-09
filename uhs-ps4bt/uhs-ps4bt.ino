// WARNING: SET OPTIMIZE TO "FAST"
// https://github.com/felis/USB_Host_Shield_2.0/issues/402

#define I2C_ADDRESS 8

#include <PS4BT.h>
#include <usbhub.h>
#include <Wire.h>

#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;

// Some dongles have a hub inside
// USBHub Hub1(&Usb);

// You have to create the Bluetooth Dongle instance like so
BTD Btd(&Usb);

// This will start an inquiry and then pair with the PS4 controller - you only have to do this once
// You will need to hold down the PS and Share button at the same time, the PS4 controller will then start to blink rapidly indicating that it is in pairing mode
PS4BT PS4(&Btd, PAIR);

// After that you can simply create the instance like so and then press the PS button on the device
// PS4BT PS4(&Btd);

// TODO: Rumble
// PS4.setRumbleOn(RumbleLow);
// PS4.setRumbleOn(RumbleHigh);

// TODO: Digital mode
// PS4.setLed(Blue);

// TODO: Analog mode
// PS4.setLed(Red);

unsigned long lastSync = 0;

byte data[6] = {
  0xFF,
  0xFF,
  0x7F,
  0x7F,
  0x7F,
  0x7F
};

void setup () {
  Wire.begin();

  if (Usb.Init() == -1) {
    while (1); // Halt
  }
}

void loop () {
  Usb.Task();

  if (PS4.connected()) {
    data[0] = 0xFF;
    if (PS4.getButtonPress(SHARE)) {
      bitClear(data[0], 0);
    }
    if (PS4.getButtonPress(L3)) {
      bitClear(data[0], 1);
    }
    if (PS4.getButtonPress(R3)) {
      bitClear(data[0], 2);
    }
    if (PS4.getButtonPress(OPTIONS)) {
      bitClear(data[0], 3);
    }
    if (PS4.getButtonPress(UP)) {
      bitClear(data[0], 4);
    }
    if (PS4.getButtonPress(RIGHT)) {
      bitClear(data[0], 5);
    }
    if (PS4.getButtonPress(DOWN)) {
      bitClear(data[0], 6);
    }
    if (PS4.getButtonPress(LEFT)) {
      bitClear(data[0], 7);
    }

    data[1] = 0xFF;
    if (PS4.getAnalogButton(L2)) {
      bitClear(data[1], 0);
    }
    if (PS4.getAnalogButton(R2)) {
      bitClear(data[1], 1);
    }
    if (PS4.getButtonPress(L1)) {
      bitClear(data[1], 2);
    }
    if (PS4.getButtonPress(R1)) {
      bitClear(data[1], 3);
    }
    if (PS4.getButtonPress(TRIANGLE)) {
      bitClear(data[1], 4);
    }
    if (PS4.getButtonPress(CIRCLE)) {
      bitClear(data[1], 5);
    }
    if (PS4.getButtonPress(CROSS)) {
      bitClear(data[1], 6);
    }
    if (PS4.getButtonPress(SQUARE)) {
      bitClear(data[1], 7);
    }

    data[2] = PS4.getAnalogHat(RightHatX);
    data[3] = PS4.getAnalogHat(RightHatY);

    data[4] = PS4.getAnalogHat(LeftHatX);
    data[5] = PS4.getAnalogHat(LeftHatY);

    if (PS4.getButtonClick(PS)) {
      PS4.disconnect();
    }
  } else {
    data[0] = 0xFF;
    data[1] = 0xFF;
    data[2] = 0x7F;
    data[3] = 0x7F;
    data[4] = 0x7F;
    data[5] = 0x7F;
  }

  unsigned long now = millis();
  if (now - lastSync >= 20) {
    lastSync = now;
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(data[0]);
    Wire.write(data[1]);
    Wire.write(data[2]);
    Wire.write(data[3]);
    Wire.write(data[4]);
    Wire.write(data[5]);
    Wire.endTransmission();
  }
}
