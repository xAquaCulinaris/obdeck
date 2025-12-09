/**
 * Button Navigation - Physical Button Input with UI Highlighting
 *
 * 3-button navigation system:
 * - LEFT: Move highlight to previous UI button
 * - RIGHT: Move highlight to next UI button
 * - SELECT: Activate currently highlighted button
 */

#ifndef BUTTON_NAV_H
#define BUTTON_NAV_H

#include "ui_common.h"
#include "../obd2/obd_data.h"
#include "dtc_page.h"  // For scrollDTCUp/Down functions

// ============================================================================
// BUTTON GPIO PINS
// ============================================================================

#define BTN_LEFT_PIN    16  // Navigate to previous button (D16)
#define BTN_RIGHT_PIN   17  // Navigate to next button (D17)
#define BTN_SELECT_PIN  22  // Activate highlighted button (D22)

#define DEBOUNCE_DELAY_MS 500  // Button debounce delay (increased to prevent rapid page changes)

// ============================================================================
// BUTTON DEFINITIONS
// ============================================================================

enum ButtonID {
    // Bottom Navigation Buttons (always available)
    BTN_NAV_DASHBOARD = 0,
    BTN_NAV_DTC = 1,
    BTN_NAV_CONFIG = 2,

    // DTC Page Buttons
    BTN_DTC_REFRESH = 3,
    BTN_DTC_CLEAR = 4,
    BTN_DTC_UP = 5,
    BTN_DTC_DOWN = 6,

    // Placeholder for future buttons
    BTN_MAX = 7
};

struct UIButton {
    ButtonID id;
    int x, y, w, h;       // Position and size
    bool enabled;         // Whether button is currently visible/active
    Page page;            // Which page this button belongs to (PAGE_MAX = all pages)
};

// Global button array
static UIButton ui_buttons[] = {
    // Bottom Navigation (always visible on all pages)
    {BTN_NAV_DASHBOARD, 0,   BOTTOM_NAV_Y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, true, (Page)99},  // Page 99 = always visible
    {BTN_NAV_DTC,       160, BOTTOM_NAV_Y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, true, (Page)99},
    {BTN_NAV_CONFIG,    320, BOTTOM_NAV_Y, NAV_BUTTON_WIDTH, BOTTOM_NAV_HEIGHT, true, (Page)99},

    // DTC Page Buttons (only visible on DTC page)
    {BTN_DTC_REFRESH, 290, CONTENT_Y_START + 3, 90, 26, true, PAGE_DTC},
    {BTN_DTC_CLEAR,   390, CONTENT_Y_START + 3, 85, 26, true, PAGE_DTC},
    {BTN_DTC_UP,      80,  BOTTOM_NAV_Y - 48, 140, 38, false, PAGE_DTC},  // Enabled dynamically
    {BTN_DTC_DOWN,    260, BOTTOM_NAV_Y - 48, 140, 38, false, PAGE_DTC},  // Enabled dynamically
};

// Current highlighted button index
static int current_button_index = 0;  // Start with Dashboard button

// ============================================================================
// BUTTON INITIALIZATION
// ============================================================================

/**
 * Initialize physical button GPIO pins
 */
inline void initButtonNav() {
    pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
    pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
    pinMode(BTN_SELECT_PIN, INPUT_PULLUP);

    // Start with Dashboard button highlighted
    current_button_index = BTN_NAV_DASHBOARD;

    Serial.println("✓ Button navigation initialized");
    Serial.printf("  LEFT=GPIO%d, RIGHT=GPIO%d, SELECT=GPIO%d\n",
                  BTN_LEFT_PIN, BTN_RIGHT_PIN, BTN_SELECT_PIN);
    Serial.printf("  Starting with button %d highlighted\n", current_button_index);
}

// ============================================================================
// BUTTON VISIBILITY
// ============================================================================

/**
 * Update button visibility based on current page and state
 * @param current_page Current active page
 * @param dtc_count Number of DTCs (for scroll button visibility)
 * @param dtc_scroll_offset Current DTC scroll position
 */
