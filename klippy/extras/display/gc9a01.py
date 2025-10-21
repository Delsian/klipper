# Support for GC9A01 (240x240 circular TFT) displays
#
# Copyright (C) 2025  Eug Krashtan <eug.krashtan@gmail.com>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
import logging
from .. import bus

BACKGROUND_PRIORITY_CLOCK = 0x7fffffff00000000

# GC9A01 timing - based on datasheet
GC9A01_CMD_DELAY = .000020  # 20us between commands

# GC9A01 commands
GC9A01_SLPOUT = 0x11  # Sleep Out
GC9A01_INVON = 0x21   # Display Inversion On
GC9A01_DISPON = 0x29  # Display On
GC9A01_CASET = 0x2A   # Column Address Set
GC9A01_RASET = 0x2B   # Row Address Set
GC9A01_RAMWR = 0x2C   # Memory Write
GC9A01_MADCTL = 0x36  # Memory Data Access Control
GC9A01_COLMOD = 0x3A  # Interface Pixel Format

# Display dimensions
DISPLAY_WIDTH = 240
DISPLAY_HEIGHT = 240

# RGB565 color definitions
COLOR_BLACK = 0x0000
COLOR_WHITE = 0xFFFF
COLOR_RED = 0xF800
COLOR_GREEN = 0x07E0
COLOR_BLUE = 0x001F
COLOR_YELLOW = 0xFFE0
COLOR_CYAN = 0x07FF
COLOR_MAGENTA = 0xF81F
COLOR_GRAY = 0x8410
COLOR_DARK_GRAY = 0x4208

