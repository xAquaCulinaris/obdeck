/**
 * UI Functions for Multi-Page Dashboard
 *
 * Layout:
 * - Top Bar (35px): Vehicle name | Page name | Status indicator
 * - Content Area (245px): Page-specific content
 * - Bottom Nav (40px): [Dashboard] [DTC] [Config]
 */

#ifndef UI_H
#define UI_H

#include <TFT_eSPI.h>
#include "config.h"

// ============================================================================
// PAGE SYSTEM
// ============================================================================

enum Page {
    PAGE_DASHBOARD = 0,
    PAGE_DTC = 1,
    PAGE_CONFIG = 2,
    PAGE_COUNT = 3
};

// UI Layout Constants
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

// Forward declarations
extern TFT_eSPI tft;
extern Page current_page;
extern bool page_needs_redraw;

// ============================================================================
// TOP BAR
// ============================================================================

/**
 * Draw top bar with vehicle name, page name, and status
 */
void drawTopBar(const char* vehicle_name, const char* page_name, uint16_t status_color, int dtc_count) {
    // Background
    tft.fillRect(0, 0, SCREEN_WIDTH, TOP_BAR_HEIGHT, COLOR_DARKGRAY);

    // Vehicle name (left)
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
    tft.setTextSize(2);
    tft.setCursor(5, 10);
    tft.print(vehicle_name);

    // Page name (center)
    tft.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
    int page_name_width = strlen(page_name) * 12;  // Approximate width
    tft.setCursor((SCREEN_WIDTH - page_name_width) / 2, 10);
    tft.print(page_name);

    // Status indicator (right)
    int status_x = SCREEN_WIDTH - 80;
    tft.fillCircle(status_x, 17, 8, status_color);

    // DTC count
    if (dtc_count > 0) {
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft.setTextSize(1);
        tft.setCursor(status_x + 15, 12);
        tft.printf("%d DTC", dtc_count);
    }
}

// ============================================================================
// BOTTOM NAVIGATION
// ============================================================================

/**
 * Draw bottom navigation bar with 3 buttons
 */
void drawBottomNav(Page active_page) {
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

// ============================================================================
// DASHBOARD PAGE
// ============================================================================

/**
 * Draw dashboard page with grid layout
 */
void drawDashboardPage(uint16_t rpm, uint8_t speed, float coolant,
                       float throttle, float battery, float intake) {
    int y = CONTENT_Y_START + 10;

    // Grid layout: 3 columns x 2 rows
    int col_width = SCREEN_WIDTH / 3;
    int row1_y = y + 30;
    int row2_y = y + 130;

    // Helper function to draw a metric
    auto drawMetric = [col_width](int x, int y, const char* label, const char* value, uint16_t label_color) {
        // Label
        tft.setTextColor(label_color, COLOR_BLACK);
        tft.setTextSize(1);
        int label_x = x + (col_width - strlen(label) * 6) / 2;
        tft.setCursor(label_x, y);
        tft.print(label);

        // Value
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        int value_x = x + (col_width - strlen(value) * 18) / 2;
        tft.setCursor(value_x, y + 20);
        tft.print(value);
    };

    // Row 1
    char buf[20];

    // RPM
    snprintf(buf, sizeof(buf), "%d", rpm);
    drawMetric(0, row1_y, "RPM", buf, COLOR_CYAN);

    // Speed
    snprintf(buf, sizeof(buf), "%d", speed);
    drawMetric(col_width, row1_y, "Speed (km/h)", buf, COLOR_CYAN);

    // Coolant
    snprintf(buf, sizeof(buf), "%.1f", coolant);
    drawMetric(col_width * 2, row1_y, "Coolant (C)", buf, COLOR_GREEN);

    // Row 2
    // Throttle
    snprintf(buf, sizeof(buf), "%.0f%%", throttle);
    drawMetric(0, row2_y, "Throttle", buf, COLOR_YELLOW);

    // Battery
    snprintf(buf, sizeof(buf), "%.1fV", battery);
    drawMetric(col_width, row2_y, "Battery", buf, COLOR_GREEN);

    // Intake
    snprintf(buf, sizeof(buf), "%.1f", intake);
    drawMetric(col_width * 2, row2_y, "Intake (C)", buf, COLOR_GREEN);
}

// ============================================================================
// DTC PAGE
// ============================================================================

/**
 * Draw DTC codes page
 */
void drawDTCPage(int dtc_count) {
    int y = CONTENT_Y_START + 20;

    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, y);
    tft.print("Diagnostic Trouble Codes");

    y += 40;

    if (dtc_count == 0) {
        // No codes
        tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(80, y + 50);
        tft.print("ALL CLEAR");

        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(1);
        tft.setCursor(100, y + 90);
        tft.print("No diagnostic codes found");
    } else {
        // Show codes (placeholder for now)
        tft.setTextColor(COLOR_YELLOW, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(20, y);
        tft.printf("Found %d codes:", dtc_count);

        y += 30;
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(1);
        tft.setCursor(20, y);
        tft.print("P0133 - O2 Sensor Slow Response");
        y += 20;
        tft.setCursor(20, y);
        tft.print("P0244 - Wastegate Solenoid");

        // Buttons (placeholder)
        y += 60;
        tft.fillRect(50, y, 150, 40, COLOR_BLUE);
        tft.drawRect(50, y, 150, 40, COLOR_WHITE);
        tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
        tft.setTextSize(2);
        tft.setCursor(70, y + 12);
        tft.print("Refresh");

        tft.fillRect(250, y, 150, 40, COLOR_RED);
        tft.drawRect(250, y, 150, 40, COLOR_WHITE);
        tft.setTextColor(COLOR_WHITE, COLOR_RED);
        tft.setCursor(275, y + 12);
        tft.print("Clear");
    }
}

// ============================================================================
// CONFIG PAGE
// ============================================================================

/**
 * Draw config page
 */
void drawConfigPage() {
    int y = CONTENT_Y_START + 20;

    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, y);
    tft.print("Configuration");

    y += 40;

    // Display settings
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(20, y);
    tft.print("Display Settings");

    y += 25;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setCursor(30, y);
    tft.printf("Refresh Rate: %d Hz", DISPLAY_REFRESH_HZ);

    y += 20;
    tft.setCursor(30, y);
    tft.printf("Brightness: Auto");

    y += 40;

    // Bluetooth settings
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setCursor(20, y);
    tft.print("Bluetooth Settings");

    y += 25;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setCursor(30, y);
    tft.printf("MAC: %s", BT_MAC_ADDRESS);

    y += 20;
    tft.setCursor(30, y);
    tft.printf("Connection: Active");

    y += 40;

    // Vehicle info
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setCursor(20, y);
    tft.print("Vehicle Information");

    y += 25;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setCursor(30, y);
    tft.printf("Make/Model: %s", VEHICLE_NAME);

    y += 20;
    tft.setCursor(30, y);
    tft.printf("Year: %d", VEHICLE_YEAR);
}

#endif // UI_H
