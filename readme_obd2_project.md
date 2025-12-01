# ESP32 OBD2 Display - Opel Corsa D

Real-time OBD2 diagnostic display system for 2010 Opel Corsa D using ESP32, ILI9488 display, and ELM327 Bluetooth adapter.

---

## Technical Stack

**Language:** MicroPython 1.23.0  
**Platform:** ESP32-WROOM-32 (Dual-core 240MHz, 520KB SRAM)  
**Communication:** Bluetooth Classic (ELM327), SPI (Display), UART (optional)

---

## Hardware Configuration

### Microcontroller
- **Board:** ESP32-WROOM-32
- **Architecture:** Xtensa Dual-Core 32-bit LX6
- **Clock:** 240 MHz
- **RAM:** 520 KB SRAM
- **Flash:** 4 MB
- **Interfaces:** WiFi 802.11n, Bluetooth Classic + BLE, SPI, I2C, UART

### Display
- **Model:** ILI9488 3.5" TFT LCD
- **Resolution:** 480×320 pixels (HVGA)
- **Interface:** SPI (4-wire + CS/DC/RST)
- **Touch Controller:** XPT2046 (Resistive)
- **Color Depth:** 18-bit RGB (262K colors)
- **SPI Clock:** 40 MHz
- **Voltage:** 3.3V

### OBD2 Adapter
- **Chip:** ELM327 v1.5 (original recommended, not clone)
- **Protocol:** ISO 15765-4 (CAN), auto-detected
- **Interface:** Bluetooth Classic (SPP profile)
- **Baud Rate:** 38400 bps
- **AT Command Set:** ELM327 standard

### Power Supply
- **Source:** USB-C Powerbank (5V/2A minimum)
- **ESP32 Input:** 5V via VIN pin
- **Display Power:** 3.3V from ESP32 onboard regulator
- **Total Current:** ~500mA @ 5V (2.5W)

---

## Pin Mapping

### SPI Display (ILI9488)

```python
# Display SPI Configuration
PIN_MOSI = 23  # SPI Data Out
PIN_MISO = 19  # SPI Data In
PIN_SCK = 18   # SPI Clock
PIN_CS = 15    # Chip Select
PIN_DC = 2     # Data/Command
PIN_RST = 4    # Reset

# Touch Controller (XPT2046)
TOUCH_CS = 21  # Touch Chip Select
```

### UART Communication (ELM327)

```python
# Bluetooth ELM327 UART Pins
BT_RX = 16  # ESP32 RX (from ELM327 TX)
BT_TX = 17  # ESP32 TX (to ELM327 RX)
```

**Note:** For Bluetooth Classic ELM327, physical UART connection is not required. Pins listed for reference if using wired UART ELM327.

---

## Project Structure

```
src/
├── main.py          # Application entry point, main loop, threading
├── ili9488.py       # ILI9488 SPI display driver (init, drawing, colors)
└── writer.py        # Text rendering library (8x8 bitmap font)
```

### Module Dependencies

```python
# Built-in MicroPython modules
from machine import Pin, SPI, UART
import time
import _thread

# Project modules
from ili9488 import ILI9488, BLACK, WHITE, GREEN, RED, CYAN, YELLOW
from writer import Writer
```

---

## OBD2 Implementation

### Vehicle Specification
- **Make/Model:** Opel Corsa D
- **Year:** 2010
- **Engine:** Varies (typically 1.2L or 1.4L petrol)
- **OBD2 Protocol:** ISO 15765-4 (CAN)
- **OBD2 Port Location:** Below steering wheel, driver side

### Supported PIDs (Mode 01 - Current Data)

