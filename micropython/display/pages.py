"""
Multi-Page UI System
Manages different pages (Dashboard, Diagnostics, Settings)

Features:
- Page switching
- Page-specific rendering
- Navigation bar management
"""

from display.colors import BLACK, WHITE, GREEN, RED, CYAN, YELLOW, GRAY, ORANGE
from display.button import Button
from display.dialog import ConfirmDialog


class Page:
    """Base class for UI pages"""

    def __init__(self, name, display, writer):
        """
        Initialize page

        Args:
            name: Page title
            display: Display driver instance
            writer: Text writer instance
        """
        self.name = name
        self.display = display
        self.writer = writer
        self.visible = False

    def draw(self, data):
        """
        Draw page content (override in subclass)

        Args:
            data: Shared data dictionary
        """
        pass

    def draw_header(self, warning_count=0):
        """
        Draw page header with title and optional warning

        Args:
            warning_count: Number of DTCs (0 = no warning)
        """
        # Clear header area
        self.display.fill_rect(0, 0, 480, 40, BLACK)

        # Draw page title
        self.writer.text(self.name, 10, 10, CYAN, size=2)

        # Draw warning indicator if DTCs present
        if warning_count > 0:
            # Warning triangle and count
            warning_x = 400
            self.writer.text("!", warning_x, 10, RED, size=3)
            self.writer.text(str(warning_count), warning_x + 25, 15, RED, size=2)


class DashboardPage(Page):
    """Main dashboard page - shows live OBD2 data"""

    def __init__(self, display, writer):
        super().__init__("Dashboard", display, writer)

    def draw(self, data):
        """Draw dashboard with live OBD2 metrics"""
        # Draw header with warning
        self.draw_header(warning_count=data.get('dtc_count', 0))

        # Clear content area (preserve header and nav bar)
        self.display.fill_rect(0, 40, 480, 230, BLACK)

        coolant = data.get('coolant_temp', 0.0)
        rpm = data.get('rpm', 0)
        speed = data.get('speed', 0)
        battery = data.get('battery_voltage', 0.0)

        # Draw labels (green)
        self.writer.text("Coolant:", 10, 55, GREEN, size=2)
        self.writer.text("RPM:", 10, 105, GREEN, size=2)
        self.writer.text("Speed:", 10, 160, GREEN, size=2)
        self.writer.text("Battery:", 10, 215, GREEN, size=2)

        # Draw values (white, larger) with padding
        self.writer.text(f"{coolant:.1f} C   ", 200, 50, WHITE, size=4)
        self.writer.text(f"{rpm} RPM   ", 200, 100, WHITE, size=4)
        self.writer.text(f"{speed} km/h   ", 200, 155, WHITE, size=4)
        self.writer.text(f"{battery:.2f} V   ", 200, 210, WHITE, size=4)


class DiagnosticsPage(Page):
    """Diagnostics page - shows DTCs and allows clearing"""

    def __init__(self, display, writer):
        super().__init__("Diagnostics", display, writer)
        self.refresh_btn = None
        self.clear_btn = None
        self.confirm_dialog = None

    def create_buttons(self):
        """Create page-specific buttons"""
        # Refresh button
        self.refresh_btn = Button(
            x=20, y=220, width=140, height=40,
            text="Refresh",
            display=self.display, writer=self.writer,
            bg_color=GRAY, text_color=WHITE, text_size=2
        )

        # Clear codes button
        self.clear_btn = Button(
            x=180, y=220, width=140, height=40,
            text="Clear DTCs",
            display=self.display, writer=self.writer,
            bg_color=RED, text_color=WHITE, text_size=2
        )

        # Confirmation dialog
        self.confirm_dialog = ConfirmDialog(self.display, self.writer)

    def draw(self, data):
        """Draw diagnostics page with DTC list"""
        self.draw_header()

        # Clear content area
        self.display.fill_rect(0, 40, 480, 230, BLACK)

        dtc_codes = data.get('dtc_codes', [])
        dtc_count = len(dtc_codes)

        if dtc_count == 0:
            # No DTCs - show success message
            self.writer.text("No Trouble Codes", 100, 100, GREEN, size=2)
            self.writer.text("System OK", 150, 140, GREEN, size=2)

            # Hide clear button
            if self.clear_btn:
                self.clear_btn.set_visible(False)
        else:
            # Show DTC list
            self.writer.text(f"Found {dtc_count} Code(s):", 10, 60, YELLOW, size=2)

            # Display up to 4 DTCs
            y_pos = 100
            for i, dtc in enumerate(dtc_codes[:4]):
                self.writer.text(dtc, 30, y_pos, RED, size=2)
                y_pos += 30

            if dtc_count > 4:
                self.writer.text(f"...+{dtc_count-4} more", 30, y_pos, YELLOW, size=1)

            # Show clear button
            if self.clear_btn:
                self.clear_btn.set_visible(True)

        # Always show refresh button
        if self.refresh_btn:
            self.refresh_btn.set_visible(True)

    def handle_touch(self, touch_x, touch_y):
        """
        Handle touch events on this page

        Returns:
            str: Action ('refresh', 'clear', 'confirm_clear', None)
        """
        # Check if confirmation dialog is visible
        if self.confirm_dialog and self.confirm_dialog.visible:
            response = self.confirm_dialog.handle_touch(touch_x, touch_y)
            if response == 'yes':
                self.confirm_dialog.hide()
                return 'confirm_clear'  # User confirmed - clear DTCs
            elif response == 'no':
                self.confirm_dialog.hide()
                return 'cancel_clear'  # User cancelled
            return None  # Touch outside buttons

        # Normal button handling
        if self.refresh_btn and self.refresh_btn.is_touched(touch_x, touch_y):
            return 'refresh'

        if self.clear_btn and self.clear_btn.visible and self.clear_btn.is_touched(touch_x, touch_y):
            # Show confirmation dialog instead of clearing immediately
            self.confirm_dialog.show(
                "Clear Trouble Codes?",
                "This will erase all DTCs from ECU"
            )
            return 'show_confirm'

        return None


