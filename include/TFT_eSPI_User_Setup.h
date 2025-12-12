/**
 * TFT_eSPI User Setup
 *
 * This file configures TFT_eSPI for the ILI9488 display
 * Place this in your project and reference it in platformio.ini
 */

// Define the driver (ILI9488 - 320x480 RGB666)
#define ILI9488_DRIVER

// Display resolution
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// ESP32 Pin Configuration
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   5   // Chip Select
#define TFT_DC   2   // Data/Command
#define TFT_RST  -1  // Reset disabled - this specific display doesn't work with RST connected

// Touch disabled (using physical buttons)
#define TOUCH_CS -1  // Required by TFT_eSPI even when not using touch

// Use VSPI (default for ESP32)
#define VSPI_HOST

// Fonts to be available
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH
#define LOAD_FONT8  // Font 8. Large 75 pixel font, needs ~3256 bytes in FLASH

// Smooth fonts
#define SMOOTH_FONT

// SPI Frequency (Testing 10 MHz for better performance)
#define SPI_FREQUENCY        10000000  // 10 MHz (increased from 250 kHz)
#define SPI_READ_FREQUENCY   10000000  // 10 MHz
#define SPI_TOUCH_FREQUENCY  2500000   // 2.5 MHz (conservative for touch if ever used)

// Color depth (16-bit RGB565)
#define TFT_RGB_ORDER TFT_BGR  // ILI9488 uses BGR ordering
