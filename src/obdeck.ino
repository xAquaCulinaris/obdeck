/**
 * ESP32 OBD2 Display - Main Application
 * 2010 Opel Corsa D Real-time Diagnostics
 *
 * Hardware:
 * - ESP32-WROOM-32 (Dual-core 240MHz)
 * - ILI9488 3.5" TFT Display (480x320 SPI)
 * - ELM327 Bluetooth OBD2 Adapter
 *
 * Author: ESP32 OBD2 Project
 * Platform: PlatformIO + Arduino Framework
 */

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "config.h"
#include "ui.h"

// ============================================================================
// PAGE SYSTEM (enum defined in ui.h)
// ============================================================================

// Current page state
Page current_page = PAGE_DASHBOARD;
bool page_needs_redraw = true;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

BluetoothSerial SerialBT;
ELM327 elm327;
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS);

// ============================================================================
// SHARED DATA STRUCTURE (Thread-safe with mutex)
// ============================================================================

struct OBDData {
    float coolant_temp;      // °C
    uint16_t rpm;            // RPM
    uint8_t speed;           // km/h
    float battery_voltage;   // V
    float intake_temp;       // °C
    float throttle;          // %
    bool connected;          // ELM327 connection status
    char error[64];          // Error message
};

OBDData obd_data = {0};
SemaphoreHandle_t data_mutex;

// ============================================================================
// MANUAL OBD2 QUERIES (Bypass ELMduino bug)
// ============================================================================

/**
 * Send OBD2 command and read response
 */
String sendOBD2Command(String cmd) {
    // Clear input buffer
    while (SerialBT.available()) {
        SerialBT.read();
    }

    // Send command
    SerialBT.print(cmd + "\r");

    // Wait for response
    unsigned long start = millis();
    String response = "";

    while (millis() - start < 2000) {  // 2 second timeout
        if (SerialBT.available()) {
            char c = SerialBT.read();
            response += c;

            // Check for end of response ('>')
            if (c == '>') {
                break;
            }
        }
        delay(1);
    }

    return response;
}

/**
 * Parse hex byte from OBD2 response
 */
int parseHexByte(String response, int byteIndex) {
    // Response format: "41 05 A0 >"
    // "41" = mode response, "05" = PID echo, "A0" = actual data
    int startPos = response.indexOf("41");
    if (startPos < 0) return -1;

    // Skip "41 XX " (mode response + PID + spaces = 6 characters)
    String data = response.substring(startPos + 6);
    data.trim();

    // Find the nth hex byte
    int currentByte = 0;
    int pos = 0;

    while (pos < data.length() && currentByte <= byteIndex) {
        // Skip spaces
        while (pos < data.length() && data[pos] == ' ') pos++;

        if (currentByte == byteIndex) {
            // Found our byte - parse it
            String hexByte = data.substring(pos, pos + 2);
            return strtol(hexByte.c_str(), NULL, 16);
        }

        // Move to next byte
        pos += 2;
        currentByte++;
    }

    return -1;
}

/**
 * Query RPM
 */
int queryRPM() {
    String response = sendOBD2Command("010C");
    int A = parseHexByte(response, 0);
    int B = parseHexByte(response, 1);

    if (A >= 0 && B >= 0) {
        return ((A * 256) + B) / 4;
    }
    return -1;
}

/**
 * Query Speed
 */
int querySpeed() {
    String response = sendOBD2Command("010D");
    int A = parseHexByte(response, 0);
    return A;
}

/**
 * Query Coolant Temperature
 */
float queryCoolantTemp() {
    String response = sendOBD2Command("0105");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return A - 40.0;
    }
    return -999;
}

/**
 * Query Throttle Position
 */
float queryThrottle() {
    String response = sendOBD2Command("0111");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return (A * 100.0) / 255.0;
    }
    return -1;
}

/**
 * Query Intake Air Temperature
 */
float queryIntakeTemp() {
    String response = sendOBD2Command("010F");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return A - 40.0;
    }
    return -999;
}

/**
 * Query Battery Voltage
 */
float queryBatteryVoltage() {
    String response = sendOBD2Command("0142");
    int A = parseHexByte(response, 0);
    int B = parseHexByte(response, 1);

    if (A >= 0 && B >= 0) {
        return ((A * 256) + B) / 1000.0;
    }
    return -1;
}

// ============================================================================
// BLUETOOTH CONNECTION
// ============================================================================

/**
 * Connect to ELM327 via Bluetooth using MAC address
 */
