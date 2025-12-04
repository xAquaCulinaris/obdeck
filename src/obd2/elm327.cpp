/**
 * ELM327 OBD2 Communication Module - Implementation
 */

#include "elm327.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

BluetoothSerial SerialBT;
ELM327 elm327;
OBDData obd_data = {0};
SemaphoreHandle_t data_mutex;

// ============================================================================
// INITIALIZATION
// ============================================================================

void initELM327() {
    // Create mutex for shared data
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        Serial.println("ERROR: Failed to create data mutex!");
        while (1) delay(1000);
    }

    // Initialize Bluetooth Serial in MASTER mode
    if (!SerialBT.begin("OBDECK", true)) {  // true = master/client mode
        Serial.println("ERROR: Bluetooth initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("✓ Bluetooth Serial initialized (Master mode)");
}

// ============================================================================
// OBD2 COMMUNICATION
// ============================================================================

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

// ============================================================================
// PID QUERIES
// ============================================================================

int queryRPM() {
    String response = sendOBD2Command("010C");
    int A = parseHexByte(response, 0);
    int B = parseHexByte(response, 1);

    if (A >= 0 && B >= 0) {
        return ((A * 256) + B) / 4;
    }
    return -1;
}

int querySpeed() {
    String response = sendOBD2Command("010D");
    int A = parseHexByte(response, 0);
    return A;
}

float queryCoolantTemp() {
    String response = sendOBD2Command("0105");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return A - 40.0;
    }
    return -999;
}

float queryThrottle() {
    String response = sendOBD2Command("0111");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return (A * 100.0) / 255.0;
    }
    return -1;
}

float queryIntakeTemp() {
    String response = sendOBD2Command("010F");
    int A = parseHexByte(response, 0);

    if (A >= 0) {
        return A - 40.0;
    }
    return -999;
}

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
// CONNECTION MANAGEMENT
// ============================================================================

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

    // Track consecutive failures to detect disconnection
    int consecutive_failures = 0;
    const int MAX_FAILURES_BEFORE_DISCONNECT = 3;

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

        // Check for errors and track consecutive failures
        if (!success) {
            Serial.printf("PID 0x%02X query failed\n", current_pid);
            consecutive_failures++;

            // If too many consecutive failures, assume disconnected
            if (consecutive_failures >= MAX_FAILURES_BEFORE_DISCONNECT) {
                Serial.printf("[OBD2 Task] %d consecutive failures - connection lost!\n", consecutive_failures);

                // Mark as disconnected
                xSemaphoreTake(data_mutex, portMAX_DELAY);
                obd_data.connected = false;
                snprintf(obd_data.error, sizeof(obd_data.error), "Connection lost (timeout)");
                xSemaphoreGive(data_mutex);

                // Close existing connection
                SerialBT.end();
                delay(1000);

                // Reinitialize Bluetooth
                SerialBT.begin("OBDECK", true);
                delay(1000);

                // Wait before attempting reconnect
                Serial.println("[OBD2 Task] Waiting 5 seconds before reconnect...");
                vTaskDelay(pdMS_TO_TICKS(5000));

                // Attempt to reconnect
                Serial.println("[OBD2 Task] Attempting to reconnect...");
                if (connectToELM327()) {
                    Serial.println("[OBD2 Task] Reconnected successfully!");
                    xSemaphoreTake(data_mutex, portMAX_DELAY);
                    obd_data.connected = true;
                    obd_data.error[0] = '\0';
                    xSemaphoreGive(data_mutex);
                    consecutive_failures = 0;  // Reset failure counter
                } else {
                    Serial.println("[OBD2 Task] Reconnection failed, will retry...");
                    consecutive_failures = 0;  // Reset to try again
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue;  // Skip to next iteration
                }
            }
        } else {
            // Success - reset failure counter
            consecutive_failures = 0;
        }

        // Move to next PID
        pid_index = (pid_index + 1) % num_pids;

        // Wait before next query
        vTaskDelay(pdMS_TO_TICKS(OBD2_QUERY_INTERVAL_MS));
    }
}