class SettingsPage(Page):
    """Settings page - shows system info and configuration"""

    def __init__(self, display, writer):
        super().__init__("Settings", display, writer)

    def draw(self, data):
        """Draw settings page with system information"""
        import config
        import gc

        self.draw_header()

        # Clear content area
        self.display.fill_rect(0, 40, 480, 230, BLACK)

        # Connection info
        self.writer.text("Connection", 10, 60, CYAN, size=2)

        mac = config.BT_MAC_ADDRESS
        # Split MAC address if too long
        if len(mac) > 17:
            self.writer.text(f"MAC: {mac[:17]}", 20, 90, WHITE, size=1)
            self.writer.text(f"     {mac[17:]}", 20, 105, WHITE, size=1)
        else:
            self.writer.text(f"MAC: {mac}", 20, 90, WHITE, size=1)

        pin_display = config.BT_PIN if config.BT_PIN else "****"
        self.writer.text(f"PIN: {pin_display}", 20, 120, WHITE, size=1)

        connected = data.get('connected', False)
        conn_status = "Connected" if connected else "Disconnected"
        conn_color = GREEN if connected else RED
        self.writer.text(f"Status: {conn_status}", 20, 135, conn_color, size=1)

        # System info
        self.writer.text("System", 10, 165, CYAN, size=2)

        # Memory usage
        mem_free = gc.mem_free()
        mem_alloc = gc.mem_alloc()
        mem_total = mem_free + mem_alloc
        mem_pct = int((mem_alloc / mem_total) * 100)

        self.writer.text(f"Memory: {mem_pct}% used", 20, 195, WHITE, size=1)
        self.writer.text(f"Free: {mem_free//1024}KB", 20, 210, WHITE, size=1)

        # Vehicle info
        self.writer.text(f"Vehicle: {config.VEHICLE_NAME}", 20, 235, WHITE, size=1)


class PageManager:
    """Manages multiple pages and navigation"""

    def __init__(self, display, writer):
        """
        Initialize page manager

        Args:
            display: Display driver instance
            writer: Text writer instance
        """
        self.display = display
        self.writer = writer
        self.pages = []
        self.current_page = 0
        self.nav_buttons = []

        # Create pages
        self.dashboard = DashboardPage(display, writer)
        self.diagnostics = DiagnosticsPage(display, writer)
        self.settings = SettingsPage(display, writer)

        self.pages = [self.dashboard, self.diagnostics, self.settings]

        # Create diagnostics page buttons
        self.diagnostics.create_buttons()

        # Create navigation bar
        self._create_nav_bar()

    def _create_nav_bar(self):
        """Create navigation bar with 3 buttons"""
        button_width = 150
        button_height = 45
        button_spacing = 10
        start_x = 10
        y_pos = 275

        # Dashboard button
        self.nav_buttons.append(Button(
            x=start_x, y=y_pos, width=button_width, height=button_height,
            text="Dashboard",
            display=self.display, writer=self.writer,
            bg_color=GRAY, text_color=WHITE, text_size=1
        ))

        # Diagnostics button
        self.nav_buttons.append(Button(
            x=start_x + button_width + button_spacing, y=y_pos,
            width=button_width, height=button_height,
            text="Diagnostics",
            display=self.display, writer=self.writer,
            bg_color=GRAY, text_color=WHITE, text_size=1
        ))

        # Settings button
        self.nav_buttons.append(Button(
            x=start_x + (button_width + button_spacing) * 2, y=y_pos,
            width=button_width, height=button_height,
            text="Settings",
            display=self.display, writer=self.writer,
            bg_color=GRAY, text_color=WHITE, text_size=1
        ))

    def draw_nav_bar(self):
        """Draw navigation bar, highlighting current page"""
        for i, btn in enumerate(self.nav_buttons):
            if i == self.current_page:
                # Highlight current page button
                btn.set_colors(CYAN, BLACK)
            else:
                # Normal appearance
                btn.set_colors(GRAY, WHITE)

            btn.draw()

    def switch_page(self, page_index):
        """
        Switch to a different page

        Args:
            page_index: Index of page to switch to (0-2)
        """
        if 0 <= page_index < len(self.pages):
            self.current_page = page_index
            # Clear screen and redraw
            self.display.fill(BLACK)

    def get_current_page(self):
        """Get currently active page"""
        return self.pages[self.current_page]

    def draw(self, data):
        """
        Draw current page and navigation bar

        Args:
            data: Shared data dictionary
        """
        # Draw current page content
        self.pages[self.current_page].draw(data)

        # Draw navigation bar
        self.draw_nav_bar()

    def handle_nav_touch(self, touch_x, touch_y):
        """
        Handle touch on navigation bar

        Args:
            touch_x: Touch X coordinate
            touch_y: Touch Y coordinate

        Returns:
            bool: True if navigation button was pressed
        """
        for i, btn in enumerate(self.nav_buttons):
            if btn.is_touched(touch_x, touch_y):
                if i != self.current_page:
                    self.switch_page(i)
                    return True
        return False

    def handle_page_touch(self, touch_x, touch_y):
        """
        Handle touch on current page content

        Args:
            touch_x: Touch X coordinate
            touch_y: Touch Y coordinate

        Returns:
            str: Action string or None
        """
        current = self.pages[self.current_page]

        # Only diagnostics page has interactive elements
        if isinstance(current, DiagnosticsPage):
            return current.handle_touch(touch_x, touch_y)

        return None
