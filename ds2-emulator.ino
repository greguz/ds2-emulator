#include <SPI.h>

#define PIN_CLK 2   // blue
#define PIN_SS 3    // yellow
#define PIN_MOSI 4  // orange
#define PIN_MISO 5  // brown
#define PIN_ACK 6   // green

// Config mode (has precedence)
bool CONFIG = false;

// Analog mode (pressure levels supported only on analog mode)
bool ANALOG = false;

// How many bytes have payload on analog mode
uint8_t ANALOG_LENGTH = 6;

//
byte ANALOG_MAPPING[3] = { 0x3F, 0x00, 0x00 };

// When true you cannot change the current mode by pressing the controller button
bool LOCKED = false;

// Detect broken communication (invalid header, invalid length)
bool BROKEN = false;

// Previous (negated) status of the SS pin
bool ATTENTION = false;

// Expected communication length in bytes
uint8_t LENGTH = 0;

// Current communication byte index
uint8_t INDEX = 0;

// Current command from PlayStation (header, second byte)
byte CMD = 0x5A;

// Small motor
bool SMOTOR_STATUS = false;
uint8_t SMOTOR_INDEX = 69;

// Large motor
byte LMOTOR_STATUS = 0x00;
uint8_t LMOTOR_INDEX = 69;

// Payload used during the config mode (calculated)
byte DATA_CFG[6] = {
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

// Payload used during the other modes (represents buttons status)
byte DATA_BTN[18] = {
  // Digital buttons (0 is pressed)
  0xFF, // Select, L3, R3, Start, Up, Right, Down, Left
  0xFF, // L2, R2, L1, R1, Triangle, Circle, Cross, Square

  // Analog sticks
  0x7F, // Right X-axis
  0x7F, // Right Y-axis
  0x7F, // Left X-axis
  0x7F, // Left Y-axis

  // Pressure levels (0 is not pressed)
  0x00, // Right
  0x00, // Left
  0x00, // Up
  0x00, // Down
  0x00, // Triangle
  0x00, // Circle
  0x00, // Cross
  0x00, // Square
  0x00, // L1
  0x00, // R1
  0x00, // L2
  0x00  // R2
};

/**
 * Sync pressure levels by reading from digital status
 */
void syncPressureLelevs () {
  DATA_BTN[ 7] = bitRead(DATA_BTN[0], 0) ? 0x00 : 0xFF; // Left
  DATA_BTN[ 9] = bitRead(DATA_BTN[0], 1) ? 0x00 : 0xFF; // Down
  DATA_BTN[ 6] = bitRead(DATA_BTN[0], 2) ? 0x00 : 0xFF; // Right
  DATA_BTN[ 8] = bitRead(DATA_BTN[0], 3) ? 0x00 : 0xFF; // Up
  DATA_BTN[13] = bitRead(DATA_BTN[1], 0) ? 0x00 : 0xFF; // Square
  DATA_BTN[12] = bitRead(DATA_BTN[1], 1) ? 0x00 : 0xFF; // Cross
  DATA_BTN[11] = bitRead(DATA_BTN[1], 2) ? 0x00 : 0xFF; // Circle
  DATA_BTN[10] = bitRead(DATA_BTN[1], 3) ? 0x00 : 0xFF; // Triangle
  DATA_BTN[15] = bitRead(DATA_BTN[1], 4) ? 0x00 : 0xFF; // R1
  DATA_BTN[14] = bitRead(DATA_BTN[1], 5) ? 0x00 : 0xFF; // L1
  DATA_BTN[17] = bitRead(DATA_BTN[1], 6) ? 0x00 : 0xFF; // R2
  DATA_BTN[16] = bitRead(DATA_BTN[1], 7) ? 0x00 : 0xFF; // L2
}

/**
 * Count how many bits are set inside a byte
 */
uint8_t countBits (byte value) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < 8; i++) {
    count += (value >> i) & 0x01;
  }
  return count;
}

/**
 * Set ANALOG_LENGTH by calculating how many bits are set inside the mapping bytes (and check the max length)
 */
