/**
 * DTC Page - Diagnostic Trouble Codes Display
 * Improved layout with action buttons
 */

#ifndef DTC_PAGE_H
#define DTC_PAGE_H

#include "ui_common.h"
#include "../obd2/elm327.h"

// Scroll state
static int dtc_scroll_offset = 0;
const int DTC_ITEMS_PER_PAGE = 4;  // Show 4 DTCs (more space for buttons)

/**
 * Draw DTC codes page with improved layout
 * @param dtc_data Pointer to DTC data array
 * @param dtc_count Number of DTCs
 */
inline void drawDTCPage(const DTC* dtc_data, int dtc_count) {
    int y = CONTENT_Y_START + 5;

    if (dtc_count == 0) {
        // No codes - all clear (centered)
        int center_y = CONTENT_Y_START + (CONTENT_HEIGHT / 2) - 60;

        tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
        tft.setTextSize(4);
        tft.setCursor(100, center_y);
        tft.print("ALL CLEAR");

        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(80, center_y + 50);
        tft.print("No trouble codes found");

        // Refresh button
        tft.fillRect(150, center_y + 100, 180, 40, COLOR_BLUE);
        tft.drawRect(150, center_y + 100, 180, 40, COLOR_WHITE);
        tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
        tft.setTextSize(2);
        tft.setCursor(180, center_y + 112);
        tft.print("REFRESH");

    } else {
        // Compact header with count and pagination
        tft.setTextColor(COLOR_YELLOW, COLOR_BLACK);
        tft.setTextSize(1);
        tft.setCursor(10, y);

        int total_pages = (dtc_count + DTC_ITEMS_PER_PAGE - 1) / DTC_ITEMS_PER_PAGE;
        int current_page = (dtc_scroll_offset / DTC_ITEMS_PER_PAGE) + 1;

        tft.printf("%d DTC(s) Found | Page %d/%d", dtc_count, current_page, total_pages);

        // Action buttons (top right) - larger size
        int btn_y = y - 2;

        // Refresh button
        tft.fillRect(300, btn_y, 95, 26, COLOR_BLUE);
        tft.drawRect(300, btn_y, 95, 26, COLOR_WHITE);
        tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
        tft.setTextSize(2);
        tft.setCursor(307, btn_y + 5);
        tft.print("REFRESH");

        // Clear All button (red)
        tft.fillRect(400, btn_y, 105, 26, COLOR_RED);
        tft.drawRect(400, btn_y, 105, 26, COLOR_WHITE);
        tft.setTextColor(COLOR_WHITE, COLOR_RED);
        tft.setTextSize(2);
        tft.setCursor(408, btn_y + 5);
        tft.print("CLEAR");

        y += 25;

        // Separator line
        tft.drawLine(5, y, SCREEN_WIDTH - 5, y, COLOR_GRAY);
        y += 5;

        // Display DTCs (with scrolling)
        int end_index = min(dtc_scroll_offset + DTC_ITEMS_PER_PAGE, dtc_count);

        for (int i = dtc_scroll_offset; i < end_index; i++) {
            const DTC& dtc = dtc_data[i];

            // Determine color based on severity
            uint16_t severity_color;
            const char* severity_badge;
            switch (dtc.severity) {
                case DTC_SEVERITY_CRITICAL:
                    severity_color = COLOR_RED;
                    severity_badge = "CRIT";
                    break;
                case DTC_SEVERITY_WARNING:
                    severity_color = COLOR_YELLOW;
                    severity_badge = "WARN";
                    break;
                default:
                    severity_color = COLOR_CYAN;
                    severity_badge = "INFO";
                    break;
            }

            // DTC code and severity badge
            tft.setTextColor(severity_color, COLOR_BLACK);
            tft.setTextSize(2);
            tft.setCursor(10, y);
            tft.print(dtc.code);

            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(1);
            tft.setCursor(85, y + 5);
            tft.printf("[%s]", severity_badge);

            y += 22;

            // Description
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(1);
            tft.setCursor(10, y);

            // Truncate description if too long
            char desc_truncated[55];
            strncpy(desc_truncated, dtc.description, 54);
            desc_truncated[54] = '\0';
            tft.print(desc_truncated);

            y += 13;

            // Separator
            tft.drawLine(5, y, SCREEN_WIDTH - 5, y, COLOR_DARKGRAY);
            y += 5;
        }

        // Scroll buttons at bottom if needed
        if (dtc_count > DTC_ITEMS_PER_PAGE) {
            int button_y = BOTTOM_NAV_Y - 48;

            // Up button
            if (dtc_scroll_offset > 0) {
                tft.fillRect(80, button_y, 140, 38, COLOR_BLUE);
                tft.drawRect(80, button_y, 140, 38, COLOR_WHITE);
                tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
                tft.setTextSize(2);
                tft.setCursor(102, button_y + 11);
                tft.print("^ UP ^");
            } else {
                tft.fillRect(80, button_y, 140, 38, COLOR_DARKGRAY);
                tft.drawRect(80, button_y, 140, 38, COLOR_GRAY);
                tft.setTextColor(COLOR_GRAY, COLOR_DARKGRAY);
                tft.setTextSize(2);
                tft.setCursor(102, button_y + 11);
                tft.print("^ UP ^");
            }

            // Down button
            if (dtc_scroll_offset + DTC_ITEMS_PER_PAGE < dtc_count) {
                tft.fillRect(260, button_y, 140, 38, COLOR_BLUE);
                tft.drawRect(260, button_y, 140, 38, COLOR_WHITE);
                tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
                tft.setTextSize(2);
                tft.setCursor(272, button_y + 11);
                tft.print("v DOWN v");
            } else {
                tft.fillRect(260, button_y, 140, 38, COLOR_DARKGRAY);
                tft.drawRect(260, button_y, 140, 38, COLOR_GRAY);
                tft.setTextColor(COLOR_GRAY, COLOR_DARKGRAY);
                tft.setTextSize(2);
                tft.setCursor(272, button_y + 11);
                tft.print("v DOWN v");
            }
        }
    }
}

/**
 * Reset scroll offset (call when page is opened)
 */
inline void resetDTCScroll() {
    dtc_scroll_offset = 0;
}

/**
 * Scroll up (show previous DTCs)
 */
inline void scrollDTCUp() {
    if (dtc_scroll_offset > 0) {
        dtc_scroll_offset -= DTC_ITEMS_PER_PAGE;
        if (dtc_scroll_offset < 0) {
            dtc_scroll_offset = 0;
        }
    }
}

/**
 * Scroll down (show next DTCs)
 */
inline void scrollDTCDown(int dtc_count) {
    if (dtc_scroll_offset + DTC_ITEMS_PER_PAGE < dtc_count) {
        dtc_scroll_offset += DTC_ITEMS_PER_PAGE;
    }
}

#endif // DTC_PAGE_H
