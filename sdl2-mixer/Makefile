# sudo apt install  libsdl2-mixer-dev
EXEC = sdl-mixer

CFLAGS = -g -O3 -Wall -Werror $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_mixer

SOURCES = $(basename $(wildcard *.c))

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: $(SOURCES)

clean:
	rm -f "$(SOURCES)" *.o
