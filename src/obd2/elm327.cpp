/**
 * ELM327 OBD2 Protocol Module - Implementation
 */

#include "elm327.h"
#include "bluetooth.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

ELM327 elm327;

// ============================================================================
// ELM327 CONNECTION
// ============================================================================

bool connectToELM327() {
    // Ensure Bluetooth is connected first
    if (!connectBluetooth()) {
        return false;
    }

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

const char* getDTCDescription(const char* code) {
    // Critical codes - Engine damage risk
    if (strcmp(code, "P0300") == 0) return "Random Misfire Detected";
    if (strcmp(code, "P0301") == 0) return "Cylinder 1 Misfire";
    if (strcmp(code, "P0302") == 0) return "Cylinder 2 Misfire";
    if (strcmp(code, "P0303") == 0) return "Cylinder 3 Misfire";
    if (strcmp(code, "P0304") == 0) return "Cylinder 4 Misfire";
    if (strcmp(code, "P0217") == 0) return "Engine Overheat Condition";
    if (strcmp(code, "P0218") == 0) return "Transmission Overheat";
    if (strcmp(code, "P0524") == 0) return "Engine Oil Pressure Too Low";
    if (strcmp(code, "P0522") == 0) return "Oil Pressure Sensor Low";
    if (strcmp(code, "P0523") == 0) return "Oil Pressure Sensor High";
    if (strcmp(code, "P0016") == 0) return "Crankshaft/Camshaft Correlation";
    if (strcmp(code, "P0017") == 0) return "Crankshaft/Camshaft Correlation B1";
    if (strcmp(code, "P0335") == 0) return "Crankshaft Position Sensor";
    if (strcmp(code, "P0340") == 0) return "Camshaft Position Sensor";
    
    // Warning codes - Performance/Emissions
    if (strcmp(code, "P0420") == 0) return "Catalyst Efficiency Low B1";
    if (strcmp(code, "P0430") == 0) return "Catalyst Efficiency Low B2";
    if (strcmp(code, "P0171") == 0) return "System Too Lean B1";
    if (strcmp(code, "P0172") == 0) return "System Too Rich B1";
    if (strcmp(code, "P0174") == 0) return "System Too Lean B2";
    if (strcmp(code, "P0175") == 0) return "System Too Rich B2";
    if (strcmp(code, "P0440") == 0) return "EVAP System Malfunction";
    if (strcmp(code, "P0442") == 0) return "EVAP System Small Leak";
    if (strcmp(code, "P0455") == 0) return "EVAP System Large Leak";
    if (strcmp(code, "P0456") == 0) return "EVAP System Very Small Leak";
    if (strcmp(code, "P0128") == 0) return "Coolant Thermostat Malfunction";
    if (strcmp(code, "P0133") == 0) return "O2 Sensor Slow Response B1S1";
    if (strcmp(code, "P0134") == 0) return "O2 Sensor No Activity B1S1";
    if (strcmp(code, "P0135") == 0) return "O2 Sensor Heater B1S1";
    if (strcmp(code, "P0141") == 0) return "O2 Sensor Heater B1S2";
    if (strcmp(code, "P0401") == 0) return "EGR Insufficient Flow";
    if (strcmp(code, "P0402") == 0) return "EGR Excessive Flow";
    if (strcmp(code, "P0411") == 0) return "Secondary Air Injection";
    if (strcmp(code, "P0606") == 0) return "ECM Processor Fault";
    if (strcmp(code, "P0244") == 0) return "Wastegate Solenoid";
    
    // Info codes - Sensor issues
    if (strcmp(code, "P0101") == 0) return "MAF Sensor Range/Performance";
    if (strcmp(code, "P0102") == 0) return "MAF Sensor Circuit Low";
    if (strcmp(code, "P0103") == 0) return "MAF Sensor Circuit High";
    if (strcmp(code, "P0106") == 0) return "MAP Sensor Range/Performance";
    if (strcmp(code, "P0107") == 0) return "MAP Sensor Circuit Low";
    if (strcmp(code, "P0108") == 0) return "MAP Sensor Circuit High";
    if (strcmp(code, "P0112") == 0) return "Intake Air Temp Sensor Low";
    if (strcmp(code, "P0113") == 0) return "Intake Air Temp Sensor High";
    if (strcmp(code, "P0116") == 0) return "Coolant Temp Sensor Range";
    if (strcmp(code, "P0117") == 0) return "Coolant Temp Sensor Low";
    if (strcmp(code, "P0118") == 0) return "Coolant Temp Sensor High";
    if (strcmp(code, "P0122") == 0) return "Throttle Position Sensor Low";
    if (strcmp(code, "P0123") == 0) return "Throttle Position Sensor High";
    if (strcmp(code, "P0562") == 0) return "System Voltage Low";
    if (strcmp(code, "P0563") == 0) return "System Voltage High";

    return "Unknown DTC";
}

uint8_t getDTCSeverity(const char* code) {
    // Critical codes (engine damage risk)
    if (strcmp(code, "P0300") == 0) return DTC_SEVERITY_CRITICAL;  // Random misfire
    if (strcmp(code, "P0301") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 1 misfire
    if (strcmp(code, "P0302") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 2 misfire
    if (strcmp(code, "P0303") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 3 misfire
    if (strcmp(code, "P0304") == 0) return DTC_SEVERITY_CRITICAL;  // Cylinder 4 misfire
    if (strcmp(code, "P0217") == 0) return DTC_SEVERITY_CRITICAL;  // Engine overheating
    if (strcmp(code, "P0218") == 0) return DTC_SEVERITY_CRITICAL;  // Transmission overheating
    if (strcmp(code, "P0524") == 0) return DTC_SEVERITY_CRITICAL;  // Oil pressure too low
    if (strcmp(code, "P0522") == 0) return DTC_SEVERITY_CRITICAL;  // Oil pressure sensor low
    if (strcmp(code, "P0523") == 0) return DTC_SEVERITY_CRITICAL;  // Oil pressure sensor high
    if (strcmp(code, "P0016") == 0) return DTC_SEVERITY_CRITICAL;  // Cam/Crank correlation
    if (strcmp(code, "P0017") == 0) return DTC_SEVERITY_CRITICAL;  // Cam/Crank correlation Bank 1
    if (strcmp(code, "P0335") == 0) return DTC_SEVERITY_CRITICAL;  // Crankshaft position sensor
    if (strcmp(code, "P0340") == 0) return DTC_SEVERITY_CRITICAL;  // Camshaft position sensor
    
    // Warning codes (performance/emissions)
    if (strcmp(code, "P0420") == 0) return DTC_SEVERITY_WARNING;   // Catalyst efficiency
    if (strcmp(code, "P0430") == 0) return DTC_SEVERITY_WARNING;   // Catalyst efficiency Bank 2
    if (strcmp(code, "P0171") == 0) return DTC_SEVERITY_WARNING;   // System too lean Bank 1
    if (strcmp(code, "P0172") == 0) return DTC_SEVERITY_WARNING;   // System too rich Bank 1
    if (strcmp(code, "P0174") == 0) return DTC_SEVERITY_WARNING;   // System too lean Bank 2
    if (strcmp(code, "P0175") == 0) return DTC_SEVERITY_WARNING;   // System too rich Bank 2
    if (strcmp(code, "P0440") == 0) return DTC_SEVERITY_WARNING;   // EVAP system
    if (strcmp(code, "P0442") == 0) return DTC_SEVERITY_WARNING;   // EVAP leak small
    if (strcmp(code, "P0455") == 0) return DTC_SEVERITY_WARNING;   // EVAP leak large
    if (strcmp(code, "P0456") == 0) return DTC_SEVERITY_WARNING;   // EVAP leak very small
    if (strcmp(code, "P0128") == 0) return DTC_SEVERITY_WARNING;   // Coolant thermostat
    if (strcmp(code, "P0133") == 0) return DTC_SEVERITY_WARNING;   // O2 sensor slow response
    if (strcmp(code, "P0134") == 0) return DTC_SEVERITY_WARNING;   // O2 sensor no activity
    if (strcmp(code, "P0135") == 0) return DTC_SEVERITY_WARNING;   // O2 sensor heater
    if (strcmp(code, "P0141") == 0) return DTC_SEVERITY_WARNING;   // O2 sensor heater Bank 1 Sensor 2
    if (strcmp(code, "P0401") == 0) return DTC_SEVERITY_WARNING;   // EGR insufficient flow
    if (strcmp(code, "P0402") == 0) return DTC_SEVERITY_WARNING;   // EGR excessive flow
    if (strcmp(code, "P0411") == 0) return DTC_SEVERITY_WARNING;   // Secondary air injection
    if (strcmp(code, "P0606") == 0) return DTC_SEVERITY_WARNING;   // ECM processor fault
    
    return DTC_SEVERITY_INFO;  // Default for unknown codes
}

void sortDTCsBySeverity() {
    // Need to access global obd_data - include obd_data.h
    extern OBDData obd_data;
    extern SemaphoreHandle_t data_mutex;

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

void queryDTCs() {
    extern OBDData obd_data;
    extern SemaphoreHandle_t data_mutex;

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

bool clearAllDTCs() {
    extern OBDData obd_data;
    extern SemaphoreHandle_t data_mutex;

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
// VEHICLE INFORMATION FUNCTIONS
// ============================================================================

void queryVIN() {
    extern OBDData obd_data;
    extern SemaphoreHandle_t data_mutex;

    Serial.println("[VIN] Querying Vehicle Identification Number...");

    // Send Mode 09, PID 02 command
    String response = sendOBD2Command("0902");

    Serial.printf("[VIN] Response: %s\n", response.c_str());

    // Parse VIN from response
    // Response format: "49 02 01 [VIN bytes in ASCII]"
    // VIN is 17 characters long
    if (response.indexOf("49 02") >= 0 || response.indexOf("4902") >= 0) {
        // Remove spaces and mode/PID prefix
        response.replace(" ", "");
        response.replace("4902", "");
        response.replace("01", "");  // Remove line counter if present

        // Extract VIN (next 34 hex chars = 17 ASCII chars)
        char vin[18] = {0};
        int vin_idx = 0;

        for (int i = 0; i < response.length() - 1 && vin_idx < 17; i += 2) {
            char hex[3] = {response[i], response[i+1], '\0'};
            int ascii_val = strtol(hex, NULL, 16);

            // Only accept printable ASCII characters (0x20-0x7E)
            if (ascii_val >= 0x20 && ascii_val <= 0x7E) {
                vin[vin_idx++] = (char)ascii_val;
            }
        }
        vin[vin_idx] = '\0';

        if (vin_idx == 17) {
            Serial.printf("[VIN] Successfully retrieved: %s\n", vin);

            xSemaphoreTake(data_mutex, portMAX_DELAY);
            strncpy(obd_data.vin, vin, sizeof(obd_data.vin) - 1);
            obd_data.vin[17] = '\0';
            obd_data.vin_fetched = true;
            xSemaphoreGive(data_mutex);
        } else {
            Serial.printf("[VIN] Invalid VIN length: %d (expected 17)\n", vin_idx);
            xSemaphoreTake(data_mutex, portMAX_DELAY);
            strcpy(obd_data.vin, "Not Available");
            obd_data.vin_fetched = false;
            xSemaphoreGive(data_mutex);
        }
    } else {
        Serial.println("[VIN] VIN not supported or invalid response");
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        strcpy(obd_data.vin, "Not Supported");
        obd_data.vin_fetched = false;
        xSemaphoreGive(data_mutex);
    }
}
