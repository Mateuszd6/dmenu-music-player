CFLAGS = -g -I . -Wall -Wextra -D_DEFAULT_SOURCE --std=c11
LDFLAGS = -lpthread -lbass
MUSIC_DIR = "$$HOME/Music"

all: obj/main.o obj/misc.o obj/music_data.o obj/track_queue.o obj/music_library.o obj/menu.o obj/player.o
	gcc $(LDFLAGS) obj/main.o obj/misc.o obj/music_data.o obj/track_queue.o obj/music_library.o obj/menu.o obj/player.o -o dmus-daemon

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

obj/player.o: src/player.c src/player.h
	gcc $(CFLAGS) -c src/player.c -o obj/player.o

.PHONY: clean
clean:
	rm -rf obj/*.o

.PHONY: make-alias
make-alias:
	@echo -e "$$(pwd)/src/dmus --executable-dir=$$(pwd) --music-database-dir=$$(pwd) --music-dir=$(MUSIC_DIR)\$$@" > ./start-dmus
	@chmod +x start-dmus
	@echo "Alias for calling music player has been created."
	@echo "You can now start music player with a script 'start-dmus'"
	@echo "which can be placed anywhere You want."
	@echo "Unless You move something other than this script"
	@echo "from this directory this should always work."
