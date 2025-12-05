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
// DTC FUNCTIONS
// ============================================================================

/**
 * Parse DTC value to code string
 * DTC format: 2 bytes encode the code
 * First 2 bits: prefix (00=P, 01=C, 10=B, 11=U)
 * Next 2 bits: first digit
 * Last 12 bits: last 3 digits
 */
void parseDTC(uint16_t dtc_value, char* code) {
    // Extract parts
    uint8_t prefix = (dtc_value >> 14) & 0x03;
    uint8_t digit1 = (dtc_value >> 12) & 0x03;
    uint8_t digit2 = (dtc_value >> 8) & 0x0F;
    uint8_t digit3 = (dtc_value >> 4) & 0x0F;
    uint8_t digit4 = dtc_value & 0x0F;

    // Determine prefix letter
    char prefix_char;
    switch (prefix) {
        case 0: prefix_char = 'P'; break;  // Powertrain
        case 1: prefix_char = 'C'; break;  // Chassis
        case 2: prefix_char = 'B'; break;  // Body
        case 3: prefix_char = 'U'; break;  // Network
        default: prefix_char = 'P'; break;
    }

    // Format code
    snprintf(code, 6, "%c%d%X%X%X", prefix_char, digit1, digit2, digit3, digit4);
}

/**
 * Get DTC description from code
 */
const char* getDTCDescription(const char* code) {
    // Common OBD2 codes database (add more as needed)
    if (strcmp(code, "P0133") == 0) return "O2 Sensor Slow Response";
    if (strcmp(code, "P0244") == 0) return "Wastegate Solenoid";
    if (strcmp(code, "P0300") == 0) return "Random Misfire Detected";
    if (strcmp(code, "P0420") == 0) return "Catalyst System Low Efficiency";
    if (strcmp(code, "P0171") == 0) return "System Too Lean (Bank 1)";
    if (strcmp(code, "P0172") == 0) return "System Too Rich (Bank 1)";
    if (strcmp(code, "P0440") == 0) return "EVAP System Malfunction";
    if (strcmp(code, "P0442") == 0) return "EVAP System Small Leak";
    if (strcmp(code, "P0455") == 0) return "EVAP System Large Leak";
    if (strcmp(code, "P0101") == 0) return "MAF Sensor Range/Performance";
    if (strcmp(code, "P0113") == 0) return "Intake Air Temp High Input";
    if (strcmp(code, "P0128") == 0) return "Coolant Temp Below Thermostat";

    return "Unknown DTC";  // Default for unknown codes
}

/**
 * Get DTC severity level
 */
