# PySDL2 clock demo
#
# Install latest stable version from PyPI
#   pip install -U pysdl2
# Install latest development verion from GitHub
#  pip install -U git+https://github.com/marcusva/py-sdl2.git

import enum
import sys
import sdl2
import sdl2.ext
import time


class HandType(enum.Enum):
    Undefined = 0
    Hour = 1
    Minute = 2
    Second = 3


class Texture:
    hand_type = HandType.Undefined
    texture = None
    angle = 0.0
    rect = (0, 0, 0, 0)
    center = sdl2.SDL_Point()

    def __init__(self, sprite_factory, filename, window_rect, hand_type):
        self.hand_type = hand_type
        self.texture = sprite_factory.from_image(filename)

        self.angle = 0.0

        # Set initial position
        w = self.texture.size[0]
        h = self.texture.size[1]
        half_w = int(window_rect[0] * 0.5)
        half_h = int(window_rect[1] * 0.5)
        x = half_w
        y = half_h - h
        self.rect = (x, y, w, h)

        # Center is based on rotation point in bitmap
        self.center = sdl2.SDL_Point(int(w * 0.5), h-int(w * 0.5))

    def __repr__(self):
        return "Texture: type=%s, angle=%s, rect=%s" % (self.hand_type, self.angle, self.rect)

    def render(self, sdl_renderer):
        sdl_renderer.copy(src=self.texture, srcrect=None,
                          dstrect=self.rect, angle=self.angle, center=self.center)

    def update_hand_position(self):
        now = time.localtime()

        if self.hand_type == HandType.Hour:
            self.set_angle_for_time(now.tm_hour, 12)
        elif self.hand_type == HandType.Minute:
            self.set_angle_for_time(now.tm_min, 59)
        elif self.hand_type == HandType.Second:
            self.set_angle_for_time(now.tm_sec, 59)

    def set_angle_for_time(self, value, max_value):
        rotation = value / max_value
        self.angle = 360 * rotation


class App:
    sdl_window = None
    sdl_renderer = None
    hands = []
    window_size = (800, 600)

    def update_hand_positions(self):
        for hand in self.hands:
            hand.update_hand_position()

    def render(self):
        self.sdl_renderer.clear()

        for hand in self.hands:
            hand.render(self.sdl_renderer)

        self.sdl_renderer.present()

    def run(self):
        sdl2.ext.init()
        self.sdl_window = sdl2.ext.Window(
            "pyclock", position=None, size=self.window_size, flags=sdl2.SDL_WINDOW_SHOWN)
        self.sdl_renderer = sdl2.ext.Renderer(self.sdl_window)
        self.sdl_renderer.logical_size = self.window_size
        self.sdl_renderer.color = 0xFF00000000

        RESOURCES = sdl2.ext.Resources(__file__, "resources")

        factory = sdl2.ext.SpriteFactory(
            sdl2.ext.TEXTURE, renderer=self.sdl_renderer)

        self.hands.append(Texture(factory, RESOURCES.get_path(
            "hour_hand.png"), self.window_size, HandType.Hour))
        self.hands.append(Texture(factory, RESOURCES.get_path(
            "min_hand.png"), self.window_size, HandType.Minute))
        self.hands.append(Texture(factory, RESOURCES.get_path(
            "sec_hand.png"), self.window_size, HandType.Second))

        running = True

        while running:
            events = sdl2.ext.get_events()
            for event in events:
                if event.type == sdl2.SDL_QUIT:
                    running = False
                    break
                elif event.type == sdl2.SDL_KEYDOWN:
                    if event.key.keysym.sym == sdl2.SDLK_ESCAPE:
                        running = False
                        break

            self.update_hand_positions()
            self.render()

            sdl2.SDL_Delay(99)

        sdl2.ext.quit()


if __name__ == "__main__":
    sys.exit(App().run())
