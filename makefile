CFLAGS = -g -I . -Wall -Wextra -D_DEFAULT_SOURCE --std=c11
LDFLAGS = -lpthread -lbass

all: main.o music_data.o
	gcc $(LDFLAGS) main.o music_data.o -o program

main.o: src/main.c src/music_data.h
	gcc $(CFLAGS) -c src/main.c

music_data.o: src/music_data.c src/music_data.h
	gcc $(CFLAGS) -c src/music_data.c

clean:
	rm -rf *.o
