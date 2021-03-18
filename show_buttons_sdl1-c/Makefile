# Using SDL1
# sudo apt install libsdl1.2debian libsdl-gfx1.2-5 libsdl-gfx1.2-dev libsdl-gfx1.2-doc libsdl-image1.2 libsdl-image1.2-dev 

EXEC = show_buttons

CFLAGS = -O3 -Wall -Werror -DDEBUG -g
LDFLAGS = 

all: $(EXEC)

clean:
	rm -f "$(EXEC)" *.o

.PHONY: all clean

sdl_cflags := $(shell pkg-config --cflags sdl)
sdl_libs := $(shell pkg-config --libs sdl SDL_image SDL_gfx)
override CFLAGS += $(sdl_cflags)
override LIBS += $(sdl_libs) -Wl,-rpath=/usr/local/lib

$(EXEC): $(EXEC).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
