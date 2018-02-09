#ifndef PLAYER_H
#define PLAYER_H

extern int player_is_paused;

// Is dmenu already on the scren?
// Also: Is seperate thread used only to call dmenu command in 
//       non-blocking mode active?
extern int menu_is_running;

// If true program breaks the main loop and quits.
extern int force_quit;

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