EXEC = enum_video

CFLAGS = -O3 -Wall -Werror -DDEBUG -g
LDFLAGS = 

all: $(EXEC)

clean:
	rm -f "$(EXEC)" *.o

.PHONY: all clean

sdl_cflags := $(shell pkg-config --cflags sdl2)
sdl_libs := $(shell pkg-config --libs sdl2)
override CFLAGS += $(sdl_cflags)
override LIBS += $(sdl_libs)

$(EXEC): $(EXEC).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
