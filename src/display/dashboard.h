/**
 * Dashboard Page - Main OBD2 Data Display
 *
 * Shows 6 key metrics in a 2x3 boxed grid layout with smart partial updates
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "ui_common.h"

/**
 * Draw dashboard with smart partial updates and boxed layout
 * Only redraws values that have changed to prevent flickering
 *
 * @param rpm Current RPM value
 * @param last_rpm Previous RPM value
 * @param speed Current speed (km/h)
 * @param last_speed Previous speed
 * @param coolant Current coolant temp (°C)
 * @param last_coolant Previous coolant temp
 * @param throttle Current throttle position (%)
 * @param last_throttle Previous throttle position
 * @param battery Current battery voltage (V)
 * @param last_battery Previous battery voltage
 * @param intake Current intake temp (°C)
 * @param last_intake Previous intake temp
 * @param force_full_redraw Force complete redraw (for page changes)
 */
inline void drawDashboardPage(uint16_t rpm, uint16_t last_rpm,
                              uint8_t speed, uint8_t last_speed,
                              float coolant, float last_coolant,
                              float throttle, float last_throttle,
                              float battery, float last_battery,
                              float intake, float last_intake,
                              bool force_full_redraw) {

    static bool first_draw = true;

    // Reset first_draw if full redraw requested
    if (force_full_redraw) {
        first_draw = true;
    }

    // Layout: 2 columns x 3 rows with boxes
    const int margin = 5;
    const int box_width = (SCREEN_WIDTH - 3 * margin) / 2;  // 2 columns
    const int box_height = (CONTENT_HEIGHT - 4 * margin) / 3;  // 3 rows
    const int start_y = CONTENT_Y_START + margin;

    // Helper function to draw a metric box
    auto drawMetricBox = [&](int col, int row, const char* label, const char* value,
                             const char* last_value, uint16_t label_color, bool force_redraw) {
        int x = margin + col * (box_width + margin);
        int y = start_y + row * (box_height + margin);

        // Draw box border and label only on first draw
        if (first_draw || force_redraw) {
            // Box border
            tft.drawRect(x, y, box_width, box_height, COLOR_GRAY);

            // Label at top - use simple print API without padding
            tft.setTextColor(label_color, COLOR_BLACK);
            tft.setTextSize(2);
            tft.setCursor(x + 10, y + 8);
            tft.print(label);
        }

        // Update value only if changed
        if (first_draw || force_redraw || strcmp(value, last_value) != 0) {
            // Clear value area manually (instead of using text padding)
            int value_y = y + 30;
            tft.fillRect(x + 5, value_y, box_width - 10, 28, COLOR_BLACK);
            delay(20);  // Minimal delay after fillRect (critical!)

            // Draw new value (centered) - use simple print API
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(3);

            // Calculate center position for text
            int text_width = strlen(value) * 18;  // Approximate width for size 3
            int text_x = x + (box_width - text_width) / 2;
            if (text_x < x + 5) text_x = x + 5;  // Ensure minimum margin

            tft.setCursor(text_x, value_y);
            tft.print(value);
        }

        // Reset text settings to prevent corruption
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(1);
    };

    // Format values
    char rpm_val[16], last_rpm_val[16];
    snprintf(rpm_val, sizeof(rpm_val), "%d", rpm);
    snprintf(last_rpm_val, sizeof(last_rpm_val), "%d", last_rpm);

    char speed_val[16], last_speed_val[16];
    snprintf(speed_val, sizeof(speed_val), "%d", speed);
    snprintf(last_speed_val, sizeof(last_speed_val), "%d", last_speed);

    char coolant_val[16], last_coolant_val[16];
    snprintf(coolant_val, sizeof(coolant_val), "%.1f", coolant);
    snprintf(last_coolant_val, sizeof(last_coolant_val), "%.1f", last_coolant);

    char throttle_val[16], last_throttle_val[16];
    snprintf(throttle_val, sizeof(throttle_val), "%.0f%%", throttle);
    snprintf(last_throttle_val, sizeof(last_throttle_val), "%.0f%%", last_throttle);

    char battery_val[16], last_battery_val[16];
    snprintf(battery_val, sizeof(battery_val), "%.1fV", battery);
    snprintf(last_battery_val, sizeof(last_battery_val), "%.1fV", last_battery);

    char intake_val[16], last_intake_val[16];
    snprintf(intake_val, sizeof(intake_val), "%.1f", intake);
    snprintf(last_intake_val, sizeof(last_intake_val), "%.1f", last_intake);

    // Draw all metrics in grid layout
    // Row 0: RPM | Speed
    drawMetricBox(0, 0, "RPM", rpm_val, last_rpm_val, COLOR_CYAN, false);
    drawMetricBox(1, 0, "Speed (km/h)", speed_val, last_speed_val, COLOR_CYAN, false);

    // Row 1: Coolant | Throttle
    drawMetricBox(0, 1, "Coolant (C)", coolant_val, last_coolant_val, COLOR_GREEN, false);
    drawMetricBox(1, 1, "Throttle", throttle_val, last_throttle_val, COLOR_YELLOW, false);

    // Row 2: Battery | Intake
    drawMetricBox(0, 2, "Battery", battery_val, last_battery_val, COLOR_GREEN, false);
    drawMetricBox(1, 2, "Intake (C)", intake_val, last_intake_val, COLOR_GREEN, false);

    Serial.println("[Dashboard] All boxes drawn");

    first_draw = false;
}

#endif // DASHBOARD_H
