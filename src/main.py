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
from display.colors import BLACK, WHITE, GREEN, RED, CYAN, YELLOW, GRAY
from display.writer import Writer
from display.touch import Touch
from display.button import Button
from display.pages import PageManager

# OBD2 modules
from obd2.bluetooth import BluetoothConnection
from obd2.commands import OBD2Commands
from obd2.parser import OBD2Parser

# Configuration
import config


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
    'error': '',              # Error message
    'dtc_codes': [],          # List of DTC codes (e.g., ['P0420', 'P0133'])
    'dtc_count': 0,           # Number of DTCs
    'dtc_read': False,        # Flag: DTCs have been read
    'dtc_refresh': False,     # Flag: User requested DTC refresh
    'dtc_clear': False        # Flag: User requested DTC clear
}

# Thread synchronization lock
data_lock = _thread.allocate_lock()


def read_dtcs(bt_connection):
    """
    Read diagnostic trouble codes from vehicle

    Args:
        bt_connection: Bluetooth connection object
    """
    global obd_data, data_lock

    print("Reading DTCs...")

    # Send Mode 03 command (read stored DTCs)
    response = bt_connection.send_command('03', timeout=3000)

    if response:
        # Parse DTCs
        dtc_list = OBD2Parser.parse_dtc_response(response)

        with data_lock:
            obd_data['dtc_codes'] = dtc_list
            obd_data['dtc_count'] = len(dtc_list)
            obd_data['dtc_read'] = True

        print(f"Found {len(dtc_list)} DTCs: {dtc_list}")
    else:
        print("No DTC response")
        with data_lock:
            obd_data['dtc_codes'] = []
            obd_data['dtc_count'] = 0
            obd_data['dtc_read'] = True


def clear_dtcs(bt_connection):
    """
    Clear diagnostic trouble codes

    Args:
        bt_connection: Bluetooth connection object
    """
    global obd_data, data_lock

    print("Clearing DTCs...")

    # Send Mode 04 command (clear DTCs)
    response = bt_connection.send_command('04', timeout=3000)

    if response and '44' in response:
        print("DTCs cleared successfully")

        # Re-read DTCs to confirm
        time.sleep(1)
        read_dtcs(bt_connection)
    else:
        print("Failed to clear DTCs")


