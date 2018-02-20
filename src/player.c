#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bass.h>

#include "misc.h"
#include "music_data.h"

int player_is_paused = 1;

int force_quit = 0;

// A handle to basslib channel.
unsigned int channel;

char *CURRENT_TRACK_INFO_PATH = "./track-info";
char *UPDATE_MUSIC_SCRIPT=NULL;

int InitPlayer()
{
    // TODO: Check if file is locked. If so another instance is active
    //       and we kill program. Is this feature such important?
    if (!BASS_Init(-1, 44100, 0, 0, NULL))
    {
        printf ("Unable to inicialize. Error code: %d\n",
            BASS_ErrorGetCode());
        return -1;
    }

    return 0;
}

int PlayerIsBusy()
{
    return (BASS_ChannelIsActive(channel) == 0);
}

void UnpauseMusic()
{
    player_is_paused = 0;
    BASS_ChannelPlay(channel, 0);
    char buf[256];
    buf[0] = '\0';
    strcat(strcat(buf, "sed -i '1c1' "), CURRENT_TRACK_INFO_PATH);
    system(buf);

    system(UPDATE_MUSIC_SCRIPT);
}

void PauseMusic()
{
    player_is_paused = 1;
    BASS_ChannelPause(channel);    
    char buf[256];
    buf[0] = '\0';
    strcat(strcat(buf, "sed -i '1c0' "), CURRENT_TRACK_INFO_PATH);
    system(buf);

    system(UPDATE_MUSIC_SCRIPT);
}

int LoadAndPlayMusic(char *file_path)
{   
    BASS_ChannelStop(channel);
    
    // Load and play the file.
    unsigned int sample = BASS_SampleLoad(
        0, file_path, 0, 0, 1, BASS_SAMPLE_MONO);

    channel = BASS_SampleGetChannel(sample, FALSE);

    UnpauseMusic();
    BASS_ChannelPlay(channel, FALSE);

    return BASS_ErrorGetCode();
}

void PrintFiledToBufferAtIdx(char *field, char *command_buffer, int *idx_ptr)
{
    char field_buffer[256];
    if (field != NULL) 
        sprintf(field_buffer, "%s\\n", field);
    else                            
        sprintf(field_buffer, "\\n");
    PrintToBufferAtIndex(command_buffer, field_buffer, idx_ptr);
}

void UpdateTrackData(char **music_data)
{
    char command_buffer[1024];
    int idx = 0;

    PrintToBufferAtIndex(command_buffer, "echo -e \"", &idx);
    PrintToBufferAtIndex(command_buffer, "1\\n", &idx);

    PrintFiledToBufferAtIdx(music_data[DATA_TITLE], command_buffer, &idx);
    PrintFiledToBufferAtIdx(music_data[DATA_ARTIST], command_buffer, &idx);
    PrintFiledToBufferAtIdx(music_data[DATA_TRACK], command_buffer, &idx);
    PrintFiledToBufferAtIdx(music_data[DATA_YEAR], command_buffer, &idx);

    PrintToBufferAtIndex(command_buffer, "\" > ", &idx);                        
    PrintToBufferAtIndex(command_buffer, CURRENT_TRACK_INFO_PATH, &idx);
    command_buffer[idx++] = '\0';

    system(command_buffer);

    if (UPDATE_MUSIC_SCRIPT != NULL)
        system(UPDATE_MUSIC_SCRIPT);
}

void ToggleMusic()
{
    // TODO: There is much more options to handle here!
    // TODO: what if channel is null?
    if (BASS_ChannelIsActive(channel) == BASS_ACTIVE_PAUSED)
    {
        UnpauseMusic();
    }
    else
    {
        PauseMusic();
    }
}

void CleanPlayer()
{
	BASS_Stop();
	BASS_Free();    
}