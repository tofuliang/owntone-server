#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <gcrypt.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "conffile.h"
#include "logger.h"
#include "misc.h"
#include "input.h"
#include "transcode.h"
#include "subsonic.h"

// Forward declarations
static int stop(struct input_source *source);

// Callback function definitions
static size_t
input_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct evbuffer *buf = (struct evbuffer *)userp;
  
  evbuffer_add(buf, contents, realsize);
  return realsize;
}

// Global variables
struct subsonic_status subsonic_status;
static struct subsonic_credentials subsonic_credentials;
static pthread_mutex_t subsonic_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private function declarations
static int 
subsonic_api_request(const char *endpoint, const char *params, char **response);

static int
subsonic_api_auth_request(const char *endpoint, const char *params, char **response);

// Initialization function
int
subsonic_init(void)
{
  cfg_t *cfg_subsonic;
  int ret;

  memset(&subsonic_status, 0, sizeof(struct subsonic_status));
  memset(&subsonic_credentials, 0, sizeof(struct subsonic_credentials));

  cfg_subsonic = cfg_getsec(cfg, "subsonic");
  if (!cfg_subsonic)
    {
      DPRINTF(E_LOG, L_SUBS, "Subsonic configuration section not found\n");
      return -1;
    }

  subsonic_status.enabled = cfg_getbool(cfg_subsonic, "enabled");
  if (!subsonic_status.enabled)
    {
      DPRINTF(E_INFO, L_SUBS, "Subsonic support is disabled\n");
      return 0;
    }

  // Get configuration
  const char *server_url = cfg_getstr(cfg_subsonic, "server_url");
  const char *username = cfg_getstr(cfg_subsonic, "username");
  const char *password = cfg_getstr(cfg_subsonic, "password");
  const char *api_version = cfg_getstr(cfg_subsonic, "api_version");
  subsonic_credentials.use_token_auth = cfg_getbool(cfg_subsonic, "use_token_auth");

  if (!server_url || !username || !password)
    {
      DPRINTF(E_LOG, L_SUBS, "Incomplete Subsonic configuration (requires server_url, username and password)\n");
      return -1;
    }

  // Save configuration
  snprintf(subsonic_status.server_url, sizeof(subsonic_status.server_url), "%s", server_url);
  snprintf(subsonic_status.username, sizeof(subsonic_status.username), "%s", username);
  snprintf(subsonic_status.api_version, sizeof(subsonic_status.api_version), "%s", 
           api_version ? api_version : "1.16.1");
  
  snprintf(subsonic_credentials.username, sizeof(subsonic_credentials.username), "%s", username);
  snprintf(subsonic_credentials.password, sizeof(subsonic_credentials.password), "%s", password);

  // Initialize CURL
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Try to login
  ret = subsonic_login(username, password, server_url, NULL);
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Subsonic login failed\n");
      return -1;
    }

  DPRINTF(E_INFO, L_SUBS, "Subsonic initialized successfully, using %s authentication\n", 
          subsonic_credentials.use_token_auth ? "token" : "plaintext password");
  return 0;
}

void
subsonic_deinit(void)
{
  if (subsonic_status.enabled)
    {
      curl_global_cleanup();
    }
}

