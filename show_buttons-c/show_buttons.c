#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL2_rotozoom.h>

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
void HSL_to_RGB(float h, float s, float l, int *red, int *green, int *blue)
{
    float r, g, b;

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

void del()
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

int init(int argc, char *argv[])
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
        del();
    }

    return success;
}

int draw_rect(SDL_Renderer *renderer, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b)
{
    int ret = 0;

    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    ret = SDL_SetRenderDrawColor(renderer, r, g, b, 255 /*a*/);
    HANDLE_SDL_ERROR(ret, "SDL_SetRenderDrawColor");

    ret = SDL_RenderDrawRect(renderer, &rect);
    HANDLE_SDL_ERROR(ret, "SDL_RenderDrawRect");

done:
    return ret;
}

int main(int argc, char *argv[])
{
    _Bool success = false;
    int ret = 0;
    float ratio = 0.0f;
    _Bool running = false;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Surface *image_surface = NULL;
    SDL_Texture *image_texture = NULL;

    success = init(argc, argv);
    if (!success)
    {
        goto done;
    }

    ret = SDL_Init(SDL_INIT_VIDEO);
    HANDLE_SDL_ERROR(ret, "SDL_Init");

    SDL_ShowCursor(SDL_DISABLE);

    window = SDL_CreateWindow("show_buttons", 0, 0, display_width, display_height, SDL_WINDOW_SHOWN);
    HANDLE_SDL_ERROR(window == NULL, "SDL_CreateWindow");

    ret = SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    HANDLE_SDL_ERROR(ret, "SDL_SetWindowFullscreen");

    image_surface = IMG_Load(image_file);
    if (image_surface == NULL)
    {
        printf("IMG_Load: %s\n", IMG_GetError());
        goto done;
    }

    if (rotation_angle == 0 || rotation_angle == 180)
    {
        ratio = min((float)display_width / image_surface->w, (float)display_height / image_surface->h);
    }
    else
    {
        // Assume 90 or 270 degree rotation.
        ratio = min((float)display_width / image_surface->h, (float)display_height / image_surface->w);
    }

    printf("display: %d x %d; image: %d x %d; ratio: %.2f; angle: %d\n", display_width, display_height, image_surface->w, image_surface->h, ratio, rotation_angle);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    HANDLE_SDL_ERROR(renderer == NULL, "SDL_CreateRenderer");

    image_texture = SDL_CreateTextureFromSurface(renderer, image_surface);
    HANDLE_SDL_ERROR(image_texture == NULL, "SDL_CreateTextureFromSurface");

    ret = SDL_RenderClear(renderer);
    HANDLE_SDL_ERROR(ret, "SDL_RenderClear");

    // Draw bounding box.
    if (draw_rect(renderer, 0, 0, display_width, display_height, 0x10, 0x10, 0x10) != 0)
    {
        goto done;
    }

    // Specify dest rect - SDL_RenderCopyEx will resize to fit.
    SDL_Rect dest;
    dest.w = image_surface->w * ratio;
    dest.h = image_surface->h * ratio;

    // Center image
    dest.x = (display_width - dest.w) / 2;
    dest.y = (display_height - dest.h) / 2;

    ret = SDL_RenderCopyEx(renderer, image_texture, NULL /*srcrect*/, &dest /*destrect*/, rotation_angle /*angle*/, NULL /*center*/, SDL_FLIP_NONE);
    HANDLE_SDL_ERROR(ret, "SDL_RenderCopyEx(");

    SDL_RenderPresent(renderer);

    const long int start_time = time_now();

    running = true;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
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
            HSL_to_RGB(ratio, 1.0f, 0.5f, &r, &g, &b);

            draw_rect(renderer,
                      0, display_height - 2,
                      (int)(display_width * ratio), 2,
                      r, g, b);
            
            SDL_RenderPresent(renderer);
        }


        SDL_Delay(1000 / 60);
    }

    success = true;

done:
    if (renderer != NULL)
    {
        SDL_DestroyRenderer(renderer);
    }

    if (image_surface != NULL)
    {
        SDL_FreeSurface(image_surface);
    }

    if (image_texture != NULL)
    {
        SDL_DestroyTexture(image_texture);
    }

    if (window != NULL)
    {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();

    del();

    // Return 0 from main on success.
    return success ? 0 : 1;
}
