# show_buttons.py
#
# parameters:
#   image_file: path to image file to display
#   width: display width
#   height: display height
#   ms_to_display: how many ms to display image
#   rotation_angle: angle to rotate displayed image by
#
# Example:
#   py show_buttons.py loadingen.png 2160 1440 5000 0
#
# Dependencies:
#   PySDL2:
#     https://pypi.org/project/PySDL2
#       pip install -U pysdl2
#       pip install pysdl2-dll
#     https://pysdl2.readthedocs.io/en/rel_0_9_7/
import os
import sys
import time
import sdl2
import sdl2.ext
import sdl2.sdlimage
import sdl2.sdlgfx


class App:
    image_file = ""
    display_width = 0
    display_height = 0
    ms_to_display = 0
    rotation_angle = 0

    def __init__(self, args):
        if len(args) != 6:
            raise IndexError(
                "Usage: image_file width height ms_to_display rotation_angle")

        self.image_file = args[1]
        self.display_width = int(args[2])
        self.display_height = int(args[3])
        self.ms_to_display = int(args[4])
        self.rotation_angle = int(args[5])

    def run(self):
        sdl2.ext.init()

        # Not expoed by PySDL2
        sdl2.SDL_ShowCursor(sdl2.SDL_DISABLE)

        window = sdl2.ext.Window("show buttons", size=(
            self.display_width, self.display_height), flags=sdl2.SDL_WINDOW_SHOWN)

        # Not exposed by PySDL2
        if sdl2.SDL_SetWindowFullscreen(window.window, sdl2.SDL_WINDOW_FULLSCREEN) != 0:
            raise RuntimeError(sdl2.SDL_GetError())

        image = sdl2.ext.load_image(self.image_file)

        if self.rotation_angle in [0, 180]:
            ratio = min(self.display_width / image.w,
                        self.display_height / image.h)
        else:
            ratio = min(self.display_width / image.h,
                        self.display_height / image.w)

        print("show_buttons: display", self.display_width, "x", self.display_height,
              ";", "image", image.w, "x", image.h, ";", "ratio", ratio)

        renderer = sdl2.ext.Renderer(window)
        sfactory = sdl2.ext.SpriteFactory(sdl2.ext.TEXTURE, renderer=renderer)
        srenderer = sfactory.create_sprite_render_system(window)

        # Not exposed by PySDL2
        sf = sdl2.sdlgfx.rotozoomSurface(
            image, self.rotation_angle, ratio, sdl2.sdlgfx.SMOOTHING_ON)
        if not sf:
            raise RuntimeError(sdl2.SDL_GetError())
        # Wrap rotozoom surface in sprite, and take ownership
        tsprite = sfactory.from_surface(sf.contents, free=True)

        renderer.clear()
        srenderer.render(tsprite)

        start_ms = int(round(time.time() * 1000))

        running = True
        event = sdl2.SDL_Event()
        while running:
            events = sdl2.ext.get_events()
            for event in events:
                if event.type == sdl2.SDL_QUIT:
                    running = False
                    break
                elif event.type == sdl2.SDL_KEYDOWN:
                    running = False
                    break

            elapsed_ms = int(round(time.time() * 1000)) - start_ms
            if elapsed_ms > self.ms_to_display:
                break

            sdl2.SDL_Delay(10)


if __name__ == "__main__":
    sys.exit(App(sys.argv).run())
