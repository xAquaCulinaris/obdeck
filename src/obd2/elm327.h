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
 * Main OBD2 communication task (runs on Core 0)
 * Handles connection, queries, and auto-reconnect
 */
void obd2Task(void *parameter);

#endif // ELM327_H
