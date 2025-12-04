/**
 * Config Page - System Configuration Display
 */

#ifndef CONFIG_PAGE_H
#define CONFIG_PAGE_H

#include "ui_common.h"

/**
 * Draw configuration page
 * Shows display, Bluetooth, and vehicle information
 */
inline void drawConfigPage() {
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
    tft.printf("Resolution: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);

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

#endif // CONFIG_PAGE_H