int
subsonic_login(const char *username, const char *password, const char *server_url, const char **errmsg)
{
  char *response;
  int ret;

  pthread_mutex_lock(&subsonic_mutex);

  strncpy(subsonic_credentials.username, username, sizeof(subsonic_credentials.username) - 1);
  strncpy(subsonic_credentials.password, password, sizeof(subsonic_credentials.password) - 1);
  
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
      *errmsg = "Authentication failed";
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_relogin(void)
{
  return subsonic_login(subsonic_credentials.username, 
                       subsonic_credentials.password,
                       subsonic_status.server_url,
                       NULL);
}

void
subsonic_status_get(struct subsonic_status *status)
{
  memcpy(status, &subsonic_status, sizeof(struct subsonic_status));
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
  url = safe_asprintf("%s/rest/%s.view?%s&f=json&v=%s&c=owntone", 
                     subsonic_status.server_url,
                     endpoint,
                     params ? params : "",
                     subsonic_status.api_version);

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

  // 打印完整的响应内容
//   DPRINTF(E_DBG, L_SUBS, "Subsonic API response for %s: %s\n", endpoint, buffer);

  evbuffer_free(evbuf);
  curl_easy_cleanup(curl);
  free(url);

  return 0;
}

int
subsonic_auth_token_generate(const char *password)
{
  // If there is already a cached token and salt, and the password hasn't changed, return directly
  if (subsonic_credentials.use_token_auth && 
      subsonic_credentials.token[0] != '\0' && 
      subsonic_credentials.salt[0] != '\0' &&
      strcmp(subsonic_credentials.password, password) == 0)
    {
      return 0;
    }

  // Save new password
  strncpy(subsonic_credentials.password, password, sizeof(subsonic_credentials.password) - 1);

  char *salt;
  char *concat;
  unsigned char md5_hash[16];
  char md5_hex[33];
  int i;
  int ret;

  // Generate random salt
  salt = malloc(17);
  if (!salt)
    return -1;

  ret = snprintf(salt, 17, "%016lx", (uint64_t)time(NULL));
  if (ret != 16)
    {
      free(salt);
      return -1;
    }

  // Concatenate password and salt
  concat = malloc(strlen(password) + strlen(salt) + 1);
  if (!concat)
    {
      free(salt);
      return -1;
    }
  sprintf(concat, "%s%s", password, salt);

  // Calculate MD5
  gcry_md_hash_buffer(GCRY_MD_MD5, md5_hash, concat, strlen(concat));
  
  for (i = 0; i < 16; i++)
    sprintf(md5_hex + i * 2, "%02x", md5_hash[i]);
  md5_hex[32] = '\0';

  // Save salt and token
  strncpy(subsonic_credentials.salt, salt, sizeof(subsonic_credentials.salt) - 1);
  strncpy(subsonic_credentials.token, md5_hex, sizeof(subsonic_credentials.token) - 1);
  subsonic_credentials.use_token_auth = true;

  free(salt);
  free(concat);
  return 0;
}

int
subsonic_auth_params_create(char **auth_params, const char *params)
{
  if (subsonic_credentials.use_token_auth)
    *auth_params = safe_asprintf("u=%s&t=%s&s=%s&%s",
                              subsonic_credentials.username,
                              subsonic_credentials.token,
                              subsonic_credentials.salt,
                              params ? params : "");
  else
    *auth_params = safe_asprintf("u=%s&p=%s&%s",
                              subsonic_credentials.username,
                              subsonic_credentials.password,
                              params ? params : "");

  return *auth_params ? 0 : -1;
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

  ret = subsonic_api_request(endpoint, auth_params, response);
  free(auth_params);

  if (ret < 0)
    return -1;

  // Check if the response contains authentication error
  json_object *json = json_tokener_parse(*response);
  if (json)
    {
      json_object *subsonic_response;
      json_object *status;
      const char *status_str;

      if (json_object_object_get_ex(json, "subsonic-response", &subsonic_response) &&
          json_object_object_get_ex(subsonic_response, "status", &status))
        {
          status_str = json_object_get_string(status);
          if (strcmp(status_str, "failed") == 0 && !retry)
            {
              // Authentication failed, regenerate token and retry once
              DPRINTF(E_LOG, L_SUBS, "Authentication failed, regenerate token\n");
              subsonic_auth_token_generate(subsonic_credentials.password);
              retry = true;
              json_object_put(json);
              free(*response);
              goto retry;
            }
        }
      json_object_put(json);
    }

  return ret;
}

// Media library related function implementations
int
subsonic_get_music_folders(void)
{
  char *response = NULL;
  int ret;

  pthread_mutex_lock(&subsonic_mutex);

  ret = subsonic_api_auth_request("getMusicFolders", NULL, &response);
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get music folders\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse music folders response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get musicFolders array
  json_object *subsonic_response;
  json_object *music_folders;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "musicFolders", &music_folders))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get music folders list\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process each music folder
  int folder_count = json_object_array_length(music_folders);
  for (int i = 0; i < folder_count; i++)
    {
      json_object *folder = json_object_array_get_idx(music_folders, i);
      json_object *id, *name;
      
      json_object_object_get_ex(folder, "id", &id);
      json_object_object_get_ex(folder, "name", &name);
      
      DPRINTF(E_DBG, L_SUBS, "Music folder: id=%s, name=%s\n",
              json_object_get_string(id),
              json_object_get_string(name));
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_get_artists(const char *music_folder_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  pthread_mutex_lock(&subsonic_mutex);

  if (music_folder_id)
    params = safe_asprintf("musicFolderId=%s", music_folder_id);

  ret = subsonic_api_auth_request("getArtists", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get artists list\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse artists response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get artists array
  json_object *subsonic_response, *artists, *index;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "artists", &artists) ||
      !json_object_object_get_ex(artists, "index", &index))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get artists list\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process each artist under each index
  int index_count = json_object_array_length(index);
  for (int i = 0; i < index_count; i++)
    {
      json_object *idx = json_object_array_get_idx(index, i);
      json_object *artist_array;
      
      if (!json_object_object_get_ex(idx, "artist", &artist_array))
        continue;

      int artist_count = json_object_array_length(artist_array);
      for (int j = 0; j < artist_count; j++)
        {
          json_object *artist = json_object_array_get_idx(artist_array, j);
          json_object *id, *name;
          
          json_object_object_get_ex(artist, "id", &id);
          json_object_object_get_ex(artist, "name", &name);
          
          DPRINTF(E_DBG, L_SUBS, "Artist: id=%s, name=%s\n",
                  json_object_get_string(id),
                  json_object_get_string(name));
        }
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_get_artist(const char *artist_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!artist_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", artist_id);
  ret = subsonic_api_auth_request("getArtist", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get artist details\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse artist details response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get artist object
  json_object *subsonic_response, *artist, *album_array;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "artist", &artist))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get artist details\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process artist information
  json_object *name;
  json_object_object_get_ex(artist, "name", &name);
  DPRINTF(E_DBG, L_SUBS, "Artist details: name=%s\n", json_object_get_string(name));

  // Process artist's albums
  if (json_object_object_get_ex(artist, "album", &album_array))
    {
      int album_count = json_object_array_length(album_array);
      for (int i = 0; i < album_count; i++)
        {
          json_object *album = json_object_array_get_idx(album_array, i);
          json_object *id, *album_name, *year;
          
          json_object_object_get_ex(album, "id", &id);
          json_object_object_get_ex(album, "name", &album_name);
          json_object_object_get_ex(album, "year", &year);
          
          DPRINTF(E_DBG, L_SUBS, "Album: id=%s, name=%s, year=%d\n",
                  json_object_get_string(id),
                  json_object_get_string(album_name),
                  json_object_get_int(year));
        }
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_get_album(const char *album_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!album_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", album_id);
  ret = subsonic_api_auth_request("getAlbum", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get album details\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse album details response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get album object
  json_object *subsonic_response, *album, *song_array;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "album", &album))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get album details\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process album information
  json_object *name, *artist, *year;
  json_object_object_get_ex(album, "name", &name);
  json_object_object_get_ex(album, "artist", &artist);
  json_object_object_get_ex(album, "year", &year);
  
  DPRINTF(E_DBG, L_SUBS, "Album details: name=%s, artist=%s, year=%d\n",
          json_object_get_string(name),
          json_object_get_string(artist),
          json_object_get_int(year));

  // Process songs in the album
  if (json_object_object_get_ex(album, "song", &song_array))
    {
      int song_count = json_object_array_length(song_array);
      for (int i = 0; i < song_count; i++)
        {
          json_object *song = json_object_array_get_idx(song_array, i);
          json_object *id, *title, *track;
          
          json_object_object_get_ex(song, "id", &id);
          json_object_object_get_ex(song, "title", &title);
          json_object_object_get_ex(song, "track", &track);
          
          DPRINTF(E_DBG, L_SUBS, "Song: id=%s, title=%s, track=%d\n",
                  json_object_get_string(id),
                  json_object_get_string(title),
                  json_object_get_int(track));
        }
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_get_song(const char *song_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!song_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", song_id);
  ret = subsonic_api_auth_request("getSong", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get song details\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse song details response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get song object
  json_object *subsonic_response, *song;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "song", &song))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get song details\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process song information
  json_object *id, *title, *album, *artist, *track, *year, *genre, *size, *contentType, *duration;
  
  json_object_object_get_ex(song, "id", &id);
  json_object_object_get_ex(song, "title", &title);
  json_object_object_get_ex(song, "album", &album);
  json_object_object_get_ex(song, "artist", &artist);
  json_object_object_get_ex(song, "track", &track);
  json_object_object_get_ex(song, "year", &year);
  json_object_object_get_ex(song, "genre", &genre);
  json_object_object_get_ex(song, "size", &size);
  json_object_object_get_ex(song, "contentType", &contentType);
  json_object_object_get_ex(song, "duration", &duration);

  DPRINTF(E_DBG, L_SUBS, "Song details: id=%s, title=%s, album=%s, artist=%s, "
          "track=%d, year=%d, genre=%s, size=%ld, type=%s, duration=%d\n",
          json_object_get_string(id),
          json_object_get_string(title),
          json_object_get_string(album),
          json_object_get_string(artist),
          json_object_get_int(track),
          json_object_get_int(year),
          json_object_get_string(genre),
          json_object_get_int64(size),
          json_object_get_string(contentType),
          json_object_get_int(duration));

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

// Playlist related function implementations
int
subsonic_get_playlists(void)
{
  char *response = NULL;
  int ret;

  pthread_mutex_lock(&subsonic_mutex);

  ret = subsonic_api_auth_request("getPlaylists", NULL, &response);
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get playlists\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse playlists response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get playlists array
  json_object *subsonic_response, *playlists, *playlist_array;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "playlists", &playlists) ||
      !json_object_object_get_ex(playlists, "playlist", &playlist_array))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get playlists\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process each playlist
  int playlist_count = json_object_array_length(playlist_array);
  for (int i = 0; i < playlist_count; i++)
    {
      json_object *playlist = json_object_array_get_idx(playlist_array, i);
      json_object *id, *name, *song_count, *duration;
      
      json_object_object_get_ex(playlist, "id", &id);
      json_object_object_get_ex(playlist, "name", &name);
      json_object_object_get_ex(playlist, "songCount", &song_count);
      json_object_object_get_ex(playlist, "duration", &duration);
      
      DPRINTF(E_DBG, L_SUBS, "Playlist: id=%s, name=%s, songs=%d, duration=%d\n",
              json_object_get_string(id),
              json_object_get_string(name),
              json_object_get_int(song_count),
              json_object_get_int(duration));
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_get_playlist(const char *playlist_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!playlist_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", playlist_id);
  ret = subsonic_api_auth_request("getPlaylist", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get playlist details\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse playlist details response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get playlist object
  json_object *subsonic_response, *playlist, *entry_array;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "playlist", &playlist))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get playlist details\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process playlist information
  json_object *name, *comment, *owner, *public;
  json_object_object_get_ex(playlist, "name", &name);
  json_object_object_get_ex(playlist, "comment", &comment);
  json_object_object_get_ex(playlist, "owner", &owner);
  json_object_object_get_ex(playlist, "public", &public);

  DPRINTF(E_DBG, L_SUBS, "Playlist details: name=%s, comment=%s, owner=%s, public=%d\n",
          json_object_get_string(name),
          comment ? json_object_get_string(comment) : "(无)",
          json_object_get_string(owner),
          json_object_get_boolean(public));

  // Process songs in the playlist
  if (json_object_object_get_ex(playlist, "entry", &entry_array))
    {
      int entry_count = json_object_array_length(entry_array);
      for (int i = 0; i < entry_count; i++)
        {
          json_object *entry = json_object_array_get_idx(entry_array, i);
          json_object *id, *title, *artist, *album;
          
          json_object_object_get_ex(entry, "id", &id);
          json_object_object_get_ex(entry, "title", &title);
          json_object_object_get_ex(entry, "artist", &artist);
          json_object_object_get_ex(entry, "album", &album);
          
          DPRINTF(E_DBG, L_SUBS, "Playlist song: id=%s, title=%s, artist=%s, album=%s\n",
                  json_object_get_string(id),
                  json_object_get_string(title),
                  json_object_get_string(artist),
                  json_object_get_string(album));
        }
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_create_playlist(const char *name, const char *song_ids)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!name)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  if (song_ids)
    params = safe_asprintf("name=%s&songId=%s", name, song_ids);
  else
    params = safe_asprintf("name=%s", name);

  ret = subsonic_api_auth_request("createPlaylist", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to create playlist\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_update_playlist(const char *playlist_id, const char *name, const char *song_ids)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!playlist_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  if (name && song_ids)
    params = safe_asprintf("playlistId=%s&name=%s&songId=%s", playlist_id, name, song_ids);
  else if (name)
    params = safe_asprintf("playlistId=%s&name=%s", playlist_id, name);
  else if (song_ids)
    params = safe_asprintf("playlistId=%s&songId=%s", playlist_id, song_ids);
  else
    params = safe_asprintf("playlistId=%s", playlist_id);

  ret = subsonic_api_auth_request("updatePlaylist", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to update playlist\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_delete_playlist(const char *playlist_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!playlist_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", playlist_id);
  ret = subsonic_api_auth_request("deletePlaylist", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to delete playlist\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

// Search function implementations
int
subsonic_search(const char *query, const char *artist_id, const char *album_id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!query && !artist_id && !album_id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  // Build search parameters
  if (query)
    params = safe_asprintf("query=%s", query);
  else if (artist_id)
    params = safe_asprintf("artistId=%s", artist_id);
  else
    params = safe_asprintf("albumId=%s", album_id);

  ret = subsonic_api_auth_request("search3", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Search failed\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Parse response
  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to parse search response\n");
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Get searchResult object
  json_object *subsonic_response, *searchResult;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "searchResult3", &searchResult))
    {
      DPRINTF(E_LOG, L_SUBS, "Cannot get search results\n");
      json_object_put(json);
      free(response);
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Process artist search results
  json_object *artist_array;
  if (json_object_object_get_ex(searchResult, "artist", &artist_array))
    {
      int artist_count = json_object_array_length(artist_array);
      for (int i = 0; i < artist_count; i++)
        {
          json_object *artist = json_object_array_get_idx(artist_array, i);
          json_object *id, *name;
          
          json_object_object_get_ex(artist, "id", &id);
          json_object_object_get_ex(artist, "name", &name);
          
          DPRINTF(E_DBG, L_SUBS, "Found artist: id=%s, name=%s\n",
                  json_object_get_string(id),
                  json_object_get_string(name));
        }
    }

  // Process album search results
  json_object *album_array;
  if (json_object_object_get_ex(searchResult, "album", &album_array))
    {
      int album_count = json_object_array_length(album_array);
      for (int i = 0; i < album_count; i++)
        {
          json_object *album = json_object_array_get_idx(album_array, i);
          json_object *id, *name, *artist;
          
          json_object_object_get_ex(album, "id", &id);
          json_object_object_get_ex(album, "name", &name);
          json_object_object_get_ex(album, "artist", &artist);
          
          DPRINTF(E_DBG, L_SUBS, "Found album: id=%s, name=%s, artist=%s\n",
                  json_object_get_string(id),
                  json_object_get_string(name),
                  json_object_get_string(artist));
        }
    }

  // Process song search results
  json_object *song_array;
  if (json_object_object_get_ex(searchResult, "song", &song_array))
    {
      int song_count = json_object_array_length(song_array);
      for (int i = 0; i < song_count; i++)
        {
          json_object *song = json_object_array_get_idx(song_array, i);
          json_object *id, *title, *artist, *album;
          
          json_object_object_get_ex(song, "id", &id);
          json_object_object_get_ex(song, "title", &title);
          json_object_object_get_ex(song, "artist", &artist);
          json_object_object_get_ex(song, "album", &album);
          
          DPRINTF(E_DBG, L_SUBS, "Found song: id=%s, title=%s, artist=%s, album=%s\n",
                  json_object_get_string(id),
                  json_object_get_string(title),
                  json_object_get_string(artist),
                  json_object_get_string(album));
        }
    }

  json_object_put(json);
  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

// Rating function implementations
int
subsonic_star(const char *id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", id);
  ret = subsonic_api_auth_request("star", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to star\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_unstar(const char *id)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s", id);
  ret = subsonic_api_auth_request("unstar", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to unstar\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

int
subsonic_set_rating(const char *id, int rating)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!id || rating < 0 || rating > 5)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  params = safe_asprintf("id=%s&rating=%d", id, rating);
  ret = subsonic_api_auth_request("setRating", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to set rating\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

// Cover image function implementations
int
subsonic_get_cover_art(const char *id, int size)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  if (!id)
    return -1;

  pthread_mutex_lock(&subsonic_mutex);

  if (size > 0)
    params = safe_asprintf("id=%s&size=%d", id, size);
  else
    params = safe_asprintf("id=%s", id);

  ret = subsonic_api_auth_request("getCoverArt", params, &response);
  free(params);
  
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_SUBS, "Failed to get cover image\n");
      pthread_mutex_unlock(&subsonic_mutex);
      return -1;
    }

  // Note: This returns binary image data, which needs special handling
  // For now, it's just an example, and actual usage needs to handle image data based on specific requirements
  DPRINTF(E_DBG, L_SUBS, "Successfully got cover image data\n");

  free(response);
  pthread_mutex_unlock(&subsonic_mutex);
  return 0;
}

static int
setup(struct input_source *source)
{
  struct transcode_decode_setup_args decode_args = { .profile = XCODE_PCM_NATIVE, .is_http = true, .len_ms = source->len_ms };
  struct transcode_encode_setup_args encode_args = { .profile = XCODE_PCM_NATIVE, };
  struct transcode_ctx *ctx;
  struct media_file_info *mfi;
  char *url;
  char *params = NULL;
  char *auth_params;
  int ret;

  mfi = db_file_fetch_byid(source->item_id);
  // Get stream URL and extract first id before comma
  params = safe_asprintf("id=%s", mfi->url);
  ret = subsonic_auth_params_create(&auth_params, params);
  if (ret < 0)
    return -1;
  url = safe_asprintf("%s/rest/stream.view?%s&f=json&v=%s&c=owntone", 
        subsonic_status.server_url,
        auth_params ? auth_params : "",
        subsonic_status.api_version);
  free(params);

  decode_args.path = url;
  ctx = transcode_setup(decode_args, encode_args);
  if (!ctx)
    {
      free(url);
      return -1;
    }

  CHECK_NULL(L_SUBS, source->evbuf = evbuffer_new());

  source->quality.sample_rate = transcode_encode_query(ctx->encode_ctx, "sample_rate");
  source->quality.bits_per_sample = transcode_encode_query(ctx->encode_ctx, "bits_per_sample");
  source->quality.channels = transcode_encode_query(ctx->encode_ctx, "channels");

  source->input_ctx = ctx;
  free(url);

  return 0;
}

static int
play(struct input_source *source)
{
  struct transcode_ctx *ctx = source->input_ctx;
  int ret;

  ret = transcode(source->evbuf, NULL, ctx, 1);
  if (ret == 0)
    {
      input_write(source->evbuf, &source->quality, INPUT_FLAG_EOF);
      stop(source);
      return -1;
    }
  else if (ret < 0)
    {
      input_write(NULL, NULL, INPUT_FLAG_ERROR);
      stop(source);
      return -1;
    }

  input_write(source->evbuf, &source->quality, 0);

  return 0;
}

static int
stop(struct input_source *source)
{
  struct transcode_ctx *ctx = source->input_ctx;

  transcode_cleanup(&ctx);

  if (source->evbuf)
    evbuffer_free(source->evbuf);

  source->input_ctx = NULL;
  source->evbuf = NULL;

  return 0;
}

static int
seek(struct input_source *source, int seek_ms)
{
  return transcode_seek(source->input_ctx, seek_ms);
}

static int
metadata_get(struct input_metadata *metadata, struct input_source *source)
{
  char *response = NULL;
  char *params = NULL;
  int ret;

  params = safe_asprintf("id=%s", source->path);
  ret = subsonic_api_auth_request("getSong", params, &response);
  free(params);
  
  if (ret < 0)
    return -1;

  json_object *json = json_tokener_parse(response);
  if (!json)
    {
      free(response);
      return -1;
    }

  json_object *subsonic_response, *song;
  json_object_object_get_ex(json, "subsonic-response", &subsonic_response);
  if (!subsonic_response || !json_object_object_get_ex(subsonic_response, "song", &song))
    {
      json_object_put(json);
      free(response);
      return -1;
    }

  json_object *title, *artist, *album;
  
  if (json_object_object_get_ex(song, "title", &title))
    metadata->title = strdup(json_object_get_string(title));
  if (json_object_object_get_ex(song, "artist", &artist))
    metadata->artist = strdup(json_object_get_string(artist));
  if (json_object_object_get_ex(song, "album", &album))
    metadata->album = strdup(json_object_get_string(album));

  json_object_put(json);
  free(response);

  return 0;
}

struct input_definition input_subsonic =
{
  .name = "subsonic",
  .type = INPUT_TYPE_SUBSONIC,
  .disabled = 0,
  .setup = setup,
  .play = play,
  .stop = stop,
  .seek = seek,
  .metadata_get = metadata_get,
  .init = subsonic_init,
  .deinit = subsonic_deinit,
};