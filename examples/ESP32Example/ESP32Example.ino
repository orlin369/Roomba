// ESP32Example.ino
//
// Example for controlling iRobot Roomba/Create with ESP32
//
// Demonstrates:
//   - Serial communication setup on ESP32
//   - Basic robot control (start, modes, movement)
//   - Sensor reading
//   - LED control
//   - Simple autonomous behavior
//
// Hardware connections:
//   ESP32 GPIO16 (RX2) -> Roomba TXD (pin 5 on Mini-DIN or pin 1 on DB25)
//   ESP32 GPIO17 (TX2) -> Roomba RXD (pin 3 on Mini-DIN or pin 2 on DB25)
//   ESP32 GND          -> Roomba GND (pin 6/7 on Mini-DIN or pin 14 on DB25)
//
// Note: Roomba operates at 5V logic, ESP32 at 3.3V.
//       A level shifter is recommended for reliable communication.
//       Roomba TXD output may need a transistor buffer (see documentation).
//
// Copyright (C) 2024 orlin369
// License: GPL V2

#include <Roomba.h>

// ESP32 Serial2 default pins
#define ROOMBA_RX_PIN 16
#define ROOMBA_TX_PIN 17
#define ROOMBA_BAUD   115200

// LED pin for status indication
#define LED_PIN 2  // Built-in LED on most ESP32 boards

// Roomba instance using Serial2
Roomba roomba(&Serial2);

// Sensor data buffer
uint8_t sensorData[52];

// State machine for demo
enum DemoState {
  STATE_INIT,
  STATE_READ_SENSORS,
  STATE_DRIVE_FORWARD,
  STATE_TURN,
  STATE_STOP
};

DemoState currentState = STATE_INIT;
unsigned long stateTimer = 0;

void setup() {
  // Initialize debug serial
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println();
  Serial.println("=== ESP32 Roomba Example ===");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize Serial2 for Roomba communication
  // ESP32 allows custom pins for Serial2
  Serial2.begin(ROOMBA_BAUD, SERIAL_8N1, ROOMBA_RX_PIN, ROOMBA_TX_PIN);
  delay(100);

  Serial.println("Initializing Roomba...");

  // Wake up Roomba (pulse the BRC pin low if connected)
  // Some setups require this to wake Roomba from sleep

  // Start the Open Interface
  roomba.start();
  delay(100);

  // Enter Safe Mode (motors enabled with safety features)
  roomba.safeMode();
  delay(100);

  Serial.println("Roomba initialized in Safe Mode");

  // Flash LEDs to indicate ready
  flashRoombaLEDs();

  // Print menu
  printMenu();
}

void loop() {
  // Handle serial commands from user
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }

  // Run demo state machine if active
  if (currentState != STATE_INIT) {
    runDemoStateMachine();
  }

  delay(10);
}

void printMenu() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  w - Drive forward");
  Serial.println("  s - Drive backward");
  Serial.println("  a - Turn left");
  Serial.println("  d - Turn right");
  Serial.println("  x - Stop");
  Serial.println("  r - Read sensors");
  Serial.println("  l - Toggle LEDs");
  Serial.println("  p - Play song");
  Serial.println("  m - Run demo (autonomous)");
  Serial.println("  f - Full mode");
  Serial.println("  n - Safe mode");
  Serial.println("  h - Print this menu");
  Serial.println();
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'w':
    case 'W':
      Serial.println("Driving forward...");
      roomba.drive(200, Roomba::DriveStraight);
      break;

    case 's':
    case 'S':
      Serial.println("Driving backward...");
      roomba.drive(-200, Roomba::DriveStraight);
      break;

    case 'a':
    case 'A':
      Serial.println("Turning left...");
      roomba.drive(150, 1);  // Turn in place counter-clockwise
      break;

    case 'd':
    case 'D':
      Serial.println("Turning right...");
      roomba.drive(150, -1);  // Turn in place clockwise
      break;

    case 'x':
    case 'X':
      Serial.println("Stopping...");
      roomba.drive(0, 0);
      currentState = STATE_INIT;
      break;

    case 'r':
    case 'R':
      readAndPrintSensors();
      break;

    case 'l':
    case 'L':
      toggleLEDs();
      break;

    case 'p':
    case 'P':
      playSong();
      break;

    case 'm':
    case 'M':
      startDemo();
      break;

    case 'f':
    case 'F':
      Serial.println("Entering Full Mode (no safety!)");
      roomba.fullMode();
      break;

    case 'n':
    case 'N':
      Serial.println("Entering Safe Mode");
      roomba.safeMode();
      break;

    case 'h':
    case 'H':
      printMenu();
      break;

    case '\n':
    case '\r':
      // Ignore newlines
      break;

    default:
      Serial.print("Unknown command: ");
      Serial.println(cmd);
      break;
  }
}

