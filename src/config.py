"""
Configuration Settings for ESP32 OBD2 Display

Modify these settings for your specific hardware and preferences
"""

# Display Configuration
PIN_MOSI = 23
PIN_MISO = 19
PIN_SCK = 18
PIN_CS = 15
PIN_DC = 2
PIN_RST = 4
TOUCH_CS = 21

# Bluetooth Configuration
# ELM327 MAC address - Find this by pairing with your phone first
# Format: "AA:BB:CC:DD:EE:FF"
# Example dummy MAC (replace with your actual ELM327 MAC):
BT_MAC_ADDRESS = "00:1D:A5:68:98:8B"

# ELM327 pairing PIN - Try this PIN when connecting
# Common PINs: "1234", "0000", "6789"
# If you know your ELM327's PIN, set it here
BT_PIN = "1234"

# Auto-try alternative PINs if primary fails
BT_AUTO_TRY_PINS = True  # Will try "0000" if BT_PIN fails

# Display Refresh Rate (Hz)
DISPLAY_REFRESH_RATE = 1  # 1 Hz = 1 update per second

# OBD2 Query Rate (seconds between queries)
OBD2_QUERY_INTERVAL = 0.5  # 2 Hz = 2 queries per second

# ELM327 Command Timeout (milliseconds)
ELM327_TIMEOUT = 2000  # 2 seconds

# Vehicle Information
VEHICLE_NAME = "Opel Corsa D"
VEHICLE_YEAR = 2010
