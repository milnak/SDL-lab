#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // geteuid
#include <sys/time.h>

// https://www.libsdl.org/release/SDL-1.2.15/docs/html
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
// https://www.libsdl.org/projects/SDL_image/docs/SDL_image_frame.html
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>

char *image_file = NULL;
int display_width = 0;
int display_height = 0;
int ms_to_display = 0;
int rotation_angle = 0;

#define min(a, b) ((a) < (b) ? a : b)

#define HANDLE_SDL_ERROR(ret, msg)                     \
    do                                                 \
    {                                                  \
        if ((ret) != 0)                                \
        {                                              \
            printf("%s: %s\n", (msg), SDL_GetError()); \
            goto done;                                 \
        }                                              \
    } while (0)

// Returns time in microseconds.
long int time_now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 1000000 + tv.tv_usec);
}

int timediff_ms(long int start)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (time_now() - start) / 1000;
}

float hue_to_RGB(float p, float q, float t)
{
    if (t < 0)
    {
        t += 1.0f;
    }
    if (t > 1)
    {
        t -= 1.0f;
    }
    if ((t * 6) < 1) // t < 1/6
    {
        return p + ((q - p) * 6.0f * t);
    }
    else if ((t * 2.0f) < 1) // t < 1/2
    {
        return q;
    }
    else if ((t * 3.0f) < 2) // t < 2/3
    {
        return p + (q - p) * ((2.0f / 3.0f) - t) * 6.0f;
    }
    return p;
}

// https://stackoverflow.com/questions/36721830/convert-hsl-to-rgb-and-hex
// h: degrees (0..360); s: percentage; l: percentage
void HSL_to_RGB(int hue, int saturation, int lightness, int *red, int *green, int *blue)
{
    float r, g, b;

    float h = (float)hue / 360;
    float s = (float)saturation / 100;
    float l = (float)lightness / 100;

    if (s == 0)
    {
        // achromatic
        r = g = b = l * 255.0f;
    }
    else
    {
        const float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - (l * s));
        const float p = (2.0f * l) - q;

        r = hue_to_RGB(p, q, h + 1.0 / 3.0);
        g = hue_to_RGB(p, q, h);
        b = hue_to_RGB(p, q, h - 1.0 / 3.0);
    }

    *red = r * 255;
    *green = g * 255;
    *blue = b * 255;
}

void usage()
{
    puts("Usage: image_file width height ms_to_display rotation");
}

void free_args()
{
    if (image_file != NULL)
    {
        free(image_file);
        image_file = NULL;
    }

    display_width = 0;
    display_height = 0;
    ms_to_display = 0;
    rotation_angle = 0;
}

int parse_args(int argc, char *argv[])
{
    _Bool success = false;

    if (argc != 6)
    {
        goto done;
    }

    image_file = strdup(argv[1]);
    if (image_file == NULL)
    {
        goto done;
    }

    display_width = atoi(argv[2]);
    if (display_width <= 0)
    {
        goto done;
    }

    display_height = atoi(argv[3]);
    if (display_height <= 0)
    {
        goto done;
    }

    ms_to_display = atoi(argv[4]);
    if (ms_to_display <= 0)
    {
        goto done;
    }

    // 0 is a valid rotation angle.
    rotation_angle = atoi(argv[5]);
    if (rotation_angle < 0 || rotation_angle > 359)
    {
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        usage();
        free_args();
    }

    return success;
}

int draw_rect(SDL_Surface *dst,
              Sint16 x, Sint16 y, Sint16 w, Sint16 h,
              Uint8 r, Uint8 g, Uint8 b)
{
    return rectangleRGBA(dst, x, y, w - x - 1, h - y - 1, r, g, b, 255 /*a*/);
}

