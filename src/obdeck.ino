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
 * - Core 0: OBD2 communication task (obd2/obd2_task.cpp)
 * - Core 1: Display rendering (main loop)
 *
 * Author: OBDeck Project
 * Platform: PlatformIO + Arduino Framework
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// OBD2 Communication Module
#include "obd2/obd2_task.h"

// Display Manager
#include "display/display_manager.h"
#include "display/button_nav.h"

// ============================================================================
// GLOBAL STATE
// ============================================================================

// Display object (must be in .ino file for Arduino build system)
TFT_eSPI tft = TFT_eSPI();

// Current page state
Page current_page = PAGE_DASHBOARD;  // Start with Dashboard page
bool page_needs_redraw = true;

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

    // Initialize OBD2 module (creates mutex and initializes Bluetooth)
    initOBD2();

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
        drawCurrentPage(current_page, page_needs_redraw);
        last_update = millis();
        first_draw = false;
    }
    // Update display at refresh rate
    else if (millis() - last_update >= DISPLAY_REFRESH_MS) {
        drawCurrentPage(current_page, page_needs_redraw);
        last_update = millis();
    }

    delay(10);  // Small delay to prevent WDT issues
}