def obd2_thread():
    """
    OBD2 Communication Thread (runs on Core 0)
    Queries ELM327 and updates shared data structure via Bluetooth
    """
    global obd_data, data_lock

    # Initialize Bluetooth connection with config values
    bt_connection = BluetoothConnection(
        mac_address=config.BT_MAC_ADDRESS,
        pin=config.BT_PIN,
        auto_try_pins=config.BT_AUTO_TRY_PINS
    )

    # Connect to ELM327 via Bluetooth
    if not bt_connection.connect():
        with data_lock:
            obd_data['connected'] = False
            obd_data['error'] = 'Bluetooth connection failed'
        return

    # Initialize ELM327
    if not bt_connection.init_elm327():
        with data_lock:
            obd_data['connected'] = False
            obd_data['error'] = 'ELM327 init failed'
        return

    with data_lock:
        obd_data['connected'] = True
        obd_data['error'] = ''

    # Wait 5 seconds after connection, then read DTCs once
    time.sleep(5)
    read_dtcs(bt_connection)

    # Get supported PIDs
    pid_names = OBD2Commands.get_supported_pids()
    pid_index = 0

    while True:
        try:
            # Check for DTC refresh request
            refresh_requested = False
            clear_requested = False

            with data_lock:
                if obd_data['dtc_refresh']:
                    refresh_requested = True
                    obd_data['dtc_refresh'] = False

                if obd_data['dtc_clear']:
                    clear_requested = True
                    obd_data['dtc_clear'] = False

            # Handle DTC operations
            if clear_requested:
                clear_dtcs(bt_connection)
            elif refresh_requested:
                read_dtcs(bt_connection)

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
    Renders multi-page UI with navigation
    Handles touch input for page switching and actions
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

    # Initialize touch controller
    touch = Touch(spi, cs_pin=Pin(TOUCH_CS), width=480, height=320)

    # Initialize page manager
    page_manager = PageManager(display, writer)

    # Create reconnect button for connection failures
    reconnect_btn = Button(
        x=140, y=180, width=200, height=60,
        text="Reconnect",
        display=display, writer=writer,
        bg_color=RED, text_color=WHITE, text_size=2
    )
    reconnect_btn.set_visible(False)

    # State tracking
    show_reconnect_screen = False
    last_touch_time = 0
    last_page_draw = 0

    while True:
        try:
            # Read shared data (thread-safe)
            with data_lock:
                data_snapshot = obd_data.copy()

            connected = data_snapshot['connected']
            error = data_snapshot.get('error', '')

            # Check if we need to show reconnect screen
            if not connected and not show_reconnect_screen:
                show_reconnect_screen = True
                display.fill(BLACK)
                writer.text(config.VEHICLE_NAME, 10, 10, CYAN, size=2)
                writer.text("Connection Failed!", 120, 100, RED, size=2)
                writer.text(f"MAC: {config.BT_MAC_ADDRESS[:17]}", 60, 140, WHITE, size=1)
                if error:
                    writer.text(f"Error: {error[:30]}", 30, 170, YELLOW, size=1)
                reconnect_btn.set_visible(True)

            elif connected and show_reconnect_screen:
                # Connection restored - switch to dashboard
                show_reconnect_screen = False
                reconnect_btn.set_visible(False)
                display.fill(BLACK)
                page_manager.switch_page(0)  # Dashboard
                last_page_draw = 0  # Force redraw

            # Handle touch input
            touch_pos = touch.get_touch()
            if touch_pos:
                touch_x, touch_y = touch_pos
                current_time = time.ticks_ms()

                # Debounce touch (500ms)
                if time.ticks_diff(current_time, last_touch_time) > 500:
                    last_touch_time = current_time

                    if show_reconnect_screen:
                        # Handle reconnect button
                        if reconnect_btn.is_touched(touch_x, touch_y):
                            reconnect_btn.press()
                            time.sleep_ms(200)
                            reconnect_btn.release()

                            with data_lock:
                                obd_data['error'] = 'Reconnecting...'
                            display.fill(BLACK)
                            writer.text(config.VEHICLE_NAME, 10, 10, CYAN, size=2)
                            writer.text("Reconnecting...", 150, 150, YELLOW, size=2)
                    else:
                        # Handle navigation bar touch
                        nav_pressed = page_manager.handle_nav_touch(touch_x, touch_y)

                        if nav_pressed:
                            last_page_draw = 0  # Force redraw on page change

                        # Handle page-specific touches
                        if not nav_pressed:
                            action = page_manager.handle_page_touch(touch_x, touch_y)

                            if action == 'refresh':
                                # Request DTC refresh
                                with data_lock:
                                    obd_data['dtc_refresh'] = True
                                print("DTC refresh requested")

                            elif action == 'confirm_clear':
                                # User confirmed - request DTC clear
                                with data_lock:
                                    obd_data['dtc_clear'] = True
                                print("DTC clear confirmed and requested")
                                last_page_draw = 0  # Force redraw

                            elif action == 'cancel_clear':
                                # User cancelled - redraw page
                                print("DTC clear cancelled")
                                last_page_draw = 0  # Force redraw

                            elif action == 'show_confirm':
                                # Confirmation dialog shown - no action needed
                                pass

            # Draw pages (only when connected)
            if connected and not show_reconnect_screen:
                # Redraw page periodically or after data changes
                current_time = time.ticks_ms()
                if time.ticks_diff(current_time, last_page_draw) > 500:  # 2 Hz update
                    page_manager.draw(data_snapshot)
                    last_page_draw = current_time

            # Garbage collection
            gc.collect()

            time.sleep_ms(100)  # 10 Hz for touch responsiveness

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
    _thread.start_new_thread(obd2_thread, ())

    # Start display thread on main core
    display_thread()


if __name__ == '__main__':
    main()
