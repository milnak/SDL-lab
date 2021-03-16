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
#   python show_buttons.py galaga.png 320 200 5000 0
#
# Dependencies:
#   PySDL2: https://pysdl2.readthedocs.io/en/rel_0_9_7/

import sys
import time
import sdl2.ext


def resize_keeping_aspect(display_width, display_height, img_width, img_height, center=False):
    # Will fit img within display, keeping ratio
    # Returns (x, y, cx, cy) for resized image.
    # if center than (x,y) will be offset to center the resized image.

    ratio = min(display_width / img_width, display_height / img_height)

    dest_width = int(img_width * ratio)
    dest_height = int(img_height * ratio)

    print (img_width, "x", img_height, ratio, "->", dest_width, "x", dest_height)

    if center:
        dest_x = int((display_width - dest_width)/2)
        dest_y = int((display_height - dest_height)/2)
    else:
        dest_x = 0
        dest_y = 0

    return (dest_x, dest_y, dest_width, dest_height)


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

        if self.rotation_angle not in [0, 90]:
            raise ValueError("rotation_angle must be 0 or 90")

    def run(self):
        # Does SDL_Init
        sdl2.ext.init()

        sdl2.SDL_ShowCursor(sdl2.SDL_DISABLE)

        sdl_window = sdl2.ext.Window(title="show_buttons", size=(
            self.display_width, self.display_height), flags=sdl2.SDL_WINDOW_FULLSCREEN_DESKTOP)
        sdl_window.show()

        sdl_renderer = sdl2.ext.Renderer(target=sdl_window)

        # Draw bounding rectangle, useful for debugging.
        sdl_renderer.draw_rect(
            rects=(0, 0, self.display_width, self.display_height), color=0x202020)

        ret = sdl2.sdlimage.IMG_Init(sdl2.sdlimage.IMG_INIT_PNG)
        if not ret:
            raise RuntimeError(sdl2.sdlimage.IMG_GetError())

        sprite = sdl2.ext.SpriteFactory(
            renderer=sdl_renderer).from_image(self.image_file)

        start_ms = int(round(time.time() * 1000))

        if self.rotation_angle == 0 or self.rotation_angle == 180:
            dest = resize_keeping_aspect(
                self.display_width, self.display_height, sprite.size[0], sprite.size[1], center=True)
        else:
            # Need to flip sprite width and height as final image will be rotated around (0,0)
            dest = resize_keeping_aspect(
                self.display_width, self.display_height, sprite.size[1], sprite.size[0], center=True)
            # Return dest=(x+width, y, height, width)
            dest = (dest[0] + dest[2], dest[1], dest[3], dest[2])

        # Does SDL_RenderCopyEx()
        sdl_renderer.copy(src=sprite, srcrect=None, dstrect=dest,
                          angle=self.rotation_angle, center=sdl2.rect.SDL_Point(0, 0))
        sdl_renderer.present()

        running = True

        while running:
            events = sdl2.ext.get_events()
            for event in events:
                if event.type == sdl2.SDL_QUIT:
                    running = False
                    break
                elif event.type == sdl2.SDL_KEYDOWN:
                    # event.key.keysym.sym == sdl2.SDLK_ESCAPE
                    running = False
                    break

            sdl2.SDL_Delay(100)

            elapsed_ms = int(round(time.time() * 1000)) - start_ms
            if elapsed_ms > self.ms_to_display:
                break


if __name__ == "__main__":
    app = App(sys.argv)
    app.run()
    sys.exit()
