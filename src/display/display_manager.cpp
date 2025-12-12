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
        delay(25);  // Increased delay between strips
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
    static uint8_t last_dtc_count = 0;  // Initialize to 0 (matches obd_data initial state)
    static bool needs_full_redraw = true;
    static bool disconnection_screen_drawn = false;
    static uint8_t animation_state = 0;
    static bool has_been_connected = false;  // Track if we've ever been connected

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

    // Detect connection state changes
    bool connection_state_changed = (data_copy.connected != last_connected);
    bool is_disconnecting = (connection_state_changed && !data_copy.connected);
    bool is_reconnecting = (connection_state_changed && data_copy.connected && has_been_connected);

    // DTC count change should only trigger full redraw on DTC page (to update the list)
    // On other pages, just the top bar status indicator needs updating (handled by partial redraw)
    bool dtc_changed_on_dtc_page = (current_page == PAGE_DTC &&
                                     data_copy.dtc_count != last_dtc_count);

    // Check if full redraw is needed
    // Full redraw on: page change, disconnection, reconnection, or DTC change while viewing DTC page
    bool do_full_redraw = (page_needs_redraw ||
                           is_disconnecting ||
                           is_reconnecting ||
                           dtc_changed_on_dtc_page ||
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

    // Handle initial connection (first time connecting after startup)
    bool is_initial_connection = (connection_state_changed &&
                                   data_copy.connected &&
                                   !has_been_connected);

    // Full redraw if page changed or connection state changed
    if (do_full_redraw) {
        Serial.println("[Display] Starting full redraw...");

        // Explicitly clear ALL nav button highlights before full redraw
        // This ensures old highlights don't persist through page changes
        // We clear all three nav buttons with black to completely remove any highlights
        Serial.println("[Display] Clearing all button highlights...");
        for (int i = 0; i < 3; i++) {  // Clear all 3 nav buttons
            // Clear with BLACK to completely remove highlight, regardless of page state
            for (int j = 0; j < 2; j++) {
                int btn_x = i * 160;  // NAV_BUTTON_WIDTH = 160
                int btn_y = 280;      // BOTTOM_NAV_Y
                int x = btn_x + j + 1;
                int y = btn_y + j + 1;
                int w = 160 - ((j + 1) * 2);
                int h = 40 - ((j + 1) * 2);
                if (w > 4 && h > 4) {
                    tft.drawRect(x, y, w, h, COLOR_BLACK);
                }
            }
        }
        delay(50);

        // Clear screen first (use safe fill to avoid power spike)
        Serial.println("[Display] Clearing screen...");
        safeFillScreen(COLOR_BLACK);
        Serial.println("[Display] Screen cleared");
        delay(100);  // Longer delay after full screen clear

        // Draw top bar
        Serial.println("[Display] Drawing top bar...");
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);
        Serial.println("[Display] Top bar drawn");
        delay(100);  // Longer delay between major UI sections

        // Draw bottom navigation
        Serial.println("[Display] Drawing bottom nav...");
        drawBottomNav(current_page);
        Serial.println("[Display] Bottom nav drawn");
        delay(100);  // Longer delay before drawing page content

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
    // Handle initial connection without full redraw (just clear error screen area)
    else if (is_initial_connection) {
        Serial.println("[Display] Initial connection - clearing error screen...");

        // Clear only the content area (not the entire screen)
        const int strip_height = 10;
        for (int y = CONTENT_Y_START; y < SCREEN_HEIGHT - BOTTOM_NAV_HEIGHT; y += strip_height) {
            int height = min(strip_height, SCREEN_HEIGHT - BOTTOM_NAV_HEIGHT - y);
            tft.fillRect(0, y, SCREEN_WIDTH, height, COLOR_BLACK);
            delay(10);
        }
        delay(50);

        // Update top bar only (status color may have changed)
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);
        delay(50);

        // Mark for connected
        last_connected = data_copy.connected;

        Serial.println("[Display] Error screen cleared, ready for dashboard...");
    }
    // Update top bar if DTC count or status changed (but not full redraw)
    else if (data_copy.dtc_count != last_dtc_count || connection_state_changed) {
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);
        last_dtc_count = data_copy.dtc_count;
        delay(50);
    }

    // Track that we've been connected
    if (data_copy.connected) {
        has_been_connected = true;
    }

    if (!data_copy.connected) {
        // Show connection error
        int center_y = CONTENT_Y_START + (CONTENT_HEIGHT / 2) - 60;

        // Draw static parts only once
        if (!disconnection_screen_drawn || do_full_redraw) {
            // Content area already cleared above - just draw the error box

            // Red box background - USE STRIPS to avoid power spike!
            const int strip_height = 10;
            for (int y_offset = 0; y_offset < 120; y_offset += strip_height) {
                tft.fillRect(50, center_y + y_offset, SCREEN_WIDTH - 100, strip_height, COLOR_DARKGRAY);
                delay(15);  // Increased delay between strips
            }
            delay(50);  // Extra delay after completing background

            // Draw borders
            tft.drawRect(50, center_y, SCREEN_WIDTH - 100, 120, COLOR_WHITE);
            delay(20);  // Delay after drawRect
            tft.drawRect(51, center_y + 1, SCREEN_WIDTH - 102, 118, COLOR_WHITE);
            delay(20);  // Delay after drawRect

            // "Connecting" text
            tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
            delay(10);
            tft.setTextSize(3);
            delay(10);
            tft.setCursor(150, center_y + 30);
            delay(10);
            tft.print("Connecting");
            delay(50);  // Delay after text print

            // Error message
            if (data_copy.error[0] != '\0') {
                tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
                delay(10);
                tft.setTextSize(1);
                delay(10);
                tft.setCursor(70, center_y + 55);
                delay(10);
                tft.print(data_copy.error);
                delay(50);  // Delay after text print
            }

            // Reset text settings to prevent corruption
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            delay(10);
            tft.setTextSize(1);
            delay(10);

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
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        delay(10);
        tft.setTextSize(2);
        delay(10);
        tft.setTextDatum(TL_DATUM);
        delay(10);
        tft.setTextPadding(100);  // Clear old dots automatically
        delay(10);
        tft.drawString(dots, 220, center_y + 80);
        delay(50);  // Delay after drawString (uses fillRect internally with padding)

        // Reset text settings after animation
        tft.setTextPadding(0);
        delay(10);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        delay(10);
        tft.setTextSize(1);
        delay(10);
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
        Serial.printf("[Display] Refreshing button highlight: button_index=%d, page=%d\n",
                      current_button_index, current_page);
        refreshButtonHighlight(current_page);
    }
}
