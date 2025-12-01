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
from machine import Pin, SPI, UART

from ili9488 import ILI9488, BLACK, WHITE, GREEN, RED, CYAN, YELLOW
from writer import Writer


# Pin Configuration
PIN_MOSI = 23
PIN_MISO = 19
PIN_SCK = 18
PIN_CS = 15
PIN_DC = 2
PIN_RST = 4
TOUCH_CS = 21

# UART Configuration (if using wired ELM327)
BT_RX = 16
BT_TX = 17
UART_BAUD = 38400

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


def init_elm327(uart):
    """
    Initialize ELM327 adapter with AT commands

    Args:
        uart: UART object for ELM327 communication

    Returns:
        bool: True if initialization successful
    """
    commands = [
        'ATZ',      # Reset ELM327
        'ATE0',     # Echo off
        'ATL0',     # Linefeeds off
        'ATS0',     # Spaces off
        'ATSP0',    # Auto protocol detection
        'ATAT1',    # Adaptive timing auto
        '0100'      # Test: Supported PIDs
    ]

    print("Initializing ELM327...")

    for cmd in commands:
        uart.write(cmd + '\r')
        time.sleep(0.5)
        response = uart.read()
        if response:
            print(f"{cmd}: {response.decode('utf-8', 'ignore').strip()}")
        else:
            print(f"{cmd}: No response")
            return False

    print("ELM327 initialized successfully")
    return True


def query_pid(uart, mode, pid):
    """
    Query a PID from the vehicle ECU

    Args:
        uart: UART object
        mode: OBD mode (e.g., '01' for current data)
        pid: PID code (e.g., '05' for coolant temp)

    Returns:
        str: Response from ELM327, or None on timeout
    """
    command = f"{mode}{pid}\r"
    uart.write(command)

    time.sleep(0.3)  # Wait for response

    response = uart.read()
    if response:
        return response.decode('utf-8', 'ignore').strip()
    return None


def parse_pid_response(response, pid):
    """
    Parse ELM327 response and convert to value

    Args:
        response: Raw response string from ELM327
        pid: PID code to determine parsing method

    Returns:
        Parsed value or None on error
    """
    if not response or 'NO DATA' in response:
        return None

    # Remove spaces and extract hex data
    data = response.replace(' ', '').replace('>', '').replace('\r', '').replace('\n', '')

    # Response format: 41[PID][DATA]
    if not data.startswith('41'):
        return None

    try:
        # Extract data bytes after mode and PID
        hex_data = data[4:]

        if pid == '05':  # Coolant temp: A - 40
            a = int(hex_data[0:2], 16)
            return a - 40

        elif pid == '0C':  # RPM: (A*256 + B) / 4
            a = int(hex_data[0:2], 16)
            b = int(hex_data[2:4], 16)
            return (a * 256 + b) / 4

        elif pid == '0D':  # Speed: A
            a = int(hex_data[0:2], 16)
            return a

        elif pid == '0F':  # Intake air temp: A - 40
            a = int(hex_data[0:2], 16)
            return a - 40

        elif pid == '11':  # Throttle: (A * 100) / 255
            a = int(hex_data[0:2], 16)
            return (a * 100) / 255

        elif pid == '42':  # Battery voltage: (A*256 + B) / 1000
            a = int(hex_data[0:2], 16)
            b = int(hex_data[2:4], 16)
            return (a * 256 + b) / 1000

    except (ValueError, IndexError) as e:
        print(f"Parse error for PID {pid}: {e}")
        return None

    return None


def obd2_thread(uart_id=1):
    """
    OBD2 Communication Thread (runs on Core 0)
    Queries ELM327 and updates shared data structure

    Args:
        uart_id: UART peripheral ID (1 or 2)
    """
    global obd_data, data_lock

    # Initialize UART for ELM327
    uart = UART(uart_id, baudrate=UART_BAUD, tx=BT_TX, rx=BT_RX, timeout=1000)

    # Initialize ELM327
    if not init_elm327(uart):
        with data_lock:
            obd_data['connected'] = False
            obd_data['error'] = 'ELM327 init failed'
        return

    with data_lock:
        obd_data['connected'] = True
        obd_data['error'] = ''

    # PID query rotation
    pids = ['05', '0C', '0D', '0F', '11', '42']
    pid_index = 0

    while True:
        try:
            # Query next PID in rotation
            pid = pids[pid_index]
            response = query_pid(uart, '01', pid)

            if response:
                value = parse_pid_response(response, pid)

                # Update shared data
                with data_lock:
                    if pid == '05' and value is not None:
                        obd_data['coolant_temp'] = value
                    elif pid == '0C' and value is not None:
                        obd_data['rpm'] = int(value)
                    elif pid == '0D' and value is not None:
                        obd_data['speed'] = int(value)
                    elif pid == '0F' and value is not None:
                        obd_data['intake_temp'] = value
                    elif pid == '11' and value is not None:
                        obd_data['throttle'] = value
                    elif pid == '42' and value is not None:
                        obd_data['battery_voltage'] = value

            # Move to next PID
            pid_index = (pid_index + 1) % len(pids)

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
    display = ILI9488(spi, cs=Pin(PIN_CS), dc=Pin(PIN_DC), rst=Pin(PIN_RST))
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

            # Draw values (white, larger)
            writer.text(f"{coolant:.1f} C  ", 150, 60, WHITE, size=3)
            writer.text(f"{rpm} RPM  ", 150, 110, WHITE, size=3)
            writer.text(f"{speed} km/h  ", 150, 160, WHITE, size=3)
            writer.text(f"{battery:.2f} V  ", 150, 210, WHITE, size=3)

            # Status bar
            if connected:
                writer.text("Status: Connected", 10, 280, GREEN, size=1)
            else:
                writer.text("Status: Disconnected", 10, 280, RED, size=1)

            if error:
                writer.text(f"Error: {error[:30]}", 10, 300, RED, size=1)

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
    _thread.start_new_thread(obd2_thread, (1,))

    # Start display thread on main core
    display_thread()


if __name__ == '__main__':
    main()
