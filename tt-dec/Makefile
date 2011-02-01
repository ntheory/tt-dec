CC = g++
CFLAGS = -O4
HEADERS =
EXTOBJECTS = ../library/DSPlib.o ../library/DSPlibFilter.o
SOURCES = tt-dec.cpp
APP = tt-dec
SDLCONFIG = `sdl-config --cflags --libs`

${APP}: $(EXTOBJECTS) $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) $(EXTOBJECTS) $(SOURCES) $(SDLCONFIG) -lrfftw -lfftw -lm -o ${APP}

clean:
	rm -f ${APP}