| PID (Hex) | Parameter | Bytes | Formula | Unit | Opel Support |
|-----------|-----------|-------|---------|------|--------------|
| 0x05 | Engine Coolant Temperature | 1 | A - 40 | °C | Yes |
| 0x0C | Engine RPM | 2 | (A×256 + B) / 4 | RPM | Yes |
| 0x0D | Vehicle Speed | 1 | A | km/h | Yes |
| 0x0F | Intake Air Temperature | 1 | A - 40 | °C | Yes |
| 0x11 | Throttle Position | 1 | (A × 100) / 255 | % | Yes |
| 0x42 | Control Module Voltage | 2 | (A×256 + B) / 1000 | V | Possibly (not all ECUs) |

### Diagnostic Trouble Codes (DTC)

| Mode | Command | Function | Response Format |
|------|---------|----------|-----------------|
| 03 | `03` | Read stored DTCs | `43 [count] [DTC1] [DTC2] ...` |
| 07 | `07` | Read pending DTCs | `47 [count] [DTC1] [DTC2] ...` |
| 04 | `04` | Clear DTCs and MIL | `44` (success) |
| 0A | `0A` | Read permanent DTCs | `4A [count] [DTC1] [DTC2] ...` |

**DTC Format:** 
- 4 hex digits (2 bytes) per code
- Example: `01 33` = P0133 (O2 Sensor Circuit Slow Response)
- First byte determines prefix: 0x=P (Powertrain), 4x=C (Chassis), 8x=B (Body), Cx=U (Network)

### ELM327 AT Commands

```python
# Initialization sequence
commands = [
    'ATZ',      # Reset ELM327
    'ATE0',     # Echo off
    'ATL0',     # Linefeeds off
    'ATS0',     # Spaces off
    'ATSP0',    # Auto protocol detection
    'ATAT1',    # Adaptive timing auto (mode 1)
    '0100'      # Test: Supported PIDs
]
```

### Communication Protocol

```python
# PID Query Format
request = "01 [PID]\r"  # Mode 01 + PID in hex + carriage return

# Response Format
# Single byte: "41 [PID] [A] >"
# Two bytes:   "41 [PID] [A] [B] >"

# Example: Coolant temp query
# Send: "01 05\r"
# Receive: "41 05 5A >" → 0x5A = 90 → 90-40 = 50°C
```

---

## Architecture

### Multi-Threading (FreeRTOS)

```python
# Thread 1: OBD2 Communication (Core 0)
# - Query ELM327 via UART/Bluetooth
# - Parse responses
# - Update shared data structure (thread-safe with lock)
# - Run frequency: 500ms (2 Hz)

# Thread 2: Display Update (Core 1)
# - Read data from shared structure
# - Render to ILI9488 display
# - Handle touch input (future)
# - Run frequency: 1000ms (1 Hz)
```

### Data Flow

```
Vehicle ECU → ELM327 (Bluetooth) → ESP32 UART → Parse → Thread Lock
                                                              ↓
Display ← ILI9488 Driver ← Render ← Thread Lock ← Read Data
```

### Shared Data Structure

```python
obd_data = {
    'coolant_temp': 0.0,      # °C (float)
    'rpm': 0,                 # RPM (int)
    'speed': 0,               # km/h (int)
    'battery_voltage': 0.0,   # V (float)
    'connected': False,       # ELM327 status (bool)
    'error': ''               # Error message (str)
}

lock = _thread.allocate_lock()  # Thread synchronization
```

---

## Performance Characteristics

### Timing
- **OBD2 Query Rate:** ~2 PIDs/second (limited by ELM327 response time)
- **Display Refresh:** 1 Hz (configurable)
- **Touch Poll Rate:** N/A (not implemented)
- **ELM327 Timeout:** 500-1000ms per query

### Memory Usage
- **RAM Usage:** ~180 KB / 520 KB available
- **Flash Usage:** ~500 KB (MicroPython + code)
- **Display Buffer:** Minimal (direct SPI writes, no framebuffer)

### CPU Usage
- **Core 0 (OBD2):** ~30% average
- **Core 1 (Display):** ~40% average during full screen refresh
- **Idle:** ~10% combined

---

