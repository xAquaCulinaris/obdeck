/**
 * OBDeck - ESP32 OBD2 Display System
 * 2010 Opel Corsa D Real-time Diagnostics
 *
 * Hardware:
 * - ESP32-WROOM-32 (Dual-core 240MHz)
 * - ILI9488 3.5" TFT Display (480x320 SPI)
 * - ELM327 Bluetooth OBD2 Adapter
 *
 * Architecture:
 * - Core 0: OBD2 communication task (obd2/elm327.cpp)
 * - Core 1: Display rendering (main loop)
 *
 * Author: OBDeck Project
 * Platform: PlatformIO + Arduino Framework
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
// Note: XPT2046_Touchscreen removed - using TFT_eSPI's built-in touch support
#include "config.h"

// OBD2 Communication Module
#include "obd2/elm327.h"

// UI Modules
#include "ui/ui_common.h"
#include "ui/nav_bar.h"
#include "ui/dashboard.h"
#include "ui/dtc_page.h"
#include "ui/config_page.h"
#include "ui/button_nav.h"  // Physical button navigation

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

TFT_eSPI tft = TFT_eSPI();

// Current page state
Page current_page = PAGE_DASHBOARD;  // Start with Dashboard page
bool page_needs_redraw = true;

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

/**
 * Initialize display
 */
void initDisplay() {
    Serial.println("Initializing display...");

    tft.init();
    delay(100);  // Give display time to initialize

    Serial.println("Setting rotation...");
    tft.setRotation(SCREEN_ROTATION);
    delay(50);

    Serial.println("Clearing screen...");
    tft.fillScreen(COLOR_BLACK);
    delay(50);

    Serial.println("✓ Display initialized");
}

/**
 * Draw current page with smart partial updates (no flickering!)
 */
void drawCurrentPage() {
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
        // Start SPI transaction for entire redraw operation
        tft.startWrite();

        tft.fillScreen(COLOR_BLACK);
        delay(10);  // Give display time to process

        // Draw top bar and bottom navigation
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);
        drawBottomNav(current_page);

        // End SPI transaction and flush buffer
        tft.endWrite();
        delay(20);  // Longer delay to ensure buffer is flushed

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

        // Clear content area with explicit transaction
        tft.startWrite();
        tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
        tft.endWrite();
        delay(20);  // Give display time to process and flush buffer
    }

    if (!data_copy.connected) {
        // Show connection error
        int center_y = CONTENT_Y_START + (CONTENT_HEIGHT / 2) - 60;

        // Draw static parts only once
        if (!disconnection_screen_drawn || do_full_redraw) {
            tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);

            // Red box background
            tft.fillRect(50, center_y, SCREEN_WIDTH - 100, 120, COLOR_DARKGRAY);
            tft.drawRect(50, center_y, SCREEN_WIDTH - 100, 120, COLOR_RED);
            tft.drawRect(51, center_y + 1, SCREEN_WIDTH - 102, 118, COLOR_RED);

            // "Connection Lost" text - use drawString instead of print
            tft.setTextColor(COLOR_RED, COLOR_DARKGRAY);
            tft.setTextSize(3);
            tft.setTextDatum(TL_DATUM);
            tft.setTextPadding(300);
            tft.drawString("Connection Lost", 80, center_y + 20);

            // Error message - use drawString instead of print
            if (data_copy.error[0] != '\0') {
                tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
                tft.setTextSize(1);
                tft.setTextDatum(TL_DATUM);
                tft.setTextPadding(300);
                tft.drawString(data_copy.error, 70, center_y + 55);
            }

            // Reset text settings to prevent corruption
            tft.setTextPadding(0);
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(1);

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

            last_rpm = data_copy.rpm;
            last_speed = data_copy.speed;
            last_coolant = data_copy.coolant_temp;
            last_throttle = data_copy.throttle;
            last_battery = data_copy.battery_voltage;
            last_intake = data_copy.intake_temp;
        } else if (current_page == PAGE_DTC) {
            if (do_full_redraw) {
                tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
                drawDTCPage(data_copy.dtc_codes, data_copy.dtc_count);
            }
        } else if (current_page == PAGE_CONFIG) {
            drawConfigPage();
        }
    }

    // Redraw button highlight (only on full redraw to avoid text buffer issues)
    if (do_full_redraw) {
        refreshButtonHighlight();
    }
}

// ============================================================================
// ARDUINO SETUP & LOOP
// ============================================================================

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("OBDeck - ESP32 OBD2 Display System");
    Serial.println("========================================");
    Serial.printf("Hardware: %s %d\n", VEHICLE_NAME, VEHICLE_YEAR);
    Serial.printf("Display: ILI9488 %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    Serial.println("OBD2: ELM327 Bluetooth");
    Serial.println("========================================\n");

    // Initialize ELM327 communication
    initELM327();

    // Initialize display
    initDisplay();

    // Initialize physical button navigation
    initButtonNav();

    // Show startup message with animated dots - use drawString instead of print
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(200);
    tft.drawString("OBDeck", 80, 100);

    // Animate "Connecting" with dots (8 cycles = ~2 seconds)
    for (int i = 0; i < 8; i++) {
        // Build connecting text with dots
        char connecting_text[20] = "Connecting";
        int dots = i % 4;
        for (int j = 0; j < dots; j++) {
            connecting_text[10 + j] = '.';
        }
        connecting_text[10 + dots] = '\0';

        // Draw text with drawString instead of print
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setTextDatum(TL_DATUM);
        tft.setTextPadding(250);
        tft.drawString(connecting_text, 100, 140);

        delay(250);  // 250ms per frame = 4 fps
    }

    // Reset text settings
    tft.setTextPadding(0);

    // Start OBD2 task on Core 0
    xTaskCreatePinnedToCore(
        obd2Task,                // Task function
        "OBD2Task",              // Task name
        OBD2_TASK_STACK_SIZE,    // Stack size
        NULL,                    // Parameters
        OBD2_TASK_PRIORITY,      // Priority
        NULL,                    // Task handle
        OBD2_TASK_CORE           // Core (0)
    );

    Serial.println("✓ OBD2 task started on Core 0");
    Serial.println("\nSetup complete! Entering main loop...\n");
}

void loop() {
    // Display task runs on Core 1 (main loop)
    static unsigned long last_update = 0;
    static bool first_draw = true;
    static bool loop_started = false;

    // Debug: Confirm loop is running
    if (!loop_started) {
        Serial.println("[Loop] Main loop started on Core 1");
        Serial.printf("[Loop] millis=%lu, DISPLAY_REFRESH_MS=%d\n", millis(), DISPLAY_REFRESH_MS);
        loop_started = true;
    }

    // Handle physical button input for navigation
    OBDData data_copy_buttons;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    data_copy_buttons = obd_data;
    xSemaphoreGive(data_mutex);
    handleButtonInput(current_page, page_needs_redraw, data_copy_buttons.dtc_count);

    // Force immediate first draw to clear startup screen
    if (first_draw) {
        Serial.println("[Display] First draw - clearing startup screen");
        drawCurrentPage();
        last_update = millis();
        first_draw = false;
    }
    // Update display at refresh rate
    else if (millis() - last_update >= DISPLAY_REFRESH_MS) {
        drawCurrentPage();
        last_update = millis();
    }

    delay(10);  // Small delay to prevent WDT issues
}
