/*
 * Copyright (C) 2024 owntone-server contributors
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <gcrypt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <curl/curl.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <json-c/json.h>

#include "db.h"
#include "inputs/subsonic.h"
#include "library.h"
#include "logger.h"
#include "misc.h"
#include "subsonic_webapi.h"

// Module definitions
#define MAX_RETRIES 3
#define RETRY_DELAY 5 // seconds

// Global variables
extern struct subsonic_status subsonic_status;
static struct subsonicwebapi_auth_info subsonic_auth;
static pthread_mutex_t subsonic_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private function declarations
static size_t
input_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct evbuffer *buf = (struct evbuffer *)userp;

  if (evbuffer_add(buf, contents, realsize) != 0)
    return 0;

  return realsize;
}

static int
subsonic_api_request(const char *endpoint, const char *params, char **response);

static int
subsonic_api_auth_request(const char *endpoint, const char *params, char **response);

// These functions are implemented in inputs/subsonic.c
int
subsonic_auth_token_generate(const char *password);
int
subsonic_auth_params_create(char **auth_params, const char *params);

static int
process_artist(json_object *artist);

static int
process_album(json_object *album);

static int
process_song(json_object *song, json_object *album_info);

// Library scanning functions
int
subsonicwebapi_fullrescan(void)
{
  char *response = NULL;
  int ret;

  pthread_mutex_lock(&subsonic_mutex);

  // First purge existing Subsonic items from the library
  db_purge_cruft_bysource(time(NULL), SCAN_KIND_SUBSONIC);

  // Enable the Subsonic virtual directory
  ret = db_directory_enable_bypath("/subsonic");
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Error enabling Subsonic directory\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  // Get all music folders
  ret = subsonic_api_auth_request("getMusicFolders", NULL, &response);
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get music folders\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  // Parse and process music folders
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse music folders response. Response content: '%s'\n", response);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  // Process each music folder
  json_object *subsonic_response, *music_folders, *music_folder_array;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response)
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'subsonic-response' field in JSON response: '%s'\n",
          json_object_to_json_string(json));
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  if (!json_object_object_get_ex(subsonic_response, "musicFolders", &music_folders))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'musicFolders' field in subsonic-response: '%s'\n",
          json_object_to_json_string(subsonic_response));
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  if (!json_object_object_get_ex(music_folders, "musicFolder", &music_folder_array))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'musicFolder' field in musicFolders: '%s'\n",
          json_object_to_json_string(music_folders));
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  int folder_count = json_object_array_length(music_folder_array);
  for (int i = 0; i < folder_count; i++)
    {
      json_object *folder = json_object_array_get_idx(music_folder_array, i);
      json_object *id;
      json_object_object_get_ex(folder, "id", &id);

      // Get artists in this folder
      char *folder_params = safe_asprintf("musicFolderId=%s", json_object_get_string(id));
      char *folder_response = NULL;
      ret = subsonic_api_auth_request("getIndexes", folder_params, &folder_response);
      free(folder_params);

      if (ret == 0)
	{
	  json_object *folder_json = json_tokener_parse(folder_response);
	  free(folder_response);

	  if (folder_json)
	    {
	      json_object *folder_response_obj, *indexes, *index_arr;
	      if (json_object_object_get_ex(folder_json, "subsonic-response", &folder_response_obj)
	          && json_object_object_get_ex(folder_response_obj, "indexes", &indexes)
	          && json_object_object_get_ex(indexes, "index", &index_arr))
		{
		  json_object *index_item;
		  int index_count = json_object_array_length(index_arr);

		  for (int j = 0; j < index_count; j++)
		    {
		      index_item = json_object_array_get_idx(index_arr, j);
		      json_object *artists;

		      if (json_object_object_get_ex(index_item, "artist", &artists))
			{
			  int artist_count = json_object_array_length(artists);
			  for (int k = 0; k < artist_count; k++)
			    {
			      json_object *artist = json_object_array_get_idx(artists, k);
			      process_artist(artist);
			    }
			}
		    }
		}
	      json_object_put(folder_json);
	    }
	}
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);

  // Notify library update
  library_update_trigger(1);
  return ret;
}

int
subsonicwebapi_rescan(void)
{
  char *response = NULL;
  int ret;
  time_t last_scan = 0;

  pthread_mutex_lock(&subsonic_mutex);

  // Enable the Subsonic virtual directory
  ret = db_directory_enable_bypath("/subsonic");
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Error enabling Subsonic directory\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  // Get last scan time from database
  ret = db_admin_getint64((int64_t *)&last_scan, "subsonic_last_scan");
  if (ret < 0)
    {
      // No previous scan time found, do full scan
      pthread_mutex_unlock(&subsonic_mutex);
      subsonicwebapi_fullrescan();
      return ret;
    }

  // Get recently added or updated albums
  char *params = safe_asprintf("fromYear=%d&fromMonth=%d&fromDay=%d", localtime(&last_scan)->tm_year + 1900,
      localtime(&last_scan)->tm_mon + 1, localtime(&last_scan)->tm_mday);

  ret = subsonic_api_auth_request("getAlbumList2", params, &response);
  free(params);

  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get recent albums\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  // Parse and process albums
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse recent albums response. Response content: '%s'\n", response);
      pthread_mutex_unlock(&subsonic_mutex);
      return ret;
    }

  json_object *subsonic_response, *album_list, *albums;
  if (!json_object_object_get_ex(json, "subsonic-response", &subsonic_response))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'subsonic-response' field in JSON response: '%s'\n",
          json_object_to_json_string(json));
      json_object_put(json);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  if (!json_object_object_get_ex(subsonic_response, "albumList2", &album_list))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'albumList2' field in subsonic-response: '%s'\n",
          json_object_to_json_string(subsonic_response));
      json_object_put(json);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  if (!json_object_object_get_ex(album_list, "album", &albums))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing 'album' field in albumList2: '%s'\n", json_object_to_json_string(album_list));
      json_object_put(json);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  int album_count = json_object_array_length(albums);
  for (int i = 0; i < album_count; i++)
    {
      json_object *album = json_object_array_get_idx(albums, i);
      process_album(album);
    }

  json_object_put(json);

  // Update last scan time
  db_admin_setint64("subsonic_last_scan", (int64_t)time(NULL));

  pthread_mutex_unlock(&subsonic_mutex);

  // Notify library update
  library_update_trigger(1);
  return ret;
}

void
subsonicwebapi_purge(void)
{
  db_purge_cruft_bysource(time(NULL), SCAN_KIND_SUBSONIC);
  library_update_trigger(1);
}

// Status functions
void
subsonicwebapi_status_info_get(struct subsonicwebapi_status_info *info)
{
  pthread_mutex_lock(&subsonic_mutex);
  memcpy(info, &subsonic_status, sizeof(struct subsonic_status));
  pthread_mutex_unlock(&subsonic_mutex);
}

void
subsonicwebapi_auth_info_get(struct subsonicwebapi_auth_info *info)
{
  pthread_mutex_lock(&subsonic_mutex);
  memcpy(info, &subsonic_auth, sizeof(struct subsonicwebapi_auth_info));
  pthread_mutex_unlock(&subsonic_mutex);
}

// Authentication functions
int
subsonicwebapi_login(const char *username, const char *password, const char *server_url)
{
  char *response = NULL;
  int ret;

  if (!username || !password || !server_url)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  // Save credentials
  strncpy(subsonic_auth.username, username, sizeof(subsonic_auth.username) - 1);
  strncpy(subsonic_auth.password, password, sizeof(subsonic_auth.password) - 1);
  strncpy(subsonic_status.server_url, server_url, sizeof(subsonic_status.server_url) - 1);
  strncpy(subsonic_status.username, username, sizeof(subsonic_status.username) - 1);

  // Generate token authentication
  ret = subsonic_auth_token_generate(password);
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot generate authentication token\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Test authentication
  ret = subsonic_api_auth_request("ping", NULL, &response);
  if (ret < 0)
    {
      subsonic_status.logged_in = false;
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  subsonic_status.logged_in = true;
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

void
subsonicwebapi_logout(void)
{
  pthread_mutex_lock(&subsonic_mutex);
  memset(&subsonic_auth, 0, sizeof(struct subsonicwebapi_auth_info));
  subsonic_status.logged_in = false;
  pthread_mutex_unlock(&subsonic_mutex);
}

// Artwork functions
char *
subsonicwebapi_artwork_url_get(const char *id, int max_w, int max_h)
{
  char *url;
  char *auth_params = NULL;
  int ret;

  if (!id)
    return NULL;

  pthread_mutex_lock(&subsonic_mutex);

  ret = subsonic_auth_params_create(&auth_params, NULL);
  if (ret < 0)
    {
      pthread_mutex_unlock(&subsonic_mutex);
      return NULL;
    }

  if (max_w > 0 && max_h > 0)
    url = safe_asprintf("%s/rest/getCoverArt.view?f=json&v=%s&c=owntone&id=%s&size=%d&%s", subsonic_status.server_url,
        subsonic_status.api_version, id, (max_w > max_h) ? max_w : max_h, auth_params);
  else
    url = safe_asprintf("%s/rest/getCoverArt.view?f=json&v=%s&c=owntone&id=%s&%s", subsonic_status.server_url,
        subsonic_status.api_version, id, auth_params);
  free(auth_params);
  pthread_mutex_unlock(&subsonic_mutex);
  return url;
}

// Private function implementations
static int
subsonic_api_request(const char *endpoint, const char *params, char **response)
{
  CURL *curl;
  CURLcode res;
  char *url;
  struct evbuffer *evbuf;
  char *buffer;
  size_t len;

  evbuf = evbuffer_new();
  if (!evbuf)
    return -1;

  // Build URL
  url = safe_asprintf("%s/rest/%s.view?%s&f=json&v=%s&c=owntone", subsonic_status.server_url, endpoint,
      params ? params : "", subsonic_status.api_version);

// 将请求 URL 写入日志文件
//   FILE *fp = fopen("/workspaces/owntone-server/subsonic_api.log", "a");
//   if (fp)
//     {
//       fprintf(fp, "Request URL: %s\n\n", url);
//       fclose(fp);
//     }

  curl = curl_easy_init();
  if (!curl)
    {
      evbuffer_free(evbuf);
      free(url);
      return -1;
    }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, input_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, evbuf);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "OwnTone");
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
    {
      DPRINTF(E_LOG, L_SUBS, "Subsonic API request failed: %s\n", curl_easy_strerror(res));
      evbuffer_free(evbuf);
      curl_easy_cleanup(curl);
      free(url);
      return -1;
    }

  len = evbuffer_get_length(evbuf);
  buffer = malloc(len + 1);
  if (!buffer)
    {
      DPRINTF(E_LOG, L_SUBS, "Memory allocation failed\n");
      evbuffer_free(evbuf);
      curl_easy_cleanup(curl);
      free(url);
      return -1;
    }

  evbuffer_remove(evbuf, buffer, len);
  buffer[len] = '\0';
  *response = buffer;

  // 将响应内容追加到日志文件
//   fp = fopen("/workspaces/owntone-server/subsonic_api.log", "a");
//   if (fp)
//     {
//       fprintf(fp, "Response:\n%s\n\n", buffer);
//       fclose(fp);
//     }

  // 移除原有的日志输出
  // DPRINTF(E_DBG, L_SUBS, "Subsonic API response for %s: %s\n", endpoint, buffer);

  evbuffer_free(evbuf);
  curl_easy_cleanup(curl);
  free(url);

  return 0;
}

static int
subsonic_api_request_with_retry(const char *endpoint, const char *params, char **response)
{
  int retries = 0;
  int ret;

  while (retries < MAX_RETRIES)
    {
      ret = subsonic_api_request(endpoint, params, response);
      if (ret == 0)
	return 0;

      retries++;
      if (retries < MAX_RETRIES)
	{
	  DPRINTF(E_LOG, L_SUBS, "Request failed, retrying in %d seconds (attempt %d/%d)\n", RETRY_DELAY, retries + 1,
	      MAX_RETRIES);
	  sleep(RETRY_DELAY);
	}
    }

  DPRINTF(E_LOG, L_SUBS, "Request failed after %d attempts\n", MAX_RETRIES);
  return -1;
}

static int
subsonic_api_auth_request(const char *endpoint, const char *params, char **response)
{
  char *auth_params;
  int ret;
  bool retry = false;

retry:
  ret = subsonic_auth_params_create(&auth_params, params);
  if (ret < 0)
    return -1;

  ret = subsonic_api_request_with_retry(endpoint, auth_params, response);
  free(auth_params);

  if (ret < 0)
    return -1;

  // Check if the response contains authentication error
  json_object *json = json_tokener_parse(*response);
  if (json)
    {
      json_object *subsonic_response;
      json_object *status;
      json_object *error;
      const char *status_str;
      const char *error_str = NULL;

      if (json_object_object_get_ex(json, "subsonic-response", &subsonic_response))
	{
	  if (json_object_object_get_ex(subsonic_response, "status", &status))
	    {
	      status_str = json_object_get_string(status);
	      if (strcmp(status_str, "failed") == 0)
		{
		  if (json_object_object_get_ex(subsonic_response, "error", &error))
		    error_str = json_object_get_string(error);

		  if (!retry && (error_str == NULL || strstr(error_str, "auth") != NULL))
		    {
		      // Authentication failed, regenerate token and retry once
		      DPRINTF(E_LOG, L_SUBS, "Authentication failed (%s), regenerating token\n",
		          error_str ? error_str : "unknown error");
		      subsonic_auth_token_generate(subsonic_auth.password);
		      retry = true;
		      json_object_put(json);
		      free(*response);
		      goto retry;
		    }
		  else
		    {
		      // Other error
		      DPRINTF(E_LOG, L_SUBS, "Subsonic API error: %s\n", error_str ? error_str : "unknown error");
		      json_object_put(json);
		      free(*response);
		      return -1;
		    }
		}
	    }
	}
      json_object_put(json);
    }

  return ret;
}

static int
process_artist(json_object *artist)
{
  json_object *id, *name;
  char *response = NULL;
  int ret;

  if (!json_object_object_get_ex(artist, "id", &id) || !json_object_object_get_ex(artist, "name", &name))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing artist id or name in response\n");
      return -1;
    }

  // Get artist details including albums
  char *params = safe_asprintf("id=%s", json_object_get_string(id));
  ret = subsonic_api_auth_request("getArtist", params, &response);
  free(params);

  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get artist details for '%s'\n", json_object_get_string(name));
      return -1;
    }

  json_object *json = json_tokener_parse(response);
  free(response);

  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse artist response for '%s'\n", json_object_get_string(name));
      return -1;
    }

  json_object *subsonic_response, *artist_info;
  if (json_object_object_get_ex(json, "subsonic-response", &subsonic_response)
      && json_object_object_get_ex(subsonic_response, "artist", &artist_info))
    {
      json_object *albums;
      if (json_object_object_get_ex(artist_info, "album", &albums))
	{
	  int album_count = json_object_array_length(albums);
	  for (int i = 0; i < album_count; i++)
	    {
	      json_object *album = json_object_array_get_idx(albums, i);
	      if (process_album(album) < 0)
		{
		  DPRINTF(E_LOG, L_SUBS, "Failed to process album %d/%d for artist '%s'\n", i + 1, album_count,
		      json_object_get_string(name));
		}
	    }
	}
    }

  json_object_put(json);
  return 0;
}

static int
process_album(json_object *album)
{
  json_object *id, *name;
  char *response = NULL;
  int ret;

  if (!json_object_object_get_ex(album, "id", &id) || !json_object_object_get_ex(album, "name", &name))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing album id or name in response\n");
      return -1;
    }

  // Get album details including songs
  char *params = safe_asprintf("id=%s", json_object_get_string(id));
  ret = subsonic_api_auth_request("getAlbum", params, &response);
  free(params);

  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get album details for '%s'\n", json_object_get_string(name));
      return -1;
    }

  json_object *json = json_tokener_parse(response);
  free(response);

  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse album response for '%s'\n", json_object_get_string(name));
      return -1;
    }

  json_object *subsonic_response, *album_info;
  if (json_object_object_get_ex(json, "subsonic-response", &subsonic_response)
      && json_object_object_get_ex(subsonic_response, "album", &album_info))
    {
      json_object *songs;
      if (json_object_object_get_ex(album_info, "song", &songs))
	{
	  int song_count = json_object_array_length(songs);
	  for (int i = 0; i < song_count; i++)
	    {
	      json_object *song = json_object_array_get_idx(songs, i);
	      if (process_song(song, album_info) < 0)
		{
		  DPRINTF(E_LOG, L_SUBS, "Failed to process song %d/%d in album '%s'\n", i + 1, song_count,
		      json_object_get_string(name));
		}
	    }
	}
    }

  json_object_put(json);
  return 0;
}

static int
process_song(json_object *song, json_object *album_info)
{
  json_object *id, *title, *album, *artist, *track, *year, *genre, *size, *duration, *suffix, *bitRate, *simplingRate,
      *bitDepth, *channelCount, *path, *songCount;
  struct media_file_info mfi;
  int ret;

  memset(&mfi, 0, sizeof(struct media_file_info));

  if (!json_object_object_get_ex(song, "id", &id) || !json_object_object_get_ex(song, "title", &title))
    {
      DPRINTF(E_LOG, L_SUBS, "Missing song id or title in response\n");
      return -1;
    }

  mfi.id = 0;
  mfi.title = strdup(json_object_get_string(title));
  mfi.data_kind = DATA_KIND_SUBSONIC;
  mfi.media_kind = MEDIA_KIND_MUSIC;
  mfi.scan_kind = SCAN_KIND_SUBSONIC;
  mfi.url = safe_asprintf("%s", strdup(json_object_get_string(id)));
  if (json_object_object_get_ex(song, "album", &album))
    mfi.album = strdup(json_object_get_string(album));
  if (json_object_object_get_ex(song, "artist", &artist))
    mfi.artist = strdup(json_object_get_string(artist));
  if (json_object_object_get_ex(song, "track", &track))
    mfi.track = json_object_get_int(track);
  if (json_object_object_get_ex(song, "year", &year))
    mfi.year = json_object_get_int(year);
  if (json_object_object_get_ex(song, "genre", &genre))
    mfi.genre = strdup(json_object_get_string(genre));
  if (json_object_object_get_ex(song, "size", &size))
    mfi.file_size = json_object_get_int64(size);
  if (json_object_object_get_ex(song, "duration", &duration))
    mfi.song_length = json_object_get_int(duration) * 1000;
  if (json_object_object_get_ex(song, "suffix", &suffix))
    {
      mfi.type = strdup(json_object_get_string(suffix));
      mfi.fname = safe_asprintf("%s - %s.%s", mfi.artist, mfi.title, json_object_get_string(suffix));
    }
  if (json_object_object_get_ex(song, "bitRate", &bitRate))
    mfi.bitrate = json_object_get_int(bitRate);
  if (json_object_object_get_ex(song, "simplingRate", &simplingRate))
    mfi.samplerate = json_object_get_int(simplingRate);
  if (json_object_object_get_ex(song, "bitDepth", &bitDepth))
    mfi.bits_per_sample = json_object_get_int(bitDepth);
  if (json_object_object_get_ex(song, "channelCount", &channelCount))
    mfi.channels = json_object_get_int(channelCount);
  if (json_object_object_get_ex(song, "path", &path))
    {
      mfi.path = strdup(json_object_get_string(path));
      // Create virtual path
      mfi.virtual_path = safe_asprintf("subsonic:/%s", strdup(json_object_get_string(path)));
    }

  if (json_object_object_get_ex(album_info, "songCount", &songCount))
    {
      mfi.total_tracks = json_object_get_int(songCount);
    }

  // Add to database
  ret = db_file_add(&mfi);

  // Free allocated memory
  free_mfi(&mfi, 1);

  return ret;
}

/* --------------------------- Library interface ---------------------------- */
/*                              Thread: library                               */

