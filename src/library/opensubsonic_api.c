/*
 * OpenSubsonic API client implementation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h> // For snprintf
#include <time.h> // For time_t used in http_client if needed later

#include <event2/buffer.h> // For evbuffer (used by http_client)
#include <json.h>

#include "opensubsonic_api.h"
#include "conffile.h" // For cfg_getstr, cfg_getbool from owntone's config
#include "logger.h"   // For DPRINTF
#include "http.h"     // For http_client_request and related structures
#include "misc.h"     // For safe_strdup, safe_asprintf, md5_hex
#include "misc_json.h" // For jparse_* helpers (if any are suitable)


static struct opensubsonic_config current_config;
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static struct http_client_session os_http_session; // Persistent session for OpenSubsonic API calls

#define API_ENDPOINT_FORMAT "%s/rest/%s.view?u=%s&%s&c=%s&v=%s&f=json%s"
// server_url, method_name, username, auth_params, client_name, api_version, specific_params

// Helper to free string if not NULL and set to NULL
static void safe_free_string(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

void opensubsonic_api_init(void) {
    DPRINTF(E_DBG, L_HTTP, "Initializing OpenSubsonic API client\n");
    memset(&current_config, 0, sizeof(struct opensubsonic_config));
    opensubsonic_api_load_config();

    // Initialize HTTP client session
    CHECK_ERR(L_HTTP, pthread_mutex_lock(&os_http_session.lock));
    http_client_session_init(&os_http_session);
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&os_http_session.lock));

    if (current_config.enabled) {
        DPRINTF(E_INFO, L_HTTP, "OpenSubsonic support enabled. Server URL: %s\n", current_config.server_url ? current_config.server_url : "Not set");
    } else {
        DPRINTF(E_INFO, L_HTTP, "OpenSubsonic support disabled in configuration.\n");
    }
}

void opensubsonic_api_deinit(void) {
    DPRINTF(E_DBG, L_HTTP, "Deinitializing OpenSubsonic API client\n");
    CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));
    safe_free_string(&current_config.server_url);
    safe_free_string(&current_config.username);
    safe_free_string(&current_config.password);
    safe_free_string(&current_config.token);
    safe_free_string(&current_config.salt);
    safe_free_string(&current_config.client_name);
    safe_free_string(&current_config.api_version);
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));

    // Deinitialize HTTP client session
    CHECK_ERR(L_HTTP, pthread_mutex_lock(&os_http_session.lock));
    http_client_session_deinit(&os_http_session);
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&os_http_session.lock));
}

void opensubsonic_api_load_config(void) {
    cfg_t *os_cfg;

    CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));

    // Free existing config strings if any
    safe_free_string(&current_config.server_url);
    safe_free_string(&current_config.username);
    safe_free_string(&current_config.password);
    safe_free_string(&current_config.token);
    safe_free_string(&current_config.salt);
    safe_free_string(&current_config.client_name);
    safe_free_string(&current_config.api_version);

    os_cfg = cfg_getsec(cfg, "opensubsonic");
    if (os_cfg) {
        current_config.enabled = cfg_getbool(os_cfg, "enabled");
        current_config.server_url = safe_strdup(cfg_getstr(os_cfg, "server_url"));
        current_config.username = safe_strdup(cfg_getstr(os_cfg, "username"));
        current_config.password = safe_strdup(cfg_getstr(os_cfg, "password"));
        current_config.token = safe_strdup(cfg_getstr(os_cfg, "token"));
        current_config.salt = safe_strdup(cfg_getstr(os_cfg, "salt"));
        current_config.legacy_auth = cfg_getbool(os_cfg, "legacy_auth");
        current_config.client_name = safe_strdup(cfg_getstr(os_cfg, "client_name"));
        current_config.api_version = safe_strdup(cfg_getstr(os_cfg, "api_version"));

        // Basic validation
        if (current_config.enabled) {
            if (!current_config.server_url || strlen(current_config.server_url) == 0) {
                DPRINTF(E_WARN, L_HTTP, "OpenSubsonic enabled but server_url is not set.\n");
                current_config.enabled = false;
            }
            if (!current_config.username || strlen(current_config.username) == 0) {
                DPRINTF(E_WARN, L_HTTP, "OpenSubsonic enabled but username is not set.\n");
                current_config.enabled = false;
            }
            if ((!current_config.password || strlen(current_config.password) == 0) &&
                (!current_config.token || strlen(current_config.token) == 0)) {
                DPRINTF(E_WARN, L_HTTP, "OpenSubsonic enabled but neither password nor token is set.\n");
                current_config.enabled = false;
            }
        }

    } else {
        DPRINTF(E_WARN, L_HTTP, "OpenSubsonic configuration section not found.\n");
        current_config.enabled = false;
    }

    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));
}

bool opensubsonic_api_is_enabled(void) {
    bool enabled;
    CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));
    enabled = current_config.enabled;
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));
    return enabled;
}

const struct opensubsonic_config *opensubsonic_api_get_config(void) {
    return &current_config;
}

// Internal helper to build authentication parameters
static char* build_auth_params(
    const char *username,
    const char *password_param,
    const char *token_param,
    const char *salt_param,
    bool legacy_auth_param) {

    char *auth_params = NULL;

    if (token_param && strlen(token_param) > 0) {
        // If token is provided, it's typically an API key, send as 'p' (password parameter)
        // Subsonic standard for password is 'p', for token is 't' (hashed with salt 's').
        // If a 'token' field is used in config, it's likely an API key.
        auth_params = safe_asprintf("p=%s", http_urlencode(token_param));
    } else if (password_param && strlen(password_param) > 0) {
        if (!legacy_auth_param && salt_param && strlen(salt_param) > 0) {
            // Modern Subsonic auth: t=MD5(password + salt)&s=salt
            char md5_input[256];
            char p_hex[33];
            snprintf(md5_input, sizeof(md5_input), "%s%s", password_param, salt_param);
            md5_hex(md5_input, strlen(md5_input), p_hex);
            auth_params = safe_asprintf("t=%s&s=%s", p_hex, salt_param);
        } else {
            // Legacy or no-salt auth: send password directly, URL-encoded
            auth_params = safe_asprintf("p=%s", http_urlencode(password_param));
        }
    }

    if (!auth_params) {
        DPRINTF(E_LOG, L_HTTP, "OpenSubsonic: Failed to build authentication parameters (no token/password provided).\n");
        return safe_strdup(""); // Return empty string to avoid NULL issues
    }
    return auth_params;
}

// Internal helper for making requests
// Now accepts override_config to use temporary settings for operations like testing.
static json_object* make_opensubsonic_request(const char *method_name,
                                              const struct opensubsonic_config *override_config,
                                              const char *specific_params,
                                              const char **errmsg) {
    struct http_client_ctx *ctx;
    char *url = NULL;
    char *auth_p = NULL;
    json_object *json_response = NULL;
    char *response_body_str = NULL;
    int ret;

    char *use_server_url = NULL, *use_username = NULL, *use_password = NULL;
    char *use_token = NULL, *use_salt = NULL, *use_client_name = NULL, *use_api_version = NULL;
    bool use_legacy_auth;
    bool free_local_copies = false;

    if (override_config) {
        // Use provided override_config directly (caller manages its lifetime and ensures it's valid)
        if (!override_config->server_url || strlen(override_config->server_url) == 0 ||
            !override_config->username || strlen(override_config->username) == 0 ||
            ((!override_config->password || strlen(override_config->password) == 0) &&
             (!override_config->token || strlen(override_config->token) == 0)) || // Must have one auth method
            !override_config->client_name || strlen(override_config->client_name) == 0 ||
            !override_config->api_version || strlen(override_config->api_version) == 0) {
            if (errmsg) *errmsg = "Override OpenSubsonic configuration is incomplete.";
            return NULL;
        }
        use_server_url = override_config->server_url;
        use_username = override_config->username;
        use_password = override_config->password;
        use_token = override_config->token;
        use_salt = override_config->salt;
        use_legacy_auth = override_config->legacy_auth;
        use_client_name = override_config->client_name;
        use_api_version = override_config->api_version;
    } else {
        // Use global current_config, requires locking and strdup
        if (!opensubsonic_api_is_enabled()) {
            if (errmsg) *errmsg = "OpenSubsonic support is not enabled.";
            return NULL;
        }
        free_local_copies = true; // Mark that we need to free these copies later
        CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));
        use_server_url = safe_strdup(current_config.server_url);
        use_username = safe_strdup(current_config.username);
        use_password = safe_strdup(current_config.password);
        use_token = safe_strdup(current_config.token);
        use_salt = safe_strdup(current_config.salt);
        use_legacy_auth = current_config.legacy_auth;
        use_client_name = safe_strdup(current_config.client_name);
        use_api_version = safe_strdup(current_config.api_version);
        CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));

        // Check if any essential strdups failed or if config was empty
        if (!use_server_url || !use_username || !use_client_name || !use_api_version ||
            ((!use_password || use_password[0] == '\0') && (!use_token || use_token[0] == '\0'))) {
            if (errmsg) *errmsg = "OpenSubsonic configuration is incomplete or memory allocation failed.";
            goto cleanup_local_copies_only; // Jump to free allocated strings
        }
    }

    auth_p = build_auth_params(use_username, use_password, use_token, use_salt, use_legacy_auth);

    url = safe_asprintf(API_ENDPOINT_FORMAT,
                        use_server_url, method_name, use_username, auth_p,
                        use_client_name, use_api_version, specific_params ? specific_params : "");
    if (!url) {
        if (errmsg) *errmsg = "Failed to construct API URL.";
        goto cleanup_auth_and_local_copies;
    }

    DPRINTF(E_DBG, L_HTTP, "OpenSubsonic request URL: %s\n", url);

    ctx = calloc(1, sizeof(struct http_client_ctx));
    if (!ctx) {
        if (errmsg) *errmsg = "Failed to allocate HTTP context.";
        goto cleanup_url_auth_local;
    }
    ctx->url = url;
    ctx->input_body = evbuffer_new();
    if (!ctx->input_body) {
        if (errmsg) *errmsg = "Failed to create evbuffer.";
        free(ctx); // Free ctx as it's not passed to http_client_request yet
        goto cleanup_url_auth_local;
    }

    CHECK_ERR(L_HTTP, pthread_mutex_lock(&os_http_session.lock));
    ret = http_client_request(ctx, &os_http_session);
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&os_http_session.lock));

    if (ret < 0) { // http_client_request frees ctx on error
        if (errmsg) *errmsg = "HTTP request to OpenSubsonic server failed.";
        goto cleanup_url_auth_local;
    }

    evbuffer_add(ctx->input_body, "", 1); // 0-terminate for safety
    response_body_str = (char *)evbuffer_pullup(ctx->input_body, -1);

    if (!response_body_str || strlen(response_body_str) == 0) {
        if (errmsg) *errmsg = "OpenSubsonic server returned an empty response.";
        // ctx is freed by http_client_request on success path too
        goto cleanup_url_auth_local;
    }

    DPRINTF(E_SPAM, L_HTTP, "OpenSubsonic response for %s: %s\n", method_name, response_body_str);
    json_response = json_tokener_parse(response_body_str);

    if (!json_response) {
        if (errmsg) *errmsg = "Failed to parse JSON response from OpenSubsonic server.";
    } else {
        json_object *subsonic_response_obj;
        if (json_object_object_get_ex(json_response, "subsonic-response", &subsonic_response_obj)) {
            json_object *status_obj;
            if (json_object_object_get_ex(subsonic_response_obj, "status", &status_obj)) {
                const char *status_str = json_object_get_string(status_obj);
                if (status_str && strcmp(status_str, "failed") == 0) {
                    json_object *error_obj;
                    if (json_object_object_get_ex(subsonic_response_obj, "error", &error_obj)) {
                        const char *err_msg_str = jparse_str_from_obj(error_obj, "message");
                        int err_code = jparse_int_from_obj(error_obj, "code");
                        char combined_err[256];
                        snprintf(combined_err, sizeof(combined_err), "OpenSubsonic API error %d: %s", err_code, err_msg_str ? err_msg_str : "Unknown error");
                        if (errmsg) *errmsg = strdup(combined_err); // strdup as json_response will be freed
                        DPRINTF(E_LOG, L_HTTP, "%s\n", combined_err);
                        jparse_free(json_response);
                        json_response = NULL;
                    } else {
                         if (errmsg) *errmsg = "OpenSubsonic request failed with unspecified error.";
                        jparse_free(json_response);
                        json_response = NULL;
                    }
                }
                // If status is "ok", json_response is good.
            }
        } else { // Missing "subsonic-response"
            if (errmsg) *errmsg = "Invalid OpenSubsonic response format: missing 'subsonic-response'.";
            jparse_free(json_response);
            json_response = NULL;
        }
    }

// Cleanup labels ensure freeing of dynamically allocated memory in this function
cleanup_url_auth_local:
    free(url);
cleanup_auth_and_local_copies:
    free(auth_p);
cleanup_local_copies_only:
    if (free_local_copies) {
        safe_free_string(&use_server_url);
        safe_free_string(&use_username);
        safe_free_string(&use_password);
        safe_free_string(&use_token);
        safe_free_string(&use_salt);
        safe_free_string(&use_client_name);
        safe_free_string(&use_api_version);
    }
    return json_response;
}

// Ping using the currently stored configuration
int opensubsonic_api_ping(const char **errmsg) {
    json_object *jresponse;
    int result = -1;

    DPRINTF(E_DBG, L_HTTP, "Pinging OpenSubsonic server (using stored config)...\n");
    jresponse = make_opensubsonic_request("ping", NULL, NULL, errmsg); // override_config is NULL

    if (jresponse) {
        // Error checking is done inside make_opensubsonic_request, which will set errmsg
        // and return NULL on "subsonic-response"->"status"=="failed".
        // So, if jresponse is not NULL here, it means the status was "ok".
        DPRINTF(E_INFO, L_HTTP, "OpenSubsonic ping successful.\n");
        result = 0;
        jparse_free(jresponse);
    }
    // If jresponse is NULL, errmsg should already be set by make_opensubsonic_request
    return result;
}

// Ping using provided details (for testing unsaved configuration)
int opensubsonic_api_ping_with_details(const char *server_url, const char *username,
                                       const char *password, const char *token, const char *salt,
                                       bool legacy_auth, const char *client_name_override, const char *api_version_override,
                                       const char **errmsg) {
    json_object *jresponse;
    int result = -1;
    struct opensubsonic_config test_config = {0}; // Must initialize all fields

    // Use provided details for test_config
    test_config.server_url = (char*)server_url;
    test_config.username = (char*)username;
    test_config.password = (char*)password;
    test_config.token = (char*)token;
    test_config.salt = (char*)salt;
    test_config.legacy_auth = legacy_auth;

    // Fallback to stored config for client_name and api_version if not provided for test
    // This requires reading current_config, so lock is needed if accessing it.
    // A safer way is if httpd_jsonapi.c always provides these from its context.
    CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));
    test_config.client_name = (client_name_override && client_name_override[0] != '\0') ? (char*)client_name_override : current_config.client_name;
    test_config.api_version = (api_version_override && api_version_override[0] != '\0') ? (char*)api_version_override : current_config.api_version;
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));

    test_config.enabled = true; // Assume enabled for a test call

    DPRINTF(E_DBG, L_HTTP, "Pinging OpenSubsonic server with provided details...\n");
    jresponse = make_opensubsonic_request("ping", &test_config, NULL, errmsg);

    if (jresponse) {
        DPRINTF(E_INFO, L_HTTP, "OpenSubsonic ping with details successful.\n");
        result = 0;
        jparse_free(jresponse);
    }
    return result;
}


json_object* opensubsonic_api_search3(const char *query,
                                      int artist_count, int artist_offset,
                                      int album_count, int album_offset,
                                      int song_count, int song_offset,
                                      const char **errmsg) {
    char *specific_params = NULL;
    json_object *jresponse = NULL;
    char *encoded_query = NULL;

    if (!query || strlen(query) == 0) {
        if (errmsg) *errmsg = "Search query cannot be empty.";
        return NULL;
    }

    encoded_query = http_urlencode(query); // Ensure query string is URL safe
    if (!encoded_query) {
        if (errmsg) *errmsg = "Failed to URL encode search query.";
        return NULL;
    }

    specific_params = safe_asprintf("&query=%s&artistCount=%d&artistOffset=%d&albumCount=%d&albumOffset=%d&songCount=%d&songOffset=%d",
                                    encoded_query,
                                    artist_count > 0 ? artist_count : 0,
                                    artist_offset > 0 ? artist_offset : 0,
                                    album_count > 0 ? album_count : 0,
                                    album_offset > 0 ? album_offset : 0,
                                    song_count > 0 ? song_count : 0,
                                    song_offset > 0 ? song_offset : 0);

    free(encoded_query);

    if (!specific_params) {
        if (errmsg) *errmsg = "Failed to allocate memory for search parameters.";
        return NULL;
    }

    jresponse = make_opensubsonic_request("search3", NULL, specific_params, errmsg); // Use stored config
    free(specific_params);

    return jresponse;
}

char* opensubsonic_api_get_cover_art_url(const char *id, int size, const char **errmsg) {
    char *auth_p = NULL;
    char *cover_art_url = NULL;
    char size_param[32] = "";

    char *l_server_url, *l_username, *l_client_name, *l_api_version;
    char *l_password, *l_token, *l_salt;
    bool l_legacy_auth;

    if (!opensubsonic_api_is_enabled() || !id) {
        if (errmsg) *errmsg = "OpenSubsonic not enabled or cover art ID is null.";
        return NULL;
    }

    CHECK_ERR(L_HTTP, pthread_mutex_lock(&config_lock));
    l_server_url = safe_strdup(current_config.server_url);
    l_username = safe_strdup(current_config.username);
    l_password = safe_strdup(current_config.password);
    l_token = safe_strdup(current_config.token);
    l_salt = safe_strdup(current_config.salt);
    l_legacy_auth = current_config.legacy_auth;
    l_client_name = safe_strdup(current_config.client_name);
    l_api_version = safe_strdup(current_config.api_version);
    CHECK_ERR(L_HTTP, pthread_mutex_unlock(&config_lock));

    if (!l_server_url || !l_username || !l_client_name || !l_api_version ||
        ((!l_password || l_password[0] == '\0') && (!l_token || l_token[0] == '\0'))) {
        if (errmsg) *errmsg = "OpenSubsonic configuration is incomplete for cover art URL.";
        goto cleanup_local_config_cover;
    }

    auth_p = build_auth_params(l_username, l_password, l_token, l_salt, l_legacy_auth);

    if (size > 0) {
        snprintf(size_param, sizeof(size_param), "&size=%d", size);
    }

    cover_art_url = safe_asprintf("%s/rest/getCoverArt.view?id=%s&u=%s&%s&c=%s&v=%s%s",
                                  l_server_url, id, l_username, auth_p,
                                  l_client_name, l_api_version, size_param);
    free(auth_p); // auth_p is always allocated by build_auth_params

cleanup_local_config_cover:
    safe_free_string(&l_server_url);
    safe_free_string(&l_username);
    safe_free_string(&l_password);
    safe_free_string(&l_token);
    safe_free_string(&l_salt);
    safe_free_string(&l_client_name);
    safe_free_string(&l_api_version);

    if (!cover_art_url && errmsg && !*errmsg) { // If safe_asprintf failed
        *errmsg = "Failed to construct cover art URL.";
    }
    return cover_art_url;
}


json_object* opensubsonic_api_get_playlists(int offset, int count, const char **errmsg) {
    char *specific_params = NULL;
    if (offset >= 0 && count > 0) {
        specific_params = safe_asprintf("&offset=%d&size=%d", offset, count);
    }
    json_object* response = make_opensubsonic_request("getPlaylists", NULL, specific_params, errmsg);
    free(specific_params);
    return response;
}

json_object* opensubsonic_api_get_playlist(const char *playlist_id, const char **errmsg) {
    if (!playlist_id) {
        if (errmsg) *errmsg = "Playlist ID is required.";
        return NULL;
    }
    char *specific_params = safe_asprintf("&id=%s", playlist_id);
    if (!specific_params) {
         if (errmsg) *errmsg = "Failed to allocate memory for playlist ID parameter.";
        return NULL;
    }
    json_object* response = make_opensubsonic_request("getPlaylist", NULL, specific_params, errmsg);
    free(specific_params);
    return response;
}

json_object* opensubsonic_api_get_album(const char *album_id, const char **errmsg) {
    if (!album_id) {
        if (errmsg) *errmsg = "Album ID is required.";
        return NULL;
    }
    char *specific_params = safe_asprintf("&id=%s", album_id);
    if (!specific_params) {
         if (errmsg) *errmsg = "Failed to allocate memory for album ID parameter.";
        return NULL;
    }
    // Subsonic uses "getAlbum" for this
    json_object* response = make_opensubsonic_request("getAlbum", NULL, specific_params, errmsg);
    free(specific_params);
    return response;
}

json_object* opensubsonic_api_get_artist(const char *artist_id, const char **errmsg) {
    if (!artist_id) {
        if (errmsg) *errmsg = "Artist ID is required.";
        return NULL;
    }
    char *specific_params = safe_asprintf("&id=%s", artist_id);
    if (!specific_params) {
         if (errmsg) *errmsg = "Failed to allocate memory for artist ID parameter.";
        return NULL;
    }
    // Subsonic uses "getArtist" for this
    json_object* response = make_opensubsonic_request("getArtist", NULL, specific_params, errmsg);
    free(specific_params);
    return response;
}

json_object* opensubsonic_api_get_song(const char *song_id, const char **errmsg) {
    if (!song_id) {
        if (errmsg) *errmsg = "Song ID is required.";
        return NULL;
    }
    char *specific_params = safe_asprintf("&id=%s", song_id);
    if (!specific_params) {
         if (errmsg) *errmsg = "Failed to allocate memory for song ID parameter.";
        return NULL;
    }
    json_object* response = make_opensubsonic_request("getSong", NULL, specific_params, errmsg);
    free(specific_params);
    return response;
}


// Implementation for opensubsonic_free_track and other free functions
void opensubsonic_free_track(struct opensubsonic_track *track) {
    if (!track) return;
    safe_free_string(&track->id);
    safe_free_string(&track->title);
    safe_free_string(&track->artist);
    safe_free_string(&track->album);
    safe_free_string(&track->album_id);
    safe_free_string(&track->artist_id);
    safe_free_string(&track->cover_art_id);
    safe_free_string(&track->path);
    safe_free_string(&track->content_type);
    safe_free_string(&track->genre);
    safe_free_string(&track->parent_id);
}

void opensubsonic_free_album(struct opensubsonic_album *album) {
    if (!album) return;
    safe_free_string(&album->id);
    safe_free_string(&album->name);
    safe_free_string(&album->artist);
    safe_free_string(&album->artist_id);
    safe_free_string(&album->cover_art_id);
    safe_free_string(&album->genre);
}

void opensubsonic_free_artist(struct opensubsonic_artist *artist) {
    if (!artist) return;
    safe_free_string(&artist->id);
    safe_free_string(&artist->name);
    safe_free_string(&artist->cover_art_id);
}

void opensubsonic_free_playlist(struct opensubsonic_playlist *playlist) {
    if (!playlist) return;
    safe_free_string(&playlist->id);
    safe_free_string(&playlist->name);
    safe_free_string(&playlist->owner);
    safe_free_string(&playlist->comment);
    safe_free_string(&playlist->cover_art_id);
}


// Convert OpenSubsonic track to db_queue_item for playback
int opensubsonic_track_to_queue_item(const struct opensubsonic_track *os_track, struct db_queue_item *item) {
    if (!os_track || !item) return -1;

    char virtual_path[PATH_MAX];
    memset(item, 0, sizeof(struct db_queue_item));

    item->file_id = DB_MEDIA_FILE_NON_PERSISTENT_ID;
    item->title = safe_strdup(os_track->title);
    item->artist = safe_strdup(os_track->artist);
    item->album = safe_strdup(os_track->album);
    item->album_artist = safe_strdup(os_track->artist);

    item->disc = os_track->disc_number;
    item->song_length = os_track->duration * 1000;
    item->track = os_track->track_number;
    item->year = os_track->year;
    item->genre = safe_strdup(os_track->genre);

    item->data_kind = DATA_KIND_OPENSUBSONIC;
    item->media_kind = MEDIA_KIND_MUSIC;

    item->path = safe_strdup(os_track->id);

    snprintf(virtual_path, PATH_MAX, "/opensubsonic:/%s", os_track->id);
    item->virtual_path = strdup(virtual_path);
    item->bitrate = os_track->bit_rate;
    return 0;
}

/*
int opensubsonic_parse_track(json_object *jobj, struct opensubsonic_track *track) {
    if (!jobj || !track) return -1;
    memset(track, 0, sizeof(struct opensubsonic_track));

    track->id = safe_strdup(jparse_str_from_obj(jobj, "id"));
    track->title = safe_strdup(jparse_str_from_obj(jobj, "title"));
    track->artist = safe_strdup(jparse_str_from_obj(jobj, "artist"));
    track->album = safe_strdup(jparse_str_from_obj(jobj, "album"));
    track->album_id = safe_strdup(jparse_str_from_obj(jobj, "albumId"));
    track->artist_id = safe_strdup(jparse_str_from_obj(jobj, "artistId"));
    track->cover_art_id = safe_strdup(jparse_str_from_obj(jobj, "coverArt"));
    track->duration = jparse_int_from_obj(jobj, "duration");
    track->bit_rate = jparse_int_from_obj(jobj, "bitRate");
    track->path = safe_strdup(jparse_str_from_obj(jobj, "path"));
    track->content_type = safe_strdup(jparse_str_from_obj(jobj, "contentType"));
    track->disc_number = jparse_int_from_obj(jobj, "discNumber");
    track->track_number = jparse_int_from_obj(jobj, "track");
    track->year = jparse_int_from_obj(jobj, "year");
    track->genre = safe_strdup(jparse_str_from_obj(jobj, "genre"));
    track->size = jparse_long_from_obj(jobj, "size");
    track->is_dir = jparse_bool_from_obj(jobj, "isDir");
    track->parent_id = safe_strdup(jparse_str_from_obj(jobj, "parent"));

    if (!track->id) {
        DPRINTF(E_WARN, L_HTTP, "OpenSubsonic track parsing: missing 'id'.\n");
        opensubsonic_free_track(track);
        return -1;
    }
    return 0;
}
*/

