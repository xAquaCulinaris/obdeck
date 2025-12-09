/**
 * ELM327 OBD2 Protocol Module
 *
 * Handles ELM327 protocol communication:
 * - ELM327 initialization
 * - OBD2 command sending and response parsing
 * - PID queries (RPM, Speed, Coolant, etc.)
 * - DTC operations (query, clear)
 * - VIN queries
 */

#ifndef ELM327_H
#define ELM327_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ELMduino.h>
#include "config.h"
#include "obd_data.h"

// ============================================================================
// GLOBAL OBJECTS (declared here, defined in elm327.cpp)
// ============================================================================

extern ELM327 elm327;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Connect to ELM327 and initialize
 * Assumes Bluetooth is already connected
 * @return true if ELM327 initialized successfully, false otherwise
 */
bool connectToELM327();

/**
 * Send OBD2 command and read response
 * @param cmd Command string (e.g., "010C" for RPM)
 * @return Response string from ELM327
 */
String sendOBD2Command(String cmd);

/**
 * Parse hex byte from OBD2 response
 * @param response The full OBD2 response string
 * @param byteIndex The data byte index (0 = first data byte after PID)
 * @return Parsed byte value, or -1 on error
 */
int parseHexByte(String response, int byteIndex);

// ============================================================================
// PID QUERY FUNCTIONS
// ============================================================================

/**
 * Query engine RPM (PID 0x0C)
 * @return RPM value, or -1 on error
 */
int queryRPM();

/**
 * Query vehicle speed (PID 0x0D)
 * @return Speed in km/h, or -1 on error
 */
int querySpeed();

/**
 * Query coolant temperature (PID 0x05)
 * @return Temperature in °C, or -999 on error
 */
float queryCoolantTemp();

/**
 * Query throttle position (PID 0x11)
 * @return Throttle position in %, or -1 on error
 */
float queryThrottle();

/**
 * Query intake air temperature (PID 0x0F)
 * @return Temperature in °C, or -999 on error
 */
float queryIntakeTemp();

/**
 * Query battery voltage (PID 0x42)
 * @return Voltage in V, or -1 on error
 */
float queryBatteryVoltage();

// ============================================================================
// DTC FUNCTIONS
// ============================================================================

/**
 * Parse DTC value to code string
 * @param dtc_value 2-byte DTC value from ECU
 * @param code Output buffer (must be at least 6 chars)
 */
void parseDTC(uint16_t dtc_value, char* code);

/**
 * Get DTC description from code
 * @param code DTC code (e.g., "P0133")
 * @return Human-readable description
 */
const char* getDTCDescription(const char* code);

/**
 * Get DTC severity level
 * @param code DTC code (e.g., "P0133")
 * @return Severity level (0=info, 1=warning, 2=critical)
 */
uint8_t getDTCSeverity(const char* code);

/**
 * Sort DTCs by severity (critical first)
 * Operates on global obd_data.dtc_codes array
 */
void sortDTCsBySeverity();

/**
 * Query DTCs from vehicle (Mode 03)
 * Updates global obd_data structure
 */
void queryDTCs();

/**
 * Clear all DTCs from ECU (Mode 04)
 * @return true if successful, false otherwise
 */
bool clearAllDTCs();

// ============================================================================
// VEHICLE INFORMATION FUNCTIONS
// ============================================================================

/**
 * Query VIN from ECU (Mode 09, PID 02)
 * Updates global obd_data structure
 */
void queryVIN();

#endif // ELM327_H
