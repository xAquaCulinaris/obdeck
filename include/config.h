/**
 * Configuration File
 * ESP32 OBD2 Display - Opel Corsa D 2010
 *
 * Hardware pins, Bluetooth settings, and display configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// HARDWARE PIN CONFIGURATION
// ============================================================================

// Display SPI Pins (ILI9488)
#define TFT_MOSI      23
#define TFT_MISO      19
#define TFT_SCLK      18
#define TFT_CS        5   // Changed from GPIO15 (strapping pin issue)
#define TFT_DC        2
#define TFT_RST       4

// Touch Controller (XPT2046)
#define TOUCH_CS      21
#define TOUCH_IRQ     -1  // Not connected

// SPI Configuration
#define SPI_FREQUENCY       40000000  // 40 MHz for display
#define SPI_READ_FREQUENCY  20000000  // 20 MHz for reading
#define SPI_TOUCH_FREQUENCY  2500000  // 2.5 MHz for touch

// ============================================================================
// BLUETOOTH CONFIGURATION
// ============================================================================

// ELM327 Device Settings
#define BT_DEVICE_NAME      "OBDII"                    // Device name (fallback)
#define BT_MAC_ADDRESS      "9C:9C:1F:C7:63:A6"       // MAC address (primary)
#define BT_USE_MAC          true                       // Use MAC instead of name
#define BT_PIN              "1234"                     // PIN (often not needed)

// Connection Settings
#define BT_RECONNECT_DELAY_MS   5000   // Wait 5s before reconnect attempt
#define BT_CONNECT_TIMEOUT_MS   10000  // 10s timeout for connection

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================

// Screen Dimensions
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   320

// Display Rotation
// 0 = Portrait, 1 = Landscape, 2 = Portrait flipped, 3 = Landscape flipped
#define SCREEN_ROTATION 1  // Landscape (480x320)

// Display Settings
#define DISPLAY_REFRESH_HZ  2      // 2 Hz = 2 updates per second
#define DISPLAY_REFRESH_MS  (1000 / DISPLAY_REFRESH_HZ)

// Color Definitions (RGB565)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_GRAY      0x7BEF
#define COLOR_DARKGRAY  0x39E7

// ============================================================================
// OBD2 CONFIGURATION
// ============================================================================

// ELM327 Settings
#define ELM327_BAUD_RATE        38400  // ELM327 standard baud rate
#define ELM327_TIMEOUT_MS       2000   // 2s timeout for commands
#define ELM327_INIT_DELAY_MS    2000   // Wait 2s after connection

// OBD2 Query Settings
#define OBD2_QUERY_INTERVAL_MS  200    // 5 Hz = query every 200ms (faster updates!)
#define OBD2_MAX_RETRIES        3      // Retry failed queries 3 times

// Supported PIDs (Mode 01)
#define PID_SUPPORTED_PIDS      0x00   // PIDs supported [01-20]
#define PID_COOLANT_TEMP        0x05   // Engine coolant temperature
#define PID_RPM                 0x0C   // Engine RPM
#define PID_SPEED               0x0D   // Vehicle speed
#define PID_INTAKE_TEMP         0x0F   // Intake air temperature
#define PID_THROTTLE            0x11   // Throttle position
#define PID_BATTERY_VOLTAGE     0x42   // Control module voltage

// ============================================================================
// THREADING CONFIGURATION
// ============================================================================

// FreeRTOS Task Configuration
#define OBD2_TASK_STACK_SIZE    8192   // 8KB stack for OBD2 task
#define DISPLAY_TASK_STACK_SIZE 8192   // 8KB stack for display task

#define OBD2_TASK_PRIORITY      1      // Priority 1 (lower)
#define DISPLAY_TASK_PRIORITY   1      // Priority 1 (lower)

#define OBD2_TASK_CORE          0      // Run on Core 0
#define DISPLAY_TASK_CORE       1      // Run on Core 1 (with Arduino loop)

// ============================================================================
// VEHICLE INFORMATION
// ============================================================================

#define VEHICLE_NAME    "Opel Corsa D"
#define VEHICLE_YEAR    2010

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

#define DEBUG_SERIAL            true   // Enable serial debug output
#define DEBUG_BLUETOOTH         true   // Debug Bluetooth connection
#define DEBUG_OBD2              false  // Debug OBD2 queries (causes issues with ELMduino)
#define DEBUG_DISPLAY           false  // Debug display updates (verbose)

#endif // CONFIG_H
