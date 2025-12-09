/**
 * Display Manager Module - Implementation
 */

#include "display_manager.h"
#include "obd2/obd_data.h"
#include "nav_bar.h"
#include "dashboard.h"
#include "dtc_page.h"
#include "config_page.h"
#include "button_nav.h"

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

/**
 * Safe fillScreen that avoids power spikes
 * Uses horizontal strips instead of full screen fill
 */
void safeFillScreen(uint16_t color) {
    const int strip_height = 10;  // Draw in 10px strips
    for (int y = 0; y < SCREEN_HEIGHT; y += strip_height) {
        tft.fillRect(0, y, SCREEN_WIDTH, strip_height, color);
        delay(10);  // Minimal safe delay
    }
}

void initDisplay() {
    Serial.println("Initializing display...");
    Serial.printf("TFT object address: %p\n", &tft);

    tft.init();
    delay(100);  // Give display time to initialize
    Serial.println("✓ tft.init() completed");

    Serial.println("Setting rotation...");
    tft.setRotation(SCREEN_ROTATION);
    delay(50);
    Serial.printf("✓ Rotation set to %d\n", SCREEN_ROTATION);

    Serial.println("Clearing screen to BLACK...");
    safeFillScreen(COLOR_BLACK);  // Use safe fill
    delay(50);
    Serial.printf("✓ Screen cleared (COLOR_BLACK = 0x%04X)\n", COLOR_BLACK);

    Serial.println("✓ Display initialized");
}

