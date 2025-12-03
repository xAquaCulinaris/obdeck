"""
Confirmation Dialog Component
Modal dialog for user confirmation

Features:
- Yes/No buttons
- Custom message
- Blocking behavior
"""

from display.colors import BLACK, WHITE, RED, GREEN, GRAY, YELLOW
from display.button import Button


class ConfirmDialog:
    """
    Modal confirmation dialog with Yes/No buttons
    """

    def __init__(self, display, writer):
        """
        Initialize confirmation dialog

        Args:
            display: Display driver instance
            writer: Text writer instance
        """
        self.display = display
        self.writer = writer
        self.visible = False

        # Dialog dimensions
        self.x = 60
        self.y = 80
        self.width = 360
        self.height = 160

        # Create Yes/No buttons
        button_width = 140
        button_height = 50
        button_y = self.y + 95

        self.yes_btn = Button(
            x=self.x + 20, y=button_y,
            width=button_width, height=button_height,
            text="Yes",
            display=display, writer=writer,
            bg_color=RED, text_color=WHITE, text_size=2
        )

        self.no_btn = Button(
            x=self.x + 180, y=button_y,
            width=button_width, height=button_height,
            text="No",
            display=display, writer=writer,
            bg_color=GRAY, text_color=WHITE, text_size=2
        )

        self.yes_btn.set_visible(False)
        self.no_btn.set_visible(False)


    def show(self, title, message):
        """
        Show confirmation dialog

        Args:
            title: Dialog title
            message: Confirmation message (single line)
        """
        if self.visible:
            return

        self.visible = True

        # Draw semi-transparent background (dark gray overlay)
        self.display.fill_rect(0, 0, 480, 320, BLACK)

        # Draw dialog box
        # Outer border (highlight)
        self.display.rect(self.x - 2, self.y - 2, self.width + 4, self.height + 4, YELLOW)

        # Background
        self.display.fill_rect(self.x, self.y, self.width, self.height, BLACK)

        # Inner border
        self.display.rect(self.x, self.y, self.width, self.height, WHITE)

        # Title
        title_x = self.x + (self.width - self.writer.text_width(title, 2)) // 2
        self.writer.text(title, title_x, self.y + 15, RED, size=2)

        # Message line 1
        msg_x = self.x + (self.width - self.writer.text_width(message, 1)) // 2
        self.writer.text(message, msg_x, self.y + 55, WHITE, size=1)

        # Show buttons
        self.yes_btn.set_visible(True)
        self.no_btn.set_visible(True)


    def hide(self):
        """Hide confirmation dialog"""
        if not self.visible:
            return

        self.visible = False
        self.yes_btn.set_visible(False)
        self.no_btn.set_visible(False)


    def handle_touch(self, touch_x, touch_y):
        """
        Handle touch on dialog

        Args:
            touch_x: Touch X coordinate
            touch_y: Touch Y coordinate

        Returns:
            str: 'yes', 'no', or None if outside buttons
        """
        if not self.visible:
            return None

        if self.yes_btn.is_touched(touch_x, touch_y):
            # Visual feedback
            self.yes_btn.press()
            return 'yes'

        if self.no_btn.is_touched(touch_x, touch_y):
            # Visual feedback
            self.no_btn.press()
            return 'no'

        return None
