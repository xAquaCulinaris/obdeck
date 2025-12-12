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

// NOTE: Display pins (TFT_MOSI, TFT_CS, TFT_RST, etc.) and SPI frequency are
//       configured in include/TFT_eSPI_User_Setup.h (used by TFT_eSPI library)

// IMPORTANT: Display Power Requirements
// - VCC: Connect to 5V (ESP32 VIN), NOT 3.3V
// - LED: Connect to 3.3V
// - RST: DO NOT CONNECT (leave floating) - this display doesn't work with RST
// - Add 100-470µF capacitor between display VCC and GND (close to display)
// - This prevents power spikes when updating screen

// Physical Buttons (3-button navigation system)
#define BTN_LEFT      33  // Navigate to previous UI button (GPIO14)
#define BTN_RIGHT     14  // Navigate to next UI button (GPIO33)
#define BTN_SELECT    26  // Activate highlighted UI button (GPIO26)

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
// Use 1 for normal landscape, use 3 for landscape flipped 180°
#define SCREEN_ROTATION 3  // Landscape flipped 180° (480x320)

// Display Refresh Rate
#define DISPLAY_REFRESH_MS  500    // 500ms = 2 Hz (2 updates per second)

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
#define PID_COOLANT_TEMP        0x05   // Engine coolant temperature
#define PID_RPM                 0x0C   // Engine RPM
#define PID_SPEED               0x0D   // Vehicle speed
#define PID_INTAKE_TEMP         0x0F   // Intake air temperature
#define PID_THROTTLE            0x11   // Throttle position
#define PID_BATTERY_VOLTAGE     0x42   // Control module voltage

// ============================================================================
// THREADING CONFIGURATION
// ============================================================================

// FreeRTOS Task Configuration (OBD2 runs on Core 0, Display runs in main loop on Core 1)
#define OBD2_TASK_STACK_SIZE    8192   // 8KB stack for OBD2 task
#define OBD2_TASK_PRIORITY      1      // Priority 1 (lower)
#define OBD2_TASK_CORE          0      // Run on Core 0

// ============================================================================
// VEHICLE INFORMATION
// ============================================================================

#define VEHICLE_NAME    "Opel Corsa D"
#define VEHICLE_YEAR    2010

#endif // CONFIG_H
