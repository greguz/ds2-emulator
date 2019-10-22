#include <PS2X_lib.h> // https://github.com/madsci1016/Arduino-PS2X

#define PS2_DAT 5 // MISO
#define PS2_CMD 4 // MOSI
#define PS2_SEL 3 // SS
#define PS2_CLK 2

#define pressures false
#define rumble false

PS2X ps2x;

int error = 0;
byte type = 0;
byte vibrate = 0;

void setup () {
  Serial.begin(57600);

  pinMode(PS2_SEL, OUTPUT);
  digitalWrite(PS2_SEL, HIGH);

  pinMode(PS2_CLK, OUTPUT);
  digitalWrite(PS2_CLK, HIGH);

  pinMode(6, INPUT);

  Serial.println("Waiting");
  delay(5000);
  Serial.println("Start");

  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);

  if (error == 0) {
    Serial.print("Found Controller, configured successful ");
    Serial.print("pressures = ");
    if (pressures) {
      Serial.println("true ");
    } else {
      Serial.println("false");
    }
    Serial.print("rumble = ");
    if (rumble) {
      Serial.println("true)");
    } else {
      Serial.println("false");
      Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
      Serial.println("holding L1 or R1 will print out the analog stick values.");
      Serial.println("Note: Go to www.billporter.info for updates and to report bugs.");
    }
  } else if(error == 1) {
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
  } else if(error == 2) {
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
  } else if(error == 3) {
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  }

  type = ps2x.readType();

  switch (type) {
    case 0:
      Serial.print("Unknown Controller type found ");
      break;
    case 1:
      Serial.print("DualShock Controller found ");
      break;
    case 2:
      Serial.print("GuitarHero Controller found ");
      break;
    case 3:
      Serial.print("Wireless Sony DualShock Controller found ");
      break;
  }
}

void loop () {
  if (error == 1) {
    Serial.println("Error");
    delay(100);
    return;
  }
  Serial.println("---------");

  ps2x.read_gamepad(false, vibrate);

  if (ps2x.Button(PSB_START)) {
    Serial.println("Start");
  }
  if (ps2x.Button(PSB_SELECT)) {
    Serial.println("Select");
  }

  if (ps2x.Button(PSB_PAD_UP)) {
    Serial.println("Up");
  }
  if (ps2x.Button(PSB_PAD_RIGHT)) {
    Serial.println("Right");
  }
  if (ps2x.Button(PSB_PAD_LEFT)) {
    Serial.println("Left");
  }
  if (ps2x.Button(PSB_PAD_DOWN)) {
    Serial.println("Down");
  }

  if (ps2x.Button(PSB_TRIANGLE)) {
    Serial.println("Triangle");
  }
  if (ps2x.Button(PSB_CIRCLE)) {
    Serial.println("Circle");
  }
  if (ps2x.Button(PSB_CROSS)) {
    Serial.println("Cross");
  }
  if (ps2x.Button(PSB_SQUARE)) {
    Serial.println("Square");
  }

  if (ps2x.Button(PSB_L1)) {
    Serial.println("L1");
  }
  if (ps2x.Button(PSB_R1)) {
    Serial.println("R1");
  }
  if (ps2x.Button(PSB_L2)) {
    Serial.println("L2");
  }
  if (ps2x.Button(PSB_R2)) {
    Serial.println("R2");
  }
  if (ps2x.Button(PSB_L3)) {
    Serial.println("L3");
  }
  if (ps2x.Button(PSB_R3)) {
    Serial.println("R3");
  }

  delay(1000);
}
