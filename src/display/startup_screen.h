/**
 * Startup Screen - OBDeck Boot Animation
 *
 * Displays branded startup screen with animation (~3 seconds)
 */

#ifndef STARTUP_SCREEN_H
#define STARTUP_SCREEN_H

#include <TFT_eSPI.h>
#include "ui_common.h"

// External display object
extern TFT_eSPI tft;

/**
 * Show animated startup screen
 * Total duration: ~3 seconds
 *
 * Animation includes:
 * - OBDeck branding
 * - Vehicle information
 * - Scanning line effect (diagnostic theme)
 * - Clean cyan/white color scheme
 */
void showStartupScreen() {
    // Clear screen with strip-based fill for stability
    for (int y = 0; y < SCREEN_HEIGHT; y += 10) {
        tft.fillRect(0, y, SCREEN_WIDTH, 10, COLOR_BLACK);
        delay(3);  // Fast clear (3ms * 32 strips = ~100ms)
    }

    delay(50);  // Let display settle

    // ===================================================================
    // PHASE 1: Logo & Branding (0-800ms)
    // ===================================================================

    // Draw main title "OBDeck"
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(4);
    tft.setTextDatum(TC_DATUM);  // Top center
    tft.drawString("OBDeck", SCREEN_WIDTH / 2, 60);
    delay(20);

    // Draw subtitle
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(2);
    tft.drawString("OBD2 Dashboard", SCREEN_WIDTH / 2, 110);
    delay(20);

    // Draw vehicle info
    tft.setTextColor(COLOR_GRAY, COLOR_BLACK);
    tft.setTextSize(1);
    tft.drawString("2010 Opel Corsa D", SCREEN_WIDTH / 2, 140);
    delay(20);

    delay(200);  // Hold branding for visibility

    // ===================================================================
    // PHASE 2: Scanning Line Animation (800-2600ms)
    // ===================================================================

    const int SCAN_START_Y = 160;
    const int SCAN_END_Y = 280;
    const int SCAN_CYCLES = 3;      // Number of complete scans
    const int SCAN_STEPS = 60;      // Steps per scan (smoother = more steps)
    const int SCAN_DELAY = 10;      // Delay per step (10ms * 60 steps * 3 cycles = 1800ms)

    // Draw scan area border (diagnostic screen frame)
    tft.drawRect(40, SCAN_START_Y - 5, SCREEN_WIDTH - 80, SCAN_END_Y - SCAN_START_Y + 10, COLOR_GRAY);

    for (int cycle = 0; cycle < SCAN_CYCLES; cycle++) {
        for (int step = 0; step < SCAN_STEPS; step++) {
            int y = SCAN_START_Y + (step * (SCAN_END_Y - SCAN_START_Y) / SCAN_STEPS);

            // Draw scanning line with gradient effect (3 lines for thickness)
            tft.drawLine(45, y - 1, SCREEN_WIDTH - 45, y - 1, COLOR_GRAY);
            tft.drawLine(45, y, SCREEN_WIDTH - 45, y, COLOR_CYAN);      // Main bright line
            tft.drawLine(45, y + 1, SCREEN_WIDTH - 45, y + 1, COLOR_GRAY);

            // Erase previous line (draw black line behind)
            if (step > 2) {
                int prev_y = SCAN_START_Y + ((step - 3) * (SCAN_END_Y - SCAN_START_Y) / SCAN_STEPS);
                tft.drawLine(45, prev_y - 1, SCREEN_WIDTH - 45, prev_y - 1, COLOR_BLACK);
                tft.drawLine(45, prev_y, SCREEN_WIDTH - 45, prev_y, COLOR_BLACK);
                tft.drawLine(45, prev_y + 1, SCREEN_WIDTH - 45, prev_y + 1, COLOR_BLACK);
            }

            delay(SCAN_DELAY);
        }

        // Clear last line at end of scan
        int last_y = SCAN_END_Y;
        tft.drawLine(45, last_y - 1, SCREEN_WIDTH - 45, last_y - 1, COLOR_BLACK);
        tft.drawLine(45, last_y, SCREEN_WIDTH - 45, last_y, COLOR_BLACK);
        tft.drawLine(45, last_y + 1, SCREEN_WIDTH - 45, last_y + 1, COLOR_BLACK);

        delay(50);  // Brief pause between scans
    }

    // ===================================================================
    // PHASE 3: Completion (2600-3000ms)
    // ===================================================================

    // Show "Ready" message
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("READY", SCREEN_WIDTH / 2, (SCAN_START_Y + SCAN_END_Y) / 2);
    delay(20);

    delay(350);  // Hold ready state

    // Total time: ~100ms (clear) + 200ms + 1800ms (scan) + 350ms = ~2.45 seconds
}

/**
 * Alternative: Simpler startup screen without animation
 * Duration: ~1 second
 */
void showStartupScreenSimple() {
    // Clear screen quickly
    for (int y = 0; y < SCREEN_HEIGHT; y += 10) {
        tft.fillRect(0, y, SCREEN_WIDTH, 10, COLOR_BLACK);
        delay(3);
    }
    delay(50);

    // Show title
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(4);
    tft.setTextDatum(MC_DATUM);  // Middle center
    tft.drawString("OBDeck", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30);
    delay(20);

    // Show subtitle
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(2);
    tft.drawString("OBD2 Dashboard", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
    delay(20);

    delay(800);  // Hold for visibility
}

#endif // STARTUP_SCREEN_H
