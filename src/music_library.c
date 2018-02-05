#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>

#include "music_library.h"
#include "music_data.h"
#include "queue.h"

const char SIGN_ARTIST = '\001';
const char SIGN_ALBUM_TITLE = '\002';
const char SIGN_SONG_TITLE = '\003';
const char SIGN_SONG_PATH = '\004';

const int MENU_STATE_NONE = 0;
const int MENU_STATE_MAIN = 1;
const int MENU_STATE_ARTISTS = 2;
const int MENU_STATE_ALBUMS = 3;
const int MENU_STATE_TRACKS = 4;
const int MENU_STATE_NUMBER = 5;

int menu_curr_state = 0;
int menu_curr_artist = 0;
int menu_curr_album = 0;

const char *UNKNOW_NAME = "<unknow>";
const char *prefix = "echo -e \"";
const char *suffix = "\" | dmenu -i";

struct MusicDatabase db;

struct AlbumInfo
{
    // These are titles displayed in the menu.
    char *title;
    // Tracks (title + 'SIGN_SONG_PATH' + path).
    char **tracks;
    // Length of both arrays.
    int length;
};

struct ArtistInfo 
{
    char *name;
    // All albums of the artist.
    struct AlbumInfo *albums;
    // Number of albums.
    int length;
};

struct MusicDatabase
{
    int length;

    struct ArtistInfo *artists;
};

int CompareStrings(const char *self, const char *other)
{
    if (self == NULL && other == NULL)
        return 0;
    else if (self == NULL)
        return 1;
    else if (other == NULL)
        return -1;
    else
        return strcmp(self, other);
}

int CompareMusicData(const void *self, const void *other)
{
    char ***elem1 = (char ***)self;
    char ***elem2 = (char ***)other;

    int cmp_artist = CompareStrings(
        (* elem1)[DATA_ALBUM_ARTIST], (* elem2)[DATA_ALBUM_ARTIST]);

    if (cmp_artist != 0)
        return cmp_artist;
    else
    {
        int cmp_album = CompareStrings(
            (* elem1)[DATA_ALBUM_TITLE], (* elem2)[DATA_ALBUM_TITLE]);
        if (cmp_album != 0)
            return cmp_album;
        else
        {
            int left_idx = 0, right_idx = 0;
            while ((unsigned int)left_idx < strlen((* elem1)[DATA_TRACK])
            && '0' <= (* elem1)[DATA_TRACK][left_idx]
            && (* elem1)[DATA_TRACK][left_idx] <= '9')
                left_idx++;

            while ((unsigned int)right_idx < strlen((* elem2)[DATA_TRACK])
            && '0' <= (* elem2)[DATA_TRACK][right_idx]
            && (* elem2)[DATA_TRACK][right_idx] <= '9')
                right_idx++;

            if (left_idx == right_idx)
            {
                int cmp_tracks = strncmp(
                    (* elem1)[DATA_TRACK],
                    (* elem2)[DATA_TRACK],
                    left_idx <= right_idx ? left_idx : right_idx);

                if (cmp_tracks != 0)
                    return cmp_tracks;
                else
                    return CompareStrings(
                        (* elem1)[DATA_TITLE], (* elem2)[DATA_TITLE]);
            }
            else
                return left_idx - right_idx;
        }
    }
}

void listdir(const char *name, int indent, char ***arr, int* idx)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            // printf("%*s[%s]\n", indent, "", entry->d_name);
            listdir(path, indent + 2, arr, idx);
        }
        else
        {
            // Later add other supported types.
            // TODO: Free this memory!
            char **music_data = CreateMusicData();
            char buf[strlen(name) + strlen(entry->d_name)+1];
            sprintf(buf, "%s/%s", name, entry->d_name);
            printf(buf);
            printf("\n");
            GetMusicDataFieldsFromMP3File(buf, music_data,
                (1 << DATA_FILE_PATH)
                    | (1 << DATA_TITLE)
                    | (1 << DATA_ALBUM_ARTIST)
                    | (1 << DATA_ALBUM_TITLE)
                    | (1 << DATA_TRACK));

            arr[(* idx)++] = music_data;
            // DeleteMusicData(music_data);
        }
    }
    closedir(dir);
    return;
}

char *ListArtists()
{
    // TODO: realloc for more characters.
    char res[256];
    int idx = 0;
    
    for (int i = 0; i < db.length; ++i)
    {
        for (unsigned int j = 0; j < strlen((db.artists)[i].name); ++j)
        {
            res[idx] = (db.artists)[i].name[j];
            idx++;
        }
        if (i < db.length -1)
        {
            res[idx++] = '\\';
            res[idx++] = 'n';
        }
    }
    res[idx++] = '\0';
    return strdup(res);
}

char *ListAlbums(struct ArtistInfo artistInfo)
{
    // TODO: realloc for more characters.
    char res[256];
    int idx = 0;
    
    for (int i = 0; i < artistInfo.length; ++i)
    {
        for (unsigned int j = 0; j < strlen(artistInfo.albums[i].title); ++j)
        {
            res[idx] = artistInfo.albums[i].title[j];
            idx++;
        }
        if (i < artistInfo.length-1)
        {
            res[idx++] = '\\';
            res[idx++] = 'n';
        }
        printf("%s\n", artistInfo.albums[i].title);
    }
    res[idx++] = '\0';
    return strdup(res);
}

