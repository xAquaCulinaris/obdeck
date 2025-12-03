# C++ Migration Plan

## Branch: cpp-migration

## Phase 1 Goals (Current Focus)
1. ✅ Bluetooth connection to ELM327 (real device + emulator)
2. ✅ Display driver (ILI9488) working with basic rendering

## Project Structure

```
obdeck/
├── src/                          # MicroPython (keep for reference)
├── cpp/                          # New C++ Arduino project
│   ├── obdeck.ino               # Main Arduino sketch
│   ├── config.h                 # Configuration (pins, BT settings)
│   ├── bluetooth/
│   │   ├── ELM327Connection.h
│   │   └── ELM327Connection.cpp
│   ├── display/
│   │   ├── DisplayDriver.h
│   │   └── DisplayDriver.cpp
│   ├── obd2/
│   │   ├── OBD2Parser.h
│   │   └── OBD2Parser.cpp
│   └── README.md
└── platformio.ini               # PlatformIO config (recommended)
```

## Libraries & Dependencies

### Bluetooth (Phase 1 Priority)
- **BluetoothSerial** (built-in ESP32 Arduino)
  - Native ESP-IDF Bluetooth Classic SPP
  - Well-tested, stable
  - No external dependencies

- **ELMduino** by PowerBroker2
  - GitHub: https://github.com/PowerBroker2/ELMduino
  - Handles ELM327 AT commands and OBD2 PIDs
  - Works with BluetoothSerial
  - Active maintenance (last updated 2024)

**Connection Approach:**
- Try MAC address first (most reliable per GitHub issues)
- Fallback to device name
- Most ELM327s don't need PIN authentication

### Display (Phase 1 Priority)
**Option A: TFT_eSPI** (Recommended)
- GitHub: https://github.com/Bodmer/TFT_eSPI
- Fast, optimized for ESP32
- Supports ILI9488 (320x480 RGB666)
- Hardware SPI with DMA
- Touch support (XPT2046)
- User configuration via User_Setup.h

**Option B: Adafruit_ILI9341**
- More generic, slower
- Better for prototyping
- ILI9488 support via GFX library

**Recommendation:** Use TFT_eSPI for performance

### Multi-threading
- **FreeRTOS** (built-in ESP32 Arduino)
  - `xTaskCreatePinnedToCore()` for dual-core
  - Core 0: Bluetooth/OBD2 queries
  - Core 1: Display rendering
  - Mutex for shared data

## Phase 1 Architecture

```
┌─────────────────────────────────────────────┐
│              Main Loop (Core 1)             │
│  - Display updates                          │
│  - Touch input handling                     │
└─────────────────────────────────────────────┘
                    │
                    ↓ (Mutex)
┌─────────────────────────────────────────────┐
│           Shared OBD Data Structure         │
│  {rpm, speed, coolant_temp, voltage, ...}  │
└─────────────────────────────────────────────┘
                    ↑ (Mutex)
                    │
┌─────────────────────────────────────────────┐
│         OBD2 Task (Core 0)                  │
│  - BluetoothSerial connection               │
│  - ELM327 initialization                    │
│  - PID queries (2Hz)                        │
└─────────────────────────────────────────────┘
```

## Configuration (config.h)

```cpp
// Hardware Pins
#define PIN_MOSI      23
#define PIN_MISO      19
#define PIN_SCK       18
#define PIN_CS        5
#define PIN_DC        2
#define PIN_RST       4
#define PIN_TOUCH_CS  21

// Bluetooth Configuration
#define BT_DEVICE_NAME "OBDII"          // ELM327 device name
#define BT_MAC_ADDRESS "9C:9C:1F:C7:63:A6"  // ELM327 MAC address
#define BT_USE_MAC     true             // Use MAC instead of name
#define BT_PIN         "1234"           // PIN (often not needed)

// Display Configuration
#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  320
#define SCREEN_ROTATION 1  // Landscape

// OBD2 Configuration
#define OBD2_QUERY_RATE_MS  500  // 2 Hz
#define ELM327_TIMEOUT_MS   2000
```

## Build System Options

### Option A: PlatformIO (Recommended)
**Pros:**
- Better dependency management
- Multiple environment support
- VS Code integration
- Automated library installation

**Cons:**
- Learning curve if new

### Option B: Arduino IDE
**Pros:**
- Simple, familiar
- Quick setup

**Cons:**
- Manual library management
- Less flexible build options

**Recommendation:** Use PlatformIO for this project size

## Phase 1 Implementation Order

1. **Project Setup**
   - Create cpp/ directory structure
   - Setup platformio.ini or Arduino sketch
   - Install libraries

2. **Bluetooth Connection (Priority 1)**
   - Basic BluetoothSerial test
   - ELMduino integration
   - Connection to emulator
   - Test with real ELM327 (if available)

3. **Display Driver (Priority 2)**
   - TFT_eSPI configuration
   - Basic shapes/text rendering
   - Test display performance

4. **Integration Test**
   - Read RPM from ELM327
   - Display on screen
   - Verify threading works

## Testing Strategy

### Bluetooth Testing
1. ✅ Connect to Arduino emulator (BluetoothSerial SPP)
2. ✅ Send AT commands, verify responses
3. ✅ Query OBD2 PIDs (0105, 010C, 010D)
4. ⏳ Test with real ELM327 in car

### Display Testing
1. ✅ Fill screen with colors
2. ✅ Draw rectangles, text
3. ✅ Measure refresh rate
4. ✅ Test SPI speed (20-40 MHz)

## Migration from MicroPython

| Component | MicroPython | C++ Equivalent |
|-----------|-------------|----------------|
| Bluetooth | btm module | BluetoothSerial + ELMduino |
| Display | Custom SPI driver | TFT_eSPI |
| Threading | _thread module | FreeRTOS xTaskCreate |
| OBD2 Parsing | Custom parser | ELMduino built-in |
| Touch | Custom XPT2046 | XPT2046_Touchscreen lib |

## Questions to Decide

1. **Build System:** PlatformIO or Arduino IDE?
2. **Display Library:** TFT_eSPI or Adafruit GFX?
3. **Connection Method:** MAC address or device name first?
4. **ELM327 Emulator:** Keep Arduino version or update to match?

## Next Steps

Let's discuss:
1. Which build system do you prefer?
2. Any specific libraries you want to avoid/use?
3. Should we keep the emulator Arduino-based or update it?

Then we'll start with Phase 1 implementation!
