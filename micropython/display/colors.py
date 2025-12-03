"""
Color Constants for RGB565 Format
Used by ILI9488 display driver

RGB565: 5 bits red, 6 bits green, 5 bits blue
Total: 65536 colors (16-bit)
"""

# Primary Colors
BLACK   = 0x0000
WHITE   = 0xFFFF
RED     = 0xF800
GREEN   = 0x07E0
BLUE    = 0x001F

# Secondary Colors
CYAN    = 0x07FF
MAGENTA = 0xF81F
YELLOW  = 0xFFE0

# Additional Colors
GRAY    = 0x8410
ORANGE  = 0xFD20
PURPLE  = 0x801F
PINK    = 0xFC18
BROWN   = 0xA145