char *ListTracks(struct AlbumInfo albumInfo)
{
    // TODO: realloc for more characters.
    char res[256];
    int idx = 0;
    
    for (int i = 0; i < albumInfo.length; ++i)
    {
#if 1
        for (int j = 0; 
            j < strchr(albumInfo.tracks[i],SIGN_SONG_PATH)-albumInfo.tracks[i];
            ++j)
        {
            res[idx] = albumInfo.tracks[i][j];
            idx++;
        }
        if (i < albumInfo.length-1)
        {
            res[idx++] = '\\';
            res[idx++] = 'n';
        }
#endif
        printf("%s\n", albumInfo.tracks[i]);
    }

    res[idx++] = '\0';
    return strdup(res);
}

// This is a thread which is called when menu is shown.
// Menu is handed separately, because it cannot block playing the music.
pthread_t menu_thread;

// Is menu currently running? If so there is no point in starting another
// process and CallMenu won't start dmenu.
int menu_is_running = 0;

void ShowMenu()
{
    char buf[1 << 14];
    int buf_idx = 0;
    char *msg_content;

    // TODO: Choose which function to call and generate content based on
    // menu state.
    if (menu_curr_state == MENU_STATE_ARTISTS)
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

    // TODO: One function to do this (for each of these)!
    for (unsigned int i = 0; i < strlen(prefix); ++i)    
        buf[buf_idx++] = prefix[i];
    for (unsigned int i = 0; i < strlen(msg_content); ++i)    
        buf[buf_idx++] = msg_content[i];
    for (unsigned int i = 0; i < strlen(suffix); ++i)    
        buf[buf_idx++] = suffix[i];    

    buf[buf_idx] = '\0';

    if (msg_content != NULL)
    {
        free(msg_content);
        msg_content = NULL;
    }

    // The pipe used to read dmenu's output.
    FILE *dmenu_pipe;
    char output_buff[1024];

    // Execute command stored in buffer and use pipe to read it's output.
    dmenu_pipe = popen(buf, "r");
    if (dmenu_pipe == NULL) 
    {
        printf("Failed to run command\n" );
        exit(1);
    }

#if 0 //??
    printf("OUTPUT:\n");
#endif

    // Dmenu should procude one line of output, so the rest is ignored.
    if (fgets(output_buff, sizeof(output_buff)-1, dmenu_pipe) == NULL) 
    {
        printf("No DMENU output!\n");
        pclose(dmenu_pipe);
        return;
    }

    pclose(dmenu_pipe);
    HandleDmenuOutput(output_buff);
}

void *StartMenuThread(void *args)
{
    ShowMenu();
    menu_is_running = 0;

    printf("\x1b[32mMenu thread terminates.\x1b[0m\n");
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
    menu_is_running = 1;

    pthread_create(&menu_thread, NULL, StartMenuThread, NULL);
}

void HandleDmenuOutput(const char *output)
{
    printf("MENU STATE: %d\n", menu_curr_state);

    if (menu_curr_state == MENU_STATE_ARTISTS)
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
    { //ListTracks        
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
        int chosen_track = -1;
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

        // Reset menu state to the default.
        menu_curr_state = MENU_STATE_ARTISTS;
    }
}

// TODO: Hudge refactor...
struct MusicDatabase CreateMusicDB()
{
    // TODO: Instead of guessing its good idea to accualy 
    //       calculate how much memory is needed.
    char ***arr = (char ***)malloc(256 * sizeof(char**));
    int idx = 0;

    listdir("/home/mateusz/Music", 0, arr, &idx);
    printf("idx = %d\n", idx);

    qsort(arr, idx, sizeof(char **), CompareMusicData);

#pragma region MAKE_MUSIC_DATABASE
    FILE *infile = fopen("music-lib","w");
    char *current_artist = NULL;
    char *current_album = NULL;