// Parses a Subsonic 'song' JSON object into an opensubsonic_track struct.
// Returns 0 on success, -1 on failure. Caller is responsible for calling opensubsonic_free_track on track_out if successful.
static int opensubsonic_json_to_track_struct(json_object *jobj_song, struct opensubsonic_track *track_out) {
    if (!jobj_song || !track_out) return -1;
    memset(track_out, 0, sizeof(struct opensubsonic_track));

    track_out->id = safe_strdup(jparse_str_from_obj(jobj_song, "id"));
    track_out->title = safe_strdup(jparse_str_from_obj(jobj_song, "title"));
    track_out->artist = safe_strdup(jparse_str_from_obj(jobj_song, "artist"));
    track_out->album = safe_strdup(jparse_str_from_obj(jobj_song, "album"));
    track_out->album_id = safe_strdup(jparse_str_from_obj(jobj_song, "albumId"));
    track_out->artist_id = safe_strdup(jparse_str_from_obj(jobj_song, "artistId"));
    track_out->cover_art_id = safe_strdup(jparse_str_from_obj(jobj_song, "coverArt"));
    track_out->duration = jparse_int_from_obj(jobj_song, "duration");
    track_out->bit_rate = jparse_int_from_obj(jobj_song, "bitRate");
    track_out->path = safe_strdup(jparse_str_from_obj(jobj_song, "path"));
    track_out->content_type = safe_strdup(jparse_str_from_obj(jobj_song, "contentType"));
    track_out->disc_number = jparse_int_from_obj(jobj_song, "discNumber");
    track_out->track_number = jparse_int_from_obj(jobj_song, "track");
    track_out->year = jparse_int_from_obj(jobj_song, "year");
    track_out->genre = safe_strdup(jparse_str_from_obj(jobj_song, "genre"));
    track_out->size = jparse_long_from_obj(jobj_song, "size");
    track_out->is_dir = jparse_bool_from_obj(jobj_song, "isDir");
    track_out->parent_id = safe_strdup(jparse_str_from_obj(jobj_song, "parent"));

    if (!track_out->id) { // ID is mandatory
        DPRINTF(E_WARN, L_HTTP, "OpenSubsonic track parsing: missing 'id'. JSON: %s\n", json_object_to_json_string(jobj_song));
        opensubsonic_free_track(track_out); // Clean up partially filled struct
        return -1;
    }
    return 0;
}

