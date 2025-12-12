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
    // Background - USE STRIPS for large area (480×40 = 19,200 pixels > 10k limit)
    const int strip_height = 10;
    for (int y = 0; y < TOP_BAR_HEIGHT; y += strip_height) {
        tft.fillRect(0, y, SCREEN_WIDTH, strip_height, COLOR_DARKGRAY);
        delay(10);  // Required delay after fillRect
    }

    // Vehicle name (left)
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
    tft.setTextSize(2);
    tft.setCursor(5, 10);
    tft.print(vehicle_name);

    // Page name (center)
    tft.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
    int page_name_width = strlen(page_name) * 12;
    tft.setCursor((SCREEN_WIDTH - page_name_width) / 2, 10);
    tft.print(page_name);

    // Status indicator (right)
    int status_x = SCREEN_WIDTH - 80;
    tft.fillCircle(status_x, 17, 8, status_color);

    // DTC count
    if (dtc_count > 0) {
        char dtc_text[16];
        snprintf(dtc_text, sizeof(dtc_text), "%d DTC", dtc_count);
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft.setTextSize(1);
        tft.setCursor(status_x + 15, 12);
        tft.print(dtc_text);
    }
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

        // Button background (each button is 160×40 = 6,400 pixels)
        uint16_t bg_color = is_active ? COLOR_GRAY : COLOR_DARKGRAY;
        tft.fillRect(x, y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, bg_color);
        delay(20);  // Required delay after fillRect

        // Button border
        tft.drawRect(x, y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, COLOR_GRAY);

        // Button text
        tft.setTextColor(COLOR_WHITE, bg_color);
        tft.setTextSize(2);

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

        tft.setCursor(text_x, y + 12);
        tft.print(label);
    }
}

#endif // NAV_BAR_H
