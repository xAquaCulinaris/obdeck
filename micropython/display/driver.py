"""
ILI9488 Display Driver for MicroPython ESP32
3.5" TFT LCD 480x320 SPI Interface

Features:
- Hardware SPI communication
- RGB565 color format
- Basic drawing primitives
- Direct write (no framebuffer)

Datasheet: ILI9488 v1.01
"""

from machine import Pin
import time


# ILI9488 Commands
CMD_NOP         = 0x00
CMD_SWRESET     = 0x01
CMD_RDDID       = 0x04
CMD_SLPOUT      = 0x11
CMD_DISPOFF     = 0x28
CMD_DISPON      = 0x29
CMD_CASET       = 0x2A  # Column address set
CMD_PASET       = 0x2B  # Page address set
CMD_RAMWR       = 0x2C  # Memory write
CMD_MADCTL      = 0x36  # Memory access control
CMD_COLMOD      = 0x3A  # Pixel format set
CMD_IFMODE      = 0xB0  # Interface mode control
CMD_FRMCTR1     = 0xB1  # Frame rate control
CMD_INVON       = 0x21  # Display inversion on
CMD_INVOFF      = 0x20  # Display inversion off


class Display:
    """
    ILI9488 Display Driver Class

    Attributes:
        width (int): Display width in pixels (480)
        height (int): Display height in pixels (320)
    """

    def __init__(self, spi, cs, dc, rst, width=480, height=320):
        """
        Initialize ILI9488 display

        Args:
            spi: SPI object (configured with baudrate, polarity, phase)
            cs: Chip Select pin (Pin object)
            dc: Data/Command pin (Pin object)
            rst: Reset pin (Pin object)
            width: Display width (default 480)
            height: Display height (default 320)
        """
        self.spi = spi
        self.cs = cs
        self.dc = dc
        self.rst = rst
        self.width = width
        self.height = height

        # Configure pins as outputs
        self.cs.init(Pin.OUT, value=1)
        self.dc.init(Pin.OUT, value=0)
        self.rst.init(Pin.OUT, value=1)


    def write_cmd(self, cmd):
        """
        Write command byte to display

        Args:
            cmd: Command byte (8-bit)
        """
        self.dc.value(0)  # Command mode
        self.cs.value(0)  # Select display
        self.spi.write(bytearray([cmd]))
        self.cs.value(1)  # Deselect


    def write_data(self, data):
        """
        Write data byte(s) to display

        Args:
            data: Single byte (int) or bytearray
        """
        self.dc.value(1)  # Data mode
        self.cs.value(0)  # Select display

        if isinstance(data, int):
            self.spi.write(bytearray([data]))
        else:
            self.spi.write(data)

        self.cs.value(1)  # Deselect


    def rgb565_to_rgb666(self, color):
        """
        Convert RGB565 (16-bit) color to RGB666 (18-bit, 3 bytes)

        Args:
            color: RGB565 color value (16-bit)

        Returns:
            Tuple of (R, G, B) bytes for RGB666
        """
        # Extract RGB565 components
        r5 = (color >> 11) & 0x1F
        g6 = (color >> 5) & 0x3F
        b5 = color & 0x1F

        # Scale to 8 bits
        r8 = (r5 << 3) | (r5 >> 2)
        g8 = (g6 << 2) | (g6 >> 4)
        b8 = (b5 << 3) | (b5 >> 2)

        return (r8, g8, b8)


    def reset(self):
        """Hardware reset display"""
        self.rst.value(1)
        time.sleep_ms(10)
        self.rst.value(0)
        time.sleep_ms(10)
        self.rst.value(1)
        time.sleep_ms(120)


    def init(self):
        """
        Initialize display with configuration sequence
        Sets up interface mode, pixel format, and enables display
        """
        print("Initializing ILI9488 display...")

        self.reset()

        # Software reset
        self.write_cmd(CMD_SWRESET)
        time.sleep_ms(120)

        # Exit sleep mode
        self.write_cmd(CMD_SLPOUT)
        time.sleep_ms(120)

        # Interface pixel format: 18-bit/pixel (RGB666)
        self.write_cmd(CMD_COLMOD)
        self.write_data(0x66)  # 18-bit color (RGB666)

        # Memory access control (rotation, RGB/BGR order)
        self.write_cmd(CMD_MADCTL)
        self.write_data(0xE8)  # Landscape (not mirrored): MY=1, MX=1, MV=1, BGR=1

        # Interface mode control
        self.write_cmd(CMD_IFMODE)
        self.write_data(0x00)  # SDI transfer mode

        # Frame rate control (normal mode)
        self.write_cmd(CMD_FRMCTR1)
        self.write_data(0xA0)  # Frame rate ~60Hz

        # Display inversion off
        self.write_cmd(CMD_INVOFF)

        # Display on
        self.write_cmd(CMD_DISPON)
        time.sleep_ms(50)

        print("ILI9488 initialized successfully")


    def set_window(self, x0, y0, x1, y1):
        """
        Set active drawing window (address window)

        Args:
            x0: Start X coordinate
            y0: Start Y coordinate
            x1: End X coordinate
            y1: End Y coordinate
        """
        # Column address set
        self.write_cmd(CMD_CASET)
        self.write_data(x0 >> 8)
        self.write_data(x0 & 0xFF)
        self.write_data(x1 >> 8)
        self.write_data(x1 & 0xFF)

        # Page address set
        self.write_cmd(CMD_PASET)
        self.write_data(y0 >> 8)
        self.write_data(y0 & 0xFF)
        self.write_data(y1 >> 8)
        self.write_data(y1 & 0xFF)

        # Write to RAM
        self.write_cmd(CMD_RAMWR)


    def pixel(self, x, y, color):
        """
        Draw single pixel

        Args:
            x: X coordinate
            y: Y coordinate
            color: RGB565 color value
        """
        if x < 0 or x >= self.width or y < 0 or y >= self.height:
            return

        self.set_window(x, y, x, y)
        r, g, b = self.rgb565_to_rgb666(color)
        self.write_data(r)
        self.write_data(g)
        self.write_data(b)


    def fill(self, color):
        """
        Fill entire display with solid color

        Args:
            color: RGB565 color value
        """
        self.fill_rect(0, 0, self.width, self.height, color)


    def fill_rect(self, x, y, w, h, color):
        """
        Draw filled rectangle

        Args:
            x: Top-left X coordinate
            y: Top-left Y coordinate
            w: Width in pixels
            h: Height in pixels
            color: RGB565 color value
        """
        if x < 0 or y < 0 or x >= self.width or y >= self.height:
            return

        # Clamp to display bounds
        if x + w > self.width:
            w = self.width - x
        if y + h > self.height:
            h = self.height - y

        self.set_window(x, y, x + w - 1, y + h - 1)

        # Convert RGB565 to RGB666
        r, g, b = self.rgb565_to_rgb666(color)

        # Write pixels efficiently in RGB666 format (3 bytes per pixel)
        self.dc.value(1)
        self.cs.value(0)

        chunk_size = 480  # Write in chunks (160 pixels * 3 bytes = 480 bytes)
        chunk = bytearray([r, g, b] * (chunk_size // 3))

        pixels_remaining = w * h
        while pixels_remaining > 0:
            pixels_to_write = min(pixels_remaining, chunk_size // 3)
            self.spi.write(chunk[:pixels_to_write * 3])
            pixels_remaining -= pixels_to_write
            time.sleep_us(50)  # Yield to watchdog

        self.cs.value(1)


    def hline(self, x, y, w, color):
        """
        Draw horizontal line

        Args:
            x: Start X coordinate
            y: Y coordinate
            w: Width in pixels
            color: RGB565 color value
        """
        self.fill_rect(x, y, w, 1, color)


    def vline(self, x, y, h, color):
        """
        Draw vertical line

        Args:
            x: X coordinate
            y: Start Y coordinate
            h: Height in pixels
            color: RGB565 color value
        """
        self.fill_rect(x, y, 1, h, color)


    def rect(self, x, y, w, h, color):
        """
        Draw rectangle outline (not filled)

        Args:
            x: Top-left X coordinate
            y: Top-left Y coordinate
            w: Width in pixels
            h: Height in pixels
            color: RGB565 color value
        """
        self.hline(x, y, w, color)
        self.hline(x, y + h - 1, w, color)
        self.vline(x, y, h, color)
        self.vline(x + w - 1, y, h, color)


    def clear(self):
        """Clear display to black"""
        from display.colors import BLACK
        self.fill(BLACK)
