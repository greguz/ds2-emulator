// Arduino Leonardo only

#define PIN_CLK 2   // blue
#define PIN_SS 3    // yellow
#define PIN_MOSI 4  // orange
#define PIN_MISO 5  // brown
#define PIN_ACK 6   // green

#define US_CLK 5

byte RX[21] = {};

void waitAck () {
  unsigned long start = micros();
  unsigned long diff = 0;
  bool ack = false;
  do {
    ack = bitRead(PIND, 7);
    diff = micros() - start;
  } while (ack && diff < 12);
}

byte routineByte (byte tx) {
  byte rx = 0x00;

  for (uint8_t i = 0; i < 8; i++) {
    // CLK (falling)
    bitClear(PORTD, 1);

    // MOSI (write)
    if (bitRead(tx, i)) {
      bitSet(PORTD, 4);
    } else {
      bitClear(PORTD, 4);
    }

    // half cycle
    delayMicroseconds(US_CLK);

    // CLK (rising)
    bitSet(PORTD, 1);

    // MISO (read)
    if (bitRead(PINC, 6)) {
      bitSet(rx, i);
    }

    // half cycle
    delayMicroseconds(US_CLK);
  }

  // Restore MOSI
  bitSet(PORTD, 4);

  // ACK
  waitAck();

  return rx;
}

void routineMessage (byte cmd, byte payload[], uint8_t size) {
  uint8_t i;
  Serial.println("START");
  digitalWrite(PIN_SS, LOW);
  RX[0] = routineByte(0x01);
  RX[1] = routineByte(cmd);
  RX[2] = routineByte(0x00);
  for (i = 0; i < size; i++) {
    RX[i + 3] = routineByte(payload[i]);
  }
  digitalWrite(PIN_SS, HIGH);
  for (i = 0; i < size + 3; i++) {
    Serial.print(RX[i], HEX);
    Serial.print(" : ");
    Serial.print(payload[i - 3], HEX);
    Serial.print("\n");
  }
  Serial.println("END");
}

void foo () {
  byte payload[2] = { 0x00, 0x00 };
  routineMessage(0x42, payload, 2);
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ACK, INPUT);
  pinMode(PIN_SS, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_CLK, OUTPUT);

  digitalWrite(PIN_SS, HIGH);
  digitalWrite(PIN_MOSI, HIGH);
  digitalWrite(PIN_CLK, HIGH);
}

void loop() {
  delay(5000);
  foo();
}
