/**
 * Minimum Delay Test
 * Find the exact minimum delay needed between operations
 */

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define COLOR_BLACK     0x0000
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_WHITE     0xFFFF
#define COLOR_GRAY      0x7BEF
#define COLOR_DARKGRAY  0x39E7

// Safe fillScreen using strips (like test/obdeck.ino does)
void safeFillScreen2(uint16_t color) {
    const int strip_height = 10;
    for (int y = 0; y < 320; y += strip_height) {
        tft.fillRect(0, y, 480, strip_height, color);
        delay(30);
    }
}

void testWithDelay(int delay_ms, const char* test_name) {
    Serial.println();
    Serial.println("========================================");
    Serial.printf("Testing: %s (delay = %dms)\n", test_name, delay_ms);
    Serial.println("========================================");
    delay(3000);  // Longer settle time (was 1000ms)

    // Test 1: fillScreen with delays
    Serial.printf("  fillScreen test (delay=%dms)...\n", delay_ms);
    tft.fillScreen(COLOR_RED);
    delay(delay_ms);
    Serial.println("    ✓ RED");  // Match TEST 14 pattern

    tft.fillScreen(COLOR_GREEN);
    delay(delay_ms);
    Serial.println("    ✓ GREEN");  // Match TEST 14 pattern

    tft.fillScreen(COLOR_BLUE);
    delay(delay_ms);
    Serial.println("    ✓ BLUE");  // Match TEST 14 pattern

    tft.fillScreen(COLOR_BLACK);
    delay(delay_ms);
    Serial.println("    ✓ BLACK");  // Match TEST 14 pattern

    Serial.println("  ✓ fillScreen test passed");

    delay(500);

    // Test 2: Large fillRect with delays
    Serial.printf("  Large fillRect test (delay=%dms)...\n", delay_ms);
    tft.fillRect(0, 0, 240, 160, COLOR_RED);
    delay(delay_ms);
    Serial.println("    ✓ Quarter 1");  // Add println after each op

    tft.fillRect(240, 0, 240, 160, COLOR_GREEN);
    delay(delay_ms);
    Serial.println("    ✓ Quarter 2");

    tft.fillRect(0, 160, 240, 160, COLOR_BLUE);
    delay(delay_ms);
    Serial.println("    ✓ Quarter 3");

    tft.fillRect(240, 160, 240, 160, COLOR_YELLOW);
    delay(delay_ms);
    Serial.println("    ✓ Quarter 4");

    Serial.println("  ✓ Large fillRect test passed");

    delay(500);

    // Test 3: Many small fillRect with delays
    Serial.printf("  Small fillRect test (delay=%dms)...\n", delay_ms);
    safeFillScreen2(COLOR_BLACK);  // Use safe method before test
    delay(100);
    for (int i = 0; i < 20; i++) {
        int x = (i % 5) * 96;
        int y = (i / 5) * 80;
        tft.fillRect(x, y, 90, 75, (i % 2) ? COLOR_RED : COLOR_BLUE);
        delay(delay_ms);
    }
    Serial.println("  ✓ Small fillRect test passed");

    delay(500);

    // Test 4: Dashboard simulation with delays
    Serial.printf("  Dashboard simulation (delay=%dms)...\n", delay_ms);
    safeFillScreen2(COLOR_BLACK);  // Use safe method before test
    delay(100);

    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 10 + row * 90;

        tft.drawRect(x, y, 230, 80, 0x7BEF);
        delay(delay_ms);

        tft.setTextColor(0x07FF, COLOR_BLACK);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print("Test");
        delay(delay_ms);

        tft.fillRect(x + 5, y + 40, 220, 30, COLOR_BLACK);
        delay(delay_ms);

        tft.setTextColor(0xFFFF, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setCursor(x + 30, y + 45);
        tft.print(i * 100);
        delay(delay_ms);
    }
    Serial.println("  ✓ Dashboard simulation passed");

    Serial.println();
    Serial.printf("✅ ALL TESTS PASSED with %dms delay!\n", delay_ms);
    Serial.println("========================================");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  Minimum Delay Finder                 ║");
    Serial.println("║  Finding exact delay requirements     ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    tft.init();
    delay(100);
    tft.setRotation(3);
    delay(100);

    Serial.println("Display initialized");
    Serial.println("Clearing screen with SAFE method (strips)...");
    safeFillScreen2(COLOR_BLACK);
    Serial.println("✓ Screen cleared");

    // Do many simple operations to match TEST 13's pattern
    Serial.println("Drawing test pattern (like TEST 13)...");

    // Draw multiple boxes with borders and text (like dashboard)
    for (int i = 0; i < 6; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = 10 + col * 240;
        int y = 40 + row * 80;

        // Draw box
        tft.drawRect(x, y, 230, 70, COLOR_GRAY);
        delay(10);

        // Draw some content
        tft.fillRect(x + 5, y + 5, 220, 60, COLOR_DARKGRAY);
        delay(10);

        // Draw text
        tft.setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft.setTextSize(2);
        tft.setCursor(x + 10, y + 10);
        tft.print("Test");
        delay(10);
    }

    Serial.println("✓ Test pattern drawn (screen now has content)");

    Serial.println();
    Serial.println("Letting display settle...");
    delay(3000);
    Serial.println();
    Serial.println("Testing different delay values...");
    Serial.println("Report which delay value FAILS");
    Serial.println();

    // Test progressively smaller delays
    // We know: 100ms works (TEST 14), 50ms fails
    // So the limit is between 50-100ms

    Serial.println("Press any key to test 100ms delay (known good)...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(250, "250ms delay - BASELINE");

    Serial.println("Press any key to test 90ms delay...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(90, "90ms delay");

    Serial.println("Press any key to test 80ms delay...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(80, "80ms delay");

    Serial.println("Press any key to test 70ms delay...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(70, "70ms delay");

    Serial.println("Press any key to test 60ms delay...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(60, "60ms delay");

    Serial.println("Press any key to test 50ms delay (expected fail)...");
    while (!Serial.available()) delay(100);
    while (Serial.available()) Serial.read();
    testWithDelay(50, "50ms delay - EXPECTED FAIL");

    // Final report
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  TESTING COMPLETE                     ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  REPORT YOUR RESULTS                  ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Report the MINIMUM delay that works:");
    Serial.println();
    Serial.println("If 100ms works but 90ms fails → need 100ms");
    Serial.println("If 90ms works but 80ms fails  → need 90ms");
    Serial.println("If 80ms works but 70ms fails  → need 80ms");
    Serial.println("If 70ms works but 60ms fails  → need 70ms");
    Serial.println("If 60ms works but 50ms fails  → need 60ms");
    Serial.println("If 50ms works                 → UNEXPECTED!");
    Serial.println();
    Serial.println("This is the EXACT minimum delay needed!");
    Serial.println("We'll use this to optimize obdeck.");
    Serial.println("========================================");
}

void loop() {
    delay(1000);
}
