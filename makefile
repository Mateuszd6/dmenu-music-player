CFLAGS = -g -I . -Wall -Wextra -D_DEFAULT_SOURCE --std=c11
LDFLAGS = -lpthread -lbass

all: obj/main.o obj/misc.o obj/music_data.o obj/track_queue.o obj/music_library.o obj/menu.o
	gcc $(LDFLAGS) obj/main.o obj/misc.o obj/music_data.o obj/track_queue.o obj/music_library.o obj/menu.o -o program

obj/misc.o: src/misc.c src/misc.h
	gcc $(CFLAGS) -c src/misc.c -o obj/misc.o

obj/main.o: src/main.c
	gcc $(CFLAGS) -c src/main.c -o obj/main.o

obj/track_queue.o: src/track_queue.c src/track_queue.h
	gcc $(CFLAGS) -c src/track_queue.c -o obj/track_queue.o

obj/music_data.o: src/music_data.c src/music_data.h
	gcc $(CFLAGS) -c src/music_data.c -o obj/music_data.o

obj/music_library.o: src/music_library.c src/music_library.h
	gcc $(CFLAGS) -c src/music_library.c -o obj/music_library.o

obj/menu.o: src/menu.c src/menu.h
	gcc $(CFLAGS) -c src/menu.c -o obj/menu.o

clean:
	rm -rf obj/*.o
