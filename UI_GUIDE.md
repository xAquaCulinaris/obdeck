# Multi-Page UI Guide

## Overview

The system now features **3 pages** with touch navigation:

1. **Dashboard** - Live OBD2 data with DTC warning indicator
2. **Diagnostics** - View and clear trouble codes
3. **Settings** - System information and configuration

## Navigation

### Bottom Navigation Bar

Always visible at bottom of screen (3 buttons):

```
┌────────────────────────────────────┐
│                                    │
│                                    │
├────────────────────────────────────┤
│[Dashboard][Diagnostics][Settings]  │
└────────────────────────────────────┘
```

- **Current page button**: Highlighted in CYAN
- **Other buttons**: Gray background
- **Tap any button** to switch to that page immediately

## Page 1: Dashboard

### Purpose
Main page showing live vehicle data in real-time.

### Layout
```
┌────────────────────────────────────┐
│ Dashboard                      ⚠️3 │ ← Warning shows DTC count
├────────────────────────────────────┤
│ Coolant:          85.0 C           │
│                                    │
│ RPM:              1500 RPM         │
│                                    │
│ Speed:            45 km/h          │
│                                    │
│ Battery:          14.2 V           │
├────────────────────────────────────┤
│[Dashboard][Diagnostics][Settings]  │
└────────────────────────────────────┘
```

### Features
- **Live Updates**: Data refreshes every 500ms
- **Warning Indicator**: Red "⚠️ X" in top-right if DTCs present
  - Shows number of active trouble codes
  - Only visible when DTCs detected
- **4 Metrics Displayed**:
  - Coolant temperature (°C)
  - Engine RPM
  - Vehicle speed (km/h)
  - Battery voltage (V)

### When to Use
- Daily driving monitoring
- Quick glance at vitals
- Check for warning indicators

---

## Page 2: Diagnostics

### Purpose
Read, display, and clear diagnostic trouble codes (DTCs).

### Layout (No Codes)
```
┌────────────────────────────────────┐
│ Diagnostics                        │
├────────────────────────────────────┤
│                                    │
│        No Trouble Codes            │
│            System OK               │
│                                    │
│  ┌──────────┐                     │
│  │ Refresh  │                     │
│  └──────────┘                     │
├────────────────────────────────────┤
│[Dashboard][Diagnostics][Settings]  │
└────────────────────────────────────┘
```

### Layout (With Codes)
```
┌────────────────────────────────────┐
│ Diagnostics                        │
├────────────────────────────────────┤
│ Found 3 Code(s):                   │
│                                    │
│   P0420 - Catalyst Efficiency      │
│   P0133 - O2 Sensor Slow Response  │
│   P0171 - System Too Lean          │
│                                    │
│ ┌──────────┐  ┌──────────────┐   │
│ │ Refresh  │  │ Clear DTCs   │   │
│ └──────────┘  └──────────────┘   │
├────────────────────────────────────┤
│[Dashboard][Diagnostics][Settings]  │
└────────────────────────────────────┘
```

### Features
- **Automatic Read**: DTCs read 5 seconds after connection
- **Manual Refresh**: Tap "Refresh" button to re-read codes
- **Clear Codes**: Tap "Clear DTCs" to erase all trouble codes
  - Only visible when codes present
  - Automatically refreshes after clearing
- **Display Limit**: Shows up to 4 codes
  - If more than 4, shows "...+X more"

### DTC Format
- **Format**: PXXXX (e.g., P0420)
- **Prefixes**:
  - P = Powertrain (engine/transmission)
  - C = Chassis (braking/steering)
  - B = Body (airbags/climate)
  - U = Network (communication)

### When to Use
- Check engine light is on
- After repairs (verify codes cleared)
- Periodic system health check
- Before vehicle inspection

---

## Page 3: Settings

### Purpose
Display system information and connection status.

### Layout
```
┌────────────────────────────────────┐
│ Settings                           │
├────────────────────────────────────┤
│ Connection                         │
│  MAC: 00:1D:A5:68:98:8B           │
│  PIN: 1234                         │
│  Status: Connected                 │
│                                    │
│ System                             │
│  Memory: 35% used                  │
│  Free: 337KB                       │
│  Vehicle: Opel Corsa D             │
│                                    │
│                                    │
├────────────────────────────────────┤
│[Dashboard][Diagnostics][Settings]  │
└────────────────────────────────────┘
```

