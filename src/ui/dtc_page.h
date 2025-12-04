/**
 * DTC Page - Diagnostic Trouble Codes Display
 */

#ifndef DTC_PAGE_H
#define DTC_PAGE_H

#include "ui_common.h"

/**
 * Draw DTC codes page
 * @param dtc_count Number of diagnostic trouble codes (0 = all clear)
 */
inline void drawDTCPage(int dtc_count) {
    int y = CONTENT_Y_START + 20;

    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, y);
    tft.print("Diagnostic Trouble Codes");

    y += 40;

    if (dtc_count == 0) {
        // No codes - all clear
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

        // Buttons (placeholder - TODO: implement with touch)
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

#endif // DTC_PAGE_H
