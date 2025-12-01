"""
ESP32 OBD2 Display - Main Application
2010 Opel Corsa D Real-time Diagnostics

Hardware:
- ESP32-WROOM-32 (Dual-core 240MHz)
- ILI9488 3.5" TFT Display (480x320 SPI)
- ELM327 Bluetooth OBD2 Adapter

Author: ESP32 OBD2 Project
"""

import time
import _thread
import gc
from machine import Pin, SPI

# Display modules
from display.driver import Display
from display.colors import BLACK, WHITE, GREEN, RED, CYAN, YELLOW
from display.writer import Writer

# OBD2 modules
from obd2.bluetooth import BluetoothConnection
from obd2.commands import OBD2Commands
from obd2.parser import OBD2Parser


# Pin Configuration
PIN_MOSI = 23
PIN_MISO = 19
PIN_SCK = 18
PIN_CS = 15
PIN_DC = 2
PIN_RST = 4
TOUCH_CS = 21

# UART Configuration
BT_TX = 17
BT_RX = 16
UART_ID = 1

# Shared Data Structure (Thread-safe with lock)
obd_data = {
    'coolant_temp': 0.0,      # Celsius
    'rpm': 0,                 # RPM
    'speed': 0,               # km/h
    'battery_voltage': 0.0,   # Volts
    'intake_temp': 0.0,       # Celsius
    'throttle': 0.0,          # Percent
    'connected': False,       # ELM327 connection status
    'error': ''               # Error message
}

# Thread synchronization lock
data_lock = _thread.allocate_lock()


def obd2_thread(uart_id=UART_ID):
    """
    OBD2 Communication Thread (runs on Core 0)
    Queries ELM327 and updates shared data structure

    Args:
        uart_id: UART peripheral ID
    """
    global obd_data, data_lock

    # Initialize Bluetooth connection
    bt_connection = BluetoothConnection(uart_id=uart_id, tx_pin=BT_TX, rx_pin=BT_RX)

    # Initialize ELM327
    if not bt_connection.init_elm327():
        with data_lock:
            obd_data['connected'] = False
            obd_data['error'] = 'ELM327 init failed'
        return

    with data_lock:
        obd_data['connected'] = True
        obd_data['error'] = ''

    # Get supported PIDs
    pid_names = OBD2Commands.get_supported_pids()
    pid_index = 0

    while True:
        try:
            # Query next PID in rotation
            pid_name = pid_names[pid_index]
            command = OBD2Commands.get_pid_command(pid_name)

            if command:
                # Send command and get response
                response = bt_connection.send_command(command)

                if response and OBD2Parser.is_valid_response(response):
                    # Extract PID hex from command (last 2 chars)
                    pid_hex = command[-2:]
                    value = OBD2Parser.parse_response(response, pid_hex)

                    # Update shared data
                    if value is not None:
                        with data_lock:
                            if pid_name == 'COOLANT_TEMP':
                                obd_data['coolant_temp'] = value
                            elif pid_name == 'RPM':
                                obd_data['rpm'] = int(value)
                            elif pid_name == 'SPEED':
                                obd_data['speed'] = int(value)
                            elif pid_name == 'INTAKE_TEMP':
                                obd_data['intake_temp'] = value
                            elif pid_name == 'THROTTLE':
                                obd_data['throttle'] = value
                            elif pid_name == 'BATTERY_VOLTAGE':
                                obd_data['battery_voltage'] = value

            # Move to next PID
            pid_index = (pid_index + 1) % len(pid_names)

            time.sleep(0.5)  # 2 Hz query rate

        except Exception as e:
            print(f"OBD2 thread error: {e}")
            with data_lock:
                obd_data['error'] = str(e)
            time.sleep(1)


def display_thread():
    """
    Display Update Thread (runs on Core 1)
    Renders data from shared structure to ILI9488 display
    """
    global obd_data, data_lock

    # Initialize SPI for display
    spi = SPI(2, baudrate=40000000, polarity=0, phase=0,
              sck=Pin(PIN_SCK), mosi=Pin(PIN_MOSI), miso=Pin(PIN_MISO))

    # Initialize display
    display = Display(spi, cs=Pin(PIN_CS), dc=Pin(PIN_DC), rst=Pin(PIN_RST))
    display.init()
    display.fill(BLACK)

    # Initialize text writer
    writer = Writer(display)

    # Draw static header
    writer.text("Opel Corsa D - OBD2", 10, 10, CYAN, size=2)

    while True:
        try:
            # Read shared data (thread-safe)
            with data_lock:
                coolant = obd_data['coolant_temp']
                rpm = obd_data['rpm']
                speed = obd_data['speed']
                battery = obd_data['battery_voltage']
                connected = obd_data['connected']
                error = obd_data['error']

            # Draw labels (green)
            writer.text("Coolant:", 10, 60, GREEN, size=2)
            writer.text("RPM:", 10, 110, GREEN, size=2)
            writer.text("Speed:", 10, 160, GREEN, size=2)
            writer.text("Battery:", 10, 210, GREEN, size=2)

            # Draw values (white, larger) with padding for overwrite
            writer.text(f"{coolant:.1f} C  ", 150, 60, WHITE, size=3)
            writer.text(f"{rpm} RPM  ", 150, 110, WHITE, size=3)
            writer.text(f"{speed} km/h  ", 150, 160, WHITE, size=3)
            writer.text(f"{battery:.2f} V  ", 150, 210, WHITE, size=3)

            # Status bar
            if connected:
                writer.text("Status: Connected   ", 10, 280, GREEN, size=1)
            else:
                writer.text("Status: Disconnected", 10, 280, RED, size=1)

            if error:
                writer.text(f"Error: {error[:30]}", 10, 300, RED, size=1)
            else:
                writer.text(" " * 40, 10, 300, BLACK, size=1)  # Clear error line

            # Garbage collection
            gc.collect()

            time.sleep(1)  # 1 Hz refresh rate

        except Exception as e:
            print(f"Display thread error: {e}")
            time.sleep(1)


def main():
    """
    Main application entry point
    Starts both OBD2 and display threads
    """
    print("ESP32 OBD2 Display Starting...")
    print("Hardware: Opel Corsa D 2010")
    print("Display: ILI9488 480x320")
    print("OBD2: ELM327 Bluetooth")

    # Start OBD2 thread on second core
    _thread.start_new_thread(obd2_thread, (UART_ID,))

    # Start display thread on main core
    display_thread()


if __name__ == '__main__':
    main()
