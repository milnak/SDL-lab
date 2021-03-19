
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#define DEFAULT_AUDIO_PATH "Cuica-1.wav"

struct AudioSpecUserdata_t
{
    char *issued;
    char *file;
    Uint32 event_count;
    Uint8 *buffer;
    Uint32 buffer_len;
    Uint32 loaded_len;
};

static int Quit = 0;

static int print_userdata(struct AudioSpecUserdata_t *userdata, char *prefix)
{
    size_t len = 0;

    if (!prefix)
        prefix = "";
    len += printf("%s  userdata:       %12p\n", prefix, userdata);
    if (userdata)
    {
        len += printf("%s    issued:       \"%s\"\n", prefix, userdata->issued);
        len += printf("%s    file:         \"%s\"\n", prefix, userdata->file);
        len += printf("%s    event_count:  %12d\n", prefix, userdata->event_count);
        len += printf("%s    buffer:     %12p\n", prefix, userdata->buffer);
        len += printf("%s    buffer_len:   %12d\n", prefix, userdata->buffer_len);
        len += printf("%s    loaded_len:   %12d\n", prefix, userdata->loaded_len);
    }

    return len;
}

static struct AudioSpecUserdata_t OpenAudio_callback_userdata;
static void OpenAudio_callback(void *userdata, Uint8 *stream, int len)
{
    struct AudioSpecUserdata_t *sdata = (struct AudioSpecUserdata_t *)userdata;
    Uint32 new_len;

    printf("OpenAudio_callback (void *userdata, Uint8 *stream, int len) - triggered\n");
    sdata->event_count++;
    print_userdata(sdata, "");
    printf("  stream:         %12p\n", stream);
    printf("  len:            %12d\n", len);

    if (len > 0 && sdata && sdata->buffer)
    {
        if ((new_len = sdata->loaded_len + len) > sdata->buffer_len)
        {
            len = sdata->buffer_len - sdata->loaded_len;
            new_len = sdata->buffer_len;
        }
        SDL_memcpy(stream, sdata->buffer + sdata->loaded_len, len); // simply copy from one buffer into the other
        sdata->loaded_len = new_len;
    }
    if (sdata->loaded_len >= sdata->buffer_len)
        Quit = 1;
}

// it might be the same but the address should be different.
static struct AudioSpecUserdata_t LoadWAV_callback_userdata;
static void LoadWAV_callback(void *userdata, Uint8 *stream, int len)
{
    struct AudioSpecUserdata_t *sdata = (struct AudioSpecUserdata_t *)userdata;
    Uint32 new_len;

    printf("LoadWAV_callback (void *userdata, Uint8 *stream, int len) - triggered\n");
    sdata->event_count++;
    print_userdata((struct AudioSpecUserdata_t *)userdata, "");
    printf("  stream:         %p\n", stream);
    printf("  len:            %12d\n", len);

    if (len > 0 && sdata && sdata->buffer)
    {
        if ((new_len = sdata->loaded_len + len) > sdata->buffer_len)
        {
            len = sdata->buffer_len - sdata->loaded_len;
            new_len = sdata->buffer_len;
        }
        SDL_memcpy(stream, sdata->buffer + sdata->loaded_len, len); // simply copy from one buffer into the other
        sdata->loaded_len = new_len;
    }
    if (sdata->loaded_len >= sdata->buffer_len)
        Quit = 1;
}

static int print_spec(SDL_AudioSpec *spec, char *label)
{
    size_t len = 0;
    SDL_AudioFormat format;

    len += printf("  %s: An SDL_AudioSpec structure representing the desired output format\n", label);
    len += printf("    frequency:    %10d DSP frequency (samples per second)\n", spec->freq);
    len += printf("    format:       0x%08x Audio data format\n", format = spec->format);
    len += printf("      SDL_AUDIO_ISSIGNED:     %d\n", SDL_AUDIO_ISSIGNED(format));
    len += printf("      SDL_AUDIO_ISBIGENDIAN:  %d\n", SDL_AUDIO_ISBIGENDIAN(format));
    len += printf("      SDL_AUDIO_ISFLOAT:      %d\n", SDL_AUDIO_ISFLOAT(format));
    len += printf("      SDL_AUDIO_BITSIZE:      %d(bits per sample)\n", SDL_AUDIO_BITSIZE(format));
    len += printf("    channels:     %10d Number of separate sound channels\n", spec->channels);
    len += printf("    silence:      %10d Audio buffer silence value (calculated)\n", spec->silence);
    len += printf("    samples:      %10d Audio buffer size in samples\n", spec->samples);
    len += printf("    size:         %10d Audio buffer size in bytes\n", spec->size);
    len += printf("    callback:     %10p Function to call when the audio device needs more data\n", spec->callback);
    //  len += printf("    userdata:     %10p A pointer that is passed to callback (otherwise ignored by SDL)\n", spec->userdata);
    len += print_userdata((struct AudioSpecUserdata_t *)spec->userdata, "  ");
    return len;
}