// TODO: Implement parsing functions for album, artist, playlist, searchResult3 etc.
// e.g. opensubsonic_json_to_album_struct, opensubsonic_json_to_artist_struct etc.

void opensubsonic_free_playlist_with_entries(struct opensubsonic_playlist_with_entries *playlist) {
    if (!playlist) return;
    safe_free_string(&playlist->id);
    safe_free_string(&playlist->name);
    safe_free_string(&playlist->owner);
    safe_free_string(&playlist->comment);
    safe_free_string(&playlist->cover_art_id);

    if (playlist->entries) {
        for (int i = 0; i < playlist->num_entries; i++) {
            opensubsonic_free_track(&playlist->entries[i]);
        }
        free(playlist->entries);
        playlist->entries = NULL;
    }
    playlist->num_entries = 0;
}

// Parses the 'playlist' object within a 'getPlaylist' response.
// Expects jobj_playlist to be the object that has "id", "name", "entry" (array of songs), etc.
int opensubsonic_parse_playlist_response(json_object *jobj_playlist, struct opensubsonic_playlist_with_entries *playlist_out) {
    if (!jobj_playlist || !playlist_out) return -1;
    memset(playlist_out, 0, sizeof(struct opensubsonic_playlist_with_entries));

    playlist_out->id = safe_strdup(jparse_str_from_obj(jobj_playlist, "id"));
    playlist_out->name = safe_strdup(jparse_str_from_obj(jobj_playlist, "name"));
    playlist_out->owner = safe_strdup(jparse_str_from_obj(jobj_playlist, "owner"));
    playlist_out->comment = safe_strdup(jparse_str_from_obj(jobj_playlist, "comment"));
    playlist_out->cover_art_id = safe_strdup(jparse_str_from_obj(jobj_playlist, "coverArt")); // Often an albumId of one of the tracks
    playlist_out->song_count = jparse_int_from_obj(jobj_playlist, "songCount");
    playlist_out->duration = jparse_int_from_obj(jobj_playlist, "duration");

    if (!playlist_out->id) { // ID is mandatory
        DPRINTF(E_WARN, L_HTTP, "OpenSubsonic playlist parsing: missing 'id'.\n");
        opensubsonic_free_playlist_with_entries(playlist_out);
        return -1;
    }

    json_object *j_entries_array;
    if (json_object_object_get_ex(jobj_playlist, "entry", &j_entries_array) &&
        json_object_is_type(j_entries_array, json_type_array)) {

        int num_json_entries = json_object_array_length(j_entries_array);
        if (num_json_entries > 0) {
            playlist_out->entries = calloc(num_json_entries, sizeof(struct opensubsonic_track));
            if (!playlist_out->entries) {
                DPRINTF(E_LOG, L_HTTP, "Failed to allocate memory for playlist entries.\n");
                opensubsonic_free_playlist_with_entries(playlist_out);
                return -1;
            }

            int actual_parsed_count = 0;
            for (int i = 0; i < num_json_entries; i++) {
                json_object *j_song = json_object_array_get_idx(j_entries_array, i);
                if (opensubsonic_json_to_track_struct(j_song, &playlist_out->entries[actual_parsed_count]) == 0) {
                    actual_parsed_count++;
                } else {
                    DPRINTF(E_WARN, L_HTTP, "Skipping invalid track in playlist %s.\n", playlist_out->id);
                }
            }
            playlist_out->num_entries = actual_parsed_count;
        }
    } else {
        // No "entry" array, or it's not an array. Could be an empty playlist.
        playlist_out->num_entries = 0;
        playlist_out->entries = NULL;
    }
    // It's possible songCount from metadata differs from actual entries parsed.
    // num_entries reflects what was actually parsed.

    return 0;
}