void updateAnalogLength () {
  ANALOG_LENGTH = countBits(ANALOG_MAPPING[0]) + countBits(ANALOG_MAPPING[1]) + countBits(ANALOG_MAPPING[2]);
  if (ANALOG_LENGTH > 18) {
    ANALOG_LENGTH = 18;
  }
}

/**
 * Handle SPI slave duplex communication (send a byte and receive a byte)
 */
byte byteRoutine (byte tx) {
  byte rx = 0x00;

  for (uint8_t i = 0; i < 8; i++) {
    // Wait low CLK (start cycle)
    while (bitRead(PIND, 2));

    // Write MISO
    if (bitRead(tx, i)) {
      bitSet(PORTD, 5);
    } else {
      bitClear(PORTD, 5);
    }

    // Wait high CLK (half cycle)
    while (!bitRead(PIND, 2));

    // Read MOSI
    if (bitRead(PIND, 4)) {
      bitSet(rx, i);
    }
  }

  // Restore MISO
  bitSet(PORTD, 5);

  return rx;
}

/**
 * Read current mode code
 */
byte getControllerMode () {
  if (CONFIG) {
    return 0xF3;
  } else if (!ANALOG) {
    return 0x41;
  } else {
    return 0x70 | (ANALOG_LENGTH  / 2);
  }
}

/**
 * Handle and validate header data
 */
void handleHeader () {
  switch (INDEX) {
    case 0:
      BROKEN = byteRoutine(0xFF) != 0x01;
      break;
    case 1:
      CMD = byteRoutine(getControllerMode());
      break;
    case 2:
      BROKEN = byteRoutine(0x5A) != 0x00;
      break;
  }
}

/**
 * Init response payload for the config mode
 */
void initConfigResponse () {
  DATA_CFG[0] = 0x00;
  DATA_CFG[1] = 0x00;
  DATA_CFG[2] = 0x00;
  DATA_CFG[3] = 0x00;
  DATA_CFG[4] = 0x00;
  DATA_CFG[5] = 0x00;

  switch (CMD) {
    case 0x41:
      if (ANALOG) {
        DATA_CFG[0] = ANALOG_MAPPING[0];
        DATA_CFG[1] = ANALOG_MAPPING[1];
        DATA_CFG[2] = ANALOG_MAPPING[2];
      }
      DATA_CFG[5] = 0x5A;
      break;

    case 0x42:
      CONFIG = false;
      break;

    case 0x44:
      ANALOG_LENGTH = 6;
      ANALOG_MAPPING[0] = 0x3F;
      ANALOG_MAPPING[1] = 0x00;
      ANALOG_MAPPING[2] = 0x00;
      break;

    case 0x45:
      DATA_CFG[0] = 0x03; // DualShock 2 controller constant
      DATA_CFG[1] = 0x02;
      DATA_CFG[2] = ANALOG ? 0x01 : 0x00;
      DATA_CFG[3] = 0x02;
      DATA_CFG[4] = 0x01;
      DATA_CFG[5] = 0x00;
      break;

    case 0x47:
      DATA_CFG[2] = 0x02;
      break;

    case 0x4D:
      // Turn off and disable the small motor
      SMOTOR_INDEX = 69;
      SMOTOR_STATUS = false;
      // Turn off and disable the large motor
      LMOTOR_INDEX = 69;
      LMOTOR_STATUS = 0x00;
      // Setup response
      DATA_CFG[0] = 0x00;
      DATA_CFG[1] = 0x01;
      DATA_CFG[2] = 0xFF;
      DATA_CFG[3] = 0xFF;
      DATA_CFG[4] = 0xFF;
      DATA_CFG[5] = 0xFF;
      break;

    case 0x4F:
      DATA_CFG[5] = 0x5A;
      break;
  }
}

/**
 * Process response from PlayStation
 */
