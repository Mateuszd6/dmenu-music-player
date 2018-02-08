#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include "misc.h"
#include "music_data.h"
#include "music_library.h"
#include "track_queue.h"

// TODO: Use some normal format like xml, 
//       not plain ASCII characters.
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

// Used to sort music data. 
// TODO: Allow different user configurations?
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
            unsigned int left_idx = 0, right_idx = 0;
            while (left_idx < strlen((* elem1)[DATA_TRACK])
            && '0' <= (* elem1)[DATA_TRACK][left_idx]
            && (* elem1)[DATA_TRACK][left_idx] <= '9')
                left_idx++;

            while (right_idx < strlen((* elem2)[DATA_TRACK])
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

// Recursive funtion used to find all music files within a directory.
// [name] is a cwd, TODO: remove indent, [arr] is an array to be filled 
// with data and [idx] in an index at which funtion writes to the [arr].
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
            GetMusicDataFromMP3File(buf, music_data);

            arr[(* idx)++] = music_data;
            // DeleteMusicData(music_data);
        }
    }
    closedir(dir);
    return;
}

// TODO: Get rid of this copypaste!

// List all artist from the music database [db].
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

// List all albubs of the given artist. 
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

// List all tracks from the given album.
char *ListTracks(struct AlbumInfo albumInfo)
{
    // TODO: realloc for more characters.
    char res[256];
    int idx = 0;
    
    for (int i = 0; i < albumInfo.length; ++i)
    {
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
        printf("%s\n", albumInfo.tracks[i]);
    }

    res[idx++] = '\0';
    return strdup(res);
}

// TODO: Hudge refactor...
struct MusicDatabase CreateMusicDB()
{
    // TODO: Instead of guessing its good idea to accualy 
    //       calculate how much memory is needed.
    char ***arr = malloc(256 * sizeof(char**));
    int idx = 0;

    // Makes an array stored in [arr] of every sound file in the
    // music folder, sorted depth first: artist -> album -> track.
    listdir("/home/mateusz/Music", 0, arr, &idx);
    printf("idx = %d\n", idx);

    qsort(arr, idx, sizeof(char **), CompareMusicData);

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

    FILE *music_db = fopen("music-lib", "r");
    char *line = malloc(256 * sizeof(char));
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

    struct ArtistInfo *db = malloc((cur_artist+1) * sizeof(struct ArtistInfo));    

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
            db[cur_artist].albums = malloc(artists[cur_artist] * 
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
            db[cur_artist].albums[idx_of_album].tracks = malloc(
                albums[cur_album] * sizeof(char *));
                
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
