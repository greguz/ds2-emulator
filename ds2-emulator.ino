#include <digitalWriteFast.h> // https://github.com/NicksonYap/digitalWriteFast
#include <SPI.h>
#include <Wire.h>

/**
 * Arduino Uno pinout (and remove the resistor on pin 13)
 */
#define ACK    9  // green
//      SS    10  // yellow
//      MOSI  11  // orange
//      MISO  12  // brown
//      SCK   13  // blue

/**
 * I2C slave address
 */
#define I2C_ADDRESS 8

// Config mode (has precedence)
bool CONFIG = false;

//
volatile bool NEXT_CONFIG = false;

// Analog mode (pressure levels supported only on analog mode)
volatile bool ANALOG = false;

// How many bytes have payload on analog mode
volatile uint8_t ANALOG_LENGTH = 6;

//
byte ANALOG_MAPPING[3] = { 0x3F, 0x00, 0x00 };

// When true you cannot change the current mode by pressing the controller button
volatile bool LOCKED = false;

// Previous (negated) status of the SS pin
bool ATTENTION = false;

// Expected communication length in bytes
uint8_t LENGTH = 0;

// Current communication byte index
uint8_t INDEX = 0;

// Current command from PlayStation (header, second byte)
volatile byte CMD = 0x5A;

// Small motor
bool SMOTOR_STATUS = false;
uint8_t SMOTOR_INDEX = 69;

// Large motor
byte LMOTOR_STATUS = 0x00;
uint8_t LMOTOR_INDEX = 69;

//
bool SEND_ACK = false;

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
  // Left, Down, Right, Up, Start, R3, L3, Select (0 is pressed)
  // 0xFE > Select is pressed
  0xBF,

  // Square, Cross, Circle, Triangle, R1, L1 ,R2, L2 (0 is pressed)
  // 0xFE > L2 is pressed
  0xFF,

  // Analog sticks
  0x7F, // Right X-axis,  0x00 = Left,  0xFF = Right
  0x7F, // Right Y-axis,  0x00 = Up,    0xFF = Down
  0x7F, // Left X-axis,   0x00 = Left,  0xFF = Right
  0x7F, // Left Y-axis,   0x00 = Up,    0xFF = Down

  // Pressure levels (0xFF is fully pressed)
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
void syncPressureLevels () {
  DATA_BTN[ 7] = bitRead(DATA_BTN[0], 7) ? 0x00 : 0xFF; // Left
  DATA_BTN[ 9] = bitRead(DATA_BTN[0], 6) ? 0x00 : 0xFF; // Down
  DATA_BTN[ 6] = bitRead(DATA_BTN[0], 5) ? 0x00 : 0xFF; // Right
  DATA_BTN[ 8] = bitRead(DATA_BTN[0], 4) ? 0x00 : 0xFF; // Up
  DATA_BTN[13] = bitRead(DATA_BTN[1], 7) ? 0x00 : 0xFF; // Square
  DATA_BTN[12] = bitRead(DATA_BTN[1], 6) ? 0x00 : 0xFF; // Cross
  DATA_BTN[11] = bitRead(DATA_BTN[1], 5) ? 0x00 : 0xFF; // Circle
  DATA_BTN[10] = bitRead(DATA_BTN[1], 4) ? 0x00 : 0xFF; // Triangle
  DATA_BTN[15] = bitRead(DATA_BTN[1], 3) ? 0x00 : 0xFF; // R1
  DATA_BTN[14] = bitRead(DATA_BTN[1], 2) ? 0x00 : 0xFF; // L1
  DATA_BTN[17] = bitRead(DATA_BTN[1], 1) ? 0x00 : 0xFF; // R2
  DATA_BTN[16] = bitRead(DATA_BTN[1], 0) ? 0x00 : 0xFF; // L2
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
      NEXT_CONFIG = false;
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
        NEXT_CONFIG = false;
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
 * Reset current communication status (prepare for the next one)
 */
void resetStatus () {
  // Reset byte index
  INDEX = 0;

  // Reset command
  CMD = 0x5A;

  // Set required config mode
  CONFIG = NEXT_CONFIG;

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
 * TODO
 */
byte getResponseByte () {
  if (INDEX >= LENGTH) {
    return 0xFF;
  } else if (INDEX == 0) {
    return getControllerMode();
  } else if (INDEX == 1) {
    return 0x5A;
  } else if (CONFIG) {
    return DATA_CFG[INDEX - 2];
  } else {
    return DATA_BTN[INDEX - 2];
  }
}

/**
 * SPI interrupt routine
 */
ISR (SPI_STC_vect) {
  // Process received byte
  byte rx = SPDR;
  if (INDEX == 1) {
    CMD = rx;
    if (CONFIG) {
      initConfigResponse();
    }
  }
  if (CONFIG) {
    processConfigResponse(rx);
  } else if (CMD == 0x42 && SMOTOR_INDEX == INDEX) {
    SMOTOR_STATUS = rx == 0xFF;
  } else if (CMD == 0x42 && LMOTOR_INDEX == INDEX) {
    LMOTOR_STATUS = rx;
  } else if (CMD == 0x43 && INDEX == 3 && rx == 0x01) {
    NEXT_CONFIG = true;
  }

  // Set next response byte
  SPDR = getResponseByte();

  // Ready for the next incoming byte
  INDEX++;

  // Schedule ACK signal
  SEND_ACK = true;
}

/**
 * I2C incoming data interrupt
 */
void receiveEvent (int count) {
  byte data;
  for (uint8_t i = 0; i < count; i++) {
    data = Wire.read();
    if (i < 6) {
      DATA_BTN[i] = data;
    }
  }
  syncPressureLevels();
}

/**
 * I2C outcoming data interrupt
 */
void requestEvent () {
  byte data = 0x00;
  bitWrite(data, 0, ANALOG);
  bitWrite(data, 1, SMOTOR_STATUS);
  bitWrite(data, 2, LOCKED);
  Wire.write(data);
  Wire.write(LMOTOR_STATUS);
}

/**
 * setup
 */
void setup () {
  // Inputs
  pinMode(MOSI, INPUT);
  pinMode(SCK, INPUT);
  pinMode(SS, INPUT);

  // Outputs
  pinMode(MISO, OUTPUT);  
  pinMode(ACK, OUTPUT);

  // I2C setup
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // Configure SPI mode (DORD=1, CPOL=1, CPHA=1)
  SPCR |= bit (SPE);
  SPCR |= 0x2C;

  // Init outputs
  digitalWrite(ACK, HIGH);

  // Enable SPI interrupt
  SPCR |= _BV(SPIE);

  // Init controller status
  resetStatus();
}

/**
 * loop
 */
void loop () {
  // Reset status when there's no attention
  bool attention = !digitalReadFast(SS);
  if (!attention && ATTENTION) {
    resetStatus();
  }
  ATTENTION = attention;

  // Send ACK signal when required
  if (SEND_ACK) {
    SEND_ACK = false;
    digitalWriteFast(ACK, LOW);
    delayMicroseconds(4);
    digitalWriteFast(ACK, HIGH);
  }
}
