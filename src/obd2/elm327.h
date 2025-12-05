/**
 * ELM327 OBD2 Communication Module
 *
 * Handles all communication with the ELM327 adapter:
 * - Bluetooth connection management
 * - OBD2 command sending and response parsing
 * - PID queries (RPM, Speed, Coolant, etc.)
 * - Connection monitoring and auto-reconnect
 */

#ifndef ELM327_H
#define ELM327_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>
#include "config.h"  // PlatformIO automatically includes the include/ directory

// ============================================================================
// SHARED DATA STRUCTURE
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
};

// ============================================================================
// GLOBAL OBJECTS (declared here, defined in elm327.cpp)
// ============================================================================

extern BluetoothSerial SerialBT;
extern ELM327 elm327;
extern OBDData obd_data;
extern SemaphoreHandle_t data_mutex;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize Bluetooth Serial and data mutex
 */
void initELM327();

/**
 * Connect to ELM327 via Bluetooth using MAC address
 */
bool connectToELM327();

/**
 * Send OBD2 command and read response
 */
String sendOBD2Command(String cmd);

/**
 * Parse hex byte from OBD2 response
 * @param response The full OBD2 response string
 * @param byteIndex The data byte index (0 = first data byte after PID)
 */
int parseHexByte(String response, int byteIndex);

/**
 * Query specific PIDs
 */
int queryRPM();
int querySpeed();
float queryCoolantTemp();
float queryThrottle();
float queryIntakeTemp();
float queryBatteryVoltage();

/**
 * DTC Functions
 */
void queryDTCs();
void parseDTC(uint16_t dtc_value, char* code);
const char* getDTCDescription(const char* code);
uint8_t getDTCSeverity(const char* code);
void sortDTCsBySeverity();
bool clearAllDTCs();  // Clear all DTCs from ECU (Mode 04)

/**
 * Main OBD2 communication task (runs on Core 0)
 * Handles connection, queries, and auto-reconnect
 */
void obd2Task(void *parameter);

#endif // ELM327_H
