/**
 * Display Manager Module
 *
 * Handles display initialization and rendering:
 * - Display hardware initialization
 * - Page rendering with smart partial updates
 * - Connection status display
 * - Data visualization
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "ui_common.h"

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Initialize display hardware
 * Sets rotation, clears screen, and displays startup message
 */
void initDisplay();

/**
 * Draw current page with smart partial updates
 * Handles all page rendering based on current_page state
 * @param current_page Current active page
 * @param page_needs_redraw Flag indicating full redraw needed
 */
void drawCurrentPage(Page current_page, bool& page_needs_redraw);

#endif // DISPLAY_MANAGER_H
