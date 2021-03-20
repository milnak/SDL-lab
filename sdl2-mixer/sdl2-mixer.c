#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mixer.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* the lower it is, the more FPS shown and CPU needed */
#define BUFFER 1024
/* NEVER make W less than BUFFER! */
#define W 640
#define H 480
#define H2 (H / 2)
#define H4 (H / 4)
#define Y(sample) (((sample)*H) / 4 / 0x7fff)

// used in postmix, refresh
int stream_buffer_index = 0;
Sint16 stream_buffers[2][BUFFER * 2 * 2];
int need_refresh = 0;

// used in postmix
int sample_size = 0;
int position = 0;

// used in refresh
Uint32 frame_count = 0;

// used in handle_keydown
int audio_rate = 0;
int volume = SDL_MIX_MAXVOLUME;

/******************************************************************************/
/* some simple exit and error routines                                        */

void errorv(char *str, va_list ap)
{
    vfprintf(stderr, str, ap);
    fprintf(stderr, ": %s.\n", SDL_GetError());
}

void cleanExit(char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    errorv(str, ap);
    va_end(ap);
    Mix_CloseAudio();
    SDL_Quit();
    exit(1);
}

static void postmix(void *udata, Uint8 *stream, int len)
{
    int w = *((int *)udata);

    position += len / sample_size;

    if (need_refresh)
    {
        return;
    }

    stream_buffer_index = (stream_buffer_index + 1) % 2;

    memcpy(stream_buffers[stream_buffer_index], stream,
           (len > w * 4) ? w * 4 : len);

    // indicate refresh() call required
    need_refresh = 1;
}

void refresh(SDL_Renderer *renderer)
{
    Sint16 *buf = stream_buffers[stream_buffer_index];
    need_refresh = 0;

    for (int x = 0; x < W * 2; x++)
    {
        const int X = x >> 1, b = x & 1, t = H4 + H2 * b;
        int y1, h1;
        if (buf[x] < 0)
        {
            h1 = -Y(buf[x]);
            y1 = t - h1;
        }
        else
        {
            y1 = t;
            h1 = Y(buf[x]);
        }

        SDL_Rect r;

        // Erase top half
        r.x = X;
        r.y = H2 * b;
        r.w = 1;
        r.h = y1 - r.y;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 /*a*/);
        SDL_RenderFillRect(renderer, &r);

        r.x = X;
        r.y = y1;
        r.w = 1;
        r.h = h1;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255 /*a*/);
        SDL_RenderFillRect(renderer, &r);

        // Erase bottom half
        r.x = X;
        r.y = y1 + h1;
        r.w = 1;
        r.h = H2 + H2 * b - r.y;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 /*a*/);
        SDL_RenderFillRect(renderer, &r);
    }

    SDL_RenderPresent(renderer);

    frame_count++;
}

void print_init_flags(int flags)
{
#define PFLAG(a)              \
    if (flags & MIX_INIT_##a) \
    printf(#a " ")

    PFLAG(FLAC);
    PFLAG(MOD);
    PFLAG(MP3);
    PFLAG(OGG);
    if (!flags)
    {
        printf("None");
    }
    printf("\n");
}

int handle_keydown(SDL_Keysym keysym)
{
    int done = 0;

    switch (keysym.sym)
    {
    case SDLK_ESCAPE: // ESC: Exit
        done = 1;
        break;
    case SDLK_LEFT: // Left, Shift-Left - Back
        if (keysym.mod & KMOD_SHIFT)
        {
            Mix_RewindMusic();
            position = 0;
        }
        else
        {
            int pos = position / audio_rate - 1;
            if (pos < 0)
                pos = 0;
            Mix_SetMusicPosition(pos);
            position = pos * audio_rate;
        }
        break;
    case SDLK_RIGHT: // Right: Forward
        switch (Mix_GetMusicType(NULL))
        {
        case MUS_MP3:
            Mix_SetMusicPosition(+5);
            position += 5 * audio_rate;
            break;
        case MUS_OGG:
        case MUS_FLAC:
            Mix_SetMusicPosition(position / audio_rate + 1);
            position += audio_rate;
            break;
        default:
            printf("cannot fast-forward this type of music\n");
            break;
        }
        break;
    case SDLK_UP: // Up: Volume
        volume = (volume + 1) << 1;
        if (volume > SDL_MIX_MAXVOLUME)
        {
            volume = SDL_MIX_MAXVOLUME;
        }
        Mix_VolumeMusic(volume);
        break;
    case SDLK_DOWN: // Down: Volume
        volume >>= 1;
        Mix_VolumeMusic(volume);
        break;
    case SDLK_SPACE: // Space: pause
        if (Mix_PausedMusic())
        {
            Mix_ResumeMusic();
        }
        else
        {
            Mix_PauseMusic();
        }
        break;
    }

    return done;
}

int main(int argc, char **argv)
{
    /* initialize SDL for audio and video */
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
        cleanExit("SDL_Init");
    }

    atexit(SDL_Quit);

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s filename [full_screen]\n"
                        "    filename is any music file supported by your SDL_mixer library\n",
                *argv);
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("sdlwave - SDL_mixer demo",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H,
                                          ((argc > 2) ? SDL_WINDOW_FULLSCREEN : 0));
    if (window == NULL)
    {
        cleanExit("SDL_CreateWindow");
    }

    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL)
    {
        cleanExit("SDL_CreateRenderer");
    }

    SDL_ShowCursor(SDL_DISABLE);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, BUFFER) < 0)
    {
        cleanExit("Mix_OpenAudio");
    }

    Mix_AllocateChannels(0);

    int audio_channels;
    Uint16 audio_format;
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
    int bits = audio_format & 0xFF;
    sample_size = bits / 8 + audio_channels;
    printf("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
           bits, audio_channels > 1 ? "stereo" : "mono", BUFFER);

    /* load the song */
    Mix_Music *music = Mix_LoadMUS(argv[1]);
    if (music == NULL)
    {
        cleanExit("Mix_LoadMUS(\"%s\")", argv[1]);
    }

    Mix_SetPostMix(postmix, &window_w);

    Uint32 elapsed_ms = SDL_GetTicks();
    if (Mix_PlayMusic(music, 1) == -1)
    {
        cleanExit("Mix_PlayMusic(0x%p,1)", music);
    }

    Mix_VolumeMusic(volume);

    int done = 0;
    while ((Mix_PlayingMusic() || Mix_PausedMusic()) && !done)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_KEYDOWN:
                done = handle_keydown(e.key.keysym);
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }

        if (need_refresh)
        {
            refresh(renderer);
        }

        SDL_Delay(0);
    }

    elapsed_ms = SDL_GetTicks() - elapsed_ms;

    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();

    printf("fps=%.2f\n", ((float)frame_count) / (elapsed_ms / 1000.0));

    return 0;
}