static int
subsonicwebapi_library_queue_item_add(
    const char *uri, int position, char reshuffle, uint32_t item_id, int *count, int *new_item_id)
{
  // TODO: Implement queue item add functionality
  return LIBRARY_PATH_INVALID;
}

static int
subsonicwebapi_library_initscan(void)
{
  int ret;
  struct subsonic_status status;

  // 获取 subsonic 状态，检查是否启用
  subsonic_status_get(&status);
  if (!status.enabled)
    {
      DPRINTF(E_INFO, L_SUBS, "Subsonic support is disabled\n");
      return 0;
    }
  //     return 0;
  /* Refresh access token for the subsonic webapi */
  ret = subsonic_relogin();
  if (ret < 0)
    {
      // User not logged in or error refreshing token
      db_purge_cruft_bysource(time(NULL), SCAN_KIND_SUBSONIC);
      return 0;
    }

  /*
   * Scan saved tracks from the web api
   */
  subsonicwebapi_fullrescan();
  return 0;
}

static int
subsonicwebapi_library_rescan(void)
{
  subsonicwebapi_rescan();
  return 0;
}

static int
subsonicwebapi_library_metarescan(void)
{
  subsonicwebapi_rescan();
  return 0;
}

static int
subsonicwebapi_library_fullrescan(void)
{
  db_purge_cruft_bysource(time(NULL), SCAN_KIND_SUBSONIC);
  subsonicwebapi_fullrescan();
  return 0;
}