inline void updateButtonVisibility(Page current_page, int dtc_count, int dtc_scroll_offset) {
    // Bottom nav buttons always enabled
    ui_buttons[BTN_NAV_DASHBOARD].enabled = true;
    ui_buttons[BTN_NAV_DTC].enabled = true;
    ui_buttons[BTN_NAV_CONFIG].enabled = true;

    // DTC page buttons
    if (current_page == PAGE_DTC) {
        // Refresh button always enabled on DTC page (even with 0 DTCs)
        ui_buttons[BTN_DTC_REFRESH].enabled = true;

        // Clear button only enabled when there are DTCs to clear
        ui_buttons[BTN_DTC_CLEAR].enabled = (dtc_count > 0);

        // Scroll buttons only if DTCs need scrolling
        if (dtc_count > 4) {  // DTC_ITEMS_PER_PAGE = 4
            ui_buttons[BTN_DTC_UP].enabled = (dtc_scroll_offset > 0);
            ui_buttons[BTN_DTC_DOWN].enabled = (dtc_scroll_offset + 4 < dtc_count);
        } else {
            ui_buttons[BTN_DTC_UP].enabled = false;
            ui_buttons[BTN_DTC_DOWN].enabled = false;
        }
    } else {
        // Not on DTC page - disable all DTC buttons
        ui_buttons[BTN_DTC_REFRESH].enabled = false;
        ui_buttons[BTN_DTC_CLEAR].enabled = false;
        ui_buttons[BTN_DTC_UP].enabled = false;
        ui_buttons[BTN_DTC_DOWN].enabled = false;
    }
}

/**
 * Get list of currently visible buttons on current page
 * @param current_page Current active page
 * @param visible_buttons Output array of button indices
 * @return Number of visible buttons
 */
inline int getVisibleButtons(Page current_page, int* visible_buttons) {
    int count = 0;

    for (int i = 0; i < BTN_MAX; i++) {
        UIButton& btn = ui_buttons[i];

        // Button is visible if:
        // 1. It's enabled
        // 2. It belongs to current page OR it's a nav button (page=99)
        if (btn.enabled && (btn.page == current_page || btn.page == (Page)99)) {
            visible_buttons[count++] = i;
        }
    }

    return count;
}

// ============================================================================
// HIGHLIGHT RENDERING
// ============================================================================

/**
 * Draw highlight border around a button
 * @param button_index Index in ui_buttons array
 * @param show If true, draw highlight; if false, clear highlight
 */
inline void drawButtonHighlight(int button_index, bool show) {
    if (button_index < 0 || button_index >= BTN_MAX) {
        return;
    }

    UIButton& btn = ui_buttons[button_index];

    if (!btn.enabled) {
        return;
    }

    // Safety check: ensure coordinates are within screen bounds
    if (btn.x < 0 || btn.y < 0 || btn.x + btn.w > SCREEN_WIDTH || btn.y + btn.h > SCREEN_HEIGHT) {
        return;
    }

    if (show) {
        // Draw yellow highlight (3 pixels thick)
        for (int i = 0; i < 3; i++) {
            int x = btn.x - i - 1;
            int y = btn.y - i - 1;
            int w = btn.w + ((i + 1) * 2);
            int h = btn.h + ((i + 1) * 2);

            // Clamp to screen bounds
            if (x < 0) { w += x; x = 0; }
            if (y < 0) { h += y; y = 0; }
            if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
            if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;

            // Only draw if valid dimensions
            if (w > 0 && h > 0) {
                tft.drawRect(x, y, w, h, COLOR_YELLOW);
            }
        }
    } else {
        // Clear highlight by filling area around button with black (3 pixels thick)
        // This completely removes the yellow highlight
        for (int i = 0; i < 3; i++) {
            int x = btn.x - i - 1;
            int y = btn.y - i - 1;
            int w = btn.w + ((i + 1) * 2);
            int h = btn.h + ((i + 1) * 2);

            // Clamp to screen bounds
            if (x < 0) { w += x; x = 0; }
            if (y < 0) { h += y; y = 0; }
            if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
            if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;

            // Only draw if valid dimensions
            if (w > 0 && h > 0) {
                tft.drawRect(x, y, w, h, COLOR_BLACK);
            }
        }
    }
}

/**
 * Redraw button highlight (call after screen updates)
 * Only draws if button is currently enabled
 */
inline void refreshButtonHighlight() {
    // Only refresh if current button is valid and enabled
    if (current_button_index >= 0 && current_button_index < BTN_MAX) {
        if (ui_buttons[current_button_index].enabled) {
            drawButtonHighlight(current_button_index, true);
        }
    }
}

// ============================================================================
// NAVIGATION LOGIC
// ============================================================================

/**
 * Move highlight to next button (RIGHT button pressed)
 * @param current_page Current active page
 */
inline void navigateNextButton(Page current_page) {
    int visible_buttons[BTN_MAX];
    int visible_count = getVisibleButtons(current_page, visible_buttons);

    if (visible_count == 0) {
        return;
    }

    // Clear current highlight
    drawButtonHighlight(current_button_index, false);

    // Find current button in visible list
    int current_pos = -1;
    for (int i = 0; i < visible_count; i++) {
        if (visible_buttons[i] == current_button_index) {
            current_pos = i;
            break;
        }
    }

    // Move to next button (wrap around)
    if (current_pos == -1) {
        // Current button not visible, start from first
        current_button_index = visible_buttons[0];
    } else {
        int next_pos = (current_pos + 1) % visible_count;
        current_button_index = visible_buttons[next_pos];
    }

    // Draw new highlight
    drawButtonHighlight(current_button_index, true);
}