void processConfigResponse (byte rx) {
  switch (CMD) {
    case 0x43:
      if (INDEX == 3 && rx == 0x00) {
        CONFIG = false;
      }
      break;

    case 0x44:
      if (INDEX == 3 && rx == 0x01) {
        ANALOG = true;
      } else if (INDEX == 3 && rx == 0x00) {
        ANALOG = false;
      } else if (INDEX == 4 && rx == 0x03) {
        LOCKED = true;
      }
      break;

    case 0x46:
      if (INDEX == 3 && rx == 0x00) {
        DATA_CFG[3] = 0x02;
        DATA_CFG[5] = 0x0A;
      } else if (INDEX == 3 && rx == 0x01) {
        DATA_CFG[5] = 0x14;
      }
      break;

    case 0x4C:
      if (INDEX == 3 && rx == 0x00) {
        DATA_CFG[3] = 0x04;
      } else if (INDEX == 3 && rx == 0x01) {
        DATA_CFG[3] = 0x06;
      }
      break;

    case 0x4D:
      if (rx == 0x00) {
        SMOTOR_INDEX = INDEX;
      } else if (rx == 0x01) {
        LMOTOR_INDEX = INDEX;
      }
      break;

    case 0x4F:
      if (INDEX >= 3 && INDEX <= 5) {
        ANALOG_MAPPING[INDEX - 3] = rx;
        ANALOG_LENGTH = countBits(ANALOG_MAPPING[0]) + countBits(ANALOG_MAPPING[1]) + countBits(ANALOG_MAPPING[2]);
      }
      break;
  }
}

/**
 * Handle communication during the config mode
 */
void configRoutine () {
  if (INDEX == 3) {
    initConfigResponse();
  }
  processConfigResponse(byteRoutine(DATA_CFG[INDEX - 3]));
}

/**
 * Handle communication during the other modes
 */
void defaultRoutine () {
  byte rx = byteRoutine(DATA_BTN[INDEX - 3]);
  if (CMD == 0x42 && SMOTOR_INDEX == INDEX) {
    SMOTOR_STATUS = rx == 0xFF;
  } else if (CMD == 0x42 && LMOTOR_INDEX == INDEX) {
    LMOTOR_STATUS = rx;
  } else if (CMD == 0x43 && INDEX == 3 && rx == 0x01) {
    CONFIG = true;
  }
}

/**
 * Handle full communication
 */
void handleCommunication () {
  // Process current byte
  if (INDEX >= LENGTH) {
    BROKEN = true;
  } else if (INDEX <= 2) {
    handleHeader();
  } else if (CONFIG) {
    configRoutine();
  } else {
    defaultRoutine();
  }

  // Send ACK signal
  if (!BROKEN) {
    delayMicroseconds(12);
    bitClear(PORTD, 6);
    delayMicroseconds(4);
    bitSet(PORTD, 6);
  }

  // Ready for the next incoming byte
  INDEX++;
}

/**
 * Reset current communication status (prepare for the next one)
 */
void resetStatus () {
  // Clean broken connection flag
  BROKEN = false;

  // Reset byte index
  INDEX = 0;

  // Calculate message length
  if (CONFIG) {
    LENGTH = 9;
  } else if (!ANALOG) {
    LENGTH = 5;
  } else {
    LENGTH = ANALOG_LENGTH + 3;
  }
}

/**
 * setup
 */
void setup () {
  pinMode(PIN_ACK, OUTPUT);
  pinMode(PIN_SS, INPUT);
  pinMode(PIN_MOSI, INPUT);
  pinMode(PIN_MISO, OUTPUT);
  pinMode(PIN_CLK, INPUT);

  digitalWrite(PIN_MISO, HIGH);
  digitalWrite(PIN_ACK, HIGH);

  // TODO: Handle I2C and retrieve controller status
  // syncPressureLelevs();

  resetStatus();
}

/**
 * loop
 */
void loop () {
  // Handle current SS status
  bool attention = !bitRead(PIND, 3);
  if (attention != ATTENTION) {
    if (attention) {
      // Disable I2C interrupt
      noInterrupts();
    } else {
      // Reset status for the next communication
      resetStatus();
      // Enable I2C interrupt
      interrupts();
    }
  }
  ATTENTION = attention;

  // Process data
  if (ATTENTION && !BROKEN) {
    handleCommunication();
  }
}