### Information Displayed

**Connection Section:**
- **MAC Address**: ELM327 Bluetooth MAC (from config.py)
- **PIN**: Pairing PIN used (from config.py)
- **Status**: Connected (green) / Disconnected (red)

**System Section:**
- **Memory**: RAM usage percentage
- **Free**: Available RAM in kilobytes
- **Vehicle**: Vehicle name (from config.py)

### When to Use
- Verify connection details
- Check system resource usage
- Confirm correct ELM327 MAC address
- Troubleshooting connection issues

---

## Touch Interaction

### Navigation Bar (Always Available)
- **Location**: Bottom 45 pixels of screen
- **Buttons**: 3 equal-width buttons
- **Action**: Tap to switch pages instantly
- **Visual Feedback**: Current page button highlighted cyan

### Page-Specific Buttons

**Dashboard Page:**
- No interactive buttons (display-only)

**Diagnostics Page:**
- **Refresh Button**: Re-read DTCs from vehicle
  - Always visible
  - Gray background
- **Clear DTCs Button**: Erase all trouble codes
  - Only visible when codes present
  - Red background (destructive action)

**Settings Page:**
- No interactive buttons (display-only)

### Touch Debouncing
- **Delay**: 500ms between touches
- **Purpose**: Prevents accidental double-taps
- **Feedback**: Button press animation (color invert for 200ms)

---

## DTC Reading Behavior

### Automatic Reading
1. ESP32 connects to ELM327
2. Wait 5 seconds (allow ECU to stabilize)
3. Automatically read DTCs once
4. Store codes in memory
5. Display warning on Dashboard if codes found

### Manual Refresh (Diagnostics Page)
1. Tap "Refresh" button
2. Request sent to OBD2 thread
3. Reads DTCs via Mode 03 command
4. Updates display with results
5. Dashboard warning updated

### Clearing Codes (Diagnostics Page)
1. Tap "Clear DTCs" button
2. Sends Mode 04 command to ECU
3. Wait 1 second
4. Automatically re-reads to confirm
5. Display updates to show cleared status

---

## Performance

### Update Rates
- **Dashboard data**: 2 Hz (every 500ms)
- **Touch polling**: 10 Hz (every 100ms)
- **OBD2 queries**: 2 Hz (every 500ms)
- **Navigation**: Instant page switch

### Memory Usage
- **Base system**: ~180KB
- **Page manager**: ~15KB additional
- **DTC storage**: Minimal (list of strings)

---

## Customization

### Change Vehicle Name
Edit `src/config.py`:
```python
VEHICLE_NAME = "My Custom Name"
```

### Adjust Dashboard Metrics
Edit `src/display/pages.py` → `DashboardPage.draw()`:
- Change labels
- Reposition elements
- Add/remove metrics

### Modify Page Colors
Colors defined in `src/display/colors.py`:
- Headers: CYAN
- Warnings: RED
- Success: GREEN
- Errors: YELLOW
- Nav buttons: GRAY / CYAN

---

## Troubleshooting

### "No DTCs read" but check engine light is on
- **Solution**: Tap "Refresh" on Diagnostics page
- **Cause**: Initial read may have timed out

### Touch not responding
- **Check**: Is debounce delay active? (wait 500ms)
- **Solution**: Run touch calibration (see SETUP_GUIDE.md)

### Page doesn't update
- **Solution**: Switch to different page and back
- **Cause**: Display may need full refresh

### Navigation buttons don't highlight
- **Cause**: Current page may not be tracked correctly
- **Solution**: Tap another button to resync

---

## Future Enhancements

Possible additions:
- **Page 4**: Data logging / trip computer
- **Dashboard**: Additional PIDs (throttle, intake temp)
- **Diagnostics**: DTC descriptions/definitions
- **Settings**: Configurable display brightness
- **Settings**: Unit selection (metric/imperial)
- **All Pages**: Swipe gesture navigation