bool connectToELM327() {
    Serial.println("\n========================================");
    Serial.println("Connecting to ELM327...");
    Serial.println("========================================");

#if BT_USE_MAC
    // Connect using MAC address (more reliable)
    Serial.printf("Using MAC address: %s\n", BT_MAC_ADDRESS);

    // Convert MAC string to uint8_t array
    uint8_t mac[6];
    if (sscanf(BT_MAC_ADDRESS, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        Serial.println("ERROR: Invalid MAC address format!");
        return false;
    }

    // Connect via MAC
    Serial.println("Attempting connection...");
    bool connected = SerialBT.connect(mac);

    Serial.printf("SerialBT.connect() returned: %s\n", connected ? "true" : "false");
    Serial.printf("SerialBT.connected() = %s\n", SerialBT.connected() ? "true" : "false");

    if (!connected) {
        Serial.println("ERROR: Bluetooth connection failed!");

        // Check if we're actually connected despite connect() returning false
        delay(1000);
        if (SerialBT.connected()) {
            Serial.println("WARNING: connect() failed but SerialBT.connected() is true!");
            Serial.println("Proceeding with connection...");
        } else {
            return false;
        }
    }
#else
    // Connect using device name
    Serial.printf("Using device name: %s\n", BT_DEVICE_NAME);

    bool connected = SerialBT.connect(BT_DEVICE_NAME);
    Serial.printf("SerialBT.connect() returned: %s\n", connected ? "true" : "false");

    if (!connected) {
        Serial.println("ERROR: Bluetooth connection failed!");
        return false;
    }
#endif

    Serial.println("✓ Bluetooth connected successfully!");
    Serial.printf("Connection status: %s\n", SerialBT.connected() ? "CONNECTED" : "DISCONNECTED");

    // Wait for connection to stabilize
    Serial.println("Waiting for connection to stabilize...");
    delay(ELM327_INIT_DELAY_MS);

    Serial.printf("After delay - connected: %s\n", SerialBT.connected() ? "true" : "false");

    // Initialize ELM327
    Serial.println("\nInitializing ELM327...");

    // Note: Pass false (not 1) for debug to avoid extra characters in queries
    if (!elm327.begin(SerialBT, false, ELM327_TIMEOUT_MS)) {
        Serial.println("ERROR: ELM327 initialization failed!");
        Serial.printf("ELM327 Status: %d\n", elm327.nb_rx_state);
        return false;
    }

    Serial.println("✓ ELM327 initialized successfully!");

    return true;
}

// ============================================================================
// OBD2 TASK (Core 0)
// ============================================================================

/**
 * OBD2 Communication Task
 * Runs on Core 0, queries ELM327 and updates shared data
 */
void obd2Task(void *parameter) {
    Serial.println("[OBD2 Task] Starting on Core 0...");

    // Connect to ELM327
    if (!connectToELM327()) {
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        obd_data.connected = false;
        snprintf(obd_data.error, sizeof(obd_data.error), "Connection failed");
        xSemaphoreGive(data_mutex);

        Serial.println("[OBD2 Task] Connection failed, task ending");
        vTaskDelete(NULL);
        return;
    }

    // Mark as connected
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    obd_data.connected = true;
    obd_data.error[0] = '\0';
    xSemaphoreGive(data_mutex);

    // PID query rotation
    uint8_t pid_index = 0;
    const uint8_t pids[] = {
        PID_RPM,
        PID_SPEED,
        PID_COOLANT_TEMP,
        PID_THROTTLE,
        PID_INTAKE_TEMP,
        PID_BATTERY_VOLTAGE
    };
    const uint8_t num_pids = sizeof(pids) / sizeof(pids[0]);

    Serial.println("[OBD2 Task] Starting query loop...\n");

    while (true) {
        uint8_t current_pid = pids[pid_index];
        float value = 0;
        bool success = false;

        // Query PID using manual functions (bypass ELMduino bug)
        switch (current_pid) {
            case PID_RPM:
                {
                    int rpm = queryRPM();
                    if (rpm >= 0) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.rpm = rpm;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("RPM: %d\n", rpm);
                        success = true;
                    }
                }
                break;

            case PID_SPEED:
                {
                    int spd = querySpeed();
                    if (spd >= 0) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.speed = spd;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("Speed: %d km/h\n", spd);
                        success = true;
                    }
                }
                break;

            case PID_COOLANT_TEMP:
                {
                    float temp = queryCoolantTemp();
                    if (temp > -100) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.coolant_temp = temp;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("Coolant: %.1f°C\n", temp);
                        success = true;
                    }
                }
                break;

            case PID_THROTTLE:
                {
                    float thr = queryThrottle();
                    if (thr >= 0) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.throttle = thr;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("Throttle: %.1f%%\n", thr);
                        success = true;
                    }
                }
                break;

            case PID_INTAKE_TEMP:
                {
                    float temp = queryIntakeTemp();
                    if (temp > -100) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.intake_temp = temp;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("Intake: %.1f°C\n", temp);
                        success = true;
                    }
                }
                break;

            case PID_BATTERY_VOLTAGE:
                {
                    float volt = queryBatteryVoltage();
                    if (volt > 0) {
                        xSemaphoreTake(data_mutex, portMAX_DELAY);
                        obd_data.battery_voltage = volt;
                        xSemaphoreGive(data_mutex);
                        Serial.printf("Battery: %.1fV\n", volt);
                        success = true;
                    }
                }
                break;
        }

        // Check for errors
        if (!success) {
            Serial.printf("PID 0x%02X query failed\n", current_pid);
        }

        // Move to next PID
        pid_index = (pid_index + 1) % num_pids;

        // Wait before next query
        vTaskDelay(pdMS_TO_TICKS(OBD2_QUERY_INTERVAL_MS));
    }
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

