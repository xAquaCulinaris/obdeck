/**
 * Config Page - System Configuration Display
 */

#ifndef CONFIG_PAGE_H
#define CONFIG_PAGE_H

#include "ui_common.h"
#include "../obd2/obd_data.h"

/**
 * Draw configuration page with 3-section layout
 * Layout: Vehicle Info (top-left), Bluetooth (bottom-left), Display (top-right)
 */
inline void drawConfigPage() {
    // Left column X position, Right column X position
    const int LEFT_X = 10;
    const int RIGHT_X = 250;
    const int TOP_Y = CONTENT_Y_START + 10;
    const int BOTTOM_Y = CONTENT_Y_START + 130;

    // Get VIN from shared data (with safety check)
    char vin[18] = "Loading...";
    if (data_mutex != NULL) {
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        if (obd_data.vin[0] != '\0') {
            strncpy(vin, obd_data.vin, sizeof(vin) - 1);
            vin[17] = '\0';
        }
        xSemaphoreGive(data_mutex);
    }

    // ========================================
    // VEHICLE INFO (Top Left)
    // ========================================
    int y = TOP_Y;
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Vehicle Information", LEFT_X, y);

    y += 20;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.drawString("Make/Model:", LEFT_X, y);
    tft.drawString(VEHICLE_NAME, LEFT_X + 10, y + 12);

    y += 30;
    tft.drawString("Year:", LEFT_X, y);
    char year_str[8];
    snprintf(year_str, sizeof(year_str), "%d", VEHICLE_YEAR);
    tft.drawString(year_str, LEFT_X + 10, y + 12);

    y += 30;
    tft.drawString("VIN:", LEFT_X, y);
    tft.drawString(vin, LEFT_X + 10, y + 12);

    // ========================================
    // BLUETOOTH (Bottom Left)
    // ========================================
    y = BOTTOM_Y;
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(1);
    tft.drawString("Bluetooth Settings", LEFT_X, y);

    y += 20;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.drawString("MAC Address:", LEFT_X, y);
    tft.drawString(BT_MAC_ADDRESS, LEFT_X + 10, y + 12);

    y += 30;
    tft.drawString("Status:", LEFT_X, y);
    tft.drawString("Connected", LEFT_X + 10, y + 12);

    // ========================================
    // DISPLAY (Top Right)
    // ========================================
    y = TOP_Y;
    tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
    tft.setTextSize(1);
    tft.drawString("Display Settings", RIGHT_X, y);

    y += 20;
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.drawString("Resolution:", RIGHT_X, y);
    char res_str[16];
    snprintf(res_str, sizeof(res_str), "%dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
    tft.drawString(res_str, RIGHT_X + 10, y + 12);

    y += 30;
    tft.drawString("Refresh Rate:", RIGHT_X, y);
    char hz_str[8];
    snprintf(hz_str, sizeof(hz_str), "%d Hz", DISPLAY_REFRESH_HZ);
    tft.drawString(hz_str, RIGHT_X + 10, y + 12);

    y += 30;
    tft.drawString("Controller:", RIGHT_X, y);
    tft.drawString("ILI9488", RIGHT_X + 10, y + 12);
}

#endif // CONFIG_PAGE_H
