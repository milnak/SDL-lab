# https://wiki.libsdl.org/Installation
#	Debian: 
# 		sudo apt-get install libsdl2-2.0-0
# 		sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev
# https://wiki.libsdl.org/FAQLinux

EXEC = show_buttons

CFLAGS = -O3 -Wall -Werror
LDFLAGS = 

all: $(EXEC)

clean:
	rm -f "$(EXEC)" *.o

.PHONY: all clean

sdl_cflags := $(shell pkg-config --cflags sdl2)
sdl_libs := $(shell pkg-config --libs sdl2 SDL2_image SDL2_gfx)
override CFLAGS += $(sdl_cflags)
override LIBS += $(sdl_libs)

$(EXEC): $(EXEC).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
