#include <digitalWriteFast.h>
#include <Wire.h>

#define I2C_ADDRESS     8

#define PIN_ATTENTION   10

#define PIN_CROSS       2
#define PIN_CIRCLE      3
#define PIN_TRIANGLE    4
#define PIN_SQUARE      5
#define PIN_UP          6
#define PIN_DOWN        7
#define PIN_LEFT        8
#define PIN_RIGH        9

void _write () {
  byte tx[2] = { 0xFF, 0xFF };

  if (!digitalReadFast(PIN_CROSS))    bitClear(tx[1], 6);
  if (!digitalReadFast(PIN_CIRCLE))   bitClear(tx[1], 5);
  if (!digitalReadFast(PIN_TRIANGLE)) bitClear(tx[1], 4);
  if (!digitalReadFast(PIN_SQUARE))   bitClear(tx[1], 7);
  if (!digitalReadFast(PIN_UP))       bitClear(tx[0], 4);
  if (!digitalReadFast(PIN_DOWN))     bitClear(tx[0], 6);
  if (!digitalReadFast(PIN_LEFT))     bitClear(tx[0], 7);
  if (!digitalReadFast(PIN_RIGH))     bitClear(tx[0], 5);

  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(tx[0]);
  Wire.write(tx[1]);
  Wire.write(0x7F);
  Wire.write(0x7F);
  Wire.write(0x7F);
  Wire.write(0x7F);
  Wire.endTransmission();
}

void setup() {
  pinMode(PIN_ATTENTION, INPUT);

  pinMode(PIN_CROSS, INPUT_PULLUP);
  pinMode(PIN_CIRCLE, INPUT_PULLUP);
  pinMode(PIN_TRIANGLE, INPUT_PULLUP);
  pinMode(PIN_SQUARE, INPUT_PULLUP);
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGH, INPUT_PULLUP);

  Wire.begin();
}

void loop() {
  if (digitalReadFast(PIN_ATTENTION)) {
    _write();
    delay(100);
  }
}
