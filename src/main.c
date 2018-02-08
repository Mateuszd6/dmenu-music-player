/*
    TODO: Line that cat's 256 bytes from each file in ~/Music dir. to tmp file.  
          use it to check for important changes in ~/Music directory.
          find . -type f -print0 | sort -z | xargs -0 head -c 256 > tmp
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <bass.h>

#include "track_queue.h"
#include "music_data.h"
#include "music_library.h"
#include "menu.h"

// TODO: Make nice messages system.
const int MSG_QUIT = 1 << 2;

const char *LOCKFILE_DIR = "/tmp/dmenu-player-lockfile";
const char *FIFO_PIPE_DIR = "/tmp/dmenu-player-pipe";

// A handle to basslib channel.
unsigned int channel;

int InitPlayer()
{
    // TODO: Check if file is locked. If so another instance is active
    //       and we kill program. Is this feature such important?
    if (!BASS_Init(-1, 44100, 0, 0, NULL))
    {
        printf ("Unable to inicialize. Error code: %d\n",
            BASS_ErrorGetCode());
        return 0;
    }

    return 1;
}

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

    // 
    (* fd) = open(FIFO_PIPE_DIR, O_RDONLY | O_NONBLOCK);
    if ((* fd) < 0)
    {
        // TODO: The program should be able to run without pipe messages.
        printf("Couldn't open a pipe.\n");
        return 0;
    }

    return 1;
}

int LoadAndPlayMusic(char *file_path)
{   
    BASS_ChannelStop(channel);
    // Load and play the file.
    unsigned int sample = BASS_SampleLoad(
        0, file_path, 0, 0, 1, BASS_SAMPLE_MONO);

    channel = BASS_SampleGetChannel(sample, FALSE);

    player_is_paused = 0;
    BASS_ChannelPlay(channel, FALSE);

    return BASS_ErrorGetCode();
}

void ToggleMusic()
{
    // TODO: There is much more options to handle here!
    // TODO: what if channel is null?
    if (BASS_ChannelIsActive(channel) == BASS_ACTIVE_PAUSED)
    {
        player_is_paused = 0;
        BASS_ChannelPlay(channel, 0);
    }
    else
    {
        player_is_paused = 1;
        BASS_ChannelPause(channel);
    }
}

int ProcessMessage(int fd, char *buffer)
{
    int read_res;
    read_res = read(fd, buffer, 1 << 20);
    if (read_res < 0)
    {
        // TODO: Don't check pipe any more!
        printf("Error with the pipe.\n");
        return -1;
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
            return MSG_QUIT;
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
        return 0;
    }
    else
    {
        printf("!\n");
        return 0;
    }
}

int main(void)
{
    // TODO: Specify lock_file dir? Or use relative path?
    int lock_file = open(LOCKFILE_DIR, O_CREAT);

    // TODO: Should the music player be able to run without lockfile?
    if (lock_file == -1 || flock(lock_file, LOCK_EX) != 0) 
    {
        printf("Error opening lock file. Exitting...");
        return 255;
    }    

    db = CreateMusicDB();

    menu_curr_state = MENU_STATE_MAIN;
    CallMenu();

    track_queue = InitializeQueue();
    
    if (!InitPlayer())
        return -1;

    // Handler to a pipe file and buffer for the messages.
    int fd;

    // Buffer for messages comming from pipe.
    char buffer[1 << 7];

    if (!InitPipe(& fd))
        return -1;

    // TODO: TEMP!
    Enqueue(track_queue, "/home/mateusz/Music/guitar.mp3");
    player_is_paused = 0;

    // The main loop to handle the incomming messages.
    while (1)
    {
        if (ProcessMessage(fd, buffer) == MSG_QUIT)
            break;

        usleep(1000 * 100);

        // Track has finished. Need to load another track from the queue.
        if (BASS_ChannelIsActive(channel) == 0)
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
                        LoadAndPlayMusic(music_data[DATA_FILE_PATH]);                    
                        printf("Playing:\n");
                        if (music_data[DATA_ARTIST] != NULL) 
                            printf("Artist: %s\n", music_data[DATA_ARTIST]);    
                        if  (music_data[DATA_ALBUM_TITLE] != NULL)
                            printf("Album: %s\n", music_data[DATA_ALBUM_TITLE]);            
                        if  (music_data[DATA_YEAR] != NULL)
                            printf("Year: %s\n", music_data[DATA_YEAR]);
                        DeleteMusicData(music_data);   
                    }

                    Dequeue(track_queue);
                }
            }
        }
    }

    // Cleanup.
    close(fd);
    
    // TODO: BASS cleanup.
	BASS_Stop();
	BASS_Free();    
    
    if (flock(lock_file, LOCK_UN) != 0)
    {
        printf("Error, could not realese the lock file.\n");
        return -1;
    }

    return 0;
}