int main(int argc, char *argv[])
{
    _Bool success = false;
    SDL_Surface *screen_surface = NULL;
    SDL_Surface *image_surface = NULL;
    SDL_Surface *rotozoom_surface = NULL;
    float ratio = 0.0;

    if (geteuid() != 0)
    {
        puts("Must be run as root.");
        goto done;
    }

    success = parse_args(argc, argv);
    if (!success)
    {
        goto done;
    }

    int ret = 0;

    ret = SDL_Init(SDL_INIT_VIDEO);
    HANDLE_SDL_ERROR(ret, "SDL_Init");

    const IMG_InitFlags init_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    ret = IMG_Init(init_flags);
    HANDLE_SDL_ERROR((ret & init_flags) == 0, "IMG_Init");

    image_surface = IMG_Load(image_file);
    HANDLE_SDL_ERROR(image_surface == NULL, "IMG_Load");

    SDL_ShowCursor(SDL_DISABLE);

    screen_surface = SDL_SetVideoMode(display_width, display_height, 24 /*bpp*/, SDL_FULLSCREEN);
    HANDLE_SDL_ERROR(screen_surface == NULL, "SDL_SetVideoMode");

    // Draw bounding box.
    const float safearea_percent = 0.90f;
    const int safe_width = display_width * safearea_percent;
    const int safe_height = display_height * safearea_percent;

    if (rotation_angle == 0 || rotation_angle == 180)
    {
        ratio = min((float)safe_width / image_surface->w, (float)safe_height / image_surface->h);
    }
    else
    {
        // Assume 90 or 270 degree rotation.
        ratio = min((float)safe_width / image_surface->h, (float)safe_height / image_surface->w);
    }

    printf("display: %d x %d; image: %d x %d; ratio: %.2f; angle: %d\n", display_width, display_height, image_surface->w, image_surface->h, ratio, rotation_angle);
    printf("safe: %d x %d\n", safe_width, safe_height);
    
    draw_rect(screen_surface, 0, 0, display_width, display_height, 0x40, 0x40, 0x40);

    rotozoom_surface = rotozoomSurfaceXY(image_surface, rotation_angle, ratio, ratio, SMOOTHING_ON);
    HANDLE_SDL_ERROR(rotozoom_surface == NULL, "rotozoom_surfaceSurfaceXY");

    // Specify dest rect - SDL_RenderCopyEx will resize to fit.
    SDL_Rect dest;
    dest.w = image_surface->w * ratio;
    dest.h = image_surface->h * ratio;

    // Center image
    if (rotation_angle == 0 || rotation_angle == 180)
    {
        dest.x = (display_width - dest.w) / 2;
        dest.y = (display_height - dest.h) / 2;
    }
    else
    {
        // Assume 90 or 270 degree rotation.
        dest.x = (display_width - dest.h) / 2;
        dest.y = (display_height - dest.w) / 2;
    }

    SDL_BlitSurface(rotozoom_surface, NULL /*srcrect*/, screen_surface, &dest);

    const long int start_time = time_now();
    _Bool running = true;

    while (running)
    {
        SDL_Event event;

        if (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
                break;
            }

            if (event.type == SDL_KEYDOWN)
            {
                running = false;
                break;
            }
        }

        const int elapsed_time = timediff_ms(start_time);
        if (elapsed_time > ms_to_display)
        {
            break;
        }

        // Draw timer.
        {
            const float ratio = min((float)elapsed_time / ms_to_display, 1.0);

            int r, g, b;
            HSL_to_RGB((ratio * 360) / 1.5, 100, 50, &r, &g, &b);

            const int x1 = (display_width - safe_width) / 2;
            hlineRGBA(screen_surface, x1, x1 + (safe_width * ratio),
                      display_height - ((display_height - safe_height) / 2) - 1, r, g, b, 255 /*a*/);
        }

        // UpdateRect
        SDL_Flip(screen_surface);

        SDL_Delay(1000 / 60);
    }

done:
    if (rotozoom_surface != NULL)
    {
        SDL_FreeSurface(rotozoom_surface);
    }

    if (image_surface != NULL)
    {
        SDL_FreeSurface(image_surface);
    }

    if (SDL_WasInit(0))
    {
        SDL_Quit();
    }

    free_args();

    // Return 0 from main on success.
    return success ? 0 : 1;
}
