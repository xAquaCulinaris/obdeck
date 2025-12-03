"""
XPT2046 Touch Controller Driver
Resistive touch screen controller for ILI9488 display

Features:
- SPI communication with touch controller
- Raw touch coordinate reading
- Touch detection
- Calibration support
"""

from machine import Pin, SPI
import time


class Touch:
    """
    XPT2046 Touch Controller Driver

    Connects via SPI to read touch coordinates
    """

    # XPT2046 Commands
    CMD_X_READ  = 0xD0  # Read X coordinate
    CMD_Y_READ  = 0x90  # Read Y coordinate
    CMD_Z1_READ = 0xB0  # Read Z1 (pressure)
    CMD_Z2_READ = 0xC0  # Read Z2 (pressure)

    def __init__(self, spi, cs_pin, width=480, height=320):
        """
        Initialize touch controller

        Args:
            spi: SPI object (shared with display)
            cs_pin: Touch CS pin (Pin object)
            width: Display width for coordinate mapping
            height: Display height for coordinate mapping
        """
        self.spi = spi
        self.cs = cs_pin
        self.width = width
        self.height = height

        # Configure CS pin
        self.cs.init(Pin.OUT, value=1)

        # Calibration values (adjust these for your display)
        # These map raw touch coordinates to display coordinates
        self.cal_x_min = 300
        self.cal_x_max = 3900
        self.cal_y_min = 300
        self.cal_y_max = 3900

        # Touch pressure threshold
        self.pressure_threshold = 400


    def read_raw(self, command):
        """
        Read raw value from touch controller

        Args:
            command: XPT2046 command byte

        Returns:
            int: 12-bit raw value
        """
        self.cs.value(0)  # Select touch controller
        time.sleep_us(10)

        # Send command and read 2 bytes response
        self.spi.write(bytearray([command]))
        data = self.spi.read(2)

        self.cs.value(1)  # Deselect
        time.sleep_us(10)

        if data and len(data) == 2:
            # Combine two bytes into 12-bit value
            value = (data[0] << 8 | data[1]) >> 3
            return value

        return 0


    def read_touch_raw(self):
        """
        Read raw touch coordinates

        Returns:
            tuple: (x, y, pressure) or None if not touched
        """
        # Read multiple samples for stability
        samples = 3
        x_sum = 0
        y_sum = 0
        z_sum = 0

        for _ in range(samples):
            x_sum += self.read_raw(self.CMD_X_READ)
            y_sum += self.read_raw(self.CMD_Y_READ)
            z1 = self.read_raw(self.CMD_Z1_READ)
            z2 = self.read_raw(self.CMD_Z2_READ)

            # Calculate pressure (z1 and z2 used to determine touch pressure)
            if z1 > 0:
                pressure = z2 / z1
            else:
                pressure = 0

            z_sum += pressure
            time.sleep_ms(1)

        # Average the samples
        x_raw = x_sum // samples
        y_raw = y_sum // samples
        pressure = z_sum / samples

        return (x_raw, y_raw, pressure)


    def is_touched(self):
        """
        Check if screen is currently being touched

        Returns:
            bool: True if touched
        """
        x_raw, y_raw, pressure = self.read_touch_raw()
        return pressure > self.pressure_threshold


    def get_touch(self):
        """
        Get calibrated touch coordinates

        Returns:
            tuple: (x, y) in display coordinates, or None if not touched
        """
        x_raw, y_raw, pressure = self.read_touch_raw()

        # Check if touch pressure is sufficient
        if pressure < self.pressure_threshold:
            return None

        # Map raw coordinates to display coordinates
        x = self._map_value(x_raw, self.cal_x_min, self.cal_x_max, 0, self.width)
        y = self._map_value(y_raw, self.cal_y_min, self.cal_y_max, 0, self.height)

        # Clamp to display bounds
        x = max(0, min(self.width - 1, x))
        y = max(0, min(self.height - 1, y))

        return (x, y)


    def _map_value(self, value, in_min, in_max, out_min, out_max):
        """
        Map value from one range to another

        Args:
            value: Input value
            in_min: Input range minimum
            in_max: Input range maximum
            out_min: Output range minimum
            out_max: Output range maximum

        Returns:
            int: Mapped value
        """
        return int((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)


    def calibrate(self):
        """
        Interactive calibration routine
        Touch corners of screen when prompted

        Returns:
            tuple: (x_min, x_max, y_min, y_max) calibration values
        """
        print("Touch calibration:")
        print("Touch top-left corner...")
        time.sleep(2)

        # Wait for touch on top-left
        while not self.is_touched():
            time.sleep_ms(50)

        x1, y1, _ = self.read_touch_raw()
        print(f"Top-left: X={x1}, Y={y1}")
        time.sleep(1)

        print("Touch bottom-right corner...")
        time.sleep(2)

        # Wait for touch on bottom-right
        while not self.is_touched():
            time.sleep_ms(50)

        x2, y2, _ = self.read_touch_raw()
        print(f"Bottom-right: X={x2}, Y={y2}")

        # Update calibration
        self.cal_x_min = min(x1, x2)
        self.cal_x_max = max(x1, x2)
        self.cal_y_min = min(y1, y2)
        self.cal_y_max = max(y1, y2)

        print(f"Calibration complete:")
        print(f"X range: {self.cal_x_min} - {self.cal_x_max}")
        print(f"Y range: {self.cal_y_min} - {self.cal_y_max}")

        return (self.cal_x_min, self.cal_x_max, self.cal_y_min, self.cal_y_max)


    def wait_for_touch(self, timeout=None):
        """
        Wait for user to touch screen

        Args:
            timeout: Maximum wait time in seconds (None = infinite)

        Returns:
            tuple: (x, y) coordinates or None on timeout
        """
        start = time.time()

        while True:
            touch = self.get_touch()
            if touch:
                return touch

            if timeout and (time.time() - start) > timeout:
                return None

            time.sleep_ms(50)


    def set_calibration(self, x_min, x_max, y_min, y_max):
        """
        Set calibration values manually

        Args:
            x_min: X minimum raw value
            x_max: X maximum raw value
            y_min: Y minimum raw value
            y_max: Y maximum raw value
        """
        self.cal_x_min = x_min
        self.cal_x_max = x_max
        self.cal_y_min = y_min
        self.cal_y_max = y_max
