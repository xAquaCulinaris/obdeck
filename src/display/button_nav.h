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
// BUTTON CONFIGURATION
// ============================================================================
// Pin definitions are in config.h: BTN_LEFT, BTN_RIGHT, BTN_SELECT

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

// Current highlighted button index (declared here, defined in obdeck.ino)
extern int current_button_index;

// ============================================================================
// BUTTON INITIALIZATION
// ============================================================================

/**
 * Initialize physical button GPIO pins
 */
inline void initButtonNav() {
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    Serial.println("✓ Button navigation initialized");
    Serial.printf("  LEFT=GPIO%d, RIGHT=GPIO%d, SELECT=GPIO%d\n",
                  BTN_LEFT, BTN_RIGHT, BTN_SELECT);
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

    // Safety check: if current button is not visible/enabled on current page,
    // reset to the appropriate nav button for this page
    if (current_button_index >= 0 && current_button_index < BTN_MAX) {
        UIButton& current_btn = ui_buttons[current_button_index];

        // Check if current button should be visible on current page
        bool should_be_visible = current_btn.enabled &&
                                 (current_btn.page == current_page || current_btn.page == (Page)99);

        if (!should_be_visible) {
            // Current button not visible - reset to appropriate nav button
            switch (current_page) {
                case PAGE_DASHBOARD:
                    current_button_index = BTN_NAV_DASHBOARD;
                    break;
                case PAGE_DTC:
                    current_button_index = BTN_NAV_DTC;
                    break;
                case PAGE_CONFIG:
                    current_button_index = BTN_NAV_CONFIG;
                    break;
                default:
                    current_button_index = BTN_NAV_DASHBOARD;
                    break;
            }
        }
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
 * @param current_page Current active page (needed to determine nav button colors)
 */
inline void drawButtonHighlight(int button_index, bool show, Page current_page) {
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
        // Draw white highlight INSIDE button boundaries (2 pixels thick)
        // This prevents overlap with content areas above/below buttons
        for (int i = 0; i < 2; i++) {
            int x = btn.x + i + 1;
            int y = btn.y + i + 1;
            int w = btn.w - ((i + 1) * 2);
            int h = btn.h - ((i + 1) * 2);

            // Only draw if valid dimensions
            if (w > 4 && h > 4) {
                tft.drawRect(x, y, w, h, COLOR_WHITE);
            }
        }
    } else {
        // Clear highlight by redrawing button border area
        // Since highlight is inside button, we redraw the inner border area in button's background color
        for (int i = 0; i < 2; i++) {
            int x = btn.x + i + 1;
            int y = btn.y + i + 1;
            int w = btn.w - ((i + 1) * 2);
            int h = btn.h - ((i + 1) * 2);

            // Only draw if valid dimensions
            if (w > 4 && h > 4) {
                // Determine clear color based on button type
                uint16_t clear_color = COLOR_BLACK;

                if (btn.id >= BTN_NAV_DASHBOARD && btn.id <= BTN_NAV_CONFIG) {
                    // Nav buttons: check if this button is the active page
                    bool is_active_page = false;
                    if (btn.id == BTN_NAV_DASHBOARD && current_page == PAGE_DASHBOARD) is_active_page = true;
                    if (btn.id == BTN_NAV_DTC && current_page == PAGE_DTC) is_active_page = true;
                    if (btn.id == BTN_NAV_CONFIG && current_page == PAGE_CONFIG) is_active_page = true;

                    // Use GRAY for active page button, DARKGRAY for inactive
                    clear_color = is_active_page ? COLOR_GRAY : COLOR_DARKGRAY;
                } else if (btn.id == BTN_DTC_REFRESH) {
                    clear_color = COLOR_BLUE;  // Refresh button background
                } else if (btn.id == BTN_DTC_CLEAR) {
                    clear_color = COLOR_RED;  // Clear All button background
                } else if (btn.id == BTN_DTC_UP || btn.id == BTN_DTC_DOWN) {
                    // Scroll buttons: use BLUE if enabled, DARKGRAY if disabled
                    clear_color = btn.enabled ? COLOR_BLUE : COLOR_DARKGRAY;
                }

                tft.drawRect(x, y, w, h, clear_color);
            }
        }
    }
}

/**
 * Redraw button highlight (call after screen updates)
 * Only draws if button is currently enabled
 * @param current_page Current active page
 */
inline void refreshButtonHighlight(Page current_page) {
    // Only refresh if current button is valid and enabled
    if (current_button_index >= 0 && current_button_index < BTN_MAX) {
        if (ui_buttons[current_button_index].enabled) {
            drawButtonHighlight(current_button_index, true, current_page);
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
    drawButtonHighlight(current_button_index, false, current_page);

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
    drawButtonHighlight(current_button_index, true, current_page);
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
    drawButtonHighlight(current_button_index, false, current_page);

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
    drawButtonHighlight(current_button_index, true, current_page);
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
                Serial.println("[Button] Switching to Dashboard page");
                current_page = PAGE_DASHBOARD;
                page_needs_redraw = true;
                // Keep Dashboard button highlighted after page change
                current_button_index = BTN_NAV_DASHBOARD;
                Serial.printf("[Button] Set current_button_index = %d (Dashboard)\n", current_button_index);
            }
            return true;

        case BTN_NAV_DTC:
            if (current_page != PAGE_DTC) {
                Serial.println("[Button] Switching to DTC page");
                current_page = PAGE_DTC;
                page_needs_redraw = true;
                // Keep DTC button highlighted after page change
                current_button_index = BTN_NAV_DTC;
                Serial.printf("[Button] Set current_button_index = %d (DTC)\n", current_button_index);
            }
            return true;

        case BTN_NAV_CONFIG:
            if (current_page != PAGE_CONFIG) {
                Serial.println("[Button] Switching to Config page");
                current_page = PAGE_CONFIG;
                page_needs_redraw = true;
                // Keep Config button highlighted after page change
                current_button_index = BTN_NAV_CONFIG;
                Serial.printf("[Button] Set current_button_index = %d (Config)\n", current_button_index);
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
    bool left_pressed = (digitalRead(BTN_LEFT) == LOW);
    bool right_pressed = (digitalRead(BTN_RIGHT) == LOW);
    bool select_pressed = (digitalRead(BTN_SELECT) == LOW);

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
