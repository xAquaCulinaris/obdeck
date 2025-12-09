/**
 * OBD2 Task Module
 *
 * Main OBD2 communication task that runs on Core 0
 * Handles:
 * - Connection initialization
 * - Continuous PID queries
 * - DTC operations
 * - Auto-reconnection on connection loss
 */

#ifndef OBD2_TASK_H
#define OBD2_TASK_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize OBD2 module
 * Creates data mutex and initializes Bluetooth
 * Must be called before starting obd2Task
 */
void initOBD2();

/**
 * Main OBD2 communication task (runs on Core 0)
 * Handles connection, queries, and auto-reconnect
 * @param parameter Unused task parameter (required by FreeRTOS)
 */
void obd2Task(void *parameter);

#endif // OBD2_TASK_H
