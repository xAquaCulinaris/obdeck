# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based OBD2 diagnostic display system for 2010 Opel Corsa D. Uses MicroPython 1.23.0 on ESP32-WROOM-32 with ILI9488 TFT display and ELM327 Bluetooth adapter for real-time vehicle diagnostics.

## Hardware Platform

- **MCU:** ESP32-WROOM-32 (Dual-core 240MHz, 520KB SRAM)
- **Display:** ILI9488 3.5" TFT (480x320, SPI, 40MHz)
- **OBD2:** ELM327 v1.5 Bluetooth Classic (38400 baud, ISO 15765-4 CAN)
- **Power:** USB-C 5V/2A minimum

## Architecture

### Multi-Threading Design
This project uses ESP32's dual cores via MicroPython's `_thread` module:

- **Core 0 (OBD2 Thread):** Queries ELM327 via UART/Bluetooth at 2Hz, parses responses, updates shared data structure with thread lock
- **Core 1 (Display Thread):** Reads shared data, renders to ILI9488 at 1Hz

Thread synchronization uses `_thread.allocate_lock()` to protect the shared `obd_data` dictionary.

### Data Flow
```
Vehicle ECU → ELM327 (BT) → ESP32 UART → Parse → Lock → Shared Dict → Lock → Display Thread → ILI9488
```

### Project Structure
```
src/
├── main.py              # Application entry, threading coordination
├── display/
│   ├── driver.py        # ILI9488 display driver (SPI communication, drawing)
│   ├── colors.py        # RGB565 color constants
│   └── writer.py        # Text rendering with 8x8 bitmap font
└── obd2/
    ├── bluetooth.py     # UART/Bluetooth connection to ELM327
    ├── commands.py      # OBD2 PID definitions and DTC commands
    └── parser.py        # Response parsing and data conversion
```

## Pin Configuration

### Display (ILI9488 via SPI)
```python
PIN_MOSI = 23  # SPI Data Out
PIN_MISO = 19  # SPI Data In
PIN_SCK = 18   # SPI Clock
PIN_CS = 15    # Chip Select
PIN_DC = 2     # Data/Command
PIN_RST = 4    # Reset
TOUCH_CS = 21  # Touch controller CS (XPT2046)
```

### UART (ELM327 - if wired, not needed for Bluetooth)
```python
BT_RX = 16  # ESP32 RX
BT_TX = 17  # ESP32 TX
```

## OBD2 Implementation

### ELM327 Initialization Sequence
```python
commands = ['ATZ', 'ATE0', 'ATL0', 'ATS0', 'ATSP0', 'ATAT1', '0100']
```

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

### DTC Codes (Future Implementation)
- Mode 03: Read stored DTCs
- Mode 04: Clear DTCs
- Mode 07: Read pending DTCs
- Format: 2 bytes per code (e.g., `0133` = P0133)

## Shared Data Structure

```python
obd_data = {
    'coolant_temp': 0.0,      # °C
    'rpm': 0,                 # RPM
    'speed': 0,               # km/h
    'battery_voltage': 0.0,   # V
    'connected': False,       # ELM327 connection status
    'error': ''               # Error message string
}

lock = _thread.allocate_lock()
```

Always acquire lock before reading/writing this dictionary.

## Display Implementation

### Color Constants (RGB565 in display/colors.py)
```python
BLACK = 0x0000   # Background
WHITE = 0xFFFF   # Text values
RED = 0xF800     # Warnings
GREEN = 0x07E0   # Labels
CYAN = 0x07FF    # Headers
YELLOW = 0xFFE0  # Status
```

### Text Rendering
- Font: 8×8 monospace bitmap
- Scaling: 1x, 2x, 3x via size parameter
- ASCII support: 32-126
- No framebuffer - direct SPI writes only

### Screen Layout (480×320)
```
Y=0-40:   Header "Opel Corsa D - OBD2" (Cyan, 2x)
Y=60:     Coolant Temp label (Green, 2x) + value (White, 3x)
Y=110:    RPM label + value
Y=160:    Speed label + value
Y=210:    Battery label + value
Y=280:    Status bar
```

## Performance Characteristics

- **OBD2 Query Rate:** ~2 PIDs/sec (ELM327-limited)
- **Display Refresh:** 1 Hz
- **ELM327 Timeout:** 500-1000ms per query
- **RAM Usage:** ~180KB / 520KB
- **CPU Usage:** Core 0 ~30%, Core 1 ~40% during refresh

## Code Conventions

- Functions/variables: `snake_case`
- Classes: `PascalCase`
- Constants: `UPPER_SNAKE_CASE`
- Hardware drivers: `device_name.py`
- Add `gc.collect()` after display updates to prevent memory fragmentation

## Development Without Vehicle

Use mock responses for testing:
```python
def mock_elm327_response(pid):
    responses = {
        '0105': '41 05 50',      # 50°C coolant
        '010C': '41 0C 0F A0',   # 1000 RPM
        '010D': '41 0D 3C',      # 60 km/h
    }
    return responses.get(pid, '41 00 00')
```

## Common Issues

### Hardware
- Display black: Check CS/DC/RST pins, verify 3.3V power and SPI wiring
- No ELM327 response: Vehicle ignition must be ON, verify 38400 baud
- Display garbage: Wrong SPI clock polarity/phase
- Touch not working: Ensure separate CS pin (TOUCH_CS=21)

### Software
- Import errors: Upload entire directory structure to ESP32 (src/ with subdirs)
- Memory errors: Add `gc.collect()` after large operations
- Timeout errors: Increase ELM327 timeout to 1000-2000ms in bluetooth.py
- Parse errors: ELM327 response format varies (handled in parser.py)

## Adding New PIDs

1. Add PID to `obd2/commands.py` in `OBD2Commands.PIDS` dict
2. Add metadata to `OBD2Commands.PID_INFO` (name, bytes, formula, unit)
3. Add parsing logic to `obd2/parser.py` in `parse_response()`
4. Update `obd_data` dictionary in `main.py` with new field
5. Add data update logic in `obd2_thread()` in `main.py`
6. Add display rendering in `display_thread()` in `main.py`

## Known Limitations

- PID 0x42 (battery voltage) not available on all Corsa D ECUs
- ELM327 clones may have inconsistent AT command support
- Bluetooth pairing must be done manually (PIN: 1234)
- Display limited to ~10-15 FPS (SPI bandwidth)
- Touch input not implemented
- No framebuffer (no partial updates possible)

## Deployment to ESP32

Upload entire src/ directory structure to ESP32:
```
/main.py
/display/
  driver.py
  colors.py
  writer.py
/obd2/
  bluetooth.py
  commands.py
  parser.py
```

Use tools like `ampy`, `rshell`, or Thonny IDE for file transfer. MicroPython supports subdirectories for module imports.
