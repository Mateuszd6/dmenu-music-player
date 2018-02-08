#ifndef MENU_H
#define MENU_H

extern const char *prefix;
extern const char *suffix;

// TODO: Move it someware else. It's not really connected
//       to menu stuff.
extern int player_is_paused;

void CallMenu();

void HandleDmenuOutput();

#endif