uint8_t getDTCSeverity(const char* code) {
    // Critical codes (engine damage risk)
    if (strcmp(code, "P0300") == 0) return DTC_SEVERITY_CRITICAL;  // Misfire
    if (strcmp(code, "P0301") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 1 misfire
    if (strcmp(code, "P0302") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 2 misfire
    if (strcmp(code, "P0303") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 3 misfire
    if (strcmp(code, "P0304") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 4 misfire

    // Warning codes (performance/emissions)
    if (strcmp(code, "P0420") == 0) return DTC_SEVERITY_WARNING;   // Catalyst
    if (strcmp(code, "P0171") == 0) return DTC_SEVERITY_WARNING;   // Lean
    if (strcmp(code, "P0172") == 0) return DTC_SEVERITY_WARNING;   // Rich
    if (strcmp(code, "P0440") == 0) return DTC_SEVERITY_WARNING;   // EVAP
    if (strcmp(code, "P0442") == 0) return DTC_SEVERITY_WARNING;   // EVAP leak
    if (strcmp(code, "P0455") == 0) return DTC_SEVERITY_WARNING;   // EVAP leak

    // Info/minor codes
    return DTC_SEVERITY_INFO;  // Default
}

/**
 * Sort DTCs by severity (critical first, then warning, then info)
 */
void sortDTCsBySeverity() {
    // Simple bubble sort by severity (critical = 2, warning = 1, info = 0)
    for (int i = 0; i < obd_data.dtc_count - 1; i++) {
        for (int j = 0; j < obd_data.dtc_count - i - 1; j++) {
            if (obd_data.dtc_codes[j].severity < obd_data.dtc_codes[j + 1].severity) {
                // Swap
                DTC temp = obd_data.dtc_codes[j];
                obd_data.dtc_codes[j] = obd_data.dtc_codes[j + 1];
                obd_data.dtc_codes[j + 1] = temp;
            }
        }
    }
}

/**
 * Query DTCs from vehicle
 * Mode 03: Request stored DTCs
 */
void queryDTCs() {
    Serial.println("[DTC] Querying diagnostic trouble codes...");

    // Send Mode 03 command
    String response = sendOBD2Command("03");

    Serial.printf("[DTC] Response: %s\n", response.c_str());

    // Parse response
    // Response format: "43 [count] [DTC1_H] [DTC1_L] [DTC2_H] [DTC2_L] ..."
    int startPos = response.indexOf("43");
    if (startPos < 0) {
        Serial.println("[DTC] No DTCs found or invalid response");
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        obd_data.dtc_count = 0;
        obd_data.dtc_fetched = true;
        xSemaphoreGive(data_mutex);
        return;
    }

    // Skip "43 " to get to data
    String data = response.substring(startPos + 3);
    data.trim();

    // Parse hex bytes
    int dtc_index = 0;
    int pos = 0;

    xSemaphoreTake(data_mutex, portMAX_DELAY);

    while (pos < data.length() && dtc_index < 12) {
        // Skip spaces
        while (pos < data.length() && data[pos] == ' ') pos++;
        if (pos >= data.length()) break;

        // Read high byte
        String high_byte_str = data.substring(pos, pos + 2);
        pos += 2;
        while (pos < data.length() && data[pos] == ' ') pos++;
        if (pos >= data.length()) break;

        // Read low byte
        String low_byte_str = data.substring(pos, pos + 2);
        pos += 2;

        // Convert to DTC value
        uint8_t high_byte = strtol(high_byte_str.c_str(), NULL, 16);
        uint8_t low_byte = strtol(low_byte_str.c_str(), NULL, 16);
        uint16_t dtc_value = (high_byte << 8) | low_byte;

        // Skip if 0x0000 (no more DTCs)
        if (dtc_value == 0x0000) break;

        // Parse DTC code
        parseDTC(dtc_value, obd_data.dtc_codes[dtc_index].code);

        // Get description and severity
        const char* desc = getDTCDescription(obd_data.dtc_codes[dtc_index].code);
        strncpy(obd_data.dtc_codes[dtc_index].description, desc, 79);
        obd_data.dtc_codes[dtc_index].description[79] = '\0';
        obd_data.dtc_codes[dtc_index].severity = getDTCSeverity(obd_data.dtc_codes[dtc_index].code);

        Serial.printf("[DTC] Found: %s - %s (severity=%d)\n",
                      obd_data.dtc_codes[dtc_index].code,
                      obd_data.dtc_codes[dtc_index].description,
                      obd_data.dtc_codes[dtc_index].severity);

        dtc_index++;
    }

    obd_data.dtc_count = dtc_index;
    obd_data.dtc_fetched = true;

    xSemaphoreGive(data_mutex);

    // Sort by severity
    if (obd_data.dtc_count > 0) {
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        sortDTCsBySeverity();
        xSemaphoreGive(data_mutex);
    }

    Serial.printf("[DTC] Total DTCs found: %d\n", obd_data.dtc_count);
}

/**
 * Clear all DTCs from ECU
 * Mode 04: Clear diagnostic information
 */
bool clearAllDTCs() {
    Serial.println("[DTC] Clearing all DTCs from ECU...");

    // Send Mode 04 command
    String response = sendOBD2Command("04");

    Serial.printf("[DTC] Clear response: %s\n", response.c_str());

    // Check for positive response (44 = Mode 04 response)
    if (response.indexOf("44") >= 0) {
        Serial.println("[DTC] DTCs cleared successfully from ECU");

        // Clear local DTC list
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        obd_data.dtc_count = 0;
        obd_data.dtc_fetched = true;
        xSemaphoreGive(data_mutex);

        return true;
    } else {
        Serial.println("[DTC] Failed to clear DTCs");
        return false;
    }
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
    obd_data.dtc_fetched = false;
    xSemaphoreGive(data_mutex);

    // Wait a bit for connection to stabilize before querying DTCs
    Serial.println("[OBD2 Task] Waiting 3 seconds before querying DTCs...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Query DTCs once after connection
    queryDTCs();

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

        // Check for DTC operation requests (from UI thread)
        bool dtc_refresh_req = false;
        bool dtc_clear_req = false;

        xSemaphoreTake(data_mutex, portMAX_DELAY);
        dtc_refresh_req = obd_data.dtc_refresh_requested;
        dtc_clear_req = obd_data.dtc_clear_requested;
        xSemaphoreGive(data_mutex);

        // Handle DTC Clear request
        if (dtc_clear_req) {
            Serial.println("[OBD2 Task] Processing DTC clear request...");
            bool clear_success = clearAllDTCs();

            xSemaphoreTake(data_mutex, portMAX_DELAY);
            obd_data.dtc_clear_requested = false;  // Clear flag
            xSemaphoreGive(data_mutex);

            if (clear_success) {
                Serial.println("[OBD2 Task] DTCs cleared successfully");
                // Query DTCs to update display
                queryDTCs();
            } else {
                Serial.println("[OBD2 Task] Failed to clear DTCs");
            }
        }

        // Handle DTC Refresh request
        if (dtc_refresh_req) {
            Serial.println("[OBD2 Task] Processing DTC refresh request...");
            queryDTCs();

            xSemaphoreTake(data_mutex, portMAX_DELAY);
            obd_data.dtc_refresh_requested = false;  // Clear flag
            xSemaphoreGive(data_mutex);

            Serial.println("[OBD2 Task] DTC refresh complete");
        }

        // Wait before next query
        vTaskDelay(pdMS_TO_TICKS(OBD2_QUERY_INTERVAL_MS));
    }
}
