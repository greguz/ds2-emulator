// Protocol details
// http://store.curiousinventor.com/guides/PS2/
// http://www.lynxmotion.com/images/files/ps2cmd01.txt

// Pins mapping
// - D2 < CLK (blue)
// - D3 < ATT (yellow)
// - D4 < MOSI (orange)
// - D5 > MISO (brown)
// - D6 > ACK (green)

// Bit manipulation utils
#define SET(x,y) (x|=(1<<y))  // Set bit y in x
#define CLR(x,y) (x&=(~(1<<y))) // Clear bit y in x
#define CHK(x,y) (x & (1<<y))   // Check if bit y in x is set
#define TGL(x,y) (x^=(1<<y))  // Toggle bit y in x

// MOSI buffer (PS2 > controller)
unsigned char CMD[21] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// MISO buffer (controller > PS2)
unsigned char DAT[21] = { 0xFF, 0x41, 0x5A, 0xFF, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Toggle between the digital and analog mode
bool ANALOG = false;

// Enable or disable the config/escape/native/wtf mode
bool CONFIG = false;

// Enable or disable the user to switch mode with "Analog" button
bool LOCKED = false;

// Current mode code
// - upper nibble
// - - current mode
// - lower nibble
// - - payloadh length
unsigned char MODE = 0x41;

// TODO: docs
unsigned char MAP[3] = { B00000011, B00000000, B00000000 };

int countSetBits(int n) { 
  if (n == 0) {
    return 0;
  } else {
    return 1 + countSetBits(n & (n - 1));
  }
} 

unsigned char byteRoutine(unsigned char tx) {
  unsigned char rx = 0x00;

  // transmit tx data and save incoming into rx
  for (int i = 0; i < 8; i++) {
    // when CLK is low, the communication start
    while (CHK(PIND, 2));

    // send the current bit to MISO pin
    if (CHK(tx, i)) {
      SET(PORTD, 5);
    } else {
      CLR(PORTD, 5);
    }

    // wait unti CLK is high
    while (!CHK(PIND, 2));

    // read and save MOSI pin
    if (CHK(PIND, 4)) {
      SET(rx, i);
    }
  }

  // restore MISO pin status
  SET(PORTD, 5);

  // send ACK signal
  delayMicroseconds(10);
  CLR(PORTD, 6);
  delayMicroseconds(3); // not sure
  SET(PORTD, 6);

  // all done
  return rx;
}

void normalRoutine() {
  // send the header
  CMD[0] = byteRoutine(0xFF);
  CMD[1] = byteRoutine(MODE);
  CMD[2] = byteRoutine(0x5A);

  // send controller state data
  int j = 3;
  for (int i = 0; i < 8; i++) {
    if (CHK(MAP[0], i)) {
      CMD[j++] = byteRoutine(DAT[i + 3]);
    }
  }
  for (int i = 0; i < 8; i++) {
    if (CHK(MAP[1], i)) {
      CMD[j++] = byteRoutine(DAT[i + 11]);
    }
  }
  for (int i = 0; i < 2; i++) {
    if (CHK(MAP[2], i)) {
      CMD[j++] = byteRoutine(DAT[i + 19]);
    }
  }

  // handle request for config mode
  if (CMD[1] == 0x43 && CMD[3] == 0x01) {
    CONFIG = true;
  } else if (CMD[1] != 0x42) {
    // TODO: this is wrong, probably...
  }
}

void configModeRoutine() {
  // send the header
  CMD[0] = byteRoutine(0xFF);
  CMD[1] = byteRoutine(0xF3);
  CMD[2] = byteRoutine(0x5A);

  // send the other 6 bytes
  switch(CMD[1]) {

    // Find out what buttons are included in poll responses
    case 0x41:
      if (ANALOG) {
        CMD[3] = byteRoutine(MAP[0]);
        CMD[4] = byteRoutine(MAP[1]);
        CMD[5] = byteRoutine(MAP[2]);
      } else {
        CMD[3] = byteRoutine(0x00);
        CMD[4] = byteRoutine(0x00);
        CMD[5] = byteRoutine(0x00);
      }
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x5A);
      break;

    // exit from config mode
    case 0x43:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      if (CMD[3] == 0x00) {
        CONFIG = false;
      }
      break;

    // switch modes between digital and analog and lock
    case 0x44:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      if (CMD[3] == 0x00) {
        ANALOG = false;
        MODE = 0x41;
        MAP[0] = B00000011;
        MAP[1] = B00000000;
        MAP[2] = B00000000;
      } else if (CMD[3] == 0x01) {
        ANALOG = true;
        MODE = 0x73;
        MAP[0] = B00111111;
        MAP[1] = B00000000;
        MAP[2] = B00000000;
      }
      LOCKED = CMD[4] == 0x03;
      break;

    // get more status info
    case 0x45:
      CMD[3] = byteRoutine(0x03); // hard coded DualShock 2 controller
      CMD[4] = byteRoutine(0x02);
      CMD[5] = byteRoutine(ANALOG ? 0x01 : 0x00);
      CMD[6] = byteRoutine(0x02);
      CMD[7] = byteRoutine(0x01);
      CMD[8] = byteRoutine(0x00);
      break;

    // unknown const
    case 0x46:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      if (CMD[3] == 0x00) {
        CMD[6] = byteRoutine(0x02);
        CMD[7] = byteRoutine(0x00);
        CMD[8] = byteRoutine(0x0A);
      } else {
        CMD[6] = byteRoutine(0x00);
        CMD[7] = byteRoutine(0x00);
        CMD[8] = byteRoutine(0x14);
      }
      break;

    // unknown const
    case 0x47:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x02);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      break;

    // unknown const
    case 0x4C:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(CMD[3] == 0x00 ? 0x04 : 0x06);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      break;

    // TODO: map bytes in the 0x42 command to actuate the vibration motors
    // 0x00 maps the corresponding byte in 0x42 to control the small motor. A 0xFF in the 0x42 command will turn it on, all other values turn it off.
    // 0x01 maps the corresponding byte in 0x42 to control the large motor. The power delivered to the large motor is then set from 0x00 to 0xFF in 0x42.
    // 0xFF disables, and is the default value when the controller is first connected. The data bytes just report the current mapping.
    case 0x4D:
      CMD[3] = byteRoutine(0xFF);
      CMD[4] = byteRoutine(0xFF);
      CMD[5] = byteRoutine(0xFF);
      CMD[6] = byteRoutine(0xFF);
      CMD[7] = byteRoutine(0xFF);
      CMD[8] = byteRoutine(0xFF);
      break;

    // add or remove analog response bytes from the main polling command 
    case 0x4F:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x5A);
      if (ANALOG) {
        MAP[0] = CMD[3];
        MAP[1] = CMD[4];
        MAP[2] = CMD[5];
        // update the payload length
        MODE &= B11110000;
        MODE |= countSetBits(MAP[0]) + countSetBits(MAP[1]) + countSetBits(MAP[2]);
      }
      break;

  }
}

void setup() {
  // enable input for CLK, ATT, MOSI
  // enable output for MISO, ACK
  DDRD = DDRD | B00111000;

  // MISO is normally high
  // ACK is normally high
  PORTD = PORTD | B00000110;
}

void loop() {
  // when the ATT pin is low the communication start
  if (!CHK(PIND, 3)) {
    if (CONFIG) {
      configModeRoutine();
    } else {
      normalRoutine();
    }
  }
}

