#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H

extern const int DATA_FILE_PATH;
extern const int DATA_TITLE;
extern const int DATA_ALBUM_TITLE;
extern const int DATA_ARTIST;
extern const int DATA_ALBUM_ARTIST;
extern const int DATA_YEAR;
extern const int DATA_TRACK;
extern const int FILE_INFO_DATA_SIZE;

// TODO: Should I typedef musicdata somehow?

// Make a new music data without any info about the song.
char **CreateMusicData();

// Free and delete music data.
void DeleteMusicData(
    char **data);

// TODO: DO I really need both of these functions?
void GetMusicDataFieldsFromMP3File(
    char *path_to_file, 
    char **music_data, 
    int field_mask);

void GetMusicDataFromMP3File(
    char *path_to_file, 
    char **music_data);

#endif