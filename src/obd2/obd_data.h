/**
 * OBD2 Data Structures
 *
 * Shared data structures used across OBD2 module
 * Includes DTC codes, vehicle data, and synchronization primitives
 */

#ifndef OBD_DATA_H
#define OBD_DATA_H

#include <Arduino.h>

// ============================================================================
// DTC DEFINITIONS
// ============================================================================

// DTC severity levels
#define DTC_SEVERITY_INFO       0
#define DTC_SEVERITY_WARNING    1
#define DTC_SEVERITY_CRITICAL   2

struct DTC {
    char code[6];           // e.g., "P0133"
    char description[80];   // e.g., "O2 Sensor Slow Response"
    uint8_t severity;       // 0=info, 1=warning, 2=critical
};

// ============================================================================
// OBD DATA STRUCTURE
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

    // Diagnostic Trouble Codes
    DTC dtc_codes[12];       // Store up to 12 DTCs
    uint8_t dtc_count;       // Number of active DTCs
    bool dtc_fetched;        // Whether DTCs have been fetched

    // DTC Request Flags (set by UI thread, cleared by OBD2 task)
    bool dtc_refresh_requested;  // Request DTC refresh from ECU
    bool dtc_clear_requested;    // Request clear all DTCs

    // Vehicle Information (fetched once at startup)
    char vin[18];                // Vehicle Identification Number (17 chars + null)
    bool vin_fetched;            // Whether VIN has been fetched
};

// ============================================================================
// GLOBAL OBJECTS (declared here, defined in obd2_task.cpp)
// ============================================================================

extern OBDData obd_data;
extern SemaphoreHandle_t data_mutex;

#endif // OBD_DATA_H
