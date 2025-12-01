# Setup Guide - ESP32 OBD2 Display

## Quick Start

### 1. Find Your ELM327 MAC Address

**Method 1: Using your phone**
1. Turn on vehicle ignition (or start engine)
2. Open Bluetooth settings on your phone
3. Scan for devices - look for "OBDII", "OBD2", or "ELM327"
4. Pair with the device (PIN usually `1234` or `0000`)
5. Once paired, tap device info to view MAC address
6. Note the MAC address (format: `AA:BB:CC:DD:EE:FF`)

**Method 2: Using Bluetooth scanner app**
- Android: "BLE Scanner" or "nRF Connect"
- iOS: "LightBlue" app
- Look for device broadcasting as "OBDII" or "ELM327"

### 2. Configure the Project

Edit `src/config.py`:

```python
# Replace with YOUR ELM327 MAC address
BT_MAC_ADDRESS = "00:1D:A5:68:98:8B"  # ← Change this!

# Set your ELM327 PIN (try these in order: 1234, 0000, 6789)
BT_PIN = "1234"

# Auto-try "0000" if "1234" fails
BT_AUTO_TRY_PINS = True  # Recommended: leave enabled
```

### 3. Upload to ESP32

**Using Thonny IDE:**
1. Connect ESP32 via USB
2. Tools → Options → Interpreter → Select "MicroPython (ESP32)"
3. Right-click `src` folder → Upload to /
4. Ensure all subdirectories (display/, obd2/) are uploaded

**Using ampy:**
```bash
ampy --port COM3 put src /
```

**Using rshell:**
```bash
rshell --port COM3
> cp -r src/* /pyboard/
```

### 4. Test the System

1. **Connect ESP32 to power** (5V USB-C powerbank, 2A minimum)
2. **Turn on vehicle ignition** (or start engine)
3. **Wait for Bluetooth connection** (5-10 seconds)
4. **Watch display:**
   - Success: Shows "Status: Connected" + live OBD2 data
   - Failure: Shows "Connection Failed!" + "Reconnect" button

### 5. If Connection Fails

**On-screen options:**
- Tap the red "Reconnect" button to retry connection

**Troubleshooting:**
1. Verify MAC address in `config.py` matches your ELM327
2. Check vehicle ignition is ON
3. Ensure ELM327 is not paired with another device (phone/tablet)
4. Try changing `BT_PIN` to `"0000"` in config.py
5. Power cycle ELM327 (unplug from OBD2 port for 10 seconds)

## Touch Calibration (If Needed)

If touch button doesn't respond correctly:

1. Connect to ESP32 via serial console (115200 baud)
2. In MicroPython REPL:
```python
from machine import Pin, SPI
from display.touch import Touch

spi = SPI(2, baudrate=40000000, polarity=0, phase=0, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
touch = Touch(spi, cs_pin=Pin(21))

# Run calibration
touch.calibrate()
# Follow prompts: touch top-left, then bottom-right corners
```

3. Note the calibration values printed
4. Edit `src/display/touch.py` lines 37-40 with your values

## Understanding the Display

**Main Screen (when connected):**
```
┌────────────────────────────────────┐
│ Opel Corsa D                       │ ← Vehicle name (config.py)
├────────────────────────────────────┤
│ Coolant: 85.0 C                    │
│ RPM: 1500 RPM                      │
│ Speed: 45 km/h                     │
│ Battery: 14.2 V                    │
├────────────────────────────────────┤
│ Status: Connected                  │ ← Green when connected
└────────────────────────────────────┘
```

**Reconnect Screen (when disconnected):**
```
┌────────────────────────────────────┐
│ Opel Corsa D                       │
├────────────────────────────────────┤
│ Connection Failed!                 │ ← Red text
│ MAC: 00:1D:A5:68:98:8B            │ ← Your configured MAC
│ Error: Bluetooth connection failed │
│                                    │
│    ┌──────────────┐               │
│    │  Reconnect   │               │ ← Touch to retry
│    └──────────────┘               │
└────────────────────────────────────┘
```

## Customization

**Change vehicle name:**
Edit `config.py`:
```python
VEHICLE_NAME = "My Car Name"
```

**Adjust display refresh rate:**
```python
DISPLAY_REFRESH_RATE = 1  # Hz (updates per second)
```

**Add more PIDs:**
See CLAUDE.md section "Adding New PIDs"

## Performance

- **Connection time:** 5-10 seconds on first boot
- **OBD2 query rate:** ~2 PIDs per second (ELM327 limited)
- **Display refresh:** 10 Hz for touch responsiveness
- **Touch debounce:** 500ms (prevents accidental double-taps)

## Common Issues

### "Connection Failed!" on every boot
- **Cause:** Wrong MAC address in config.py
- **Fix:** Double-check MAC address from phone pairing

### Touch button not responding
- **Cause:** Touch calibration off
- **Fix:** Run `touch.calibrate()` (see above)

### Display shows garbage/wrong colors
- **Cause:** Wrong SPI pins or polarity
- **Fix:** Verify pin connections match config.py

### ELM327 keeps disconnecting
- **Cause:** Cheap clone adapter with firmware bugs
- **Solution:** Use genuine ELM327 v1.5 or v2.1

## Hardware Connections

**Display (ILI9488):**
```
ESP32 Pin → Display Pin
MOSI (23) → SDI/MOSI
MISO (19) → SDO/MISO
SCK  (18) → SCK
CS   (15) → CS
DC   (2)  → DC
RST  (4)  → RST
3.3V      → VCC
GND       → GND
```

**Touch (XPT2046):**
```
ESP32 Pin → Touch Pin
(shares SPI with display)
CS   (21) → T_CS
```

**Power:**
```
ESP32 VIN ← 5V USB-C Powerbank (2A minimum)
```

**No wires to ELM327!** Connection is wireless via Bluetooth.

## Support

- **Documentation:** See CLAUDE.md for architecture details
- **Bluetooth issues:** See BLUETOOTH_SETUP.md
- **Project README:** readme_obd2_project.md

## Next Steps

Once working:
- Calibrate touch if needed
- Customize vehicle name
- Add more PIDs for additional sensors
- Implement DTC (diagnostic trouble code) reading
