"""
Simple Button UI Component
Touch-enabled button for ILI9488 display

Features:
- Rectangular button with text
- Touch detection
- Visual feedback (pressed state)
- Customizable colors and size
"""

from display.colors import WHITE, BLACK, GRAY


class Button:
    """
    Touch-enabled button UI component
    """

    def __init__(self, x, y, width, height, text, display, writer,
                 bg_color=GRAY, text_color=WHITE, text_size=2):
        """
        Initialize button

        Args:
            x: Top-left X coordinate
            y: Top-left Y coordinate
            width: Button width
            height: Button height
            text: Button label text
            display: Display driver instance
            writer: Text writer instance
            bg_color: Background color (RGB565)
            text_color: Text color (RGB565)
            text_size: Text size multiplier
        """
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.text = text
        self.display = display
        self.writer = writer
        self.bg_color = bg_color
        self.text_color = text_color
        self.text_size = text_size
        self.pressed = False
        self.visible = True


    def draw(self):
        """Draw button on display"""
        if not self.visible:
            return

        # Determine colors based on pressed state
        if self.pressed:
            bg = self.text_color
            fg = self.bg_color
        else:
            bg = self.bg_color
            fg = self.text_color

        # Draw button background
        self.display.fill_rect(self.x, self.y, self.width, self.height, bg)

        # Draw button border
        self.display.rect(self.x, self.y, self.width, self.height, fg)

        # Calculate text position (centered)
        text_width = self.writer.text_width(self.text, self.text_size)
        text_height = self.writer.text_height(self.text_size)

        text_x = self.x + (self.width - text_width) // 2
        text_y = self.y + (self.height - text_height) // 2

        # Draw text
        self.writer.text(self.text, text_x, text_y, fg, size=self.text_size)


    def is_touched(self, touch_x, touch_y):
        """
        Check if touch coordinates are within button bounds

        Args:
            touch_x: Touch X coordinate
            touch_y: Touch Y coordinate

        Returns:
            bool: True if button is touched
        """
        if not self.visible:
            return False

        return (self.x <= touch_x <= self.x + self.width and
                self.y <= touch_y <= self.y + self.height)


    def press(self):
        """Mark button as pressed (visual feedback)"""
        self.pressed = True
        self.draw()


    def release(self):
        """Mark button as released"""
        self.pressed = False
        self.draw()


    def set_text(self, text):
        """
        Update button text

        Args:
            text: New button label
        """
        self.text = text
        if self.visible:
            self.draw()


    def set_visible(self, visible):
        """
        Show or hide button

        Args:
            visible: True to show, False to hide
        """
        self.visible = visible
        if visible:
            self.draw()
        else:
            # Clear button area
            self.display.fill_rect(self.x, self.y, self.width, self.height, BLACK)


    def set_colors(self, bg_color, text_color):
        """
        Update button colors

        Args:
            bg_color: New background color
            text_color: New text color
        """
        self.bg_color = bg_color
        self.text_color = text_color
        if self.visible:
            self.draw()
