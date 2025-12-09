/**
 * OBD2 Task Module - Implementation
 */

#include "obd2_task.h"
#include "obd_data.h"
#include "bluetooth.h"
#include "elm327.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

OBDData obd_data = {0};
SemaphoreHandle_t data_mutex;

// ============================================================================
// INITIALIZATION
// ============================================================================

void initOBD2() {
    // Create mutex for shared data
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        Serial.println("ERROR: Failed to create data mutex!");
        while (1) delay(1000);
    }

    // Initialize Bluetooth
    initBluetooth();
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

    // Wait a bit for connection to stabilize before querying vehicle info
    Serial.println("[OBD2 Task] Waiting 3 seconds before querying vehicle info...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Query DTCs once after connection
    queryDTCs();

    // Query VIN once after connection
    Serial.println("[OBD2 Task] Querying VIN...");
    queryVIN();

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
                disconnectBluetooth();

                // Reinitialize Bluetooth
                initBluetooth();

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
