// Protocol details
// http://store.curiousinventor.com/guides/PS2/
// http://www.lynxmotion.com/images/files/ps2cmd01.txt

// Pins mapping
// - D2 < CLK (blue)
// - D3 < ATT (yellow)
// - D4 < MOSI (orange)
// - D5 > MISO (brown)
// - D6 > ACK (green)

// Toggle between the digital and analog mode
bool ANALOG = false;

// Enable or disable the config/escape/native/wtf mode
bool CONFIG = false;

// Enable or disable the user to switch mode with "Analog" button
bool LOCKED = false;

// Enable or disable the small motor
bool SMALL_MOTOR = false;

// Controls the vibration of the large motor
unsigned char LARGE_MOTOR = 0x00;

// Small Motor Index
// The byte index into CMD buffer that contains the motor data
unsigned int SMI = 22;

// Large Motor Index
// The byte index into CMD buffer that contains the motor data
unsigned int LMI = 22;

// Current mode code
// - upper nibble
// - - current mode
// - lower nibble
// - - payloadh length
unsigned char MODE = 0x41;

// TODO: not sure because of command 0x41 and 0x4F
unsigned int PAYLOAD_LENGTH = 2;

// MOSI (Master Out Slave In) buffer
// This buffer will be populated with the incoming data from the PlayStation 2
unsigned char CMD[21] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// MISO (Master In Slave Out) buffer
// This buffer contains the controller status
// - 3 bytes  > header (ignored)
// - 2 bytes  > digital buttons
// - 4 bytes  > sticks position
// - 12 bytes > pressure data
unsigned char DAT[21] = { 0xFF, MODE, 0x5A, 0xFF, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/**
 * TODO: docs
 */

int countSetBits(int n) {
  if (n == 0) {
    return 0;
  } else {
    return 1 + countSetBits(n & (n - 1));
  }
}

/**
 * Send a byte to MISO and receive a byte from MOSI
 */

unsigned char byteRoutine(unsigned char tx) {
  unsigned char rx = 0x00;

  // transmit tx data and save incoming into rx
  for (unsigned int i = 0; i < 8; i++) {
    // wait CLK to be low (cycle start)
    while (bitRead(PIND, 2));

    // send the current bit to MISO pin
    if (bitRead(tx, i)) {
      bitSet(PORTD, 5);
    } else {
      bitClear(PORTD, 5);
    }

    // wait CLK to be high (half cycle)
    while (!bitRead(PIND, 2));

    // read MOSI pin and save
    if (bitRead(PIND, 4)) {
      bitSet(rx, i);
    }
  }

  // restore MISO pin status
  bitSet(PORTD, 5);

  // send ACK signal
  delayMicroseconds(10); // TODO: not sure
  bitClear(PORTD, 6);
  delayMicroseconds(3); // TODO: not sure
  bitSet(PORTD, 6);

  // Return the received byte
  return rx;
}

/**
 * Main routine, sends controller state data
 */

void normalRoutine() {
  unsigned int length = 3 + PAYLOAD_LENGTH;

  // Send the header
  CMD[0] = byteRoutine(0xFF);
  CMD[1] = byteRoutine(MODE);
  CMD[2] = byteRoutine(0x5A);

  // Send controller state data
  for (unsigned int i = 3; i < length; i++) {
    CMD[i] = byteRoutine(DAT[i]);
  }

  // Handle motors data
  if (SMI < length) {
    SMALL_MOTOR = CMD[SMI] == 0xFF;
  }
  if (LMI < length) {
    LARGE_MOTOR = CMD[LMI];
  }

  // Handle request for config mode
  if (CMD[1] == 0x43 && CMD[3] == 0x01) {
    CONFIG = true;
  } else if (CMD[1] != 0x42) {
    // Not main polling cammand, this is probably wrong
  }
}

/**
 * Routine when config mode is enabled (handle commands)
 */

void configRoutine() {
  // Send the header
  CMD[0] = byteRoutine(0xFF);
  CMD[1] = byteRoutine(0xF3);
  CMD[2] = byteRoutine(0x5A);

  // Send the other 6 bytes (in config mode the payload is fixed)
  switch(CMD[1]) {

    // TODO: case 0x40: break;

    // Find out what buttons are included in poll responses
    case 0x41:
      if (ANALOG) {
        // TODO: fix this
        // CMD[3] = byteRoutine(MAP[0]);
        // CMD[4] = byteRoutine(MAP[1]);
        // CMD[5] = byteRoutine(MAP[2]);
      } else {
        CMD[3] = byteRoutine(0x00);
        CMD[4] = byteRoutine(0x00);
        CMD[5] = byteRoutine(0x00);
      }
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x5A);
      break;

    // Exit from config mode
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

    // Switch modes between digital and analog, also lock the current mode
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
        PAYLOAD_LENGTH = 2;
      } else if (CMD[3] == 0x01) {
        ANALOG = true;
        MODE = 0x73;
        PAYLOAD_LENGTH = 6;
      }
      LOCKED = CMD[4] == 0x03;
      break;

    // Get more status info
    case 0x45:
      CMD[3] = byteRoutine(0x03); // Hard coded DualShock 2 controller
      CMD[4] = byteRoutine(0x02);
      CMD[5] = byteRoutine(ANALOG ? 0x01 : 0x00);
      CMD[6] = byteRoutine(0x02);
      CMD[7] = byteRoutine(0x01);
      CMD[8] = byteRoutine(0x00);
      break;

    // Send unknown const
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

    // Send unknown const
    case 0x47:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x02);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      break;

    // Send unknown const
    case 0x4C:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(CMD[3] == 0x00 ? 0x04 : 0x06);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x00);
      break;

    // Map bytes in the 0x42 command to actuate the vibration motors
    case 0x4D:
      // TODO: send the current configuration
      // SMI == i ? 0x00 : 0xFF
      // LMI == i ? 0x01 : 0xFF
      CMD[3] = byteRoutine(0xFF);
      CMD[4] = byteRoutine(0xFF);
      CMD[5] = byteRoutine(0xFF);
      CMD[6] = byteRoutine(0xFF);
      CMD[7] = byteRoutine(0xFF);
      CMD[8] = byteRoutine(0xFF);
      // Reset the motors mapping
      SMI = 22;
      LMI = 22;
      // Save the received mapping
      for (unsigned int i = 3; i < 9; i++) {
        if (CMD[i] == 0x00) {
          SMI = i;
        } else if (CMD[i] == 0x01) {
          LMI = i;
        }
      }
      break;

    // Add or remove analog response bytes from the main polling command
    case 0x4F:
      CMD[3] = byteRoutine(0x00);
      CMD[4] = byteRoutine(0x00);
      CMD[5] = byteRoutine(0x00);
      CMD[6] = byteRoutine(0x00);
      CMD[7] = byteRoutine(0x00);
      CMD[8] = byteRoutine(0x5A);
      if (ANALOG) {
        // Update the payload length
        PAYLOAD_LENGTH = countSetBits(CMD[3]) + countSetBits(CMD[4]) + countSetBits(CMD[5]);
        // Update mode (only the lower nibble)
        MODE &= B11110000;
        MODE |= (PAYLOAD_LENGTH  / 2);
      }
      break;

    // This is bad
    default:
      CMD[3] = byteRoutine(0x5A);
      CMD[4] = byteRoutine(0x5A);
      CMD[5] = byteRoutine(0x5A);
      CMD[6] = byteRoutine(0x5A);
      CMD[7] = byteRoutine(0x5A);
      CMD[8] = byteRoutine(0x5A);
      break;

  }
}

/**
 * Setup function, one shot
 */

void setup() {
  // CLK, ATT, MOSI as input
  // MISO, ACK as output
  DDRD = DDRD | B01100000;

  // MISO normally high
  // ACK normally high
  PORTD = PORTD | B01100000;

  // Disable all interrupts
  noInterrupts();
}

/**
 * Loop function, the main program
 */

void loop() {
  // Watch the ATT pin
  if (!bitRead(PIND, 3)) {
    // Enter into the correct routine
    if (CONFIG) {
      configRoutine();
    } else {
      normalRoutine();
    }
  }
}