int main(int argc, char *argv[])
{
    char *file;
    SDL_AudioSpec loadWAV_spec = {};
    SDL_AudioSpec openAudio_obtained_spec;
    Uint32 audio_len;
    Uint8 *audio_buf;

    if (argc < 2)
    {
        file = DEFAULT_AUDIO_PATH;
        printf("Using default wav file: \"%s\"\n", file);
    }
    else if (*argv[1] != '-')
    {
        file = argv[1];
    }
    else
    {
        printf("Usage is %s <filename>\n", argv[0]);
        return 0;
    }

    // Initialize SDL.
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL_Init(SDL_INIT_AUDIO) < 0\n");
        return 1;
    }

    /* open wav file, put specification in have, audio location in audiobuf and length in length */
    loadWAV_spec.callback = LoadWAV_callback;
    loadWAV_spec.userdata = &LoadWAV_callback_userdata;
    LoadWAV_callback_userdata.file = file;
    LoadWAV_callback_userdata.loaded_len = 0;
    if (!SDL_LoadWAV(file, &loadWAV_spec, &audio_buf, &audio_len))
    {
        printf("[SDL] Failed: SDL_LoadWAV(\"%s\", ...): %s\n", file, SDL_GetError());
        SDL_Quit();
        return 1;
    }

    LoadWAV_callback_userdata.issued = "SDL_LoadWAV";
    LoadWAV_callback_userdata.file = file;
    LoadWAV_callback_userdata.buffer = audio_buf;
    LoadWAV_callback_userdata.buffer_len = audio_len;
    LoadWAV_callback_userdata.event_count = 0;
    printf("[SDL] SDL_LoadWAV(\"%s\", ...) obtained:\n", file);
    print_spec(&loadWAV_spec, "loadWAV_spec");
    printf("  audio_buf:  %p The audio buffer\n", audio_buf);
    printf("  audio_len:    %12d The length of the audio buffer in bytes\n", audio_len);

    /*
  * open audio device
  */
    loadWAV_spec.callback = OpenAudio_callback;
    loadWAV_spec.userdata = (void *)&OpenAudio_callback_userdata;
    OpenAudio_callback_userdata.issued = "OpenAudio";
    OpenAudio_callback_userdata.file = file;
    OpenAudio_callback_userdata.buffer = audio_buf;
    OpenAudio_callback_userdata.buffer_len = audio_len;
    OpenAudio_callback_userdata.event_count = 0;
    printf("[SDL]SDL_OpenAudio(loadWAV_spec, openAudio_obtained_spec)\n");
    if (SDL_OpenAudio(&loadWAV_spec, &openAudio_obtained_spec) != 0)
    {
        printf("[SDL]  Couldn't open audio: %s\n", SDL_GetError());
        exit(-1);
    }
    print_spec(&loadWAV_spec, "loadWAV_spec");
    print_spec(&openAudio_obtained_spec, "openAudio_obtained_spec");

    /*
  * Play audio
  */

    printf("[SDL] SDL_PauseAudio(0)\n");
    SDL_PauseAudio(0);

    // wait until we're done playing
    //	while ( ! Quit && OpenAudio_callback_userdata.loaded_len < OpenAudio_callback_userdata.buffer_len) {
    while (!Quit)
    {
        SDL_Delay(100);
    }

    printf("wav file (\"%s\") all done playing\n", file);
    print_spec(&loadWAV_spec, "loadWAV_spec");
    printf("  audio_buf:  %p The audio buffer\n", LoadWAV_callback_userdata.buffer);
    printf("  audio_len:    %12d The length of the audio buffer in bytes\n", LoadWAV_callback_userdata.buffer_len);
    print_spec(&openAudio_obtained_spec, "openAudio_obtained_spec");
    printf("  audio_buf:  %p The audio buffer\n", OpenAudio_callback_userdata.buffer);
    printf("  audio_len:    %12d The length of the audio buffer in bytes\n", OpenAudio_callback_userdata.buffer_len);

    // shut everything down
    printf("[SDL] SDL_CloseAudio()\n");
    SDL_CloseAudio();

    printf("[SDL] SDL_FreeWAV(%p)\n", audio_buf);
    SDL_FreeWAV(audio_buf);
    printf("[SDL] SDL_Quit()\n");
    SDL_Quit();

    return 0;
}
