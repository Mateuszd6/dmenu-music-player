#ifndef PLAYER_H
#define PLAYER_H


extern int player_is_paused;
extern unsigned int channel;

void UnpauseMusic();

void PauseMusic();

// Play if [player_is_paused], pause oin other case.
void ToggleMusic();

// inicialize BASS player.
int InitPlayer();

int PlayerIsBusy();

// Play music on the given path.
int LoadAndPlayMusic(char *file_path);

// Make a cleanup.
void CleanPlayer();

#endif