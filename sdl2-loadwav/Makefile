EXEC = sdl2-loadwav

CFLAGS = -O3 -Wall -Werror -DDEBUG -g
LDFLAGS = 

all: $(EXEC)

clean:
	rm -f "$(EXEC)" *.o

.PHONY: all clean

sdl_cflags := $(shell sdl2-config --cflags)
sdl_libs := $(shell sdl2-config --libs)
override CFLAGS += $(sdl_cflags)
override LIBS += $(sdl_libs)

$(EXEC): $(EXEC).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
