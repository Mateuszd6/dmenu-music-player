#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "track_queue.h"
#include "misc.h"
#include "music_data.h"
#include "music_library.h"
#include "player.h"
#include "menu.h"

// These are used to call dmenu command. 
// TODO: Let user modify the command with which dmenu is called?
const char *prefix = "echo -e \"";
const char *suffix = "\" | dmenu -i";

// These are commands used to create main menu msg.
const char *menu_main_queue     = "[Queue]\n";
const char *menu_main_quit      = "[Quit]\n";
const char *menu_main_next      = "[Next]\n";
// TODO: Making them const results with a warning:
char *menu_main_pause           = "[Pause]\n";
char *menu_main_play            = "[Play]\n";

// This is a thread which is called when menu is shown.
// Menu is handed separately, because it cannot block playing the music.
pthread_t menu_thread;

// Is menu currently running? If so there is no point in starting another
// process and CallMenu won't start dmenu.
int menu_is_running = 0;

// Create msg contant for main menu.
char *MakeMainMenu()
{
    int pause_len = strlen(menu_main_pause);
    int play_len = strlen(menu_main_play);
    
    int msg_len = (pause_len > play_len ? pause_len : play_len) 
        + strlen(menu_main_queue) 
        + strlen(menu_main_quit) 
        + strlen(menu_main_next)
        + 1;

    char *res = malloc(msg_len * sizeof(char));
    res[0] = '\0';
    
    char *play_toggle = player_is_paused ? menu_main_play : menu_main_pause;
    strcat(strcat(strcat(strcat(
        res, play_toggle), menu_main_next), menu_main_queue), menu_main_quit);    
    
    // It's because final msg cannot have '\n' on its end or there will be a 
    // empty option in the dmenu.
    res[msg_len-2] = '\0';

    return res;
}

void ShowMenu()
{
    // TODO: Think about the size of buffer before allocating 8K bytes...
    char buf[1 << 14];
    int buf_idx = 0;
    char *msg_content = NULL;

    // TODO: Choose which function to call and generate content based on
    // menu state.
    if (menu_curr_state == MENU_STATE_MAIN)
        msg_content = MakeMainMenu();
    else if (menu_curr_state == MENU_STATE_ARTISTS)
        msg_content = ListArtists();        
    else if (menu_curr_state == MENU_STATE_ALBUMS)
        msg_content = ListAlbums((db.artists)[menu_curr_artist]);
    else if (menu_curr_state == MENU_STATE_TRACKS)
        msg_content = ListTracks((db.artists)[menu_curr_artist].albums[
            menu_curr_album]);
    else
    {
        printf("Unexpected menu state: %d\n", menu_curr_state);
        return;
    }

    PrintToBufferAtIndex(buf, prefix, &buf_idx);
    PrintToBufferAtIndex(buf, msg_content, &buf_idx);
    PrintToBufferAtIndex(buf, suffix, &buf_idx);
    PrintToBufferAtIndex(buf, "\0", &buf_idx);

    if (msg_content != NULL)
    {
        free(msg_content);
        msg_content = NULL;
    }

    // The pipe used to read dmenu's output.
    char output_buff[1024];

    GetSystemCommandOneLineOutput(buf, output_buff, 1024 - 1);
    printf("DMENU OUTPUT: %s", output_buff);

    HandleDmenuOutput(output_buff);
}

void *StartMenuThread(void *args)
{
    menu_is_running = 1;

    // Silent a warning about [args] being not used.  
    if (args) {}

    ShowMenu();
    printf("\x1b[32mMenu thread terminates.\x1b[0m\n");

    menu_is_running = 0;
    return NULL;
}

void CallMenu()
{
    if (menu_is_running)
    {
        // TODO: Or mayby terminate former menu and call new one?
        printf("MENU_IS_ALREADY_RUNNING!\n");
        return;
    }

    pthread_create(&menu_thread, NULL, StartMenuThread, NULL);
}

void HandleDmenuOutput(const char *output)
{
    printf("MENU STATE: %d\n", menu_curr_state);

    if (menu_curr_state == MENU_STATE_MAIN)
    {
        if (strcmp(output, menu_main_play) == 0)
        {        
            UnpauseMusic();
        }
        else if (strcmp(output, menu_main_pause) == 0)
        {
            PauseMusic();                
        }
        else if (strcmp(output, menu_main_queue) == 0)
        {
            menu_curr_state = MENU_STATE_ARTISTS;
            ShowMenu();    
        }
        else if (strcmp(output, menu_main_quit) == 0)
        {
            force_quit = 1;
            printf("QUITTING!\n");
        }
    }    

    else if (menu_curr_state == MENU_STATE_ARTISTS)
    {
        menu_curr_artist = -1;
        for (int i = 0; i < db.length; ++i)
        {
            printf("%d: %s\n", i, (db.artists)[i].name);
            if (strncmp((db.artists)[i].name, output, 
                strlen((db.artists)[i].name)) == 0)
            {
                menu_curr_artist = i;
                break;
            }
        }        
        if (menu_curr_artist >= 0)
        {
            menu_curr_state = MENU_STATE_ALBUMS;
            ShowMenu();
        }
        else
        {
            printf("No artist found!\n");
            menu_curr_state = MENU_STATE_MAIN;
        }
    }

    else if (menu_curr_state == MENU_STATE_ALBUMS)
    {        
        menu_curr_album = -1;
        for (int i = 0; i < (db.artists)[menu_curr_artist].length; ++i)
        {
            printf("%d: %s\n",i,(db.artists)[menu_curr_artist].albums[i].title);
            if (strncmp((db.artists)[menu_curr_artist].albums[i].title, output, 
                strlen((db.artists)[menu_curr_artist].albums[i].title)) == 0)
            {
                menu_curr_album = i;
                break;
            }
        }        
        if (menu_curr_album >= 0)
        {
            menu_curr_state = MENU_STATE_TRACKS;
            ShowMenu();
        }
        else
        {
            printf("No album found!\n");
            menu_curr_state = MENU_STATE_MAIN;
        }
    }

    else if (menu_curr_state == MENU_STATE_TRACKS)
    {
        if (strcmp(output, "[.]\n") == 0)
        {
            for (int i = 0; 
                i < (db.artists)[menu_curr_artist].albums[menu_curr_album].length; 
                ++i)
            {
                char *trck = (db.artists)[menu_curr_artist]
                    .albums[menu_curr_album].tracks[i];
                Enqueue(track_queue, strchr(trck, SIGN_SONG_PATH) + 1);
            }            
        }
        else
        {
            for (int i = 0; 
                i < (db.artists)[menu_curr_artist].albums[menu_curr_album].length; 
                ++i)
            {
                char *trck = (db.artists)[
                    menu_curr_artist].albums[menu_curr_album].tracks[i];
                
                int output_len = strlen(output);
                if (strncmp(output, trck, output_len - 1) == 0 
                && trck[output_len-1] == SIGN_SONG_PATH)
                {
                    printf("CHOSEN: %s\n", trck);
                    Enqueue(track_queue, strchr(trck, SIGN_SONG_PATH) + 1);
                }
            }
        }

        // Reset menu state to the default.
        menu_curr_state = MENU_STATE_MAIN;
    }
}
