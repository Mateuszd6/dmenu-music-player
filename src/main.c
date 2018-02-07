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
    char *myfifo = "/tmp/my-fifo-pipe";

    // If this file already exist delete it first.
    if(access(myfifo, F_OK) != -1) 
    {
        // TODO: Handle the case when program cannot remove this file.
        unlink(myfifo);
    }

    if (mkfifo(myfifo, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) != 0)
    {
        // TODO: Should the program be able to run without pipe messages?
        printf("Couldn't make fifo pipe.\n");
        return 0;
    }

    (* fd) = open(myfifo, O_RDONLY | O_NONBLOCK);
    if ((* fd) < 0)
    {
        // TODO: The program should be able to run without pipe messages.
        printf("Couldn't open a pipe.\n");
        return 0;
    }

    return 1;
}

// TODO: This is a handle to bass stuff!
unsigned int channel;

void SetMusicInfo(char *file_path)
{
    char **music_data = CreateMusicData();
    GetMusicDataFromMP3File(file_path, music_data);

    printf("Song info:\n");
    if (music_data[DATA_ARTIST] != NULL) 
        printf("Artist: %s\n", music_data[DATA_ARTIST]);    
    if  (music_data[DATA_ALBUM_ARTIST] != NULL) 
        printf("Album artist: %s\n", music_data[DATA_ALBUM_ARTIST]);    
    if  (music_data[DATA_ALBUM_TITLE] != NULL)
        printf("Album: %s\n", music_data[DATA_ALBUM_TITLE]);            
    if  (music_data[DATA_YEAR] != NULL)
        printf("Year: %s\n", music_data[DATA_YEAR]);    

    // TODO: Refactor this. 
    //       Also add a posibility to call given script after each song 
    //       starts/ends/is paused etc.
    // NOTE: This is not important stuff...
    if  (music_data[DATA_TITLE] != NULL) 
    {
        printf("Title: %s\n", music_data[DATA_TITLE]);
        char buf[512];
        sprintf(buf, 
                "/home/mateusz/work/bass-player/src/set_music.sh PLAY \"%s\"", 
                music_data[DATA_TITLE]);
        system(buf);
    }
    else
        system("/home/mateusz/work/bass-player/src/set_music.sh PLAY \\<unknown\\>");

    if  (music_data[DATA_TRACK] != NULL)
        printf("Track: %s\n", music_data[DATA_TRACK]);

    DeleteMusicData(music_data);
}

int LoadAndPlayMusic(char *file_path)
{   
    BASS_ChannelStop(channel);
    // Load and play the file.
    unsigned int sample = BASS_SampleLoad(
        0, file_path, 0, 0, 1, BASS_SAMPLE_MONO);

    channel = BASS_SampleGetChannel(sample, FALSE);

    BASS_ChannelPlay(channel, FALSE);

    SetMusicInfo(file_path);

    return BASS_ErrorGetCode();
}

void PauseMusic()
{
    // TODO: what if channel is null?
    BASS_ChannelPause(channel);
}

void ContinueMusic()
{
    // TODO: what if channel is null?
    BASS_ChannelPlay(channel, 0);
}

void ToggleMusic()
{
    // TODO: There is much more options to handle here!
    if (BASS_ChannelIsActive(channel) == BASS_ACTIVE_PAUSED)
        ContinueMusic();
    else
        PauseMusic();
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
            // TODO: What here?
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
    int fi = open("./locked_file", O_CREAT);

    // TODO: Should the music player be able to run without lockfile?
    if (fi == -1 || flock(fi, LOCK_EX) != 0) 
    {
        printf("Error opening lock file. Exitting...");
        return 255;
    }    

    db = CreateMusicDB();

    menu_curr_state = MENU_STATE_ARTISTS;
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
    LoadAndPlayMusic("/home/mateusz/Music/guitar.mp3");

    // The main loop to handle the incomming messages.
    while (1)
    {
        if (ProcessMessage(fd, buffer) == MSG_QUIT)
            break;

        usleep(1000 * 100);

        // Track has finished. Need to load another track from the queue.
        if (BASS_ChannelIsActive(channel) == 0)
        {
            if (EmptyQueue(track_queue))
                // Shut down if no more tracks to play...
                break;
            else
            {
                LoadAndPlayMusic(Peek(track_queue));
                Dequeue(track_queue);
            }
        }
    }

    // Cleanup.
    close(fd);
    
    // TODO: BASS cleanup.
	BASS_Stop();
	BASS_Free();

    system("/home/mateusz/work/bass-player/src/set_music.sh END"); 

    return 0;
}