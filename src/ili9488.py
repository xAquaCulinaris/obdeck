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


# Color Constants (RGB565 format)
BLACK   = 0x0000
WHITE   = 0xFFFF
RED     = 0xF800
GREEN   = 0x07E0
BLUE    = 0x001F
CYAN    = 0x07FF
MAGENTA = 0xF81F
YELLOW  = 0xFFE0
GRAY    = 0x8410
ORANGE  = 0xFD20


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


class ILI9488:
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

        # Interface pixel format: 16-bit/pixel (RGB565)
        self.write_cmd(CMD_COLMOD)
        self.write_data(0x55)  # 16-bit color

        # Memory access control (rotation, RGB/BGR order)
        self.write_cmd(CMD_MADCTL)
        self.write_data(0x48)  # MX, BGR

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
        self.write_data(color >> 8)
        self.write_data(color & 0xFF)


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

        # Prepare color bytes
        color_high = color >> 8
        color_low = color & 0xFF

        # Write pixels efficiently
        self.dc.value(1)
        self.cs.value(0)

        chunk_size = 512  # Write in chunks
        chunk = bytearray([color_high, color_low] * (chunk_size // 2))

        pixels_remaining = w * h
        while pixels_remaining > 0:
            pixels_to_write = min(pixels_remaining, chunk_size // 2)
            self.spi.write(chunk[:pixels_to_write * 2])
            pixels_remaining -= pixels_to_write

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
        self.fill(BLACK)