class GC9A01:
    def __init__(self, config):
        printer = config.get_printer()

        # SPI setup
        self.spi = bus.MCU_SPI_from_config(
            config, 3, default_speed=4000000)
        self.mcu = self.spi.get_mcu()

        # Pin setup
        ppins = printer.lookup_object('pins')
        dc_pin_params = ppins.lookup_pin(config.get('dc_pin'))
        reset_pin_params = ppins.lookup_pin(config.get('rst_pin'))

        if dc_pin_params['chip'] != self.mcu:
            raise ppins.error("gc9a01 dc_pin must be on same mcu as spi")
        if reset_pin_params['chip'] != self.mcu:
            raise ppins.error("gc9a01 rst_pin must be on same mcu as spi")

        self.dc_pin = dc_pin_params['pin']
        self.reset_pin = reset_pin_params['pin']

        # Display configuration
        self.width = DISPLAY_WIDTH
        self.height = DISPLAY_HEIGHT
        self.center_x = self.width // 2
        self.center_y = self.height // 2
        self.radius = min(self.width, self.height) // 2

        # Framebuffer for 240x240 display in RGB565 format (2 bytes per pixel)
        fb_size = self.width * self.height * 2
        self.framebuffer = bytearray(fb_size)
        self.old_framebuffer = bytearray(fb_size)

        # Display colors
        self.fg_color = config.getint('fg_color', COLOR_WHITE,
                                      minval=0, maxval=0xFFFF)
        self.bg_color = config.getint('bg_color', COLOR_BLACK,
                                      minval=0, maxval=0xFFFF)

        # Rotation (0, 90, 180, 270)
        self.rotation = config.getint('rotation', 0, minval=0, maxval=270)
        if self.rotation not in [0, 90, 180, 270]:
            raise config.error("gc9a01 rotation must be 0, 90, 180, or 270")

        # Setup microcontroller communication
        self.oid = self.mcu.create_oid()
        self.mcu.register_config_callback(self.build_config)
        self.send_cmds_cmd = None
        self.send_data_cmd = None
        self.set_window_cmd = None

        # Register with display system
        printer.register_event_handler("klippy:ready", self._handle_ready)

    def _handle_ready(self):
        """Called when Klipper is ready"""
        self.clear()
        self.flush()

    def build_config(self):
        # Configure the GC9A01 device on the microcontroller
        cmd_delay_ticks = self.mcu.seconds_to_clock(GC9A01_CMD_DELAY)
        self.mcu.add_config_cmd(
            "config_gc9a01 oid=%d spi_oid=%d dc_pin=%s reset_pin=%s"
            " delay_ticks=%d" % (
                self.oid, self.spi.get_oid(),
                self.dc_pin, self.reset_pin, cmd_delay_ticks))

        # Setup command queue and lookup commands
        cmd_queue = self.spi.get_command_queue()
        self.send_cmds_cmd = self.mcu.lookup_command(
            "gc9a01_send_cmds oid=%c cmds=%*s", cq=cmd_queue)
        self.send_data_cmd = self.mcu.lookup_command(
            "gc9a01_send_data oid=%c data=%*s", cq=cmd_queue)
        self.set_window_cmd = self.mcu.lookup_command(
            "gc9a01_set_window oid=%c x0=%hu y0=%hu x1=%hu y1=%hu",
            cq=cmd_queue)

    def send_cmds(self, cmds):
        """Send command bytes to the display"""
        self.send_cmds_cmd.send([self.oid, cmds],
                               reqclock=BACKGROUND_PRIORITY_CLOCK)

    def send_data(self, data):
        """Send data bytes to the display"""
        self.send_data_cmd.send([self.oid, data],
                               reqclock=BACKGROUND_PRIORITY_CLOCK)

    def set_window(self, x0, y0, x1, y1):
        """Set the drawing window on the display"""
        self.set_window_cmd.send([self.oid, x0, y0, x1, y1],
                                reqclock=BACKGROUND_PRIORITY_CLOCK)

    def init(self):
        """Initialize the display (called by display manager)"""
        # The C driver handles initialization in config_gc9a01
        # This method is here for compatibility with Klipper's display system
        self.clear()

    def is_point_in_circle(self, x, y):
        """Check if a point is within the circular display area"""
        dx = x - self.center_x
        dy = y - self.center_y
        return (dx * dx + dy * dy) <= (self.radius * self.radius)

    def flush(self):
        """Send updated framebuffer regions to the display"""
        # Simple full-screen update for initial implementation
        # For better performance, could implement dirty region tracking

        if self.framebuffer == self.old_framebuffer:
            return

        # Find bounding box of changes
        min_x, min_y = self.width, self.height
        max_x, max_y = 0, 0

        changed = False
        for y in range(self.height):
            for x in range(self.width):
                pos = (y * self.width + x) * 2
                if (self.framebuffer[pos] != self.old_framebuffer[pos] or
                    self.framebuffer[pos + 1] !=
                    self.old_framebuffer[pos + 1]):
                    changed = True
                    min_x = min(min_x, x)
                    min_y = min(min_y, y)
                    max_x = max(max_x, x)
                    max_y = max(max_y, y)

        if not changed:
            return

        # Send the changed region
        self.set_window(min_x, min_y, max_x, max_y)

        # Extract and send pixel data row by row
        for y in range(min_y, max_y + 1):
            row_start = (y * self.width + min_x) * 2
            row_width = (max_x - min_x + 1) * 2
            row_data = self.framebuffer[row_start:row_start + row_width]

            # Send in chunks to respect MCU buffer limits
            chunk_size = 512
            for i in range(0, len(row_data), chunk_size):
                chunk = row_data[i:i+chunk_size]
                self.send_data(chunk)

        # Update old framebuffer
        self.old_framebuffer[:] = self.framebuffer[:]

    def clear(self):
        """Clear the display (fill with background color)"""
        color_bytes = bytes([(self.bg_color >> 8) & 0xFF,
                             self.bg_color & 0xFF])
        for i in range(0, len(self.framebuffer), 2):
            self.framebuffer[i:i+2] = color_bytes

    def rgb_to_rgb565(self, r, g, b):
        """Convert RGB888 to RGB565 format"""
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

    def fill_rect(self, x, y, w, h, color):
        """Fill a rectangle with a color (RGB565 format)"""
        if x >= self.width or y >= self.height:
            return

        # Clip to display bounds
        w = min(w, self.width - x)
        h = min(h, self.height - y)

        # Convert color to bytes
        color_bytes = bytes([(color >> 8) & 0xFF, color & 0xFF])

        for py in range(y, y + h):
            row_start = py * self.width * 2 + x * 2
            for px in range(w):
                pos = row_start + px * 2
                self.framebuffer[pos:pos+2] = color_bytes

    def draw_pixel(self, x, y, color):
        """Draw a single pixel (RGB565 format)"""
        if x >= self.width or y >= self.height or x < 0 or y < 0:
            return

        pos = y * self.width * 2 + x * 2
        self.framebuffer[pos] = (color >> 8) & 0xFF
        self.framebuffer[pos + 1] = color & 0xFF

    def get_dimensions(self):
        """Return display dimensions"""
        return (self.width, self.height)

    def write_text(self, x, y, data):
        """Compatibility method for text-based display interface"""
        pass

    def write_graphics(self, x, y, data):
        """Write graphics data to the framebuffer"""
        # data is expected to be pixel data in RGB565 format
        if x >= self.width or y >= self.height:
            return

        for i, pixel_data in enumerate(data):
            if isinstance(pixel_data, int):
                # Single byte - expand to RGB565
                # Assuming it's a grayscale value
                r = (pixel_data >> 3) & 0x1F
                g = (pixel_data >> 2) & 0x3F
                b = (pixel_data >> 3) & 0x1F
                color = (r << 11) | (g << 5) | b
            else:
                color = pixel_data

            self.draw_pixel(x + i, y, color)

    def write_glyph(self, x, y, glyph_name):
        """Compatibility method for glyph-based display interface"""
        return 0

    def set_glyphs(self, glyphs):
        """Compatibility method for glyph-based display interface"""
        pass


def load_config(config):
    """Klipper config loader"""
    return GC9A01(config)