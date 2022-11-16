all : main.c
	clang `sdl2-config --cflags` -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer main.c -o viewer.exe
