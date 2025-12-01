"""
Bluetooth UART Communication for ELM327
Handles connection and low-level communication with ELM327 adapter

Features:
- UART initialization
- ELM327 initialization sequence
- Send/receive commands
- Connection management
"""

from machine import UART
import time


class BluetoothConnection:
    """
    Bluetooth UART connection handler for ELM327

    Attributes:
        uart: UART object for communication
        connected: Connection status
    """

    def __init__(self, uart_id=1, tx_pin=17, rx_pin=16, baudrate=38400):
        """
        Initialize Bluetooth UART connection

        Args:
            uart_id: UART peripheral ID (1 or 2)
            tx_pin: TX pin number
            rx_pin: RX pin number
            baudrate: Communication speed (default 38400 for ELM327)
        """
        self.uart = UART(uart_id, baudrate=baudrate, tx=tx_pin, rx=rx_pin, timeout=1000)
        self.connected = False
        self.tx_pin = tx_pin
        self.rx_pin = rx_pin
        self.baudrate = baudrate


    def init_elm327(self):
        """
        Initialize ELM327 adapter with AT command sequence

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
            response = self.send_command(cmd)
            if response:
                print(f"{cmd}: {response[:50]}")  # Limit output
            else:
                print(f"{cmd}: No response")
                self.connected = False
                return False

            time.sleep(0.3)  # Wait between commands

        print("ELM327 initialized successfully")
        self.connected = True
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
        # Clear any pending data
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

        return None


    def is_connected(self):
        """
        Check connection status

        Returns:
            bool: True if connected to ELM327
        """
        return self.connected


    def disconnect(self):
        """Disconnect from ELM327"""
        self.uart.deinit()
        self.connected = False
        print("Bluetooth connection closed")


    def reconnect(self):
        """
        Attempt to reconnect to ELM327

        Returns:
            bool: True if reconnection successful
        """
        print("Attempting to reconnect...")
        self.uart = UART(1, baudrate=self.baudrate, tx=self.tx_pin, rx=self.rx_pin, timeout=1000)
        time.sleep(1)
        return self.init_elm327()