// Fetches a playlist and parses it into C structs.
int opensubsonic_api_get_parsed_playlist(const char *playlist_id, struct opensubsonic_playlist_with_entries *result_out, const char **errmsg) {
    if (!result_out) {
        if (errmsg) *errmsg = "Output struct for parsed playlist is NULL.";
        return -1;
    }
    memset(result_out, 0, sizeof(struct opensubsonic_playlist_with_entries));

    json_object *j_response = opensubsonic_api_get_playlist(playlist_id, errmsg); // This gets the raw JSON
    if (!j_response) {
        // errmsg should be set by opensubsonic_api_get_playlist or make_opensubsonic_request
        return -1;
    }

    int parse_ret = -1;
    json_object *j_subsonic_response;
    if (json_object_object_get_ex(j_response, "subsonic-response", &j_subsonic_response)) {
        json_object *j_playlist_obj;
        if (json_object_object_get_ex(j_subsonic_response, "playlist", &j_playlist_obj)) {
            if (opensubsonic_parse_playlist_response(j_playlist_obj, result_out) == 0) {
                parse_ret = 0; // Success
            } else {
                if (errmsg && !*errmsg) *errmsg = "Failed to parse playlist object from API response.";
                // result_out might be partially filled, opensubsonic_free_playlist_with_entries should handle it
            }
        } else {
            if (errmsg && !*errmsg) *errmsg = "API response for getPlaylist missing 'playlist' object.";
        }
    } else {
        if (errmsg && !*errmsg) *errmsg = "API response for getPlaylist missing 'subsonic-response' object.";
    }

    jparse_free(j_response);

    if (parse_ret != 0) { // If parsing failed, ensure result_out is cleaned up
        opensubsonic_free_playlist_with_entries(result_out);
    }
    return parse_ret;
}

