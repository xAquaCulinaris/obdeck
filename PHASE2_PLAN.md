# Phase 2: Polish & Features

## Priority Order

### 1. Fix Display Flickering ⚡ (HIGH PRIORITY)
**Problem:** Full screen redraw every second causes visible flickering

**Solution:** Smart partial updates
- Only redraw values that changed
- Keep static elements (labels, layout)
- Use double buffering if needed
- Track previous values to detect changes

**Implementation:**
```cpp
struct DisplayState {
    int last_rpm;
    int last_speed;
    float last_coolant;
    // ... etc
    bool needs_full_redraw;
};

// Only update if value changed
if (rpm != last_rpm) {
    // Clear old value area
    tft.fillRect(x, y, width, height, BLACK);
    // Draw new value
    tft.drawNumber(rpm, x, y);
    last_rpm = rpm;
}
```

**Estimated Time:** 30 minutes

---

### 2. Improve Dashboard Visuals 🎨
**Current:** Basic text layout
**Goal:** Modern, readable, automotive-style dashboard

**Ideas to Implement:**
- Larger, bold numbers for key metrics (RPM, Speed)
- Color coding (green=normal, yellow=warning, red=critical)
- Icons instead of text labels
- Progress bars for throttle position
- Gauge-style visualizations for temperature
- Better use of 480x320 screen space

**Waiting for:** User inspiration images/examples

**Questions:**
1. What style do you prefer? (minimalist, gauges, retro, modern)
2. What metrics are most important to see at a glance?
3. Any specific color scheme?

---

### 3. Add Touch Support 👆
**Hardware:** XPT2046 touch controller (CS=21)

**Implementation Plan:**
1. Add XPT2046_Touchscreen library (already in plan)
2. Calibrate touch coordinates
3. Create touch zones/buttons
4. Multi-page navigation:
   - Page 1: Dashboard (RPM, Speed, Temp)
   - Page 2: Advanced (Throttle, Intake, etc.)
   - Page 3: DTCs (Error codes)
   - Page 4: Settings

**Touch UI Elements:**
- Left/Right arrows for page navigation
- Tap metric to see history/details
- Hold button to refresh/clear DTCs

**Estimated Time:** 1-2 hours

---

### 4. DTC Error Code Reading 📋
**Already Have:** Basic DTC code from MicroPython version

**Implementation:**
- Read stored DTCs (Mode 03)
- Read pending DTCs (Mode 07)
- Clear DTCs (Mode 04) with confirmation
- Decode DTC format (P0133, etc.)
- Show DTC descriptions if available

**DTC Display:**
```
┌────────────────────────────┐
│ Diagnostic Trouble Codes   │
├────────────────────────────┤
│ P0133  O2 Sensor Slow      │
│ P0244  Wastegate Solenoid  │
│                            │
│ [Clear Codes] [Refresh]    │
└────────────────────────────┘
```

**Estimated Time:** 1 hour

---

## Implementation Order

### Session 1: Fix Flickering (Now)
- Implement partial screen updates
- Track previous values
- Test with emulator

### Session 2: Visuals (After inspiration)
- Design new layout based on user preference
- Implement gauges/bars
- Color coding

### Session 3: Touch
- Add touch library
- Calibrate
- Page navigation
- Touch zones

### Session 4: DTCs
- Read/parse DTCs
- Display list
- Clear functionality

---

## Technical Notes

### Display Performance
Current: ~2 FPS (500ms refresh)
- Each `tft.printf()` redraws everything
- `fillScreen(BLACK)` clears entire display

Target: Smooth updates
- Only redraw changed regions
- Use sprites for complex elements
- Pre-render static content

### Memory Considerations
- ESP32 has 520KB RAM
- TFT_eSPI sprites use RAM for buffering
- Large sprites (full screen) = ~300KB
- Small sprites (value boxes) = ~10KB each
- Balance performance vs memory

### Touch Calibration
```cpp
// Touch coordinates need mapping to screen coordinates
// Will need to determine these values:
#define TS_MINX 300
#define TS_MINY 300
#define TS_MAXX 3900
#define TS_MAXY 3900
```

---

## Questions for User

1. **Display flickering:** Should I start with this immediately?
2. **Visual style:** Do you have example images of dashboards you like?
3. **Touch priority:** How important is touch vs other features?
4. **DTC codes:** Do you need DTC descriptions or just codes?

Let me know where you want to start!
