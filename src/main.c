/*
    TODO: Use it to check for important changes in ~/Music directory.
          find . -type f -exec stat \{} --printf="%y\n" \; | sort -n -r | head -n 1
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "track_queue.h"
#include "music_data.h"
#include "music_library.h"
#include "menu.h"
#include "player.h"
#include "misc.h"

const char *SET_MUSIC_DIR_FLAG = "--music-dir";
const char *SET_MUSIC_DB_DIR_FLAG = "--music-database-dir";
const char *UPDATE_MUSIC_INFO_COMMAND_FLAG = "--update-music-info-scirpt";


// TODO: Let user specify them too?
const char *LOCKFILE_DIR = "/tmp/dmenu-player-lockfile";
const char *FIFO_PIPE_DIR = "/tmp/dmenu-player-pipe";

int InitPipe(int *fd)
{
    // If this file already exist delete it first.
    if(access(FIFO_PIPE_DIR, F_OK) != -1) 
    {
        // TODO: Handle the case when program cannot remove this file.
        unlink(FIFO_PIPE_DIR);
    }

    // Make a new pipe.
    if (mkfifo(FIFO_PIPE_DIR, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) != 0)
    {
        // TODO: Should the program be able to run without pipe messages?
        printf("Couldn't make fifo pipe.\n");
        return 0;
    }

    (* fd) = open(FIFO_PIPE_DIR, O_RDONLY | O_NONBLOCK);
    if ((* fd) < 0)
    {
        // TODO: The program should be able to run without pipe messages.
        printf("Couldn't open a pipe.\n");
        return 0;
    }

    return 1;
}

void ProcessMessage(int fd, char *buffer)
{
    int read_res;
    read_res = read(fd, buffer, 1 << 20);
    if (read_res < 0)
    {
        // TODO: Don't check pipe any more!
        printf("Error with the pipe.\n");
        return;
    }

    // If there is a message hande it!
    else if (read_res > 0)
    {
        if (strncmp(buffer, "player-queue\n", read_res) == 0)
        {
            // TODO: What here? Probobly nothing coz it is handled another way.
            //       But check it later.
        }
        else if (strncmp(buffer, "player-toggle-pause\n", read_res) == 0)
        {
            printf("Toggling pause...\n");
            ToggleMusic();
        }
        else if (strncmp(buffer, "player-quit\n", read_res) == 0)
        {
            printf("Quitting...\n");
            force_quit = 1;
        }
        // NOTE: This is purely debug operation, becasue it is pritned to
        //       the deamons output, not the bash script, who has called it.
        else if (strncmp(buffer, "player-print-queue\n", read_res) == 0)
        {
            PrintQueue(track_queue); 
        }
        // TODO: Call dmenu via bash or shortcut.
        else if (strncmp(buffer, "menu-show\n", read_res) == 0)
        {
            CallMenu();
        }
        else if (strncmp(buffer, "menu-pick", strlen("menu-pick")) == 0)
        {
            HandleDmenuOutput(buffer + strlen("menu-pick") + 1);
        }
        else
        {
            printf("Received unrecognized message: %.*s\n", read_res, buffer);
        }
        // TODO: Change to action code?
    }

    else
    {
        printf("!\n");
    }
}

int main(int argc, char **argv)
{
    system("pwd > ~/my_pwd");
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], SET_MUSIC_DIR_FLAG, 
            strlen(SET_MUSIC_DIR_FLAG)-1) == 0)
        {
            char *value = strchr(argv[i], '=');
            if (value == NULL) return 255; // TODO: Handle this case better?
            value++;
            MUSIC_DIR = value; 
        }
        else if (strncmp(argv[i], SET_MUSIC_DB_DIR_FLAG, 
            strlen(SET_MUSIC_DB_DIR_FLAG)-1) == 0)
        {
            char *value = strchr(argv[i], '=');
            if (value == NULL) return 255; // TODO: Handle this case better?
            value++;
            MUSIC_DATABASE_DIR = value; 
            CURRENT_TRACK_INFO_PATH=malloc(256 * sizeof(char));
            CURRENT_TRACK_INFO_PATH[0] = '\0';
            strcat(strcat(CURRENT_TRACK_INFO_PATH, value), "/track-info");
        }
        else if (strncmp(argv[i], UPDATE_MUSIC_INFO_COMMAND_FLAG,
            strlen(UPDATE_MUSIC_INFO_COMMAND_FLAG)-1) == 0)
        {
            char *value = strchr(argv[i], '=');
            if (value == NULL) return 255; // TODO: Handle this case better?
            value++;
            if (strcmp(value, "") != 0)
                UPDATE_MUSIC_SCRIPT = value;
            else
            {
                printf("No scrpit to execute on song load.\n");
            }
        }
        else
            printf("Unrecognized parameter: %s\n", argv[i]);
    }

    // TODO: Specify lock_file dir? Or use relative path?
    int lock_file = open(LOCKFILE_DIR, O_CREAT | O_WRONLY, 
        // TODO: Setting these flags ends proglem with a lockfile. 
        //       But are they all necesary?
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    // TODO: Should the music player be able to run without lockfile?
    if (lock_file == -1) 
    {
        printf("Error opening lock file. Exitting...");
        return 255;
    }    
    else if (flock(lock_file, LOCK_EX | LOCK_NB) != 0)
    {
        printf("Error locking file. Exitting...");
        return 255;
    }

    if (InitPlayer() != 0) return -1;

    db = CreateMusicDB();
    track_queue = InitializeQueue();
    
    // Handler to a pipe file and buffer for the messages.
    int fd;

    // Buffer for messages comming from pipe.
    char buffer[1 << 7];

    if (!InitPipe(& fd))
        return -1;

    player_is_paused = 1;

    menu_curr_state = MENU_STATE_MAIN;
    CallMenu();

    // The main loop to handle the incomming messages.
    while (1)
    {
        ProcessMessage(fd, buffer);
        if (force_quit) break;

        usleep(1000 * 100);

        // Track has finished. Need to load another track from the queue.
        if (PlayerIsBusy())
        {
            if (!player_is_paused)
            {
                if (EmptyQueue(track_queue))
                    // Shut down if no more tracks to play...
                    break;
                else
                {
                    char **music_data = NULL;
                    if (Peek(track_queue, &music_data) == 0)
                    {
                        UpdateTrackData(music_data);
                        LoadAndPlayMusic(music_data[DATA_FILE_PATH]);
                    }
                    music_data = NULL;

                    Dequeue(track_queue);
                }
            }
        }
    }

    // Cleanup.

    // Remove track-info file and update the script.
    unlink(CURRENT_TRACK_INFO_PATH);
    if (UPDATE_MUSIC_SCRIPT)
        system(UPDATE_MUSIC_SCRIPT);

    close(fd);    
    CleanPlayer();

    if (flock(lock_file, LOCK_UN) != 0)
    {
        printf("Error, could not realese the lock file.\n");
        return -1;
    }

    return 0;
}