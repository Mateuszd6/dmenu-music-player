#include <stdlib.h>
#include <stdio.h>

#include <bass.h>

int player_is_paused = 1;

int force_quit = 0;

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
}

void PauseMusic()
{
    player_is_paused = 1;
    BASS_ChannelPause(channel);    
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