/**
 * Move highlight to previous button (LEFT button pressed)
 * @param current_page Current active page
 */
inline void navigatePreviousButton(Page current_page) {
    int visible_buttons[BTN_MAX];
    int visible_count = getVisibleButtons(current_page, visible_buttons);

    if (visible_count == 0) {
        return;
    }

    // Clear current highlight
    drawButtonHighlight(current_button_index, false);

    // Find current button in visible list
    int current_pos = -1;
    for (int i = 0; i < visible_count; i++) {
        if (visible_buttons[i] == current_button_index) {
            current_pos = i;
            break;
        }
    }

    // Move to previous button (wrap around)
    if (current_pos == -1) {
        // Current button not visible, start from last
        current_button_index = visible_buttons[visible_count - 1];
    } else {
        int prev_pos = (current_pos - 1 + visible_count) % visible_count;
        current_button_index = visible_buttons[prev_pos];
    }

    // Draw new highlight
    drawButtonHighlight(current_button_index, true);
}

/**
 * Activate currently highlighted button (SELECT button pressed)
 * @param current_page Current page reference (may be changed by function)
 * @param page_needs_redraw Redraw flag (set to true if page changes)
 * @param dtc_count Current DTC count
 * @return True if action was performed
 */
inline bool activateButton(Page& current_page, bool& page_needs_redraw, int dtc_count) {
    ButtonID btn_id = ui_buttons[current_button_index].id;

    switch (btn_id) {
        // Bottom Navigation
        case BTN_NAV_DASHBOARD:
            if (current_page != PAGE_DASHBOARD) {
                current_page = PAGE_DASHBOARD;
                page_needs_redraw = true;
            }
            return true;

        case BTN_NAV_DTC:
            if (current_page != PAGE_DTC) {
                current_page = PAGE_DTC;
                page_needs_redraw = true;
            }
            return true;

        case BTN_NAV_CONFIG:
            if (current_page != PAGE_CONFIG) {
                current_page = PAGE_CONFIG;
                page_needs_redraw = true;
            }
            return true;

        // DTC Actions
        case BTN_DTC_REFRESH:
            Serial.println("[Button] Refresh DTCs requested");
            // Set flag for OBD2 task to handle (thread-safe)
            xSemaphoreTake(data_mutex, portMAX_DELAY);
            obd_data.dtc_refresh_requested = true;
            xSemaphoreGive(data_mutex);
            return true;

        case BTN_DTC_CLEAR:
            Serial.println("[Button] Clear all DTCs requested");
            // Set flag for OBD2 task to handle (thread-safe)
            xSemaphoreTake(data_mutex, portMAX_DELAY);
            obd_data.dtc_clear_requested = true;
            xSemaphoreGive(data_mutex);
            return true;

        case BTN_DTC_UP:
            if (ui_buttons[BTN_DTC_UP].enabled) {
                scrollDTCUp();
                page_needs_redraw = true;
            }
            return true;

        case BTN_DTC_DOWN:
            if (ui_buttons[BTN_DTC_DOWN].enabled) {
                scrollDTCDown(dtc_count);
                page_needs_redraw = true;
            }
            return true;

        default:
            return false;
    }
}

/**
 * Handle physical button presses
 * @param current_page Current page reference (may be changed)
 * @param page_needs_redraw Redraw flag
 * @param dtc_count Current DTC count
 */
inline void handleButtonInput(Page& current_page, bool& page_needs_redraw, int dtc_count) {
    static unsigned long last_button_time = 0;

    // Debounce - ignore button presses within 300ms
    if (millis() - last_button_time < DEBOUNCE_DELAY_MS) {
        return;
    }

    // Check buttons (active LOW with pull-up resistors)
    bool left_pressed = (digitalRead(BTN_LEFT_PIN) == LOW);
    bool right_pressed = (digitalRead(BTN_RIGHT_PIN) == LOW);
    bool select_pressed = (digitalRead(BTN_SELECT_PIN) == LOW);

    if (left_pressed) {
        navigatePreviousButton(current_page);
        last_button_time = millis();
    }
    else if (right_pressed) {
        navigateNextButton(current_page);
        last_button_time = millis();
    }
    else if (select_pressed) {
        activateButton(current_page, page_needs_redraw, dtc_count);
        last_button_time = millis();
    }
}

#endif // BUTTON_NAV_H
