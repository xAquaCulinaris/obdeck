# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based OBD2 diagnostic display system for 2010 Opel Corsa D. Uses ESP32-WROOM-32 with ILI9488 TFT display (touch not used) and ELM327 Bluetooth adapter for real-time vehicle diagnostics.

**Key Features:**
- 3 physical buttons for UI navigation
- MAC address-based Bluetooth connection
- Auto-retry with multiple PINs (1234, 0000)
- Real-time OBD2 data display
- Multi-core architecture (ESP32 dual-core)

## Hardware Platform
- **MCU:** ESP32-WROOM-32 (Dual-core 240MHz, 520KB SRAM)
- **Display:** KMRTM35018-SPI (ILI9488 controller, 3.5" TFT, 480×320 landscape, RGB666 18-bit color)
- **OBD2:** ELM327 v1.5 Bluetooth Classic (38400 baud, ISO 15765-4 CAN)
- **Power:** USB-C 5V/2A

### Display Specifications
- **Model:** KMRTM35018-SPI
- **Controller:** ILI9488
- **Native Resolution:** 320×480 (portrait), rotated to 480×320 (landscape via MADCTL)
- **Color Format:** RGB666 (18-bit, 3 bytes per pixel)
- **Interface:** SPI (running at 250 kHz for stability)
- **Power Requirements:**
  - VCC: 5V (NOT 3.3V - critical!)
  - LED (backlight): 3.3V
  - Logic pins: 3.3V compatible

### Display Timing Requirements

**CRITICAL:** This ILI9488 display has strict timing requirements to prevent white screen failures due to power constraints.

#### Operations That Cause Issues (White Screen):
1. **Large fillRect operations** (>10,000 pixels) without delays
2. **Multiple consecutive fillRect operations** without recovery time
3. **Fast full-screen fills** without strip-based approach

#### Tested & Working Delay Values (Optimized):

**Strip-Based Operations** (for large areas):
- Use 10px horizontal strips with **10ms delay between strips**
- Applied in: `safeFillScreen()`, error box background, top bar background
- Full screen clear: 32 strips × 10ms = ~320ms
- Example:
  ```cpp
  for (int y = 0; y < SCREEN_HEIGHT; y += 10) {
      tft.fillRect(0, y, SCREEN_WIDTH, 10, color);
      delay(10);  // Required for stability
  }
  ```

**Small fillRect Operations** (dashboard value clearing, nav buttons):
- Size: ~6,000-7,000 pixels per operation
- Requires **20ms delay after each fillRect**
- Can be consecutive with 20ms gaps
- Example: Dashboard value boxes, bottom navigation buttons

**Operations That DON'T Need Delays:**
- `drawRect()` - border drawing
- `drawLine()` - line drawing
- `drawCircle()` / `fillCircle()` - small circles
- `print()` / `drawString()` - text rendering
- `setTextColor()` - text color setting
- `setTextSize()` - text size setting
- `setCursor()` - cursor positioning
- `setTextDatum()` - text alignment
- `setRotation()` - screen rotation
- Any operation after 20ms has elapsed from last fillRect

**Special Case - drawString with padding:**
- `drawString()` with `setTextPadding()` uses fillRect internally
- Requires **20ms delay** after drawString when padding is set
- No delay needed if padding is 0 (default)

#### Performance Summary (After Optimization):
- **Full page redraw:** ~250ms (down from ~680ms - 63% faster!)
- **Value updates only:** ~50ms (down from ~120ms - 58% faster!)
- **Top bar redraw:** ~40ms (down from ~310ms - 87% faster!)
- **Bottom nav redraw:** ~60ms (down from ~270ms - 78% faster!)
- **Startup screen:** ~2.0 seconds (animated with scanning effect)

#### Key Insights:
1. The display controller needs recovery time only after fillRect operations (memory writes)
2. Strip-based approach (10px strips + 10ms delays) prevents power spikes for large areas
3. Text and drawing operations are fast and safe without any delays
4. Small fillRect (<10k pixels) + 20ms delay = stable and reliable
5. Large fillRect (>10k pixels) must use strip-based approach
6. **Never add delays after text operations** - they provide no benefit and slow down rendering

## Architecture

### Multi-Threading Design
This project uses ESP32's dual cores with FreeRTOS:

- **Core 0 (OBD2 Task):** Queries ELM327 via Bluetooth, parses responses, updates shared data structure with mutex protection
- **Core 1 (Display Task):** Runs in main loop, reads shared data, renders to ILI9488 at 2Hz

### Data Flow
```
Vehicle ECU → ELM327 (BT) → ESP32 Core 0 → Parse → Mutex → Shared OBDData → Mutex → Core 1 → ILI9488
```

### Project Structure
```
obdeck/
├── include/
│   ├── config.h                    # Hardware pins, Bluetooth, display config
│   └── TFT_eSPI_User_Setup.h       # TFT_eSPI library configuration
├── src/
│   ├── obdeck.ino                  # Main application (setup, loop)
│   ├── obd2/                       # OBD2 Communication Module
│   │   ├── obd_data.h              # Shared data structures (OBDData, DTC, mutex)
│   │   ├── bluetooth.h/.cpp        # Bluetooth connection management
│   │   ├── elm327.h/.cpp           # ELM327 protocol & PID queries
│   │   └── obd2_task.h/.cpp        # Main OBD2 task (Core 0)
│   └── display/                    # Display & UI Module
│       ├── display_manager.h/.cpp  # Display initialization & rendering
│       ├── ui_common.h             # Shared UI constants & enums
│       ├── nav_bar.h               # Top bar & bottom navigation
│       ├── startup_screen.h        # Animated startup screen
│       ├── dashboard.h             # Dashboard page (6 metrics)
│       ├── dtc_page.h              # DTC codes page with scrolling
│       ├── config_page.h           # Configuration display page
│       └── button_nav.h            # Physical button input handling
└── platformio.ini                  # PlatformIO configuration
```

### Module Breakdown

**OBD2 Module (`src/obd2/`):**
- `obd_data.h` - Data structures shared between cores (DTC struct, OBDData struct, mutex declarations)
- `bluetooth.h/.cpp` - Bluetooth Classic SPP connection (init, connect, disconnect, status)
- `elm327.h/.cpp` - ELM327 protocol implementation (PID queries, DTC operations, VIN)
- `obd2_task.h/.cpp` - FreeRTOS task running on Core 0 (queries, reconnection, error handling)

**Display Module (`src/display/`):**
- `display_manager.h/.cpp` - TFT initialization and page rendering coordinator
- `ui_common.h` - Page enums, layout constants, color definitions
- `nav_bar.h` - Top status bar and bottom page navigation buttons
- `startup_screen.h` - Animated startup screen with scanning effect
- `dashboard.h` - Main dashboard with 6 real-time metrics (RPM, speed, coolant, etc.)
- `dtc_page.h` - Diagnostic Trouble Codes page with scrolling and action buttons
- `config_page.h` - System configuration and vehicle info display
- `button_nav.h` - Physical button input with UI button highlighting system

**Main File (`src/obdeck.ino`):**
- Arduino setup() and loop()
- Global objects (TFT, page state)
- Calls startup screen animation
- Display refresh timing

## Pin Configuration

### Display (ILI9488 via SPI)
```cpp
#define TFT_MOSI  23  // SPI Data Out (SDI on display)
#define TFT_MISO  19  // SPI Data In (NOT CONNECTED - read operations not used)
#define TFT_SCLK  18  // SPI Clock
#define TFT_CS    5   // Chip Select (CRITICAL: NOT GPIO 15 - strapping pin!)
#define TFT_DC    2   // Data/Command (may be labeled RS on some displays)
#define TFT_RST   -1  // Reset (NOT CONNECTED - disabled for stability)
```

**SPI Configuration:**
- **Frequency:** 250 kHz (ultra-low for maximum stability with power constraints)
- **Mode:** SPI_MODE0

**IMPORTANT NOTES:**
- **GPIO 15 CANNOT be used for CS** - it's a strapping pin that causes SPI communication issues. Use GPIO 5 instead.
- **TFT_RST must be -1 and physically not connected** - connecting reset causes display instability
- **TFT_MISO (GPIO 19) not physically connected** - read operations not needed, saves a pin
- LED (backlight) connected to 3.3V (NOT 5V for this display)

### Physical Buttons (Navigation)
```cpp
#define BTN_LEFT    16  // Navigate to previous UI button (D16)
#define BTN_RIGHT   17  // Navigate to next UI button (D17)
#define BTN_SELECT  22  // Activate highlighted UI button (D22)
```
- Active LOW with internal pull-up resistors
- 500ms debounce delay

### Bluetooth (ELM327 via UART2)
Bluetooth Classic SPP uses UART2 internally - no physical wiring needed between ESP32 and ELM327.

**How to find MAC address:**
1. Pair ELM327 with your phone via Bluetooth settings
2. View device details to see MAC address
3. Update `BT_MAC_ADDRESS` in `include/config.h`

## OBD2 Implementation

### Supported PIDs (Mode 01)
| PID | Parameter | Bytes | Formula | Unit |
|-----|-----------|-------|---------|------|
| 0x05 | Coolant Temp | 1 | A - 40 | °C |
| 0x0C | Engine RPM | 2 | (A×256 + B) / 4 | RPM |
| 0x0D | Vehicle Speed | 1 | A | km/h |
| 0x0F | Intake Air Temp | 1 | A - 40 | °C |
| 0x11 | Throttle Position | 1 | (A × 100) / 255 | % |
| 0x42 | Battery Voltage | 2 | (A×256 + B) / 1000 | V |

### Communication Protocol
- Request format: `"01[PID]\r"` (Mode 01 + PID hex + carriage return)
- Response format: `"41[PID][A][B]>"` (Mode 41 response + data bytes)
- Example: Query `010C` (RPM) → Response `410C0FA0` → Parse: (0x0F×256 + 0xA0)/4 = 1000 RPM
- Query interval: 200ms (5 Hz) rotating through all PIDs

### DTC Codes
- **Mode 03:** Read stored DTCs
- **Mode 04:** Clear DTCs
- Format: 2 bytes per code (e.g., `0133` = P0133)
- Severity levels: CRITICAL (red), WARNING (yellow), INFO (cyan)
- Automatic sorting by severity

### VIN Query
- **Mode 09, PID 02:** Vehicle Identification Number
- Queried once at startup
- 17 character alphanumeric string

## UI Navigation System

### Pages
1. **Dashboard** - Real-time OBD2 metrics (RPM, speed, coolant, throttle, battery, intake)
2. **DTC Codes** - Diagnostic trouble codes with scrolling and actions (refresh, clear)
3. **Config** - System configuration and vehicle information

### Button Navigation
- **LEFT:** Move highlight to previous button
- **RIGHT:** Move highlight to next button
- **SELECT:** Activate highlighted button
- Yellow highlight shows current selection
- Buttons wrap around (circular navigation)

### UI Buttons
- Bottom navigation: Dashboard, DTC, Config (always visible)
- DTC page: Refresh, Clear All, Scroll Up, Scroll Down (context-sensitive)

## Threading & Synchronization

### Core 0 (OBD2 Task)
```cpp
void obd2Task(void *parameter)
```
- Runs on ESP32 Core 0
- Connects to ELM327 via Bluetooth
- Queries PIDs in rotation (200ms interval)
- Handles reconnection on connection loss (max 3 failures)
- Updates `obd_data` with mutex protection
- Processes DTC refresh/clear requests from UI

### Core 1 (Display Loop)
```cpp
void loop()
```
- Runs on ESP32 Core 1 (Arduino main loop)
- Reads `obd_data` with mutex protection
- Renders pages at 2Hz (500ms interval)
- Handles button input with debouncing
- Smart partial updates (only redraws changed values)

### Shared Data
```cpp
extern OBDData obd_data;           // Shared between cores
extern SemaphoreHandle_t data_mutex;  // Mutex for thread safety
```

## Known Limitations

- **PID 0x42 (battery voltage):** Not available on all Corsa D ECUs
- **ELM327 clones:** May have inconsistent AT command support
- **Bluetooth pairing:** Must be done manually before first use (PIN: 1234 or 0000)
- **Display timing constraints:** fillRect operations require delays (10-20ms) to prevent white screen; text operations require no delays
- **Rendering speed:** 250 kHz SPI speed required for stability, full redraw takes ~250ms (optimized)
- **GPIO constraints:** Cannot use GPIO 15 (strapping pin), GPIO 5 required for CS
- **Reset pin:** TFT_RST must be -1 (disabled) and physically not connected for stability
- **MISO pin:** TFT_MISO not physically connected (read operations not needed)

## Display Behavior

### Connection States
- **Connected:** Green status indicator, DTC count shown if any
- **Connecting:** "Connecting..." animation with dots (startup)
- **Disconnected:** Red status indicator, "Connection Lost" screen with animated dots, auto-reconnect

### Smart Rendering
- Only redraws changed values to prevent flickering
- Full redraw on page change or connection state change
- Strip-based screen clearing (10px strips with 10ms delays) prevents white screen
- 20ms delays after small fillRect operations for display stability
- Text rendering (drawRect, drawLine, print, drawString) requires **no delays**
- Text configuration (setTextColor, setTextSize, setCursor) requires **no delays**
- Optimized rendering: ~250ms full redraw, ~50ms value updates

## Development Setup

### Build & Upload (PlatformIO)
1. Open project in VS Code with PlatformIO extension
2. Click "Build" (checkmark icon) to compile
3. Click "Upload" (arrow icon) to flash ESP32
4. Click "Serial Monitor" to view debug output

### Serial Monitor
- Baud rate: 115200
- Shows connection status, PID queries, DTC operations
- Debug output for display rendering (every 50 frames)
- Error messages for troubleshooting

### Dependencies (platformio.ini)
- `bodmer/TFT_eSPI` - Display driver library
- `powerbroker2/ELMduino` - ELM327 communication (note: has bugs, using manual queries)

## Troubleshooting

### Display Issues
- **White screen:** Most common cause is fillRect operations without proper delays. See "Display Timing Requirements" section above. Always use strip-based approach for large fills (>10k pixels) with 10ms delays, and 20ms delay after small fillRect operations. Do NOT add delays after text operations. Also check: SPI pins, verify TFT_RST = -1, check for hardware shorts.
- **No display:** Verify 5V VCC power, check SPI connections, ensure TFT_RST = -1
- **Flickering:** Smart partial updates should prevent this, check for full redraws
- **Slow rendering:** Normal at 250 kHz SPI speed, required for power stability. Optimized performance: full page redraw ~250ms, value updates ~50ms. If slower, check for unnecessary delays after text operations.

### Bluetooth Issues
- **Connection fails:** Verify MAC address in config.h, check ELM327 pairing
- **Frequent disconnects:** Check signal strength, verify ELM327 power supply
- **No response:** Verify baud rate (38400), check ELM327 compatibility

### OBD2 Issues
- **PID returns -1:** Vehicle ECU may not support that PID
- **Slow updates:** Normal, PIDs queried in rotation every 200ms
- **No DTCs when expected:** Try "Refresh" button, check if codes are pending vs stored