void drawCurrentPage(Page current_page, bool& page_needs_redraw) {
    static int draw_count = 0;
    static uint16_t last_rpm = 0xFFFF;
    static uint8_t last_speed = 0xFF;
    static float last_coolant = -999;
    static float last_throttle = -999;
    static float last_battery = -999;
    static float last_intake = -999;
    static bool last_connected = false;
    static uint8_t last_dtc_count = 0xFF;
    static bool needs_full_redraw = true;
    static bool disconnection_screen_drawn = false;
    static uint8_t animation_state = 0;

    draw_count++;

    // Debug output every 50 frames
    if (draw_count % 50 == 1) {
        Serial.printf("[Display] Frame %d, Page=%d, Redraw=%d\n",
                      draw_count, current_page, page_needs_redraw);
    }

    // Get data copy (thread-safe)
    OBDData data_copy;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    data_copy = obd_data;
    xSemaphoreGive(data_mutex);

    // Update button visibility based on current state
    updateButtonVisibility(current_page, data_copy.dtc_count, getDTCScrollOffset());

    // Check if full redraw is needed
    bool do_full_redraw = (page_needs_redraw ||
                           data_copy.connected != last_connected ||
                           data_copy.dtc_count != last_dtc_count ||
                           needs_full_redraw);

    // Determine page name
    const char* page_name;
    switch (current_page) {
        case PAGE_DASHBOARD: page_name = "Dashboard"; break;
        case PAGE_DTC:       page_name = "DTC Codes"; break;
        case PAGE_CONFIG:    page_name = "Config"; break;
        default:             page_name = "Unknown"; break;
    }

    // Determine status color
    uint16_t status_color = STATUS_OK;
    if (!data_copy.connected) {
        status_color = STATUS_ERROR;
    } else if (data_copy.dtc_count > 0) {
        bool has_critical = false;
        for (int i = 0; i < data_copy.dtc_count; i++) {
            if (data_copy.dtc_codes[i].severity == DTC_SEVERITY_CRITICAL) {
                has_critical = true;
                break;
            }
        }
        status_color = has_critical ? STATUS_ERROR : STATUS_WARNING;
    }

    // Full redraw if page changed or connection state changed
    if (do_full_redraw) {
        Serial.println("[Display] Starting full redraw...");

        // Clear screen first (use safe fill to avoid power spike)
        Serial.println("[Display] Clearing screen...");
        safeFillScreen(COLOR_BLACK);
        Serial.println("[Display] Screen cleared");

        // Draw top bar
        Serial.println("[Display] Drawing top bar...");
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);
        Serial.println("[Display] Top bar drawn");

        // Draw bottom navigation
        Serial.println("[Display] Drawing bottom nav...");
        drawBottomNav(current_page);
        Serial.println("[Display] Bottom nav drawn");

        // Reset DTC scroll when switching to DTC page
        if (current_page == PAGE_DTC) {
            resetDTCScroll();
        }

        // Reset flags
        page_needs_redraw = false;
        last_connected = data_copy.connected;
        last_dtc_count = data_copy.dtc_count;
        needs_full_redraw = false;

        // Force redraw of all values
        last_rpm = 0xFFFF;
        last_speed = 0xFF;
        last_coolant = -999;
        last_throttle = -999;
        last_battery = -999;
        last_intake = -999;

        // Content area already cleared by safeFillScreen above - no need to clear again
        Serial.println("[Display] Ready to draw page content...");
    }

    if (!data_copy.connected) {
        // Show connection error
        Serial.println("[Display] >>> DISCONNECTED - Drawing error screen...");
        int center_y = CONTENT_Y_START + (CONTENT_HEIGHT / 2) - 60;

        // Draw static parts only once
        if (!disconnection_screen_drawn || do_full_redraw) {
            // Content area already cleared above - just draw the error box

            // Red box background - USE STRIPS to avoid power spike!
            const int strip_height = 10;
            for (int y_offset = 0; y_offset < 120; y_offset += strip_height) {
                tft.fillRect(50, center_y + y_offset, SCREEN_WIDTH - 100, strip_height, COLOR_DARKGRAY);
                delay(10);  // Minimal safe delay
            }

            // Draw borders
            tft.drawRect(50, center_y, SCREEN_WIDTH - 100, 120, COLOR_RED);
            tft.drawRect(51, center_y + 1, SCREEN_WIDTH - 102, 118, COLOR_RED);

            // "Connection Lost" text
            tft.setTextColor(COLOR_RED, COLOR_DARKGRAY);
            tft.setTextSize(3);
            tft.setCursor(80, center_y + 20);
            tft.print("Connection Lost");

            // Error message
            if (data_copy.error[0] != '\0') {
                tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
                tft.setTextSize(1);
                tft.setCursor(70, center_y + 55);
                tft.print(data_copy.error);
            }

            // Reset text settings to prevent corruption
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(1);

            Serial.println("[Display] >>> Error screen drawn successfully!");
            disconnection_screen_drawn = true;
        }

        // Animate "Reconnecting" with dots (updates every frame)
        animation_state = (animation_state + 1) % 4;

        // Build dots string
        char dots[5] = "";
        for (int i = 0; i < animation_state; i++) {
            dots[i] = '.';
        }
        dots[animation_state] = '\0';

        // Draw animated dots - use drawString instead of print
        tft.setTextColor(COLOR_YELLOW, COLOR_DARKGRAY);
        tft.setTextSize(2);
        tft.setTextDatum(TL_DATUM);
        tft.setTextPadding(100);  // Clear old dots automatically
        tft.drawString(dots, 235, center_y + 80);

        // Reset text settings after animation
        tft.setTextPadding(0);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(1);
    } else {
        // Reset disconnection screen flag when connected
        disconnection_screen_drawn = false;

        // Draw page content based on current page
        if (current_page == PAGE_DASHBOARD) {
            Serial.println("[Display] >>> STEP 5: Drawing dashboard page...");
            bool is_full_redraw = (last_rpm == 0xFFFF);
            drawDashboardPage(
                data_copy.rpm, last_rpm,
                data_copy.speed, last_speed,
                data_copy.coolant_temp, last_coolant,
                data_copy.throttle, last_throttle,
                data_copy.battery_voltage, last_battery,
                data_copy.intake_temp, last_intake,
                is_full_redraw
            );

            Serial.println("[Display] Dashboard page drawn OK");

            last_rpm = data_copy.rpm;
            last_speed = data_copy.speed;
            last_coolant = data_copy.coolant_temp;
            last_throttle = data_copy.throttle;
            last_battery = data_copy.battery_voltage;
            last_intake = data_copy.intake_temp;
        } else if (current_page == PAGE_DTC) {
            if (do_full_redraw) {
                // Already cleared above, just draw page
                drawDTCPage(data_copy.dtc_codes, data_copy.dtc_count);
            }
        } else if (current_page == PAGE_CONFIG) {
            // Config page is static - only draw on page change
            if (do_full_redraw) {
                // Already cleared above, just draw page
                drawConfigPage();
            }
        }
    }

    // Redraw button highlight (only on full redraw to avoid text buffer issues)
    if (do_full_redraw) {
        refreshButtonHighlight();
    }
}
