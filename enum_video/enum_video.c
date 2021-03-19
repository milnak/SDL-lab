#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#define HANDLE_SDL_ERROR(ret, msg)                     \
    do                                                 \
    {                                                  \
        if ((ret) != 0)                                \
        {                                              \
            printf("%s: %s\n", (msg), SDL_GetError()); \
            goto done;                                 \
        }                                              \
    } while (0)

void dump_sdl_info()
{
    const int drivers = SDL_GetNumVideoDrivers();
    for (int driverIndex = 0; driverIndex < drivers; ++driverIndex)
    {
        printf("Video driver %d:\n", driverIndex);

        const char *driver_name = SDL_GetVideoDriver(driverIndex);
        if (driver_name != NULL)
        {
            printf("  Name: %s\n", driver_name);

            if (SDL_VideoInit(driver_name) == 0)
            {
                SDL_VideoQuit();
            }
            else
            {
                printf("  SDL_VideoInit: %s\n", SDL_GetError());
            }
        }
    }

    const int videodisplays = SDL_GetNumVideoDisplays();
    printf("Num video displays: %d\n", videodisplays);
    for (int displayIndex = 0; displayIndex < videodisplays; ++displayIndex)
    {
        printf("Video display %d:\n", displayIndex);

        int ret = SDL_GetNumDisplayModes(displayIndex);
        printf("  Num display modes: %d\n", ret);
        for (int modeIndex = 0; modeIndex < ret; ++modeIndex)
        {
            SDL_DisplayMode mode = {};
            if (SDL_GetDisplayMode(displayIndex, modeIndex, &mode) == 0)
            {
                printf("  Mode(%d): format=%d; %d x %d; rate=%d\n", modeIndex, mode.format, mode.w, mode.h, mode.refresh_rate);
            }
        }

        // int SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode * mode);
        // int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode * mode);

        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0)
        {
            printf("  DPI: d=%f h=%f v=%f\n", ddpi, hdpi, vdpi);
        }

        SDL_Rect rect;
        if (SDL_GetDisplayBounds(displayIndex, &rect) == 0)
        {
            printf("  Bounds: %d, %d (%d x %d)\n", rect.x, rect.y, rect.w, rect.h);
        }

        if (SDL_GetDisplayUsableBounds(displayIndex, &rect) == 0)
        {
            printf("  Usable Bounds: %d, %d (%d x %d)\n", rect.x, rect.y, rect.w, rect.h);
        }
    }
}

int main(int argc, char *argv[])
{
    _Bool success = false;
    int ret = 0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    // NOTE: Calling SDL_VideoInit() after SDL_Init(INIT_VIDEO)
    // will corrupt the stack!
    ret = SDL_Init(0);
    HANDLE_SDL_ERROR(ret, "SDL_Init(0)");

    dump_sdl_info();

    ret = SDL_Init(SDL_INIT_VIDEO);
    HANDLE_SDL_ERROR(ret, "SDL_Init(SDL_INIT_VIDEO)");

    SDL_RendererInfo renderer_info;

    const int renderDrivers = SDL_GetNumRenderDrivers();
    printf("Render drivers: %d\n", renderDrivers);
    for (int index = 0; index < renderDrivers; ++index)
    {
        if (SDL_GetRenderDriverInfo(index, &renderer_info) == 0)
        {
            printf("Render driver %d: %s\n", index, renderer_info.name);
        }
        else
        {
            printf("SDL_GetRenderDriverInfo %d: %s\n", index, SDL_GetError());
        }
    }

    window = SDL_CreateWindow("SDL2", 0, 0, 320, 240, 
                              SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
    HANDLE_SDL_ERROR(window == NULL, "SDL_CreateWindow");

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    HANDLE_SDL_ERROR(renderer == NULL, "SDL_CreateRenderer");

    ret = SDL_GetRendererInfo(renderer, &renderer_info);
    HANDLE_SDL_ERROR(ret, "SDL_GetRendererInfo");
    printf("active renderer: %s\n", renderer_info.name);

    Uint8 color = 0;

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

        ret = SDL_SetRenderDrawColor(renderer, color, (color + 128) % 255, 255 - color, 
            SDL_ALPHA_OPAQUE);
        HANDLE_SDL_ERROR(ret, "SDL_GetRendererInfo");

        ret = SDL_RenderClear(renderer);
        HANDLE_SDL_ERROR(ret, "SDL_RenderClear");

        SDL_RenderPresent(renderer);

        SDL_Delay(1000 / 60);

        ++color;
    }

done:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    if (SDL_WasInit(0))
    {
        SDL_Quit();
    }

    // Return 0 from main on success.
    return success ? 0 : 1;
}
