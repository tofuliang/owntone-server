/*
 * OpenSubsonic API client
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef OPENSUBSONIC_API_H
#define OPENSUBSONIC_API_H

#include <json.h> // for json_object
#include "db.h"    // For db_queue_item (and potentially others later)

// Structure to hold OpenSubsonic server configuration
struct opensubsonic_config {
    char *server_url;
    char *username;
    char *password; // Consider storing hashed password or token
    char *token;
    char *salt;
    bool legacy_auth;
    char *client_name;
    char *api_version;
    bool enabled;
};

// Structure for an OpenSubsonic track
struct opensubsonic_track {
    char *id;
    char *title;
    char *artist;
    char *album;
    char *album_id; // ID of the album the track belongs to
    char *artist_id; // ID of the artist
    char *cover_art_id; // ID to be used with getCoverArt
    int duration; // in seconds
    int bit_rate; // in kbps
    char *path; // stream path or identifier
    char *content_type;
    int disc_number;
    int track_number;
    int year;
    char *genre;
    long long size; // file size in bytes
    bool is_dir; // if it's a directory-like entry (for some Subsonic servers)
    char *parent_id; // ID of the parent, if applicable
};

// Structure for an OpenSubsonic album
struct opensubsonic_album {
    char *id;
    char *name;
    char *artist;
    char *artist_id;
    char *cover_art_id;
    int song_count;
    int duration; // total duration in seconds
    int year;
    char *genre;
    // Potentially a list of opensubsonic_track structures if fetched together
};

// Structure for an OpenSubsonic artist
struct opensubsonic_artist {
    char *id;
    char *name;
    char *cover_art_id; // May not always be available directly
    int album_count;
};

// Structure for an OpenSubsonic playlist
struct opensubsonic_playlist {
    char *id;
    char *name;
    char *owner;
    int song_count;
    int duration; // total duration in seconds
    char *comment;
    char *cover_art_id; // May be an album ID from one of its songs
    // Potentially a list of opensubsonic_track structures if fetched together
};


// Initialization and Configuration
void opensubsonic_api_init(void);
void opensubsonic_api_deinit(void);
void opensubsonic_api_load_config(void);
bool opensubsonic_api_is_enabled(void);
const struct opensubsonic_config *opensubsonic_api_get_config(void);

// API Calls
int opensubsonic_api_ping(const char **errmsg);

json_object* opensubsonic_api_search3(const char *query,
                                      int artist_count, int artist_offset,
                                      int album_count, int album_offset,
                                      int song_count, int song_offset,
                                      const char **errmsg);

char* opensubsonic_api_get_cover_art_url(const char *id, int size, const char **errmsg); // Returns URL string

json_object* opensubsonic_api_get_playlists(int offset, int count, const char **errmsg);
json_object* opensubsonic_api_get_playlist(const char *playlist_id, const char **errmsg);


// Data parsing helpers (to be implemented in .c file)
// These will parse json_object responses into the structs defined above
// Example:
// int opensubsonic_parse_track(json_object *jobj, struct opensubsonic_track *track);
// int opensubsonic_parse_album(json_object *jobj, struct opensubsonic_album *album);
// int opensubsonic_parse_artist(json_object *jobj, struct opensubsonic_artist *artist);
// int opensubsonic_parse_playlist(json_object *jobj, struct opensubsonic_playlist *playlist);

// Utility to free parsed structs
void opensubsonic_free_track(struct opensubsonic_track *track);
void opensubsonic_free_album(struct opensubsonic_album *album);
void opensubsonic_free_artist(struct opensubsonic_artist *artist);
void opensubsonic_free_playlist(struct opensubsonic_playlist *playlist);

// Convert OpenSubsonic track to db_queue_item for playback
int opensubsonic_track_to_queue_item(const struct opensubsonic_track *os_track, struct db_queue_item *queue_item);

// Structure to hold a playlist with its entries (tracks)
struct opensubsonic_playlist_with_entries {
    char *id;
    char *name;
    char *owner;
    char *comment;
    char *cover_art_id; // May be an album ID from one of its songs
    int song_count;     // As reported by playlist metadata
    int duration;       // As reported by playlist metadata

    struct opensubsonic_track *entries; // Array of tracks
    int num_entries;                    // Number of tracks in the entries array
};

// Parses the 'getPlaylist' JSON object
// The caller gets ownership of playlist_out->entries and must call opensubsonic_free_playlist_with_entries.
int opensubsonic_parse_playlist_response(json_object *j_playlist_response, struct opensubsonic_playlist_with_entries *playlist_out);
void opensubsonic_free_playlist_with_entries(struct opensubsonic_playlist_with_entries *playlist);

// Function to fetch and parse a playlist directly into the C struct
// Returns 0 on success, -1 on failure. Caller must free result via opensubsonic_free_playlist_with_entries.
int opensubsonic_api_get_parsed_playlist(const char *playlist_id, struct opensubsonic_playlist_with_entries *result_out, const char **errmsg);

// Function to fetch a single song and parse it into the C struct
// Returns 0 on success, -1 on failure. Caller must free result_out via opensubsonic_free_track.
int opensubsonic_api_fetch_and_parse_song_details(const char *song_id, struct opensubsonic_track *result_out, const char **errmsg);

// Structure to hold an album with its entries (tracks)
struct opensubsonic_album_with_entries {
    char *id;
    char *name;
    char *artist;
    char *artist_id;
    char *cover_art_id;
    int song_count;     // As reported by album metadata
    int duration;       // As reported by album metadata
    int year;
    char *genre;

    struct opensubsonic_track *songs; // Array of tracks in the album
    int num_songs;                  // Number of tracks in the songs array
};

// Parses the 'getAlbum' JSON object (which should contain an 'album' object with 'song' array)
// The caller gets ownership of album_out->songs and must call opensubsonic_free_album_with_entries.
int opensubsonic_parse_album_response(json_object *j_album_response_root, struct opensubsonic_album_with_entries *album_out);
void opensubsonic_free_album_with_entries(struct opensubsonic_album_with_entries *album);

// Function to fetch and parse an album directly into the C struct
// Returns 0 on success, -1 on failure. Caller must free result_out via opensubsonic_free_album_with_entries.
int opensubsonic_api_get_parsed_album(const char *album_id, struct opensubsonic_album_with_entries *result_out, const char **errmsg);


#endif // OPENSUBSONIC_API_H
