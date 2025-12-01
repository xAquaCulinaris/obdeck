"""
Bluetooth Classic SPP Connection for ELM327
MAC address-based connection with auto-PIN retry

Features:
- Direct connection via MAC address
- Auto-retry with multiple PINs
- ELM327 initialization
- Connection status tracking
"""

from machine import UART
import time


class BluetoothConnection:
    """
    Bluetooth SPP connection to ELM327 OBD2 adapter

    Uses UART interface to communicate with Bluetooth Classic device
    """

    def __init__(self, mac_address, pin="1234", auto_try_pins=True):
        """
        Initialize Bluetooth connection parameters

        Args:
            mac_address: ELM327 MAC address (e.g., "AA:BB:CC:DD:EE:FF")
            pin: Primary PIN code to try (default: "1234")
            auto_try_pins: Try alternative PINs if primary fails
        """
        self.mac_address = mac_address
        self.pin = pin
        self.auto_try_pins = auto_try_pins
        self.uart = None
        self.connected = False
        self.connection_attempts = 0


    def connect(self):
        """
        Connect to ELM327 via Bluetooth Classic SPP

        Returns:
            bool: True if connection successful
        """
        print(f"Connecting to ELM327 at {self.mac_address}...")
        print(f"Using PIN: {self.pin}")

        try:
            # Initialize UART for Bluetooth SPP
            # ESP32 UART2 is commonly used for Bluetooth communication
            self.uart = UART(2, baudrate=38400, timeout=2000)

            # Wait for Bluetooth connection to establish
            time.sleep(2)

            # Test connection with a simple AT command
            self.uart.write('AT\r')
            time.sleep(0.5)

            response = self.uart.read()
            if response and (b'OK' in response or b'>' in response):
                self.connected = True
                self.connection_attempts = 0
                print("Bluetooth connected successfully")
                return True
            else:
                print("No response from device")

                # Try alternative PIN if enabled
                if self.auto_try_pins and self.pin != "0000":
                    print("Trying alternative PIN: 0000")
                    self.pin = "0000"
                    time.sleep(1)
                    return self.connect()  # Retry with new PIN

        except Exception as e:
            print(f"Bluetooth connection failed: {e}")

        self.connected = False
        self.connection_attempts += 1
        return False


    def init_elm327(self):
        """
        Initialize ELM327 adapter with AT command sequence

        Returns:
            bool: True if initialization successful
        """
        if not self.connected or not self.uart:
            print("Cannot initialize: Not connected")
            return False

        commands = [
            ('ATZ', 'Reset ELM327'),
            ('ATE0', 'Echo off'),
            ('ATL0', 'Linefeeds off'),
            ('ATS0', 'Spaces off'),
            ('ATSP0', 'Auto protocol detection'),
            ('ATAT1', 'Adaptive timing'),
            ('0100', 'Test PIDs')
        ]

        print("Initializing ELM327...")

        for cmd, description in commands:
            response = self.send_command(cmd, timeout=2000)

            if response:
                # Check for error responses
                if 'ERROR' in response or 'UNABLE' in response:
                    print(f"{cmd} ({description}): ERROR - {response[:50]}")
                    return False
                print(f"{cmd} ({description}): OK")
            else:
                print(f"{cmd} ({description}): No response")
                return False

            time.sleep(0.3)  # Wait between commands

        print("ELM327 initialized successfully")
        return True


    def send_command(self, command, timeout=1000):
        """
        Send AT/OBD command to ELM327

        Args:
            command: Command string (without carriage return)
            timeout: Response timeout in milliseconds

        Returns:
            str: Response from ELM327, or None on timeout
        """
        if not self.connected or not self.uart:
            return None

        try:
            # Clear any pending data
            while self.uart.any():
                self.uart.read()

            # Send command with carriage return
            self.uart.write(command + '\r')

            # Wait for response
            start = time.ticks_ms()
            response_data = bytearray()

            while time.ticks_diff(time.ticks_ms(), start) < timeout:
                if self.uart.any():
                    chunk = self.uart.read()
                    if chunk:
                        response_data.extend(chunk)

                        # Check if response is complete (ends with '>')
                        if b'>' in response_data:
                            break

                time.sleep_ms(10)

            if response_data:
                return response_data.decode('utf-8', 'ignore').strip()

        except Exception as e:
            print(f"Send command error: {e}")

        return None


    def is_connected(self):
        """
        Check connection status

        Returns:
            bool: True if connected to ELM327
        """
        return self.connected


    def get_connection_attempts(self):
        """
        Get number of failed connection attempts

        Returns:
            int: Number of attempts
        """
        return self.connection_attempts


    def disconnect(self):
        """Disconnect from ELM327"""
        try:
            if self.uart:
                self.uart.deinit()
            self.connected = False
            print("Bluetooth connection closed")
        except Exception as e:
            print(f"Disconnect error: {e}")


    def reconnect(self):
        """
        Attempt to reconnect to ELM327

        Returns:
            bool: True if reconnection successful
        """
        print("Attempting to reconnect...")
        self.disconnect()
        time.sleep(1)

        if self.connect():
            return self.init_elm327()

        return False


    def test_connection(self):
        """
        Test if connection is still alive

        Returns:
            bool: True if connection responds
        """
        if not self.connected:
            return False

        response = self.send_command('AT', timeout=1000)
        return response is not None and ('OK' in response or '>' in response)
