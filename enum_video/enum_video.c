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

int main(int argc, char *argv[])
{
    _Bool success = false;
    int ret = 0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_RendererInfo renderer_info;

    ret = SDL_Init(0);
    HANDLE_SDL_ERROR(ret, "SDL_Init(0)");

    const int drivers = SDL_GetNumVideoDrivers();
    for (int i = 0; i < drivers; ++i)
    {
        const char *driver_name = SDL_GetVideoDriver(i);
        if (driver_name != NULL)
        {
            if (SDL_VideoInit(driver_name) == 0)
            {
                SDL_VideoQuit();
            }
            else
            {
                printf("SDL_VideoInit %d: %s\n", i, SDL_GetError());
            }
        }
        else
        {
            printf("Failed to get name of video driver %d\n", i);
        }
    }

    ret = SDL_Init(SDL_INIT_VIDEO);
    HANDLE_SDL_ERROR(ret, "SDL_Init(SDL_INIT_VIDEO)");

    window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED_MASK,
                              640, 480, SDL_WINDOW_SHOWN);
    HANDLE_SDL_ERROR(window == NULL, "SDL_CreateWindow");

    for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i)
    {
        if (SDL_GetRenderDriverInfo(i, &renderer_info) == 0)
        {
            printf("Render driver %d: %s\n", i, renderer_info.name);
        }
        else
        {
            printf("SDL_GetRenderDriverInfo %d: %s\n", i, SDL_GetError());
        }
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    HANDLE_SDL_ERROR(renderer == NULL, "SDL_CreateRenderer");

    ret = SDL_GetRendererInfo(renderer, &renderer_info);
    HANDLE_SDL_ERROR(ret, "SDL_GetRendererInfo");

    printf("active renderer: %s\n", renderer_info.name);

    Uint8 current_color = 0;

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

            if ((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE))
            {
                running = false;
                break;
            }
        }

        ret = SDL_SetRenderDrawColor(renderer, current_color, current_color, current_color, SDL_ALPHA_OPAQUE);
        HANDLE_SDL_ERROR(ret, "SDL_GetRendererInfo");

        ret = SDL_RenderClear(renderer);
        HANDLE_SDL_ERROR(ret, "SDL_RenderClear");

        SDL_RenderPresent(renderer);

        SDL_Delay(1000 / 60);

        ++current_color;
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