static int
subsonicwebapi_library_init()
{
  int ret;
  struct subsonic_status status;

  ret = subsonic_init();
  if (ret < 0)
    return -1;

  // 同步subsonic状态到webapi模块
  subsonic_status_get(&status);
  pthread_mutex_lock(&subsonic_mutex);
  memcpy(&subsonic_status, &status, sizeof(struct subsonic_status));
  pthread_mutex_unlock(&subsonic_mutex);

  return 0;
}

static int
subsonicwebapi_playlist_item_add(const char *vp_playlist, const char *vp_item)
{
  // TODO: Implement playlist item addition
  return -1;
}

static int
subsonicwebapi_playlist_remove(const char *virtual_path)
{
  // TODO: Implement playlist removal
  return -1;
}

static int
subsonicwebapi_queue_save(const char *path)
{
  // TODO: Implement queue saving
  return -1;
}

static void
subsonicwebapi_library_deinit()
{
  subsonic_deinit();
}

struct library_source subsonicsource = {
  .scan_kind = SCAN_KIND_SUBSONIC,
  .disabled = 0,
  .init = subsonicwebapi_library_init,
  .deinit = subsonicwebapi_library_deinit,
  .initscan = subsonicwebapi_library_initscan,
  .rescan = subsonicwebapi_library_rescan,
  .metarescan = subsonicwebapi_library_metarescan,
  .fullrescan = subsonicwebapi_library_fullrescan,
  .queue_item_add = subsonicwebapi_library_queue_item_add,
  .playlist_item_add = subsonicwebapi_playlist_item_add,
  .playlist_remove = subsonicwebapi_playlist_remove,
  .queue_save = subsonicwebapi_queue_save,
};

/* ------------------------ Public API command callbacks -------------------- */
/*                              Thread: library                               */

/* ------------------------------ Public API -------------------------------- */