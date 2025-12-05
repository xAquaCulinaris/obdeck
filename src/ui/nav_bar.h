/**
 * Navigation Bar - Top Bar and Bottom Navigation
 */

#ifndef NAV_BAR_H
#define NAV_BAR_H

#include "ui_common.h"

// ============================================================================
// TOP BAR
// ============================================================================

/**
 * Draw top bar with vehicle name, page name, and status
 * @param vehicle_name Name to display on left
 * @param page_name Current page name (center)
 * @param status_color Status indicator color (green/yellow/red)
 * @param dtc_count Number of diagnostic trouble codes
 */
inline void drawTopBar(const char* vehicle_name, const char* page_name,
                       uint16_t status_color, int dtc_count) {
    // Background
    tft.fillRect(0, 0, SCREEN_WIDTH, TOP_BAR_HEIGHT, COLOR_DARKGRAY);

    // Vehicle name (left) - use drawString instead of print
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
    tft.setTextSize(2);
    tft.setTextDatum(TL_DATUM);  // Top-left alignment
    tft.setTextPadding(150);  // Clear old text automatically
    tft.drawString(vehicle_name, 5, 10);

    // Page name (center) - use drawString instead of print
    tft.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
    tft.setTextDatum(TC_DATUM);  // Top-center alignment
    tft.setTextPadding(200);  // Clear old text automatically
    tft.drawString(page_name, SCREEN_WIDTH / 2, 10);

    // Status indicator (right)
    int status_x = SCREEN_WIDTH - 80;
    tft.fillCircle(status_x, 17, 8, status_color);

    // DTC count - use drawString instead of printf
    if (dtc_count > 0) {
        char dtc_text[16];
        snprintf(dtc_text, sizeof(dtc_text), "%d DTC", dtc_count);
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextPadding(50);
        tft.drawString(dtc_text, status_x + 15, 12);
    }

    // Reset text settings to prevent corruption
    tft.setTextPadding(0);  // Disable padding
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
}

// ============================================================================
// BOTTOM NAVIGATION
// ============================================================================

/**
 * Draw bottom navigation bar with 3 buttons
 * @param active_page Currently active page (highlighted)
 */
inline void drawBottomNav(Page active_page) {
    int y = BOTTOM_NAV_Y;

    // Draw buttons
    for (int i = 0; i < 3; i++) {
        int x = i * NAV_BUTTON_WIDTH;
        bool is_active = (i == active_page);

        // Button background
        uint16_t bg_color = is_active ? COLOR_BLUE : COLOR_DARKGRAY;
        tft.fillRect(x, y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, bg_color);

        // Button border
        tft.drawRect(x, y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, COLOR_GRAY);

        // Button text - use drawString instead of print
        tft.setTextColor(COLOR_WHITE, bg_color);
        tft.setTextSize(2);
        tft.setTextDatum(TL_DATUM);  // Top-left alignment
        tft.setTextPadding(NAV_BUTTON_WIDTH - 10);  // Clear old text automatically

        const char* label;
        int text_x;

        switch(i) {
            case 0:
                label = "Dashboard";
                text_x = x + 15;
                break;
            case 1:
                label = "DTC";
                text_x = x + 50;
                break;
            case 2:
                label = "Config";
                text_x = x + 30;
                break;
        }

        tft.drawString(label, text_x, y + 12);
    }

    // Reset text settings to prevent corruption
    tft.setTextPadding(0);  // Disable padding
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(1);
}

#endif // NAV_BAR_H
