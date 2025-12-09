/**
 * Bluetooth Connection Module - Implementation
 */

#include "bluetooth.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

BluetoothSerial SerialBT;

// ============================================================================
// BLUETOOTH FUNCTIONS
// ============================================================================

void initBluetooth() {
    // Initialize Bluetooth Serial in MASTER mode
    if (!SerialBT.begin("OBDECK", true)) {  // true = master/client mode
        Serial.println("ERROR: Bluetooth initialization failed!");
        while (1) delay(1000);
    }
    Serial.println("✓ Bluetooth Serial initialized (Master mode)");
}

bool connectBluetooth() {
    Serial.println("\n========================================");
    Serial.println("Connecting to ELM327 via Bluetooth...");
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

    return true;
}

void disconnectBluetooth() {
    Serial.println("Disconnecting Bluetooth...");
    SerialBT.end();
    delay(1000);
}

bool isBluetoothConnected() {
    return SerialBT.connected();
}
