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
#include "config.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

BluetoothSerial SerialBT;
ELM327 elm327;
TFT_eSPI tft = TFT_eSPI();

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
    // Remove "41 " prefix and split by spaces
    int startPos = response.indexOf("41");
    if (startPos < 0) return -1;

    String data = response.substring(startPos + 3);  // Skip "41 "
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
 * Draw dashboard UI
 */
void drawDashboard() {
    // Get data copy (thread-safe)
    OBDData data_copy;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    data_copy = obd_data;
    xSemaphoreGive(data_mutex);

    // Clear screen
    tft.fillScreen(COLOR_BLACK);

    // Header
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.printf("%s - OBD2", VEHICLE_NAME);

    // Check connection status
    if (!data_copy.connected) {
        tft.setTextColor(COLOR_RED, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(80, 140);
        tft.print("NOT CONNECTED");

        if (data_copy.error[0] != '\0') {
            tft.setTextSize(1);
            tft.setCursor(10, 180);
            tft.printf("Error: %s", data_copy.error);
        }

        return;
    }

    // RPM
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 60);
    tft.print("RPM:");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(120, 55);
    tft.printf("%4d", data_copy.rpm);

    // Speed
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 110);
    tft.print("Speed:");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(120, 105);
    tft.printf("%3d km/h", data_copy.speed);

    // Coolant Temperature
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 160);
    tft.print("Coolant:");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(120, 155);
    tft.printf("%5.1f C", data_copy.coolant_temp);

    // Battery Voltage
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 210);
    tft.print("Battery:");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(120, 205);
    tft.printf("%4.1fV", data_copy.battery_voltage);

    // Status indicator
    tft.fillCircle(460, 20, 10, COLOR_GREEN);
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

    if (millis() - last_update >= DISPLAY_REFRESH_MS) {
        drawDashboard();
        last_update = millis();
    }

    delay(10);  // Small delay to prevent WDT issues
}
