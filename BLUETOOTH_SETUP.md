# Bluetooth Setup Guide

## ESP32 Bluetooth Classic with ELM327

This project uses **Bluetooth Classic SPP (Serial Port Profile)** to communicate with the ELM327 OBD2 adapter wirelessly.

## Important Notes

### MicroPython Bluetooth Limitations

Standard MicroPython for ESP32 primarily supports **Bluetooth Low Energy (BLE)**, not Bluetooth Classic. For Bluetooth Classic SPP connectivity with ELM327, you have several options:

### Option 1: Pre-Paired Connection (Recommended)

1. **Pair ELM327 with ESP32 via your phone/computer first:**
   - Use Android Bluetooth settings to pair with ELM327
   - Default PIN is usually `1234` or `0000`
   - Note the ELM327 MAC address

2. **ESP32 connects automatically:**
   - Once paired, ESP32 can connect via UART
   - The connection is maintained wirelessly

### Option 2: Custom MicroPython Build

Use a MicroPython build with Bluetooth Classic support:
- **ESP32-IDF MicroPython** with Bluetooth Classic SPP enabled
- Build from source with `CONFIG_BT_CLASSIC_ENABLED=y`
- Requires custom firmware flashing

### Option 3: Use Arduino-ESP32 Instead

Consider using Arduino framework instead of MicroPython:
- Full Bluetooth Classic SPP support out of the box
- `BluetoothSerial` library available
- Same hardware, different programming environment

## Configuration

Edit `src/config.py` to match your ELM327 device:

```python
# ELM327 device name (check with Bluetooth scanner)
BT_DEVICE_NAME = "OBDII"  # Common: "OBDII", "OBD2", "ELM327", "CHX"

# Pairing PIN
BT_PIN = "1234"  # Common: "1234", "0000", "6789"
```

## Checking Your ELM327 Device

**Device Name:** Use your phone's Bluetooth settings to see the advertised name
**PIN:** Check the ELM327 manual or try common PINs (`1234`, `0000`)
**MAC Address:** Note this for direct connection

## Troubleshooting

### "Bluetooth connection failed"
- Ensure ELM327 is powered (vehicle ignition ON)
- Check if ELM327 is already paired with another device
- Verify Bluetooth is enabled on ESP32
- Try resetting ELM327 (unplug OBD2 connector for 10 seconds)

### "No response from ELM327"
- Check baud rate (should be 38400)
- Verify ELM327 firmware version (AT@1 command)
- Ensure vehicle is in ignition ON or engine running state

### Alternative: Wired Connection

If Bluetooth proves problematic, you can use a wired UART ELM327 adapter:
- Connect ELM327 TX → ESP32 GPIO16 (RX)
- Connect ELM327 RX → ESP32 GPIO17 (TX)
- Connect GND
- Modify `bluetooth.py` to skip Bluetooth initialization

## Recommended ELM327 Adapters

For best compatibility:
- **Genuine ELM327 v1.5 or v2.1** (not cheap clones)
- Bluetooth Classic (not BLE-only versions)
- Supports SPP profile

Avoid cheap clones with fake firmware - they often have bugs in AT command handling.
