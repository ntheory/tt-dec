CFLAGS = -O4
SDLCONFIG = `sdl-config --cflags`

all: DSPlib.o DSPlibFilter.o

clean:
	rm -rf *.o

DSPlib.o: DSPlib.cpp DSPlib.h
	g++ -c ${CFLAGS} DSPlib.cpp ${SDLCONFIG} -o DSPlib.o

DSPlibFilter.o: DSPlibFilter.cpp DSPlibFilter.h
	g++ -c ${CFLAGS} DSPlibFilter.cpp -o DSPlibFilter.o