void readAndPrintSensors() {
  Serial.println("Reading sensors...");
  digitalWrite(LED_PIN, HIGH);

  uint8_t buf[10];

  // Read bump and wheel drop sensors (packet ID 7)
  if (roomba.getSensors(Roomba::SensorBumpsAndWheelDrops, buf, 1)) {
    Serial.print("  Bumps/Wheels: 0x");
    Serial.println(buf[0], HEX);

    if (buf[0] & ROOMBA_MASK_BUMP_RIGHT) Serial.println("    - Right bumper pressed");
    if (buf[0] & ROOMBA_MASK_BUMP_LEFT) Serial.println("    - Left bumper pressed");
    if (buf[0] & ROOMBA_MASK_WHEELDROP_RIGHT) Serial.println("    - Right wheel dropped");
    if (buf[0] & ROOMBA_MASK_WHEELDROP_LEFT) Serial.println("    - Left wheel dropped");
    if (buf[0] & ROOMBA_MASK_WHEELDROP_CASTER) Serial.println("    - Caster wheel dropped");
  } else {
    Serial.println("  Failed to read bump sensors");
  }

  // Read wall sensor (packet ID 8)
  if (roomba.getSensors(Roomba::SensorWall, buf, 1)) {
    Serial.print("  Wall detected: ");
    Serial.println(buf[0] ? "Yes" : "No");
  }

  // Read cliff sensors (packet IDs 9-12)
  if (roomba.getSensors(Roomba::SensorCliffLeft, buf, 1)) {
    Serial.print("  Cliff Left: ");
    Serial.println(buf[0] ? "Yes" : "No");
  }

  if (roomba.getSensors(Roomba::SensorCliffRight, buf, 1)) {
    Serial.print("  Cliff Right: ");
    Serial.println(buf[0] ? "Yes" : "No");
  }

  // Read battery charge (packet ID 25) - 2 bytes
  if (roomba.getSensors(Roomba::SensorBatteryCharge, buf, 2)) {
    uint16_t charge = (buf[0] << 8) | buf[1];
    Serial.print("  Battery charge: ");
    Serial.print(charge);
    Serial.println(" mAh");
  }

  // Read battery capacity (packet ID 26) - 2 bytes
  if (roomba.getSensors(Roomba::SensorBatteryCapacity, buf, 2)) {
    uint16_t capacity = (buf[0] << 8) | buf[1];
    Serial.print("  Battery capacity: ");
    Serial.print(capacity);
    Serial.println(" mAh");
  }

  digitalWrite(LED_PIN, LOW);
}

void toggleLEDs() {
  static bool ledState = false;
  ledState = !ledState;

  if (ledState) {
    Serial.println("LEDs ON (Play LED green, Power LED orange)");
    roomba.leds(ROOMBA_MASK_LED_PLAY, 128, 255);  // Play LED on, Power LED orange
  } else {
    Serial.println("LEDs OFF");
    roomba.leds(ROOMBA_MASK_LED_NONE, 0, 0);
  }
}

void flashRoombaLEDs() {
  // Flash LEDs 3 times to indicate ready
  for (int i = 0; i < 3; i++) {
    roomba.leds(ROOMBA_MASK_LED_PLAY | ROOMBA_MASK_LED_ADVANCE, 0, 255);  // Green
    delay(200);
    roomba.leds(ROOMBA_MASK_LED_NONE, 0, 0);
    delay(200);
  }
}

void playSong() {
  Serial.println("Playing song...");

  // Define a simple melody (note, duration pairs)
  // Notes: 31-127 (MIDI note numbers), Duration: 1-255 (in 1/64th of a second)
  uint8_t melody[] = {
    72, 16,   // C5
    74, 16,   // D5
    76, 16,   // E5
    77, 16,   // F5
    79, 32,   // G5
    79, 32,   // G5
    81, 16,   // A5
    81, 16,   // A5
    81, 16,   // A5
    81, 16,   // A5
    79, 48    // G5
  };

  // Store song in slot 0
  roomba.song(0, melody, sizeof(melody));
  delay(50);

  // Play the song
  roomba.playSong(0);
}

void startDemo() {
  Serial.println("Starting autonomous demo...");
  Serial.println("Press 'x' to stop");
  currentState = STATE_READ_SENSORS;
  stateTimer = millis();
}

void runDemoStateMachine() {
  uint8_t buf[1];

  switch (currentState) {
    case STATE_READ_SENSORS:
      // Read bump sensors
      if (roomba.getSensors(Roomba::SensorBumpsAndWheelDrops, buf, 1)) {
        if (buf[0] & (ROOMBA_MASK_BUMP_RIGHT | ROOMBA_MASK_BUMP_LEFT)) {
          // Bumper hit - back up and turn
          Serial.println("Bump detected! Backing up...");
          roomba.drive(-150, Roomba::DriveStraight);
          currentState = STATE_TURN;
          stateTimer = millis();
        } else {
          // No bump - drive forward
          currentState = STATE_DRIVE_FORWARD;
        }
      }
      break;

    case STATE_DRIVE_FORWARD:
      roomba.drive(200, Roomba::DriveStraight);
      digitalWrite(LED_PIN, HIGH);
      currentState = STATE_READ_SENSORS;
      break;

    case STATE_TURN:
      // Wait 500ms while backing up, then turn
      if (millis() - stateTimer > 500) {
        Serial.println("Turning...");
        // Turn randomly left or right
        int turnDir = (millis() % 2 == 0) ? 1 : -1;
        roomba.drive(150, turnDir);  // Spin in place
        stateTimer = millis();
        currentState = STATE_STOP;
      }
      break;

    case STATE_STOP:
      // Turn for random duration between 500-1500ms
      if (millis() - stateTimer > 500 + (millis() % 1000)) {
        digitalWrite(LED_PIN, LOW);
        currentState = STATE_READ_SENSORS;
      }
      break;

    default:
      break;
  }
}
