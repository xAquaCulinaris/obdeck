/**
 * ILI9488 Display Stress Test
 *
 * Systematically tests display operations to find the exact cause of white screen
 * Tests are numbered - note which test causes the white screen
 */

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Colors
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_YELLOW    0xFFE0
#define COLOR_DARKGRAY  0x39E7
#define COLOR_GRAY      0x7BEF

int test_number = 0;

// Safe fillScreen that uses strips (like obdeck does)
void safeFillScreen2(uint16_t color) {
    const int strip_height = 10;
    for (int y = 0; y < 320; y += strip_height) {
        tft.fillRect(0, y, 480, strip_height, color);
        delay(30);
    }
}

void printTestHeader(const char* test_name) {
    test_number++;
    Serial.println();
    Serial.println("========================================");
    Serial.printf("TEST %d: %s\n", test_number, test_name);
    Serial.println("========================================");
    delay(2000);  // 2 second pause before each test
}

void waitForSerial() {
    Serial.println("Press any key to continue...");
    while (!Serial.available()) {
        delay(100);
    }
    while (Serial.available()) Serial.read();  // Clear buffer
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ILI9488 Display Stress Test          ║");
    Serial.println("║  Systematically find white screen bug ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // Display pin configuration
    Serial.println("Pin Configuration:");
    Serial.println("  MOSI: GPIO 23");
    Serial.println("  MISO: GPIO 19");
    Serial.println("  SCLK: GPIO 18");
    Serial.println("  CS:   GPIO 5");
    Serial.println("  DC:   GPIO 2");
    Serial.printf("  RST:  GPIO %d\n", TFT_RST);
    Serial.println();

    // CRITICAL CHECK
    if (TFT_RST == -1) {
        Serial.println("⚠️  WARNING: TFT_RST is DISABLED (-1)");
        Serial.println("    This may cause white screen issues!");
    } else {
        Serial.printf("✓ TFT_RST enabled on GPIO %d\n", TFT_RST);
    }
    Serial.println();

    // Initialize display
    printTestHeader("Display Initialization");
    tft.init();
    Serial.println("✓ tft.init() completed");

    tft.setRotation(3);  // Landscape
    Serial.println("✓ Rotation set to 3 (landscape)");

    Serial.println("Clearing screen (SAFE MODE - strips with delays)...");
    safeFillScreen2(COLOR_BLACK);
    Serial.println("✓ Screen cleared to BLACK");

    waitForSerial();

    // ========================================
    // TEST 1: Single fillRect
    // ========================================
    printTestHeader("Single fillRect (small)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    tft.fillRect(10, 10, 50, 50, COLOR_RED);
    Serial.println("✓ Drew 50x50 red rectangle");
    waitForSerial();

    // ========================================
    // TEST 2: Multiple fillRect (no delays)
    // ========================================
    printTestHeader("5 fillRect operations (no delays)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    for (int i = 0; i < 5; i++) {
        tft.fillRect(10 + i*60, 10, 50, 50, COLOR_RED);
        Serial.printf("  Rectangle %d drawn\n", i+1);
    }
    Serial.println("✓ Drew 5 rectangles");
    waitForSerial();

    // ========================================
    // TEST 3: Multiple fillRect (with delays)
    // ========================================
    printTestHeader("10 fillRect operations (50ms delays)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    for (int i = 0; i < 10; i++) {
        int x = (i % 5) * 90;
        int y = (i / 5) * 90;
        tft.fillRect(10 + x, 10 + y, 80, 80, COLOR_GREEN);
        delay(50);
        Serial.printf("  Rectangle %d drawn\n", i+1);
    }
    Serial.println("✓ Drew 10 rectangles with delays");
    waitForSerial();

    // ========================================
    // TEST 4: Strip-based full screen fill
    // ========================================
    printTestHeader("Full screen fill (10px strips, 30ms delays)");
    for (int y = 0; y < 320; y += 10) {
        tft.fillRect(0, y, 480, 10, COLOR_BLUE);
        delay(30);
        if (y % 50 == 0) {
            Serial.printf("  Filled to y=%d\n", y);
        }
    }
    Serial.println("✓ Screen filled with strips");
    waitForSerial();

    // ========================================
    // TEST 5: Simple text rendering
    // ========================================
    printTestHeader("Simple text rendering (print API)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    delay(100);

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("Hello World");
    Serial.println("✓ Text drawn");
    waitForSerial();

    // ========================================
    // TEST 6: Multiple text lines
    // ========================================
    printTestHeader("Multiple text lines (10 lines)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    delay(100);

    for (int i = 0; i < 10; i++) {
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 10 + i*25);
        tft.printf("Line %d: Test", i);
        delay(20);
        Serial.printf("  Line %d drawn\n", i);
    }
    Serial.println("✓ 10 text lines drawn");
    waitForSerial();

    // ========================================
    // TEST 7: Box with border + text
    // ========================================
    printTestHeader("Box with border + text (1 box)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    delay(100);

    // Draw box
    tft.drawRect(10, 10, 200, 100, COLOR_GRAY);
    delay(10);
    Serial.println("  Border drawn");

    // Label
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 20);
    tft.print("RPM");
    delay(10);
    Serial.println("  Label drawn");

    // Value
    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setCursor(50, 50);
    tft.print("1500");
    delay(10);
    Serial.println("  Value drawn");

    Serial.println("✓ Box with text drawn");
    waitForSerial();

    // ========================================
    // TEST 8: Multiple boxes (simulating dashboard)
    // ========================================
    printTestHeader("6 boxes with text (dashboard simulation)");
    safeFillScreen2(COLOR_BLACK);  // Use safe method
    delay(100);

    const char* labels[] = {"RPM", "Speed", "Coolant", "Throttle", "Battery", "Intake"};
    const char* values[] = {"1500", "65", "85.0", "45%", "12.6V", "25.0"};

    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        Serial.printf("  Drawing box %d (%s)...\n", i+1, labels[i]);

        // Border
        tft.drawRect(x, y, 230, 80, COLOR_GRAY);
        delay(10);

        // Label
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print(labels[i]);
        delay(10);

        // Value
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 40);
        tft.print(values[i]);
        delay(10);

        delay(50);  // Delay between boxes
        Serial.printf("  ✓ Box %d complete\n", i+1);
    }
    Serial.println("✓ All 6 boxes drawn");
    waitForSerial();

    // ========================================
    // TEST 9: Complex screen (top bar + boxes + bottom bar)
    // ========================================
    printTestHeader("Full complex screen (simulating OBDeck)");

    // Clear screen
    Serial.println("  Clearing screen...");
    for (int y = 0; y < 320; y += 10) {
        tft.fillRect(0, y, 480, 10, COLOR_BLACK);
        delay(30);
    }
    Serial.println("  ✓ Screen cleared");

    // Top bar
    Serial.println("  Drawing top bar...");
    tft.fillRect(0, 0, 480, 35, COLOR_DARKGRAY);
    delay(10);
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
    tft.setTextSize(2);
    tft.setCursor(5, 10);
    tft.print("OBDeck");
    delay(10);
    tft.setTextColor(COLOR_CYAN, COLOR_DARKGRAY);
    tft.setCursor(200, 10);
    tft.print("Dashboard");
    delay(10);
    Serial.println("  ✓ Top bar complete");

    // Bottom bar
    Serial.println("  Drawing bottom bar...");
    tft.fillRect(0, 280, 160, 40, COLOR_BLUE);
    delay(10);
    tft.fillRect(160, 280, 160, 40, COLOR_DARKGRAY);
    delay(10);
    tft.fillRect(320, 280, 160, 40, COLOR_DARKGRAY);
    delay(10);
    tft.setTextColor(COLOR_WHITE, COLOR_BLUE);
    tft.setCursor(30, 292);
    tft.print("Dashboard");
    delay(10);
    Serial.println("  ✓ Bottom bar complete");

    // Dashboard boxes
    Serial.println("  Drawing dashboard boxes...");
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 5 + col * 240;
        int y = 40 + row * 75;

        Serial.printf("    Box %d...\n", i+1);

        tft.drawRect(x, y, 230, 70, COLOR_GRAY);
        delay(10);

        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 8);
        tft.print(labels[i]);
        delay(10);

        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 35);
        tft.print(values[i]);
        delay(10);

        delay(50);
        Serial.printf("    ✓ Box %d complete\n", i+1);
    }

    Serial.println("✓ FULL SCREEN RENDERED SUCCESSFULLY!");
    Serial.println();
    waitForSerial();

    // ========================================
    // TEST 10: drawString with setTextPadding (obdeck startup uses this)
    // ========================================
    printTestHeader("drawString with setTextPadding");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // This is what obdeck startup screen does
    tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(200);
    tft.drawString("OBDeck", 80, 100);
    delay(50);
    Serial.println("  ✓ First drawString with padding");

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(2);
    tft.setTextPadding(250);
    tft.drawString("Connecting...", 100, 140);
    delay(50);
    Serial.println("  ✓ Second drawString with padding");

    // Reset
    tft.setTextPadding(0);

    Serial.println("✓ drawString with padding works");
    waitForSerial();

    // ========================================
    // TEST 11: Multiple small fillRect clears (dashboard value clearing)
    // ========================================
    printTestHeader("Multiple small fillRect clears");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // Draw 6 boxes first
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        tft.drawRect(x, y, 230, 80, COLOR_GRAY);
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print(labels[i]);
    }
    delay(100);
    Serial.println("  ✓ Boxes with labels drawn");

    // Now clear value areas and redraw (simulating dashboard update)
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        // Clear value area
        tft.fillRect(x + 5, y + 40, 220, 30, COLOR_BLACK);
        delay(10);

        // Redraw value
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 45);
        tft.print(values[i]);
        delay(10);

        Serial.printf("  Box %d value updated\n", i+1);
    }

    Serial.println("✓ Multiple value clears/redraws successful");
    waitForSerial();

    // ========================================
    // TEST 12: Rapid updates (minimal delays)
    // ========================================
    printTestHeader("Rapid box updates (5ms delays)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // Draw boxes with minimal delays
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        tft.drawRect(x, y, 230, 80, COLOR_GRAY);
        delay(5);

        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print(labels[i]);
        delay(5);

        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 40);
        tft.print(values[i]);
        delay(5);

        Serial.printf("  Box %d (fast)\n", i+1);
    }

    Serial.println("✓ Rapid updates successful");
    waitForSerial();

    // ========================================
    // TEST 13: Simulated live dashboard (continuous updates)
    // ========================================
    printTestHeader("Simulated live updates (10 cycles)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // Draw top/bottom bars
    tft.fillRect(0, 0, 480, 35, COLOR_DARKGRAY);
    delay(10);
    tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
    tft.setTextSize(2);
    tft.setCursor(5, 10);
    tft.print("OBDeck - Live Test");
    delay(10);

    tft.fillRect(0, 285, 480, 35, COLOR_DARKGRAY);
    delay(10);

    // Draw dashboard boxes (static parts)
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 40 + row * 75;

        tft.drawRect(x, y, 230, 70, COLOR_GRAY);
        delay(5);

        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 8);
        tft.print(labels[i]);
        delay(5);
    }
    Serial.println("  ✓ Static elements drawn");

    // Simulate 10 updates
    for (int cycle = 0; cycle < 10; cycle++) {
        Serial.printf("  Update cycle %d/10...\n", cycle + 1);

        // Update all 6 values
        for (int i = 0; i < 6; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = 10 + col * 240;
            int y = 40 + row * 75;

            // Clear value area
            tft.fillRect(x + 5, y + 35, 220, 25, COLOR_BLACK);
            delay(10);

            // Draw new value (simulated change)
            tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
            tft.setTextSize(3);
            tft.setCursor(x + 30, y + 35);

            // Modify values slightly each cycle
            char modified_value[16];
            if (i == 0) {  // RPM
                snprintf(modified_value, sizeof(modified_value), "%d", 1500 + cycle * 100);
            } else if (i == 1) {  // Speed
                snprintf(modified_value, sizeof(modified_value), "%d", 65 + cycle);
            } else {
                strcpy(modified_value, values[i]);
            }

            tft.print(modified_value);
            delay(10);
        }

        delay(500);  // Wait between update cycles
    }

    Serial.println("✓ 10 update cycles completed successfully!");
    waitForSerial();

    // ========================================
    // FINAL SUMMARY
    // ========================================
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ALL TESTS PASSED!                    ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    Serial.println("If all tests passed, the display hardware");
    Serial.println("is working correctly with slow rendering.");
    Serial.println();
    Serial.println("If OBDeck still fails, the issue is:");
    Serial.println("1. Different rendering approach in obdeck");
    Serial.println("2. FreeRTOS threading interference");
    Serial.println("3. OBD2 task causing timing issues");
    Serial.println("========================================");
    waitForSerial();

    // ========================================
    // AGGRESSIVE TESTS - Finding Breaking Points
    // ========================================
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  AGGRESSIVE TESTS                     ║");
    Serial.println("║  Testing suspected problem operations ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    waitForSerial();

    // ========================================
    // TEST 14: Fast fillScreen() (EXPECTED TO FAIL)
    // ========================================
    printTestHeader("FAST fillScreen() - NO STRIPS");
    Serial.println("⚠️  This test uses fast fillScreen()");
    Serial.println("    Expected to cause WHITE SCREEN");
    delay(2000);

    tft.fillScreen(COLOR_RED);
    delay(100);
    Serial.println("✓ Fast RED fill completed");

    tft.fillScreen(COLOR_GREEN);
    delay(100);
    Serial.println("✓ Fast GREEN fill completed");

    tft.fillScreen(COLOR_BLUE);
    delay(100);
    Serial.println("✓ Fast BLUE fill completed");

    tft.fillScreen(COLOR_BLACK);
    delay(100);
    Serial.println("✓ Fast BLACK fill completed");

    Serial.println("🎉 FAST fillScreen() WORKS! (unexpected!)");
    waitForSerial();

    // ========================================
    // TEST 15: Large fillRect operations (no delays)
    // ========================================
    printTestHeader("Large fillRect operations (NO delays)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // Fill screen in large chunks with no delays
    tft.fillRect(0, 0, 240, 160, COLOR_RED);
    Serial.println("  Quarter 1 (RED)");

    tft.fillRect(240, 0, 240, 160, COLOR_GREEN);
    Serial.println("  Quarter 2 (GREEN)");

    tft.fillRect(0, 160, 240, 160, COLOR_BLUE);
    Serial.println("  Quarter 3 (BLUE)");

    tft.fillRect(240, 160, 240, 160, COLOR_YELLOW);
    Serial.println("  Quarter 4 (YELLOW)");

    Serial.println("✓ Large fillRect with no delays works");
    waitForSerial();

    // ========================================
    // TEST 16: Rapid fillRect spam (100 operations)
    // ========================================
    printTestHeader("Rapid fillRect spam (100 rects, NO delays)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    for (int i = 0; i < 100; i++) {
        int x = (i % 10) * 48;
        int y = (i / 10) * 32;
        uint16_t color = (i % 2 == 0) ? COLOR_RED : COLOR_BLUE;
        tft.fillRect(x, y, 48, 32, color);
        // NO DELAY
    }

    Serial.println("✓ 100 rapid fillRect operations completed");
    waitForSerial();

    // ========================================
    // TEST 17: drawString with MASSIVE padding
    // ========================================
    printTestHeader("drawString with MASSIVE padding");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
    tft.setTextSize(3);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(450);  // HUGE padding (almost full width)
    tft.drawString("HUGE PADDING TEST", 10, 50);
    delay(50);
    Serial.println("  ✓ Padding = 450 pixels");

    tft.setTextPadding(480);  // FULL WIDTH padding
    tft.drawString("MAX PADDING", 10, 100);
    delay(50);
    Serial.println("  ✓ Padding = 480 pixels (full width)");

    tft.setTextPadding(0);
    Serial.println("✓ Massive text padding works");
    waitForSerial();

    // ========================================
    // TEST 18: Many small fillRect clears (NO delays)
    // ========================================
    printTestHeader("50 small fillRect clears (NO delays)");
    safeFillScreen2(COLOR_WHITE);
    delay(100);

    // Clear 50 small rectangles rapidly
    for (int i = 0; i < 50; i++) {
        int x = (i % 10) * 48;
        int y = (i / 10) * 32;
        tft.fillRect(x, y, 40, 25, COLOR_BLACK);
        // NO DELAY
    }

    Serial.println("✓ 50 rapid small clears completed");
    waitForSerial();

    // ========================================
    // TEST 19: Rapid text rendering (NO delays)
    // ========================================
    printTestHeader("Rapid text rendering (50 lines, NO delays)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    for (int i = 0; i < 50; i++) {
        int x = (i % 2) * 240;
        int y = (i / 2) * 13;
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(1);
        tft.setCursor(x, y);
        tft.printf("Line %d: Rapid text test", i);
        // NO DELAY
    }

    Serial.println("✓ 50 rapid text lines completed");
    waitForSerial();

    // ========================================
    // TEST 20: Mixed rapid operations (NO delays)
    // ========================================
    printTestHeader("Mixed rapid operations (NO delays)");
    safeFillScreen2(COLOR_BLACK);
    delay(100);

    for (int i = 0; i < 20; i++) {
        int x = (i % 4) * 120;
        int y = (i / 4) * 64;

        // Fill rect
        tft.fillRect(x, y, 115, 60, COLOR_DARKGRAY);

        // Draw border
        tft.drawRect(x, y, 115, 60, COLOR_CYAN);

        // Draw text
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft.setTextSize(2);
        tft.setCursor(x + 5, y + 5);
        tft.printf("Box %d", i);

        // Draw value
        tft.setTextSize(3);
        tft.setCursor(x + 20, y + 30);
        tft.print(i * 100);

        // NO DELAY between operations
    }

    Serial.println("✓ 20 boxes with mixed operations (no delays)");
    waitForSerial();

    // ========================================
    // TEST 21: Continuous rapid clears (stress test)
    // ========================================
    printTestHeader("Continuous rapid clears (20 full clears)");
    Serial.println("⚠️  This is the ultimate stress test");
    Serial.println("    Clearing screen 20 times with minimal delay");
    delay(2000);

    for (int cycle = 0; cycle < 20; cycle++) {
        // Clear to alternating colors
        uint16_t color = (cycle % 4 == 0) ? COLOR_RED :
                        (cycle % 4 == 1) ? COLOR_GREEN :
                        (cycle % 4 == 2) ? COLOR_BLUE : COLOR_BLACK;

        // Use strip method but faster
        for (int y = 0; y < 320; y += 20) {  // Larger strips
            tft.fillRect(0, y, 480, 20, color);
            delay(5);  // Minimal delay (was 30ms)
        }

        Serial.printf("  Clear cycle %d/20 completed\n", cycle + 1);
        delay(100);
    }

    Serial.println("✓ 20 rapid full screen clears completed!");
    waitForSerial();

    // ========================================
    // TEST 22: Absolute fastest rendering possible
    // ========================================
    printTestHeader("ABSOLUTE FASTEST - Zero delays");
    Serial.println("⚠️  WARNING: Testing ABSOLUTE LIMITS");
    Serial.println("    NO DELAYS between ANY operations");
    delay(2000);

    safeFillScreen2(COLOR_BLACK);
    delay(100);

    // Draw 6 boxes as fast as possible
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        tft.drawRect(x, y, 230, 80, COLOR_GRAY);
        tft.setTextColor(COLOR_CYAN, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print(labels[i]);
        tft.fillRect(x + 5, y + 40, 220, 30, COLOR_BLACK);
        tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 45);
        tft.print(values[i]);
        // ABSOLUTELY NO DELAYS
    }

    Serial.println("✓ ZERO-DELAY rendering completed!");
    Serial.println();
    Serial.println("🎉 If you can read this, your display");
    Serial.println("   can handle MUCH faster operation!");
    waitForSerial();

    // ========================================
    // FINAL REPORT
    // ========================================
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  AGGRESSIVE TEST RESULTS              ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Record which tests passed/failed:");
    Serial.println();
    Serial.println("TEST 14: Fast fillScreen() (expected fail)");
    Serial.println("TEST 15: Large fillRect (no delays)");
    Serial.println("TEST 16: 100 rapid fillRect");
    Serial.println("TEST 17: Massive text padding");
    Serial.println("TEST 18: 50 rapid small clears");
    Serial.println("TEST 19: 50 rapid text lines");
    Serial.println("TEST 20: 20 boxes mixed ops (no delays)");
    Serial.println("TEST 21: 20 full screen clears (fast)");
    Serial.println("TEST 22: Absolute fastest (zero delays)");
    Serial.println();
    Serial.println("========================================");
}

void loop() {
    // Do nothing
    delay(1000);
}
