/*
 * Copyright (C) 2024 OwnTone contributors
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SRC_OPENSUBSONIC_WEBAPI_H_
#define SRC_OPENSUBSONIC_WEBAPI_H_

#include <event2/event.h>
#include <stdbool.h>
#include <json-c/json.h>

#include "http.h"
#include "misc.h"

struct opensubsonic_status_info
{
  bool enabled;
  bool connected;
  char server_url[256];
  char username[128];
  char api_version[16];
  char client_name[64];
  int timeout;
  bool artwork_enabled;
  int artwork_max_size;
};

struct opensubsonic_artist
{
  const char *id;
  const char *name;
  int album_count;
  const char *starred;
};

struct opensubsonic_album
{
  const char *id;
  const char *name;
  const char *artist;
  const char *artist_id;
  int song_count;
  int duration;
  const char *created;
  const char *starred;
  int year;
  const char *genre;
  const char *cover_art;
};

struct opensubsonic_song
{
  const char *id;
  const char *parent;
  const char *title;
  const char *album;
  const char *artist;
  bool is_dir;
  const char *cover_art;
  int size;
  const char *content_type;
  const char *suffix;
  int duration;
  int bit_rate;
  const char *path;
  bool is_video;
  int disc_number;
  int track;
  int year;
  const char *genre;
  const char *type;
  const char *starred;
  const char *album_id;
  const char *artist_id;
};

struct opensubsonic_playlist
{
  const char *id;
  const char *name;
  const char *comment;
  const char *owner;
  bool is_public;
  int song_count;
  int duration;
  const char *created;
  const char *changed;
  const char *cover_art;
};

int
opensubsonic_connect(const char *server_url, const char *username, const char *password, const char **errmsg);

void
opensubsonic_disconnect(void);

char *
opensubsonic_artwork_url_get(const char *id, int max_w, int max_h);

void
opensubsonic_status_info_get(struct opensubsonic_status_info *info);

// API endpoint functions
struct json_object *
opensubsonic_request_artists(void);

struct json_object *
opensubsonic_request_artist(const char *id);

struct json_object *
opensubsonic_request_albums(void);

struct json_object *
opensubsonic_request_album(const char *id);

struct json_object *
opensubsonic_request_playlists(void);

struct json_object *
opensubsonic_request_playlist(const char *id);

struct json_object *
opensubsonic_request_search(const char *query);

char *
opensubsonic_stream_url_get(const char *id);

#endif /* SRC_OPENSUBSONIC_WEBAPI_H_ */