    for (int i = 0; i < idx; ++i)
    {
        char **music_data = arr[i];
        printf("PATH: %s\nTITLE: %s\nALBUM: %s\nARTIST: %s\nTRACK: %s\n\n",
                music_data[DATA_FILE_PATH],
                music_data[DATA_TITLE], music_data[DATA_ALBUM_TITLE],
                music_data[DATA_ALBUM_ARTIST], music_data[DATA_TRACK]);

        if (music_data[DATA_ALBUM_ARTIST] == NULL)
            music_data[DATA_ALBUM_ARTIST] = strdup(UNKNOW_NAME);

        if (music_data[DATA_ALBUM_TITLE] == NULL)
            music_data[DATA_ALBUM_TITLE] = strdup(UNKNOW_NAME);

        if (current_artist == NULL
        || strcmp(current_artist, music_data[DATA_ALBUM_ARTIST]) != 0)
        {
            if (current_artist != NULL)
                free(current_artist);
            current_artist = strdup(music_data[DATA_ALBUM_ARTIST]);
            fprintf(infile, "%c%s\n", SIGN_ARTIST, current_artist);
        }

        if (current_album == NULL 
        // TODO: This stays here in the case there are same named albums from
        //       different artist. But it is now checked twice... Big deal?
        || strcmp(current_artist, music_data[DATA_ALBUM_ARTIST]) != 0
        || strcmp(current_album, music_data[DATA_ALBUM_TITLE]) != 0)
        {
            if (current_album != NULL)
                free(current_album);
            current_album = strdup(music_data[DATA_ALBUM_TITLE]);
            fprintf(infile, "%c%s\n", SIGN_ALBUM_TITLE, current_album);
        }

        fprintf(infile, "%c%s%c%s\n", 
            SIGN_SONG_TITLE, music_data[DATA_TITLE], 
            SIGN_SONG_PATH, music_data[DATA_FILE_PATH]);
    }
    fflush(infile);
    fclose(infile);
#pragma endregion
    FILE *music_db = fopen("music-lib", "r");
    char *line = (char *)malloc(256 * sizeof(char));
    ssize_t nread;
    size_t len;

    int artists[256];
    int albums[512];

    int cur_artist = -1;
    int cur_album = -1;

    while ((nread = getline(&line, &len, music_db)) != -1) 
    {
        // printf(line);
        if (line[0] == SIGN_ARTIST)
        {
            cur_artist++;
            artists[cur_artist] = 0;
            // printf("\tInc. index; cur_artist = %d\n", cur_artist);
        }
        else if (line[0] == SIGN_ALBUM_TITLE)
        {
            artists[cur_artist]++;
            cur_album++;
            albums[cur_album] = 0;
            // printf("\t\tInc. album index; cur_album = %d\n", cur_album);
            // printf("\t\tFound album, cur_artist: %d, artists[%d]++ = %d\n", 
                // cur_artist, cur_artist, artists[cur_artist]);
        }
        else
        {
            // printf("\t\t\tFound track\n");
            albums[cur_album]++;
        }
    }

    printf("%d ARTISTS:\n", cur_artist);
    for (int i = 0; i <= cur_artist; ++i)
        printf("\tARTIST %d, %d ALBUMS\n", i, artists[i]);

    fseek(music_db, 0, SEEK_SET);

    struct ArtistInfo *db = 
        (struct ArtistInfo *)malloc((cur_artist+1) * sizeof(struct ArtistInfo));    

    cur_artist = -1;
    cur_album = -1;
    int idx_of_album = -1;
    int idx_of_song = -1;
    while ((nread = getline(&line, &len, music_db)) != -1) 
    {
        printf(line);
        if (line[0] == SIGN_ARTIST)
        {
            idx_of_album = -1;
            cur_artist++;
            db[cur_artist].length = artists[cur_artist];
            db[cur_artist].albums = 
                (struct AlbumInfo *)malloc(artists[cur_artist] * 
                 sizeof(struct AlbumInfo));
            char *endl_idx = strchr(line + 1, '\n');
            if (endl_idx == NULL)
            {
                printf("Bad geline - no \"\\n\"\n");
                break;   
            }
            else
                db[cur_artist].name = strndup(line+1, endl_idx-line-1);
        }
        else if (line[0] == SIGN_ALBUM_TITLE)
        {
            idx_of_album++;
            // artists[cur_artist]++;
            cur_album++;
            db[cur_artist].albums[idx_of_album].length = albums[cur_album];
            db[cur_artist].albums[idx_of_album].tracks = 
                (char **)malloc(albums[cur_album] * sizeof(char *));
            char *endl_idx = strchr(line + 1, '\n');
            if (endl_idx == NULL)
            {
                printf("Bad geline - no \"\\n\"\n");
                break;   
            }
            else
                db[cur_artist].albums[idx_of_album].title = 
                    strndup(line+1, endl_idx-line-1);
            idx_of_song = -1;
            // albums[cur_album] = 0;

            // printf("\t\tInc. album index; cur_album = %d\n", cur_album);
            // printf("\t\tFound album, cur_artist: %d, artists[%d]++ = %d\n", 
                // cur_artist, cur_artist, artists[cur_artist]);
        }
        else
        {
            idx_of_song++;
            char *endl_idx = strchr(line + 1, '\n');
            if (endl_idx == NULL)
            {
                printf("Bad geline - no \"\\n\"\n");
                break;   
            }
            else
                db[cur_artist].albums[idx_of_album].tracks[idx_of_song] = 
                    strndup(line+1, endl_idx-line-1);
        }
    }

    for (int i = 0; i <= cur_artist; ++i)
    {
        printf("ARTIST %s\n", db[i].name);
        for (int j = 0; j < db[i].length; ++j)
        {
            printf("\tTITLE %s\n", db[i].albums[j].title);
            for (int k = 0; k < db[i].albums[j].length; ++k)
                printf("\t\tTRACK:%s\n", db[i].albums[j].tracks[k]);
        }
    }

    fclose(music_db);

    struct MusicDatabase res;
    res.length = cur_artist;
    res.artists = db;

    return res;
}
