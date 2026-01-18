# CLAUDE.md

This file provides guidance for AI assistants working with the Roomba Arduino Library codebase.

## Project Overview

The **Roomba Arduino Library** is an object-oriented C++ library for Arduino microcontrollers that provides complete control over iRobot Roomba and Create robot platforms via serial communication. It implements the iRobot Open Interface (OI) protocol.

- **Version:** 1.3.0
- **License:** GPL V2 or Commercial
- **Maintainer:** orlin369
- **Original Author:** Mike McCauley

## Directory Structure

```
Roomba/
├── Roomba.h              # Main header - class definition, enums, constants, masks
├── Roomba.cpp            # Implementation of all class methods
├── library.properties    # Arduino library metadata
├── examples/
│   ├── RoombaTest1/      # Basic sensor streaming test
│   ├── TestSuite/        # Comprehensive test suite
│   ├── RoombaRCRx/       # WiFi remote control example
│   └── ESP32Example/     # ESP32 communication example
├── doc/                  # Pre-generated Doxygen HTML documentation
├── API.md                # API reference documentation
├── README.md             # User documentation
├── Makefile              # Build automation (doxygen, dist, upload)
└── project.cfg           # Doxygen configuration
```

## Architecture

### Single Main Class: `Roomba`

The library centers around a single class with methods organized by functionality:

| Category | Key Methods |
|----------|-------------|
| **Initialization** | `Roomba()`, `start()`, `reset()`, `baud()` |
| **Operating Modes** | `safeMode()`, `fullMode()`, `power()` |
| **Movement** | `drive()`, `driveDirect()`, `dock()`, `cover()` |
| **LEDs & Display** | `leds()`, `digitLedsRaw()`, `digitLedsASCII()` |
| **Peripherals** | `digitalOut()`, `pwmDrivers()`, `drivers()`, `sendIR()` |
| **Audio** | `song()`, `playSong()` |
| **Sensors** | `getSensors()`, `getSensorsList()`, `stream()`, `pollSensors()` |
| **Scripting** | `script()`, `playScript()`, `wait*()` methods |

### Key Enumerations

- `Roomba::Baud` - Serial baud rates (300 to 115200)
- `Roomba::Mode` - Operating modes (Off, Passive, Safe, Full)
- `Roomba::Demo` - Built-in demo programs
- `Roomba::Drive` - Special drive values (straight, spin)
- `Roomba::Sensor` - 36 sensor packet IDs

### Bit Masks

80+ `ROOMBA_MASK_*` constants for LED control, digital outputs, drivers, bumpers, wheels, and buttons.

## Build System

Uses Arduino IDE standard library structure with no external dependencies.

**Makefile targets:**
- `make doxygen` - Generate HTML documentation
- `make dist` - Create distribution zip
- `make upload` - Deploy to web server

## Coding Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `Roomba` |
| Methods | camelCase | `driveDirect()` |
| Constants | UPPER_CASE | `ROOMBA_MASK_LED_PLAY` |
| Private members | Underscore prefix | `_serial`, `_pollState` |

## Serial Communication Pattern

1. Commands are single or multi-byte writes to HardwareSerial
2. Follows iRobot OI protocol command bytes
3. Response data read with 200ms timeout (`ROOMBA_READ_TIMEOUT`)
4. Checksum validation for streamed sensor data
5. State machine in `pollSensors()` for parsing streams

## Platform Support

- Arduino: Uno, Mega, Due, Duemilanove
- ESP8266, ESP32
- Any platform with HardwareSerial support

**Note:** The primary Serial port on Uno/similar boards requires a PNP transistor buffer circuit for reliable communication (see `doc/Create-Arduino.pdf`). Serial1/Serial2/Serial3 on Mega/Due work without modification.

### ESP32 Wiring

```
ESP32 GPIO16 (RX2) → Roomba TXD (pin 5 on Mini-DIN)
ESP32 GPIO17 (TX2) → Roomba RXD (pin 3 on Mini-DIN)
ESP32 GND          → Roomba GND (pin 6/7 on Mini-DIN)
```

