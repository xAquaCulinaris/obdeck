/**
 * UI Common - Shared UI Constants and Utilities
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <TFT_eSPI.h>
#include "config.h"  // PlatformIO automatically includes the include/ directory

// ============================================================================
// PAGE SYSTEM
// ============================================================================

enum Page {
    PAGE_DASHBOARD = 0,
    PAGE_DTC = 1,
    PAGE_CONFIG = 2,
    PAGE_COUNT = 3
};

// ============================================================================
// UI LAYOUT CONSTANTS
// ============================================================================

#define TOP_BAR_HEIGHT      35
#define BOTTOM_NAV_HEIGHT   40
#define CONTENT_Y_START     TOP_BAR_HEIGHT
#define CONTENT_HEIGHT      (SCREEN_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_NAV_HEIGHT)
#define BOTTOM_NAV_Y        (SCREEN_HEIGHT - BOTTOM_NAV_HEIGHT)

// Bottom Navigation Touch Zones
#define NAV_BUTTON_WIDTH    (SCREEN_WIDTH / 3)
#define NAV_DASHBOARD_X     0
#define NAV_DTC_X           NAV_BUTTON_WIDTH
#define NAV_CONFIG_X        (NAV_BUTTON_WIDTH * 2)

// Colors for status indicator
#define STATUS_OK           COLOR_GREEN
#define STATUS_WARNING      COLOR_YELLOW
#define STATUS_ERROR        COLOR_RED

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

extern TFT_eSPI tft;
extern Page current_page;
extern bool page_needs_redraw;

#endif // UI_COMMON_H