void opensubsonic_free_album_with_entries(struct opensubsonic_album_with_entries *album) {
    if (!album) return;
    safe_free_string(&album->id);
    safe_free_string(&album->name);
    safe_free_string(&album->artist);
    safe_free_string(&album->artist_id);
    safe_free_string(&album->cover_art_id);
    safe_free_string(&album->genre);

    if (album->songs) {
        for (int i = 0; i < album->num_songs; i++) {
            opensubsonic_free_track(&album->songs[i]);
        }
        free(album->songs);
        album->songs = NULL;
    }
    album->num_songs = 0;
}

// Parses the 'album' object within a 'getAlbum' response.
// Expects jobj_album to be the object that has "id", "name", "song" (array of songs), etc.
int opensubsonic_parse_album_response(json_object *jobj_album, struct opensubsonic_album_with_entries *album_out) {
    if (!jobj_album || !album_out) return -1;
    memset(album_out, 0, sizeof(struct opensubsonic_album_with_entries));

    album_out->id = safe_strdup(jparse_str_from_obj(jobj_album, "id"));
    album_out->name = safe_strdup(jparse_str_from_obj(jobj_album, "name"));
    album_out->artist = safe_strdup(jparse_str_from_obj(jobj_album, "artist"));
    album_out->artist_id = safe_strdup(jparse_str_from_obj(jobj_album, "artistId"));
    album_out->cover_art_id = safe_strdup(jparse_str_from_obj(jobj_album, "coverArt"));
    album_out->song_count = jparse_int_from_obj(jobj_album, "songCount");
    album_out->duration = jparse_int_from_obj(jobj_album, "duration");
    album_out->year = jparse_int_from_obj(jobj_album, "year");
    album_out->genre = safe_strdup(jparse_str_from_obj(jobj_album, "genre"));


    if (!album_out->id) { // ID is mandatory
        DPRINTF(E_WARN, L_HTTP, "OpenSubsonic album parsing: missing 'id'.\n");
        opensubsonic_free_album_with_entries(album_out);
        return -1;
    }

    json_object *j_songs_array;
    if (json_object_object_get_ex(jobj_album, "song", &j_songs_array) &&
        json_object_is_type(j_songs_array, json_type_array)) {

        int num_json_songs = json_object_array_length(j_songs_array);
        if (num_json_songs > 0) {
            album_out->songs = calloc(num_json_songs, sizeof(struct opensubsonic_track));
            if (!album_out->songs) {
                DPRINTF(E_LOG, L_HTTP, "Failed to allocate memory for album songs.\n");
                opensubsonic_free_album_with_entries(album_out);
                return -1;
            }

            int actual_parsed_count = 0;
            for (int i = 0; i < num_json_songs; i++) {
                json_object *j_song = json_object_array_get_idx(j_songs_array, i);
                if (opensubsonic_json_to_track_struct(j_song, &album_out->songs[actual_parsed_count]) == 0) {
                    actual_parsed_count++;
                } else {
                     DPRINTF(E_WARN, L_HTTP, "Skipping invalid track in album %s.\n", album_out->id);
                }
            }
            album_out->num_songs = actual_parsed_count;
        }
    } else {
        album_out->num_songs = 0;
        album_out->songs = NULL;
    }
    return 0;
}

