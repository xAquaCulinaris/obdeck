"""
Bluetooth Classic SPP Connection for ELM327
Supports both custom BTM firmware and standard MicroPython with pre-pairing

Features:
- Bluetooth Master (BTM) support for custom firmware
- Direct UART fallback for pre-paired devices
- Auto-retry with multiple PINs
- ELM327 initialization
- Connection status tracking
"""

from machine import UART, Pin
import time

# Try to import Bluetooth Master module (custom firmware)
try:
    import btm
    BTM_AVAILABLE = True
    print("BTM (Bluetooth Master) module found - using custom BT firmware")
except ImportError:
    BTM_AVAILABLE = False
    print("BTM module not available - will try direct UART")


class BluetoothConnection:
    """
    Bluetooth SPP connection to ELM327 OBD2 adapter

    Supports two modes:
    1. BTM mode (custom firmware) - Full Bluetooth Master capability
    2. UART mode (standard firmware) - Requires pre-paired device
    """

    def __init__(self, mac_address, pin="1234", auto_try_pins=True, uart_id=2, tx_pin=None, rx_pin=None, device_name=None):
        """
        Initialize Bluetooth connection parameters

        Args:
            mac_address: ELM327 MAC address (e.g., "AA:BB:CC:DD:EE:FF") - for reference only
            pin: Primary PIN code to try (default: "1234")
            auto_try_pins: Try alternative PINs if primary fails
            uart_id: UART ID to use for fallback mode (default: 2)
            tx_pin: TX pin number (optional, uses default if None)
            rx_pin: RX pin number (optional, uses default if None)
            device_name: Bluetooth device name to connect to (e.g., "ESP32", "OBDII")
                        If None, will try common names automatically
        """
        self.mac_address = mac_address
        self.pin = pin
        self.auto_try_pins = auto_try_pins
        self.uart_id = uart_id
        self.tx_pin = tx_pin
        self.rx_pin = rx_pin
        self.device_name = device_name  # Target device name or None for auto-discovery
        self.uart = None
        self.btm_mode = False
        self.connected = False
        self.connection_attempts = 0


    def _scan_btm_devices(self, timeout=10):
        """
        Scan for nearby Bluetooth Classic devices

        Args:
            timeout: Scan timeout in seconds

        Returns:
            list: List of discovered device names
        """
        if not BTM_AVAILABLE:
            return []

        try:
            print(f"Scanning for Bluetooth devices (timeout: {timeout}s)...")

            # Check if BTM has a scan function
            if hasattr(btm, 'scan'):
                devices = btm.scan(timeout)
                if devices:
                    print(f"Found {len(devices)} device(s):")
                    for dev in devices:
                        print(f"  - {dev}")
                    return devices
            else:
                print("BTM module doesn't have scan function")
                return []

        except Exception as e:
            print(f"Scan error: {e}")
            return []


    def _connect_btm(self):
        """
        Connect using Bluetooth Master (BTM) module from custom firmware

        Returns:
            bool: True if connection successful
        """
        if not BTM_AVAILABLE:
            return False

        try:
            print("Using BTM (Bluetooth Master) mode")

            # Print BTM module info for debugging
            print(f"BTM module attributes: {dir(btm)}")

            # Initialize Bluetooth Master
            print("Initializing BTM with name 'OBDECK'...")
            btm.init("OBDECK")
            print("BTM initialized")

            # Start Bluetooth
            print("Starting BTM...")
            btm.up()
            print("BTM started")
            time.sleep(2)  # Give more time for BT to fully start

            # Check BTM status if available
            if hasattr(btm, 'status'):
                status = btm.status()
                print(f"BTM status: {status}")

            # If device name is specified, try only that name
            if self.device_name:
                device_names_to_try = [self.device_name]
                print(f"Using specified device name: '{self.device_name}'")
            else:
                # Try to scan for devices first
                discovered_devices = self._scan_btm_devices(timeout=5)

                # List of device names to try (based on common ELM327 names)
                device_names_to_try = [
                    "ESP32",           # Generic ESP32 name
                    "OBDII",           # Common OBD-II adapter name
                    "ELM327",          # Standard ELM327 name
                    "OBD-II",          # Alternative OBD-II name
                    "OBD2",            # Short OBD name
                    "ESP_SPP_SERVER",  # ESP32 SPP server default name
                ]

                # Add discovered devices to the list (prioritize them)
                if discovered_devices:
                    device_names_to_try = discovered_devices + device_names_to_try

            # Try each device name
            for device_name in device_names_to_try:
                print(f"\n>>> Trying to connect to '{device_name}' with PIN '{self.pin}'...")

                try:
                    # Check if already connected/busy
                    if hasattr(btm, 'ready'):
                        if btm.ready():
                            print("BTM already connected, closing first...")
                            btm.close()
                            time.sleep(1)

                    # Try to open connection
                    print(f"Calling btm.open('{device_name}', '{self.pin}')...")
                    result = btm.open(device_name, self.pin)
                    print(f"btm.open() returned: {result} (type: {type(result)})")

                    # Some implementations might return None instead of False on failure
                    if result or result is None:
                        # Wait longer for connection to establish
                        print(f"BTM open call completed for '{device_name}', waiting for ready...")
                        max_wait = 15  # seconds

                        for i in range(max_wait):
                            try:
                                is_ready = btm.ready()
                                print(f"  [{i+1}/{max_wait}] btm.ready() = {is_ready}")

                                if is_ready:
                                    print(f"\n*** BTM connected successfully to '{device_name}'! ***")
                                    self.btm_mode = True
                                    self.connected = True
                                    self.device_name = device_name  # Store connected device name
                                    return True
                            except Exception as e:
                                print(f"  Error checking ready: {e}")

                            time.sleep(1)

                        print(f"Timeout waiting for BTM ready for '{device_name}'")

                        # Try to close
                        try:
                            btm.close()
                        except:
                            pass
                        time.sleep(0.5)
                    else:
                        print(f"BTM open explicitly failed (returned False) for '{device_name}'")

                except Exception as e:
                    print(f"Exception trying '{device_name}': {e}")
                    import sys
                    sys.print_exception(e)
                    continue

            print("Failed to connect to any device")
            return False

        except Exception as e:
            print(f"BTM connection error: {e}")
            import sys
            sys.print_exception(e)
            return False


    def _connect_uart(self):
        """
        Connect using direct UART (requires pre-paired device)

        Returns:
            bool: True if connection successful
        """
        try:
            print("Using direct UART mode")
            print("NOTE: Device must be pre-paired at OS level!")

            # Initialize UART for Bluetooth SPP communication
            uart_config = {
                'baudrate': 38400,
                'timeout': 2000,
                'bits': 8,
                'parity': None,
                'stop': 1
            }

            # Add TX/RX pins if specified
            if self.tx_pin is not None and self.rx_pin is not None:
                uart_config['tx'] = Pin(self.tx_pin)
                uart_config['rx'] = Pin(self.rx_pin)
                print(f"Using UART{self.uart_id} with TX={self.tx_pin}, RX={self.rx_pin}")
            else:
                print(f"Using UART{self.uart_id} with default pins")

            self.uart = UART(self.uart_id, **uart_config)

            # Clear any existing data
            while self.uart.any():
                self.uart.read()

            # Wait for Bluetooth connection to establish
            print("Waiting for connection...")
            time.sleep(2)

            # Test connection with AT commands
            print("Testing connection with AT command...")
            self.uart.write(b'AT\r\n')
            time.sleep(0.5)

            response = self.uart.read()
            print(f"AT response: {response}")

            if response and (b'OK' in response or b'>' in response):
                self.connected = True
                print("UART connected successfully")
                return True

            # Try ATZ (reset)
            print("Trying ATZ (reset)...")
            self.uart.write(b'ATZ\r\n')
            time.sleep(2)  # Reset takes longer

            response = self.uart.read()
            print(f"ATZ response: {response}")

            if response and (b'ELM' in response or b'>' in response or b'OK' in response):
                self.connected = True
                print("UART connected successfully (via ATZ)")
                return True

            print("No response from UART")
            return False

        except Exception as e:
            print(f"UART connection error: {e}")
            import sys
            sys.print_exception(e)
            return False


    def connect(self):
        """
        Connect to ELM327 via Bluetooth Classic SPP

        Tries BTM mode first (if available), then falls back to UART mode

        Returns:
            bool: True if connection successful
        """
        print(f"Connecting to ELM327 at {self.mac_address}...")
        print(f"Using PIN: {self.pin}")

        # Try BTM mode first (if available)
        if BTM_AVAILABLE:
            if self._connect_btm():
                return True
            print("BTM connection failed, trying UART fallback...")

        # Try UART mode
        if self._connect_uart():
            return True

        # Try alternative PIN if enabled
        if self.auto_try_pins and self.pin != "0000":
            print("\nTrying alternative PIN: 0000")
            self.pin = "0000"
            time.sleep(1)

            # Disconnect first
            if self.uart:
                self.uart.deinit()
                self.uart = None
            if self.btm_mode and BTM_AVAILABLE:
                btm.close()
                self.btm_mode = False

            return self.connect()  # Retry with new PIN

        # Connection failed
        print("\n=== CONNECTION FAILED ===")
        if not BTM_AVAILABLE:
            print("Standard MicroPython does not support Bluetooth Classic client!")
            print("\nSOLUTIONS:")
            print("1. Flash custom MicroPython with BTM support:")
            print("   https://github.com/shariltumin/esp32-bluetooth-classic-micropython")
            print("2. Pre-pair the devices and ensure they auto-connect")
            print("3. Check UART wiring if using external BT module")

        self.connected = False
        self.connection_attempts += 1
        return False


    def send_command(self, command, timeout=1000):
        """
        Send AT/OBD command to ELM327

        Args:
            command: Command string (without carriage return)
            timeout: Response timeout in milliseconds

        Returns:
            str: Response from ELM327, or None on timeout
        """
        if not self.connected:
            return None

        try:
            # BTM mode
            if self.btm_mode and BTM_AVAILABLE:
                # Send command
                cmd_bytes = (command + '\r').encode('utf-8')
                btm.send_bin(cmd_bytes)

                # Wait for response
                start = time.ticks_ms()
                response_data = bytearray()

                while time.ticks_diff(time.ticks_ms(), start) < timeout:
                    if btm.data() > 0:
                        # Read available data
                        chunk = btm.get_bin(btm.data())
                        if chunk:
                            response_data.extend(chunk)

                            # Check if response is complete (ends with '>')
                            if b'>' in response_data:
                                break

                    time.sleep_ms(10)

                if response_data:
                    return response_data.decode('utf-8', 'ignore').strip()

            # UART mode
            elif self.uart:
                # Clear any pending data
                while self.uart.any():
                    self.uart.read()

                # Send command with carriage return
                self.uart.write((command + '\r').encode('utf-8'))

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


    def init_elm327(self):
        """
        Initialize ELM327 adapter with AT command sequence

        Returns:
            bool: True if initialization successful
        """
        if not self.connected:
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
            if self.btm_mode and BTM_AVAILABLE:
                btm.close()
            if self.uart:
                self.uart.deinit()
            self.connected = False
            self.btm_mode = False
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