A level shifter is recommended (ESP32 uses 3.3V, Roomba uses 5V logic).

## Testing

Run examples via Arduino IDE:

1. **RoombaTest1.ino** - Basic sensor streaming with LED feedback
2. **TestSuite.ino** - Comprehensive validation of library functions
3. **RoombaRCRx.ino** - WiFi integration test
4. **ESP32Example.ino** - Interactive ESP32 control with serial commands

## Platform-Specific Features

**Create-only methods:**
- `driveDirect()`, `digitalOut()`, `pwmDrivers()`, `sendIR()`
- `stream()`, `script()`, `playScript()`
- `waitDistance()`, `waitAngle()`, `waitEvent()`
- `getSensorsList()`

**Roomba-only methods:**
- `power()` - Sleep mode
- `dock()` - Auto-docking

## Common Tasks

### Adding a new command
1. Add command byte constant to `Roomba.h`
2. Add method declaration to public section of class
3. Implement in `Roomba.cpp` following existing patterns
4. Update API.md documentation

### Modifying sensor handling
- Sensor packet IDs are in `Roomba::Sensor` enum
- `getSensors()` reads single packets
- `stream()` + `pollSensors()` for continuous data
- State machine in `pollSensors()` handles parsing

### Working with examples
- Examples use either `Serial` (Uno) or `Serial1` (Mega/Due/ESP)
- Buffer sizes: 52 bytes typical for sensor data
- LED feedback indicates success/failure states

## Important Files to Read First

1. `Roomba.h` - All public API, enums, and constants
2. `Roomba.cpp` - Implementation patterns
3. `examples/TestSuite/TestSuite.ino` - Usage examples
4. `API.md` - Detailed method documentation

## Git Workflow

### Branching Strategy
- **master**: Production-ready code, only receives merges from dev
- **dev**: Development branch, created from master, where integration happens
- **feature branches**: Created from dev for each new feature or change

### Branch Naming
- Feature branches: `feature/<short-description>` (e.g., `feature/add-dimmer-support`)
- Bug fixes: `fix/<short-description>` (e.g., `fix/mac-validation`)

### Commit Workflow (Step by Step)

1. **Checkout dev branch:**
   ```bash
   git checkout dev
   ```

2. **Create feature branch from dev:**
   ```bash
   git checkout -b feature/<short-description>
   ```

3. **Stage and commit changes with descriptive message:**
   ```bash
   git add <file>
   git commit -m "$(cat <<'EOF'
   Short summary of changes

   - Detailed bullet point 1
   - Detailed bullet point 2
   - Detailed bullet point 3

   Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
   EOF
   )"
   ```

4. **Test the feature before merging:**
   - For software-only changes: Run build and verify functionality
   - For hardware-dependent changes: **ASK the user to test manually**
   - Never merge untested code into dev

5. **ASK before merging to dev:**
   - Always ask the user for approval before merging feature into dev
   - Example: "Feature is ready and committed. May I merge to dev and master?"

6. **Merge feature branch to dev (with --no-ff to preserve branch history):**
   ```bash
   git checkout dev
   git merge feature/<short-description> --no-ff -m "Merge feature/<short-description> into dev"
   ```

7. **Merge dev to master (with --no-ff to preserve branch history):**
   ```bash
   git checkout master
   git merge dev --no-ff -m "Merge dev into master"
   ```

8. **Push both branches and clean up:**
   ```bash
   git push origin master
   git push origin dev
   git branch -d feature/<short-description>
   ```

### Important: Always Use --no-ff

Always use `--no-ff` (no fast-forward) when merging to create merge commits. This preserves the branch topology and makes the history visible in GitLens:

```
*   Merge dev into master
|\
| *   Merge feature/xyz into dev
| |\
| | * Actual commit message
| |/
```

### Commit Message Format

```
Short summary (imperative mood, max 50 chars)

- Bullet point describing change 1
- Bullet point describing change 2
- Bullet point describing change 3

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
```