// Fetches an album and parses it into C structs.
int opensubsonic_api_get_parsed_album(const char *album_id, struct opensubsonic_album_with_entries *result_out, const char **errmsg) {
    if (!result_out) {
        if (errmsg) *errmsg = "Output struct for parsed album is NULL.";
        return -1;
    }
    memset(result_out, 0, sizeof(struct opensubsonic_album_with_entries));

    json_object *j_response = opensubsonic_api_get_album(album_id, errmsg); // Gets raw JSON
    if (!j_response) {
        return -1;
    }

    int parse_ret = -1;
    json_object *j_subsonic_response;
    if (json_object_object_get_ex(j_response, "subsonic-response", &j_subsonic_response)) {
        json_object *j_album_obj;
        // Subsonic getAlbum response nests the album under an "album" key
        if (json_object_object_get_ex(j_subsonic_response, "album", &j_album_obj)) {
            if (opensubsonic_parse_album_response(j_album_obj, result_out) == 0) {
                parse_ret = 0; // Success
            } else {
                if (errmsg && !*errmsg) *errmsg = "Failed to parse album object from API response.";
            }
        } else {
            if (errmsg && !*errmsg) *errmsg = "API response for getAlbum missing 'album' object.";
        }
    } else {
        if (errmsg && !*errmsg) *errmsg = "API response for getAlbum missing 'subsonic-response' object.";
    }

    jparse_free(j_response);

    if (parse_ret != 0) {
        opensubsonic_free_album_with_entries(result_out);
    }
    return parse_ret;
}

