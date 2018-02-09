#ifndef MUSIC_LIBRARY_H
#define MUSIC_LIBRARY_H

extern const char SIGN_ARTIST;
extern const char SIGN_ALBUM_TITLE;
extern const char SIGN_SONG_TITLE;
extern const char SIGN_SONG_PATH;

extern const int MENU_STATE_NONE;
extern const int MENU_STATE_MAIN;
extern const int MENU_STATE_ARTISTS;
extern const int MENU_STATE_ALBUMS;
extern const int MENU_STATE_TRACKS;
extern const int MENU_STATE_NUMBER;

extern int menu_curr_state;
extern int menu_curr_artist;
extern int menu_curr_album;

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
} db;

void ShowMenu();
void HandleDmenuOutput(const char *output);
struct MusicDatabase CreateMusicDB();
char *ListArtists();
char *ListAlbums(struct ArtistInfo artistInfo);
char *ListTracks(struct AlbumInfo albumInfo);

#endif