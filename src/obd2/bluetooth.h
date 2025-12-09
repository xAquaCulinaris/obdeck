/**
 * Bluetooth Connection Module
 *
 * Handles Bluetooth Serial connection to ELM327 adapter:
 * - Initialization
 * - Connection management (MAC address or device name)
 * - Connection monitoring
 * - Automatic reconnection
 */

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "config.h"

// ============================================================================
// GLOBAL OBJECTS (declared here, defined in bluetooth.cpp)
// ============================================================================

extern BluetoothSerial SerialBT;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize Bluetooth Serial in master mode
 * Must be called before any other Bluetooth functions
 */
void initBluetooth();

/**
 * Connect to ELM327 via Bluetooth
 * Uses MAC address (if BT_USE_MAC is true) or device name
 * @return true if connection successful, false otherwise
 */
bool connectBluetooth();

/**
 * Disconnect Bluetooth connection
 */
void disconnectBluetooth();

/**
 * Check if Bluetooth is connected
 * @return true if connected, false otherwise
 */
bool isBluetoothConnected();

#endif // BLUETOOTH_H
