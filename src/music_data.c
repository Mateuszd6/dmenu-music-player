#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <alloca.h> // TODO: Try to remove it from here!

#include "music_data.h"

const int DATA_FILE_PATH = 0;
const int DATA_TITLE = 1;
const int DATA_ALBUM_TITLE = 2;
const int DATA_ARTIST = 3;
const int DATA_ALBUM_ARTIST = 4;
const int DATA_YEAR = 5;
const int DATA_TRACK = 6;
const int FILE_INFO_DATA_SIZE = 7;

const char *title_tag_id = "TIT2";
const char *year_tag_id = "TYER";
const char *publisher_tag_id = "TPUB";
const char *content_type_tag_id = "TCON";
const char *album_title_tag_id = "TALB";
const char *track_tag_id = "TRCK";
// TODO: Check it with the docs:
const char *song_artist_tag_id = "TPE1";
const char *album_artist_tag_id = "TPE2";

// Clear the contents of music data but do not delete it.
void ClearMusicData(char **music_data)
{
    for (int i = 0; i < FILE_INFO_DATA_SIZE; ++i)
    {
        free(music_data[i]); 
        music_data[i] = NULL;
    }
}

// Set a given field of the music data.
void SetMusicDataField(char **music_data, const char *content, 
    int tag_size, int field_idx)
{
    if (music_data[field_idx] != NULL) 
        free(music_data[field_idx]);
    
    // TODO: There is some binary stuff at the beginning of the tag contents.
    //       Check if with the docs, this soultion is temporary.
    int size = 0;
    while (content[size] < '0' || content[size] > 'Z')
        size++;

    if (size < tag_size)
    {
        music_data[field_idx] = malloc((tag_size-size+1) * sizeof(char));
        memcpy(music_data[field_idx], content+size, (tag_size-size) * sizeof(char));
        // TODO: Sure?
        music_data[field_idx][tag_size-size] = 0;
    }
}

// Given a ID3 tag, parse it and if its field is interesting (is in [field_mask]) 
// then set this tag in the [music_data] using [SetMusicDataField].
void ID3ParseNextTag(char **music_data, const unsigned char *content, 
    int *tag_size, int field_mask)
{    
    for (int i = 0; i < 4; ++i)
        (* tag_size) += content[7-i] * (1 << (8*i));
 
    if (strncmp((char *)content, title_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_TITLE)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_TITLE);
    else if (strncmp((char *)content, album_title_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_ALBUM_TITLE)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_ALBUM_TITLE);
    else if (strncmp((char *)content, song_artist_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_ARTIST)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_ARTIST);
    else if (strncmp((char *)content, album_artist_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_ALBUM_ARTIST)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_ALBUM_ARTIST);
    else if (strncmp((char *)content, year_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_YEAR)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_YEAR);
    else if (strncmp((char *)content, track_tag_id, 4) == 0 
    && (field_mask & (1 << DATA_TRACK)))
        SetMusicDataField(music_data, (char *)(content+10), 
            (* tag_size), DATA_TRACK);
}

// Given a file, read only the ID3 data and store important fields
// in [music_data].
void GetMusicDataFieldsFromMP3File(
    char *path_to_file, char **music_data, int field_mask)
{
    int file;
    unsigned char header[10];
    unsigned char *id3_content;
    int tag_size = 0;

    // TODO: check if strdup is OK.
    if (field_mask & (1 << DATA_FILE_PATH))
        music_data[DATA_FILE_PATH] = strdup(path_to_file);    

    file = open(path_to_file, O_RDONLY);
    if (file == -1)
    {
        printf("Error opening file\n");
        return;
    }
    read(file, header, 10);    

    // TODO: This gives warning. But is more clear...    
    if (strncmp((char *)header, "ID3", 3) == 0)
    {
        // Calculate size. Size is indicated by four bytes, but
        // in each, 7th bit is set to 0 and ignored.
        for (int i = 0; i < 4; ++i)
            tag_size += header[9-i] * (1 << (7*i));
        
        id3_content = alloca(sizeof(char) * tag_size);

        read(file, id3_content, tag_size);

        int idx = 0;
        while (idx < tag_size)
        {
            // Skipping should happen only at the end...
            // TODO: Should I assert that somehow? 
            while (idx < tag_size && id3_content[idx] == '\0')            
                idx++;
            if (idx >= tag_size) 
                break;

            int size = 0;
            ID3ParseNextTag(music_data, &id3_content[idx], &size, 
                field_mask);
            idx += size + 10;
        }
    }
    // TODO: check last 128 bytes because mp3 metadata might
    //       be also stored there (ID3v1, or its extended version).
    else
        printf("No I3 data in the file!\n");

    close(file);
}

char **CreateMusicData()
{
    char **res = malloc(FILE_INFO_DATA_SIZE * sizeof(char *));
    for (int i = 0; i < FILE_INFO_DATA_SIZE; ++i)
        res[i] = NULL;
    return res;
}

void DeleteMusicData(char **music_data)
{
    ClearMusicData(music_data);
    free (music_data);
    music_data = NULL;
}

void GetMusicDataFromMP3File (char *path_to_file, char **music_data)
{
    GetMusicDataFieldsFromMP3File(
        path_to_file, music_data, 
        (1 << FILE_INFO_DATA_SIZE)-1);
}