// Fetches a single song by ID and parses it into the provided track_out C struct.
int opensubsonic_api_fetch_and_parse_song_details(const char *song_id, struct opensubsonic_track *track_out, const char **errmsg) {
    if (!track_out) {
        if (errmsg) *errmsg = "Output struct for parsed song is NULL.";
        return -1;
    }
    memset(track_out, 0, sizeof(struct opensubsonic_track));

    json_object *j_response = opensubsonic_api_get_song(song_id, errmsg); // Gets raw JSON for the song
    if (!j_response) {
        // errmsg should be set by opensubsonic_api_get_song or make_opensubsonic_request
        return -1;
    }

    int parse_ret = -1;
    json_object *j_subsonic_response;
    if (json_object_object_get_ex(j_response, "subsonic-response", &j_subsonic_response)) {
        json_object *j_song_obj;
        if (json_object_object_get_ex(j_subsonic_response, "song", &j_song_obj)) {
            if (opensubsonic_json_to_track_struct(j_song_obj, track_out) == 0) {
                parse_ret = 0; // Success
            } else {
                if (errmsg && !*errmsg) *errmsg = "Failed to parse song object from API response.";
                // track_out might be partially filled, opensubsonic_free_track should handle it
            }
        } else {
            if (errmsg && !*errmsg) *errmsg = "API response for getSong missing 'song' object.";
        }
    } else {
        if (errmsg && !*errmsg) *errmsg = "API response for getSong missing 'subsonic-response' object.";
    }

    jparse_free(j_response);

    if (parse_ret != 0) { // If parsing failed, ensure track_out is cleaned up
        opensubsonic_free_track(track_out);
    }
    return parse_ret;
}
