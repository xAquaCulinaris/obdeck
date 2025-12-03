# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-based OBD2 diagnostic display system for 2010 Opel Corsa D. Uses MicroPython 1.23.0 on ESP32-WROOM-32 with ILI9488 TFT touchscreen display and ELM327 Bluetooth adapter for real-time vehicle diagnostics.

**Key Features:**
- Touch-enabled UI with reconnect button
- MAC address-based Bluetooth connection
- Auto-retry with multiple PINs (1234, 0000)
- Real-time OBD2 data display

## Hardware Platform

- **MCU:** ESP32-WROOM-32 (Dual-core 240MHz, 520KB SRAM)
- **Display:** KMRTM35018-SPI (ILI9488 controller, 3.5" TFT, 480×320 landscape, RGB666 18-bit color)
- **OBD2:** ELM327 v1.5 Bluetooth Classic (38400 baud, ISO 15765-4 CAN)
- **Power:** USB-C 5V/2A minimum

### Display Specifications
- **Model:** KMRTM35018-SPI
- **Controller:** ILI9488
- **Native Resolution:** 320×480 (portrait), rotated to 480×320 (landscape via MADCTL)
- **Color Format:** RGB666 (18-bit, 3 bytes per pixel)
- **Interface:** SPI (tested up to 20MHz)
- **Power Requirements:**
  - VCC: 5V (NOT 3.3V - critical!)
  - LED (backlight): 3.3V or 5V
  - Logic pins: 3.3V compatible

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
├── main.py              # Application entry, threading, touch handling
├── config.py            # Configuration (MAC address, PIN, display settings)
├── display/
│   ├── driver.py        # ILI9488 display driver (SPI communication, drawing)
│   ├── colors.py        # RGB565 color constants
│   ├── writer.py        # Text rendering with 8x8 bitmap font
│   ├── touch.py         # XPT2046 touch controller driver
│   └── button.py        # Touch button UI component
└── obd2/
    ├── bluetooth.py     # Bluetooth Classic SPP connection to ELM327
    ├── commands.py      # OBD2 PID definitions and DTC commands
    └── parser.py        # Response parsing and data conversion
```

## Pin Configuration

### Display (ILI9488 via SPI)
```python
PIN_MOSI = 23  # SPI Data Out (SDI on display)
PIN_MISO = 19  # SPI Data In (SDO on display)
PIN_SCK = 18   # SPI Clock
PIN_CS = 5     # Chip Select (CRITICAL: NOT GPIO 15 - strapping pin!)
PIN_DC = 2     # Data/Command (may be labeled RS on some displays)
PIN_RST = 4    # Reset
TOUCH_CS = 21  # Touch controller CS (XPT2046)
```

**IMPORTANT NOTES:**
- **GPIO 15 CANNOT be used for CS** - it's a strapping pin that causes SPI communication issues. Use GPIO 5 instead.
- VCC must be connected to **5V**, not 3.3V (see Power Requirements above)
- LED (backlight) can be connected to 3.3V or 5V

### SPI Configuration
```python
spi = SPI(2, baudrate=20000000, polarity=0, phase=0,
          sck=Pin(PIN_SCK), mosi=Pin(PIN_MOSI), miso=Pin(PIN_MISO))
```
- **Bus:** SPI(2) - tested and working
- **Baudrate:** 10-20MHz recommended (40MHz may work but not tested thoroughly)
- **Mode:** 0 (CPOL=0, CPHA=0)

### Bluetooth (ELM327 via UART2)
Bluetooth Classic SPP uses UART2 internally - no physical wiring needed between ESP32 and ELM327.

## Configuration

Edit `src/config.py` to set up your ELM327:

```python
# Find MAC address by pairing ELM327 with phone first
BT_MAC_ADDRESS = "00:1D:A5:68:98:8B"  # Replace with your ELM327 MAC

# Primary PIN to try
BT_PIN = "1234"  # Common PINs: "1234", "0000", "6789"

# Auto-try "0000" if primary PIN fails
BT_AUTO_TRY_PINS = True
```

**How to find MAC address:**
1. Pair ELM327 with your phone via Bluetooth settings
2. View device details to see MAC address
3. Copy MAC address to config.py

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

### Display Initialization (driver.py)
The ILI9488 is initialized with these critical settings:
```python
# Pixel format: RGB666 (18-bit, 3 bytes per pixel)
CMD_COLMOD: 0x66

# Memory Access Control: Landscape orientation (480×320)
CMD_MADCTL: 0xE8  # MY=1, MX=1, MV=1, BGR=1
# - MY (bit 7): Row address order reversed
# - MX (bit 6): Column address order reversed
# - MV (bit 5): Row/column exchange (enables landscape)
# - BGR (bit 3): BGR color order
```

**Why RGB666 instead of RGB565:**
- ILI9488 native format is RGB666 (18-bit)
- Better color accuracy and display compatibility
- Driver automatically converts RGB565 color constants to RGB666

### Color Constants (RGB565 format in display/colors.py)
Colors are defined in RGB565 format but **automatically converted to RGB666** by the driver:
```python
BLACK = 0x0000   # Background
WHITE = 0xFFFF   # Text values
RED = 0xF800     # Warnings
GREEN = 0x07E0   # Labels
CYAN = 0x07FF    # Headers
YELLOW = 0xFFE0  # Status
```

The `rgb565_to_rgb666()` method in driver.py handles conversion automatically.

### Text Rendering (Optimized Buffered Approach)
- **Font:** 8×8 monospace bitmap
- **Scaling:** 1x, 2x, 3x via size parameter
- **ASCII support:** 32-126
- **Rendering method:** Single buffered write per character
  - Pre-renders entire character to buffer (including background)
  - Sets display window once per character
  - Writes entire buffer in single SPI transaction
  - **~50-100x faster** than pixel-by-pixel rendering
- **No framebuffer:** Direct SPI writes only

### Drawing Performance
- **Rectangles:** Very fast (optimized chunked writes with watchdog yields)
- **Text:** Fast (single buffered write per character)
- **Single pixels:** Slower (use sparingly, prefer rectangles)

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

## Common Issues & Troubleshooting

### Hardware Issues

#### Display Shows All White or Backlight Only
1. **VCC voltage wrong:** KMRTM35018-SPI requires **5V for VCC**, not 3.3V
   - Connect VCC to ESP32 VIN (5V) when powered via USB
   - LED/backlight can use 3.3V
2. **Wrong CS pin:** Using GPIO 15 causes SPI communication to hang
   - **Solution:** Use GPIO 5 for CS (configured in config.py)
3. **Missing __init__.py files:** Display/OBD2 modules won't import
   - Ensure `display/__init__.py` and `obd2/__init__.py` exist on ESP32

#### Display Shows Wrong Orientation or Mirrored
- **MADCTL value incorrect:** Must be 0xE8 for correct 480×320 landscape
- Located in `display/driver.py` initialization sequence
- Wrong MADCTL causes portrait mode or mirrored display

#### SPI Communication Hangs
- **GPIO 15 issue:** ESP32 strapping pin, cannot be used for CS
- **Watchdog timeout:** Large operations need `time.sleep_us(50)` yields
- **MISO pin issues:** Try removing MISO parameter from SPI init (write-only mode)

#### Display Black Screen
- Check backlight LED pin connection (needs 3.3V or 5V)
- Verify SPI wiring: MOSI→SDI, SCK→SCK, CS→CS, DC→DC/RS, RST→RST
- Test with simple fill commands before complex rendering

### Software Issues

#### Module Import Errors
- **Missing __init__.py:** Both `display/` and `obd2/` folders need `__init__.py` files
- **Wrong directory structure:** Files must be uploaded from `src/` contents to ESP32 root
  - NOT: `/src/display/driver.py`
  - YES: `/display/driver.py`
- **File upload tool:** Use MicroPico "sync_folder": "src" in .micropico.json

#### Slow Text Rendering
- **Old implementation:** Character rendering must use buffered approach
- Check `writer.py` char() method uses single buffer write, not pixel-by-pixel
- Expected: Text renders as fast as rectangles

#### Memory Errors
- Add `gc.collect()` after large display operations
- Use chunked writes for large fills (see fill_rect implementation)
- Monitor with `gc.mem_free()` during development

#### OBD2 Connection Issues
- **No ELM327 response:** Vehicle ignition must be in ON or ACC position
- **Timeout errors:** Increase timeout to 1000-2000ms in bluetooth.py
- **Parse errors:** ELM327 response format varies by clone (handled in parser.py)
- **MAC address:** Pair with phone first to find correct MAC address

### ESP32 Boot Issues
- **Won't boot with display connected:** GPIO 15 (strapping pin) conflict
  - Must use GPIO 5 for CS instead
- **Import errors on boot:** Check boot.py adds `/src` to sys.path
- **Watchdog resets:** Add yields in long operations

## Adding New PIDs

1. Add PID to `obd2/commands.py` in `OBD2Commands.PIDS` dict
2. Add metadata to `OBD2Commands.PID_INFO` (name, bytes, formula, unit)
3. Add parsing logic to `obd2/parser.py` in `parse_response()`
4. Update `obd_data` dictionary in `main.py` with new field
5. Add data update logic in `obd2_thread()` in `main.py`
6. Add display rendering in `display_thread()` in `main.py`

## Known Limitations

- **PID 0x42 (battery voltage):** Not available on all Corsa D ECUs
- **ELM327 clones:** May have inconsistent AT command support
- **Bluetooth pairing:** Must be done manually (PIN: 1234 or 0000)
- **Display refresh rate:** Limited to ~1-2 Hz for full screen updates (by design to reduce CPU load)
- **No framebuffer:** Direct SPI writes only, no partial screen updates
- **GPIO constraints:** Cannot use GPIO 15 (strapping pin), GPIO 5 required for CS
- **Power requirements:** Display requires 5V for VCC (ESP32 must be powered via USB/VIN)

## Touch UI

### Reconnect Screen
When Bluetooth connection fails:
- Display shows "Connection Failed!" message
- Shows configured MAC address
- Displays error message
- Shows "Reconnect" button (touch to retry)

### Touch Calibration
If touch coordinates are off, run calibration:
```python
touch.calibrate()  # Touch corners when prompted
```

Or set manually in `touch.py`:
```python
touch.set_calibration(x_min=300, x_max=3900, y_min=300, y_max=3900)
```

## Deployment to ESP32

### Required File Structure on ESP32
Upload the **contents** of `src/` directory to ESP32 root:
```
/ (ESP32 root)
├── config.py           # IMPORTANT: Edit MAC address first!
├── main.py
├── display/
│   ├── __init__.py     # REQUIRED for Python module import
│   ├── driver.py
│   ├── colors.py
│   ├── writer.py
│   ├── touch.py
│   ├── button.py
│   ├── dialog.py
│   └── pages.py
└── obd2/
    ├── __init__.py     # REQUIRED for Python module import
    ├── bluetooth.py
    ├── commands.py
    └── parser.py
```

### Upload Methods

#### Option 1: VS Code with MicroPico (Recommended)
1. Configure `.micropico.json` with `"sync_folder": "src"`
2. Use Command Palette: "MicroPico: Upload project to Pico"
3. Reset ESP32

#### Option 2: mpremote
```bash
cd F:\programming\claude\obdeck\src
mpremote connect auto fs cp -r . :
mpremote connect auto reset
```

#### Option 3: Thonny IDE
1. Open Thonny, connect to ESP32
2. Navigate to `src/` folder in local files
3. Select all files and folders
4. Right-click → "Upload to /"

### Pre-Deployment Checklist
- ✅ Edit `config.py` with your ELM327 MAC address
- ✅ Verify `display/__init__.py` exists
- ✅ Verify `obd2/__init__.py` exists
- ✅ CS pin wired to GPIO 5 (not GPIO 15)
- ✅ VCC connected to 5V (not 3.3V)
- ✅ LED/backlight connected to 3.3V or 5V

### Post-Deployment Testing
After uploading, the display should:
1. Show "Opel Corsa D - OBD2" header
2. Display "Connection Failed!" with reconnect button (if vehicle off)
3. Respond to touch input on reconnect button
4. Text and graphics render quickly (no lag)