## Display Implementation

### Color Palette (RGB565)

```python
# Pre-defined colors in ili9488.py
BLACK   = 0x0000  # Background
WHITE   = 0xFFFF  # Text
RED     = 0xF800  # Warnings
GREEN   = 0x07E0  # Labels
CYAN    = 0x07FF  # Headers
YELLOW  = 0xFFE0  # Status
```

### Text Rendering

- **Font:** 8×8 pixel bitmap (monospace)
- **Scaling:** 1x, 2x, 3x (size parameter)
- **Character Set:** ASCII 32-126 (limited implementation in writer.py)
- **Performance:** ~50ms for full screen text update

### Screen Layout (480×320)

```
┌────────────────────────────────────┐
│ Header (320px wide)                │ Y: 0-40
│ "Opel Corsa D - OBD2" (Cyan, 2x)  │
├────────────────────────────────────┤
│ Coolant Temp (Green label, 2x)    │ Y: 60
│ [Value] (White, 3x)                │
├────────────────────────────────────┤
│ RPM (Green, 2x)                    │ Y: 110
│ [Value] (White, 3x)                │
├────────────────────────────────────┤
│ Speed (Green, 2x)                  │ Y: 160
│ [Value] (White, 3x)                │
├────────────────────────────────────┤
│ Battery (Green, 2x)                │ Y: 210
│ [Value] (White, 3x)                │
├────────────────────────────────────┤
│ Status Bar                         │ Y: 280-320
└────────────────────────────────────┘
```

---

## Code Conventions

### Naming
- Functions: `snake_case`
- Classes: `PascalCase`
- Constants: `UPPER_SNAKE_CASE`
- Variables: `snake_case`

### File Organization
- Hardware drivers: `device_name.py` (e.g., `ili9488.py`)
- Application logic: `main.py`
- Utilities: `helper_name.py` (e.g., `writer.py`)

### Comments
- Hardware registers/commands: Explain purpose
- Complex algorithms: Add inline comments
- Function docstrings: Not required but helpful

---

## Known Limitations

1. **PID 0x42 (Battery Voltage):** Not available on all Opel Corsa D ECUs
2. **ELM327 Clones:** Cheap clones may have inconsistent AT command support
3. **Bluetooth Pairing:** Must be done manually before first use (PIN: 1234)
4. **Display Refresh:** Limited to ~10-15 FPS due to SPI bandwidth
5. **Touch Input:** Not implemented in current version
6. **No Framebuffer:** Direct SPI writes only (no partial updates)

---

## Troubleshooting Context

### Common Hardware Issues
- **Display black:** Check CS/DC/RST pins, 3.3V power, SPI wiring
- **No ELM327 response:** Ignition must be ON, check baud rate (38400)
- **Garbage on display:** Wrong SPI clock polarity/phase
- **Touch not working:** Separate SPI CS pin (TOUCH_CS)

### Common Software Issues
- **Import errors:** Files not uploaded to ESP32 root directory
- **Memory errors:** Add `gc.collect()` after display updates
- **Timeout errors:** Increase ELM327 response timeout (1000-2000ms)
- **Parse errors:** ELM327 response format varies (spaces, linefeeds)

---

## Development Notes

### Testing Without Vehicle
```python
# Mock ELM327 responses for development
def mock_elm327_response(pid):
    responses = {
        '0105': '41 05 50',  # 50°C coolant
        '010C': '41 0C 0F A0',  # 1000 RPM
        '010D': '41 0D 3C',  # 60 km/h
    }
    return responses.get(pid, '41 00 00')
```

### Adding New PIDs
1. Find PID code from OBD2 standard
2. Determine byte count (1 or 2)
3. Implement formula in query function
4. Test with real vehicle
5. Add to display layout

### Future Extensions
- DTC reading/clearing (Mode 03/04)
- Multi-screen navigation via touch
- Data logging to internal storage
- Warning thresholds and alerts