/**
 * Initialize display
 */
void initDisplay() {
    Serial.println("Initializing display...");

    tft.init();
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BLACK);

    Serial.println("✓ Display initialized");
}

/**
 * Draw current page with smart partial updates (no flickering!)
 */
void drawCurrentPage() {
    static int draw_count = 0;
    static uint16_t last_rpm = 0xFFFF;
    static uint8_t last_speed = 0xFF;
    static float last_coolant = -999;
    static float last_throttle = -999;
    static float last_battery = -999;
    static float last_intake = -999;
    static bool last_connected = false;
    static bool needs_full_redraw = true;

    draw_count++;

    // Get data copy (thread-safe)
    OBDData data_copy;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    data_copy = obd_data;
    xSemaphoreGive(data_mutex);

    // Debug output every 10 draws
    if (draw_count % 10 == 1) {
        Serial.printf("[Display] drawCurrentPage called (count=%d, connected=%d, page=%d)\n",
                      draw_count, data_copy.connected, current_page);
    }

    // Check if full redraw is needed
    bool do_full_redraw = (page_needs_redraw || data_copy.connected != last_connected || needs_full_redraw);

    // Determine page name
    const char* page_name;
    switch (current_page) {
        case PAGE_DASHBOARD: page_name = "Dashboard"; break;
        case PAGE_DTC:       page_name = "DTC Codes"; break;
        case PAGE_CONFIG:    page_name = "Config"; break;
        default:             page_name = "Unknown"; break;
    }

    // Determine status color
    uint16_t status_color = data_copy.connected ? STATUS_OK : STATUS_ERROR;
    int dtc_count = 0;  // TODO: Implement DTC reading

    // Full redraw if page changed or connection state changed
    if (do_full_redraw) {
        Serial.println("[Display] Full screen redraw");
        tft.fillScreen(COLOR_BLACK);

        // Draw top bar
        drawTopBar(VEHICLE_NAME, page_name, status_color, dtc_count);

        // Draw bottom navigation
        drawBottomNav(current_page);

        // Reset flags
        page_needs_redraw = false;
        last_connected = data_copy.connected;
        needs_full_redraw = false;

        // Force redraw of all values
        last_rpm = 0xFFFF;
        last_speed = 0xFF;
        last_coolant = -999;
        last_throttle = -999;
        last_battery = -999;
        last_intake = -999;
    }

    if (!data_copy.connected) {
        // Show connection error
        tft.setTextColor(COLOR_RED, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(80, 140);
        tft.print("NOT CONNECTED");

        if (data_copy.error[0] != '\0') {
            tft.setTextSize(1);
            tft.setCursor(10, 180);
            tft.printf("Error: %s", data_copy.error);
        }
    } else {
        // Draw page content based on current page
        if (current_page == PAGE_DASHBOARD) {
            // Smart partial updates for dashboard
            bool is_full_redraw = (last_rpm == 0xFFFF);  // Check if this is first draw
            drawDashboardPageSmart(
                data_copy.rpm, last_rpm,
                data_copy.speed, last_speed,
                data_copy.coolant_temp, last_coolant,
                data_copy.throttle, last_throttle,
                data_copy.battery_voltage, last_battery,
                data_copy.intake_temp, last_intake,
                is_full_redraw
            );

            // Update last values
            last_rpm = data_copy.rpm;
            last_speed = data_copy.speed;
            last_coolant = data_copy.coolant_temp;
            last_throttle = data_copy.throttle;
            last_battery = data_copy.battery_voltage;
            last_intake = data_copy.intake_temp;
        }
    }
}

/**
 * Draw dashboard with smart partial updates and boxed layout
 */
void drawDashboardPageSmart(uint16_t rpm, uint16_t last_rpm,
                             uint8_t speed, uint8_t last_speed,
                             float coolant, float last_coolant,
                             float throttle, float last_throttle,
                             float battery, float last_battery,
                             float intake, float last_intake,
                             bool force_full_redraw) {

    static bool first_draw = true;

    // Reset first_draw if full redraw requested
    if (force_full_redraw) {
        first_draw = true;
    }

    // Layout: 2 columns x 3 rows with boxes
    const int margin = 5;
    const int box_width = (SCREEN_WIDTH - 3 * margin) / 2;  // 2 columns
    const int box_height = (CONTENT_HEIGHT - 4 * margin) / 3;  // 3 rows
    const int start_y = CONTENT_Y_START + margin;

    // Helper function to draw a metric box
    auto drawMetricBox = [&](int col, int row, const char* label, const char* value,
                             const char* last_value, uint16_t label_color, bool force_redraw) {
        int x = margin + col * (box_width + margin);
        int y = start_y + row * (box_height + margin);

        // Draw box border and label only on first draw
        if (first_draw || force_redraw) {
            // Box border
            tft.drawRect(x, y, box_width, box_height, COLOR_GRAY);

            // Label at top
            tft.setTextColor(label_color, COLOR_BLACK);
            tft.setTextSize(2);
            tft.setCursor(x + 10, y + 8);
            tft.print(label);
        }

        // Update value only if changed
        if (first_draw || force_redraw || strcmp(value, last_value) != 0) {
            // Clear value area
            int value_y = y + 35;
            tft.fillRect(x + 5, value_y, box_width - 10, box_height - 40, COLOR_BLACK);

            // Draw new value (large and centered)
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(3);  // Size 3 for better fit
            int text_width = strlen(value) * 18;  // Approximate width for size 3
            int value_x = x + (box_width - text_width) / 2;
            tft.setCursor(value_x, value_y + 10);
            tft.print(value);
        }
    };

    // Format values
    char rpm_val[16], last_rpm_val[16];
    snprintf(rpm_val, sizeof(rpm_val), "%d", rpm);
    snprintf(last_rpm_val, sizeof(last_rpm_val), "%d", last_rpm);

    char speed_val[16], last_speed_val[16];
    snprintf(speed_val, sizeof(speed_val), "%d", speed);
    snprintf(last_speed_val, sizeof(last_speed_val), "%d", last_speed);

    char coolant_val[16], last_coolant_val[16];
    snprintf(coolant_val, sizeof(coolant_val), "%.1f", coolant);
    snprintf(last_coolant_val, sizeof(last_coolant_val), "%.1f", last_coolant);

    char throttle_val[16], last_throttle_val[16];
    snprintf(throttle_val, sizeof(throttle_val), "%.0f%%", throttle);
    snprintf(last_throttle_val, sizeof(last_throttle_val), "%.0f%%", last_throttle);

    char battery_val[16], last_battery_val[16];
    snprintf(battery_val, sizeof(battery_val), "%.1fV", battery);
    snprintf(last_battery_val, sizeof(last_battery_val), "%.1fV", last_battery);

    char intake_val[16], last_intake_val[16];
    snprintf(intake_val, sizeof(intake_val), "%.1f", intake);
    snprintf(last_intake_val, sizeof(last_intake_val), "%.1f", last_intake);

    // Draw all metrics in grid layout
    // Row 0
    drawMetricBox(0, 0, "RPM", rpm_val, last_rpm_val, COLOR_CYAN, false);
    drawMetricBox(1, 0, "Speed (km/h)", speed_val, last_speed_val, COLOR_CYAN, false);

    // Row 1
    drawMetricBox(0, 1, "Coolant (C)", coolant_val, last_coolant_val, COLOR_GREEN, false);
    drawMetricBox(1, 1, "Throttle", throttle_val, last_throttle_val, COLOR_YELLOW, false);

    // Row 2
    drawMetricBox(0, 2, "Battery", battery_val, last_battery_val, COLOR_GREEN, false);
    drawMetricBox(1, 2, "Intake (C)", intake_val, last_intake_val, COLOR_GREEN, false);

    first_draw = false;
}

/**
 * Handle touch input for page navigation
 */
void handleTouch() {
    if (touch.touched()) {
        TS_Point p = touch.getPoint();

        // Map touch coordinates to screen coordinates
        // Note: Calibration may be needed - these are approximate values
        int x = map(p.x, 300, 3900, 0, SCREEN_WIDTH);
        int y = map(p.y, 300, 3900, 0, SCREEN_HEIGHT);

        // Check if touch is in bottom navigation area
        if (y >= BOTTOM_NAV_Y) {
            // Determine which button was pressed
            Page new_page = current_page;

            if (x < NAV_DTC_X) {
                // Dashboard button
                new_page = PAGE_DASHBOARD;
            } else if (x < NAV_CONFIG_X) {
                // DTC button
                new_page = PAGE_DTC;
            } else {
                // Config button
                new_page = PAGE_CONFIG;
            }

            // Change page if different
            if (new_page != current_page) {
                current_page = new_page;
                page_needs_redraw = true;

                Serial.printf("[Touch] Page changed to: %d\n", current_page);
            }
        }

        // Debounce - wait for touch release (with timeout)
        unsigned long debounce_start = millis();
        while (touch.touched() && (millis() - debounce_start < 1000)) {
            delay(10);
        }
        delay(100);  // Additional debounce delay
    }
}

// ============================================================================
// ARDUINO SETUP & LOOP
// ============================================================================

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("ESP32 OBD2 Display");
    Serial.println("========================================");
    Serial.printf("Hardware: %s %d\n", VEHICLE_NAME, VEHICLE_YEAR);
    Serial.printf("Display: ILI9488 %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    Serial.println("OBD2: ELM327 Bluetooth");
    Serial.println("========================================\n");

    // Create mutex for shared data
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        Serial.println("ERROR: Failed to create mutex!");
        while (1) delay(1000);
    }

    // Initialize Bluetooth Serial in MASTER mode
    if (!SerialBT.begin("OBDECK", true)) {  // true = master/client mode
        Serial.println("ERROR: Bluetooth initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("✓ Bluetooth Serial initialized (Master mode)");

    // Initialize display
    initDisplay();

    // Initialize touch controller
    if (!touch.begin()) {
        Serial.println("WARNING: Touch controller not found!");
        // Continue anyway - touch may not be required for basic operation
    } else {
        Serial.println("✓ Touch controller initialized");
    }

    // Show startup message
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(80, 100);
    tft.print("OBDECK");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(100, 140);
    tft.print("Connecting...");

    delay(2000);

    // Start OBD2 task on Core 0
    xTaskCreatePinnedToCore(
        obd2Task,                // Task function
        "OBD2Task",              // Task name
        OBD2_TASK_STACK_SIZE,    // Stack size
        NULL,                    // Parameters
        OBD2_TASK_PRIORITY,      // Priority
        NULL,                    // Task handle
        OBD2_TASK_CORE           // Core (0)
    );

    Serial.println("✓ OBD2 task started on Core 0");
    Serial.println("\nSetup complete! Entering main loop...\n");
}

void loop() {
    // Display task runs on Core 1 (main loop)
    static unsigned long last_update = 0;
    static bool first_draw = true;
    static bool loop_started = false;

    // Debug: Confirm loop is running
    if (!loop_started) {
        Serial.println("[Loop] Main loop started on Core 1");
        Serial.printf("[Loop] millis=%lu, DISPLAY_REFRESH_MS=%d\n", millis(), DISPLAY_REFRESH_MS);
        loop_started = true;
    }

    // TEMPORARILY DISABLED: Handle touch input (check frequently for responsiveness)
    // handleTouch();

    // Force immediate first draw to clear startup screen
    if (first_draw) {
        Serial.println("[Display] First draw - clearing startup screen");
        drawCurrentPage();
        last_update = millis();
        first_draw = false;
    }
    // Update display at refresh rate
    else if (millis() - last_update >= DISPLAY_REFRESH_MS) {
        drawCurrentPage();
        last_update = millis();
    }

    delay(10);  // Small delay to prevent WDT issues
}
