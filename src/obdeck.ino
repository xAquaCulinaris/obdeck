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
#include <XPT2046_Touchscreen.h>
#include "config.h"

// OBD2 Communication Module
#include "obd2/elm327.h"

// UI Modules
#include "ui/ui_common.h"
#include "ui/nav_bar.h"
#include "ui/dashboard.h"
#include "ui/dtc_page.h"
#include "ui/config_page.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS);

// Current page state
Page current_page = PAGE_DTC;  // Start with DTC page (until touch is working)
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
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BLACK);

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
    static uint8_t last_dtc_count = 0xFF;  // Track DTC count changes
    static bool needs_full_redraw = true;
    static bool disconnection_screen_drawn = false;  // Track if disconnection screen is already drawn
    static uint8_t animation_state = 0;  // For animated dots (0-3)

    draw_count++;

    // Get data copy (thread-safe)
    OBDData data_copy;
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    data_copy = obd_data;
    xSemaphoreGive(data_mutex);

    // Debug output every 10 draws
    if (draw_count % 10 == 1) {
        Serial.printf("[Display] drawCurrentPage called (count=%d, connected=%d, page=%d)\n",
                      draw_count, data_copy.connected, current_page);
    }

    // Check if full redraw is needed (including when DTCs change)
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

    // Determine status color (red if DTCs present)
    uint16_t status_color = STATUS_OK;
    if (!data_copy.connected) {
        status_color = STATUS_ERROR;
    } else if (data_copy.dtc_count > 0) {
        // Check for critical DTCs
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
        Serial.println("[Display] Full screen redraw");
        tft.fillScreen(COLOR_BLACK);

        // Draw top bar
        drawTopBar("OBDeck", page_name, status_color, data_copy.dtc_count);

        // Draw bottom navigation
        drawBottomNav(current_page);

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

        // Clear content area only on full redraw
        tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
    }

    if (!data_copy.connected) {
        // Show connection error - prominent and centered
        int center_y = CONTENT_Y_START + (CONTENT_HEIGHT / 2) - 60;

        // Draw static parts only once (no flickering)
        if (!disconnection_screen_drawn || do_full_redraw) {
            // Clear content area for connection error display
            tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);

            // Red box background
            tft.fillRect(50, center_y, SCREEN_WIDTH - 100, 120, COLOR_DARKGRAY);
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

            disconnection_screen_drawn = true;
        }

        // Animate "Reconnecting" with dots (updates every frame)
        animation_state = (animation_state + 1) % 4;  // Cycle 0-3

        // Clear animation area only
        tft.fillRect(70, center_y + 75, 340, 35, COLOR_DARKGRAY);

        tft.setTextColor(COLOR_YELLOW, COLOR_DARKGRAY);
        tft.setTextSize(2);
        tft.setCursor(90, center_y + 80);
        tft.print("Reconnecting");

        // Draw animated dots
        for (int i = 0; i < animation_state; i++) {
            tft.print(".");
        }
    } else {
        // Reset disconnection screen flag when connected
        disconnection_screen_drawn = false;
        // Draw page content based on current page
        if (current_page == PAGE_DASHBOARD) {
            // Smart partial updates for dashboard
            bool is_full_redraw = (last_rpm == 0xFFFF);  // Check if this is first draw
            drawDashboardPage(
                data_copy.rpm, last_rpm,
                data_copy.speed, last_speed,
                data_copy.coolant_temp, last_coolant,
                data_copy.throttle, last_throttle,
                data_copy.battery_voltage, last_battery,
                data_copy.intake_temp, last_intake,
                is_full_redraw
            );

            // Update last values
            last_rpm = data_copy.rpm;
            last_speed = data_copy.speed;
            last_coolant = data_copy.coolant_temp;
            last_throttle = data_copy.throttle;
            last_battery = data_copy.battery_voltage;
            last_intake = data_copy.intake_temp;
        } else if (current_page == PAGE_DTC) {
            // Draw DTC page only on full redraw (DTCs are static)
            if (do_full_redraw) {
                tft.fillRect(0, CONTENT_Y_START, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
                drawDTCPage(data_copy.dtc_codes, data_copy.dtc_count);
            }
        } else if (current_page == PAGE_CONFIG) {
            drawConfigPage();
        }
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

    // Initialize touch controller (disabled for now - no wiring yet)
    if (!touch.begin()) {
        Serial.println("WARNING: Touch controller not found!");
        // Continue anyway - touch not required for basic operation
    } else {
        Serial.println("✓ Touch controller initialized (currently disabled)");
    }

    // Show startup message with animated dots
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(80, 100);
    tft.print("OBDeck");

    // Animate "Connecting" with dots (8 cycles = ~2 seconds)
    for (int i = 0; i < 8; i++) {
        // Clear previous text
        tft.fillRect(80, 135, 320, 30, COLOR_BLACK);

        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(100, 140);
        tft.print("Connecting");

        // Draw dots (0-3)
        int dots = i % 4;
        for (int j = 0; j < dots; j++) {
            tft.print(".");
        }

        delay(250);  // 250ms per frame = 4 fps
    }

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

    // TOUCH HANDLING DISABLED (no wiring yet)
    // Touch will be enabled later when hardware is connected
    // handleTouch();

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
