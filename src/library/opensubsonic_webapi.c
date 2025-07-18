/*
 * Copyright (C) 2024 OwnTone contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <json-c/json.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gcrypt.h>
#include "conffile.h"
#include "http.h"
#include "logger.h"
#include "misc.h"
#include "misc_json.h"
#include "inputs/opensubsonic.h"
#include "library/opensubsonic_webapi.h"
#include "library/opensubsonic_cache.h"
#include "library.h"
#include "db.h"

// Forward declarations of backend functions
static int opensubsonic_webapi_init(void);
static void opensubsonic_webapi_deinit(void);
static void opensubsonic_webapi_status_get(struct opensubsonic_status *status);

// Export the backend structure
struct opensubsonic_backend opensubsonic_webapi = {
    .init = opensubsonic_webapi_init,
    .deinit = opensubsonic_webapi_deinit,
    .connect = opensubsonic_connect,
    .disconnect = opensubsonic_disconnect,
    .status_get = opensubsonic_webapi_status_get,
};

// OpenSubsonic connection credentials
struct opensubsonic_credentials {
    char *server_url;
    char *username;
    char *password;
    char *api_version;
    char *client_name;
    int timeout;
    bool artwork_enabled;
    int artwork_max_size;
    bool connected;
    // Cached authentication tokens
    char *cached_salt;
    char *cached_token;
    time_t token_generated_time;
};

static struct opensubsonic_credentials opensubsonic_credentials;
static pthread_mutex_t opensubsonic_credentials_lock = PTHREAD_MUTEX_INITIALIZER;

// Cache authentication tokens for this duration (in seconds)
#define AUTH_TOKEN_CACHE_DURATION 86400  // 24 hours

// Generate token for OpenSubsonic authentication
static char *
generate_auth_token(const char *password, const char *salt)
{
  char *combined;
  char *token;

  if (!password || !salt)
    return NULL;

  combined = malloc(strlen(password) + strlen(salt) + 1);
  if (!combined)
    return NULL;

  sprintf(combined, "%s%s", password, salt);

  // Use gcrypt for MD5 hash like other parts of the codebase
  gcry_md_hd_t md_hdl;
  unsigned char *hash_bytes;
  int hash_len;
  int i;
  gcry_error_t gc_err;

  gc_err = gcry_md_open(&md_hdl, GCRY_MD_MD5, 0);
  if (gc_err != GPG_ERR_NO_ERROR)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Could not open MD5: %s\n", gcry_strerror(gc_err));
      free(combined);
      return NULL;
    }

  gcry_md_write(md_hdl, combined, strlen(combined));
  hash_bytes = gcry_md_read(md_hdl, GCRY_MD_MD5);
  if (!hash_bytes)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Could not read MD5 hash\n");
      gcry_md_close(md_hdl);
      free(combined);
      return NULL;
    }

  hash_len = gcry_md_get_algo_dlen(GCRY_MD_MD5);
  token = calloc(1, 2 * hash_len + 1);
  if (token)
    {
      for (i = 0; i < hash_len; i++)
	sprintf(token + (2 * i), "%02x", hash_bytes[i]);
    }

  gcry_md_close(md_hdl);

  free(combined);
  return token;
}

// Generate random salt for authentication
static char *
generate_salt(void)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *salt = malloc(7); // 6 chars + null terminator
    int i;
    
    srand(time(NULL));
    for (i = 0; i < 6; i++)
        salt[i] = charset[rand() % (sizeof(charset) - 1)];
    salt[6] = '\0';
    
    return salt;
}

// Build OpenSubsonic API URL with authentication parameters
static char *
build_api_url(const char *endpoint)
{
    char *salt;
    char *token;
    char *url;
    time_t current_time;
    bool need_new_token = false;
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
    
    if (!opensubsonic_credentials.connected || !opensubsonic_credentials.server_url)
    {
        CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
        return NULL;
    }
    
    current_time = time(NULL);
    
    // Check if we need to generate new tokens
    if (!opensubsonic_credentials.cached_salt || !opensubsonic_credentials.cached_token ||
        (current_time - opensubsonic_credentials.token_generated_time) > AUTH_TOKEN_CACHE_DURATION)
    {
        need_new_token = true;
    }
    
    if (need_new_token)
    {
        // Free old cached tokens
        free(opensubsonic_credentials.cached_salt);
        free(opensubsonic_credentials.cached_token);
        
        // Generate new tokens
        opensubsonic_credentials.cached_salt = generate_salt();
        opensubsonic_credentials.cached_token = generate_auth_token(opensubsonic_credentials.password, 
                                                                   opensubsonic_credentials.cached_salt);
        opensubsonic_credentials.token_generated_time = current_time;
        
        if (!opensubsonic_credentials.cached_salt || !opensubsonic_credentials.cached_token)
        {
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to generate authentication tokens\n");
            CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
            return NULL;
        }
    }
    
    // Use cached tokens
    salt = opensubsonic_credentials.cached_salt;
    token = opensubsonic_credentials.cached_token;
    
    // Check if endpoint already contains parameters
    const char *separator = strchr(endpoint, '?') ? "&" : "?";
    url = safe_asprintf("%s/rest/%s%su=%s&t=%s&s=%s&v=%s&c=%s&f=json",
                    opensubsonic_credentials.server_url,
                    endpoint,
                    separator,
                    opensubsonic_credentials.username,
                    token,
                    salt,
                    opensubsonic_credentials.api_version,
                    opensubsonic_credentials.client_name);
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
    
    return url;
}

/* -------------- Public API Implementation -------------- */

static int
opensubsonic_webapi_init(void)
{
    cfg_t *opensubsonic_cfg = cfg_getsec(cfg, "opensubsonic");
    bool enabled;
    const char *errmsg;
    
    if (!opensubsonic_cfg)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic configuration section not found\n");
        return 0;
    }
    
    // Initialize cache system
    if (opensubsonic_cache_init() < 0)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to initialize OpenSubsonic cache\n");
        return -1;
    }
    
    enabled = cfg_getbool(opensubsonic_cfg, "enabled");
    DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic initialization - enabled: %s\n", enabled ? "true" : "false");
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
    
    const char *server_url = cfg_getstr(opensubsonic_cfg, "server_url");
    const char *username = cfg_getstr(opensubsonic_cfg, "username");
    const char *password = cfg_getstr(opensubsonic_cfg, "password");
    const char *api_version = cfg_getstr(opensubsonic_cfg, "api_version");
    const char *client_name = cfg_getstr(opensubsonic_cfg, "client_name");
    
    opensubsonic_credentials.server_url = safe_strdup(server_url ? server_url : "");
    opensubsonic_credentials.username = safe_strdup(username ? username : "");
    opensubsonic_credentials.password = safe_strdup(password ? password : "");
    opensubsonic_credentials.api_version = safe_strdup(api_version && strlen(api_version) > 0 ? api_version : "1.16.1");
    opensubsonic_credentials.client_name = safe_strdup(client_name && strlen(client_name) > 0 ? client_name : "OwnTone");
    opensubsonic_credentials.timeout = cfg_getint(opensubsonic_cfg, "timeout");
    if (opensubsonic_credentials.timeout <= 0)
        opensubsonic_credentials.timeout = 30;
    opensubsonic_credentials.artwork_enabled = cfg_getbool(opensubsonic_cfg, "artwork_enabled");
    opensubsonic_credentials.artwork_max_size = cfg_getint(opensubsonic_cfg, "artwork_max_size");
    if (opensubsonic_credentials.artwork_max_size <= 0)
        opensubsonic_credentials.artwork_max_size = 600;
    opensubsonic_credentials.connected = false;
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Configuration loaded - server_url: '%s', username: '%s', api_version: '%s'\n",
            opensubsonic_credentials.server_url ? opensubsonic_credentials.server_url : "(empty)",
            opensubsonic_credentials.username ? opensubsonic_credentials.username : "(empty)",
            opensubsonic_credentials.api_version ? opensubsonic_credentials.api_version : "(empty)");
    
    // Make local copies of credentials for auto-connect to avoid race conditions
    char *local_server_url = NULL;
    char *local_username = NULL;
    char *local_password = NULL;
    bool should_auto_connect = false;
    
    if (enabled && 
        opensubsonic_credentials.server_url && strlen(opensubsonic_credentials.server_url) > 0 &&
        opensubsonic_credentials.username && strlen(opensubsonic_credentials.username) > 0 &&
        opensubsonic_credentials.password && strlen(opensubsonic_credentials.password) > 0)
    {
        local_server_url = safe_strdup(opensubsonic_credentials.server_url);
        local_username = safe_strdup(opensubsonic_credentials.username);
        local_password = safe_strdup(opensubsonic_credentials.password);
        should_auto_connect = true;
    }
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
    
    // Attempt auto-connect if enabled and all required credentials are configured
    if (should_auto_connect)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Attempting auto-connect to OpenSubsonic server...\n");
        
        if (opensubsonic_connect(local_server_url, local_username, local_password, &errmsg) == 0)
        {
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Auto-connect successful - connected to '%s' as user '%s'\n",
                    local_server_url, local_username);
        }
        else
        {
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Auto-connect failed: %s\n", errmsg ? errmsg : "Unknown error");
        }
        
        free(local_server_url);
        free(local_username);
        free(local_password);
    }
    else if (enabled)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Auto-connect skipped - missing required configuration\n");
    }
    else
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic is disabled in configuration\n");
    }
    
    return 0;
}

static void
opensubsonic_webapi_deinit(void)
{
    // Cleanup cache system
    opensubsonic_cache_deinit();
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
    
    free(opensubsonic_credentials.server_url);
    free(opensubsonic_credentials.username);
    free(opensubsonic_credentials.password);
    free(opensubsonic_credentials.api_version);
    free(opensubsonic_credentials.client_name);
    
    // Free cached authentication tokens
    free(opensubsonic_credentials.cached_salt);
    free(opensubsonic_credentials.cached_token);
    
    memset(&opensubsonic_credentials, 0, sizeof(struct opensubsonic_credentials));
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
}

int
opensubsonic_connect(const char *server_url, const char *username, const char *password, const char **errmsg)
{
    char *url;
    char *salt;
    char *token;
    struct http_client_ctx ctx;
    char *response_body;
    struct json_object *json_response = NULL;
    struct json_object *subsonic_response = NULL;
    const char *status;
    const char *version;
    int ret = -1;
    
    *errmsg = "";
    
    salt = generate_salt();
    token = generate_auth_token(password, salt);
    
    url = safe_asprintf("%s/rest/ping?u=%s&t=%s&s=%s&v=1.16.1&c=OwnTone&f=json",
                    server_url, username, token, salt);
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Connecting to OpenSubsonic server '%s' as user '%s'\n", server_url, username);
    
    memset(&ctx, 0, sizeof(struct http_client_ctx));
    ctx.url = url;
    ctx.input_body = evbuffer_new();
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Sending ping request to: %s\n", server_url);
    DPRINTF(E_LOG, L_OPENSUBSONIC, "URL: %s\n", url);

    ret = http_client_request(&ctx, NULL);
    if (ret < 0)
    {
        *errmsg = "Failed to connect to OpenSubsonic server - network error";
        DPRINTF(E_LOG, L_OPENSUBSONIC, "HTTP request failed with code: %d\n", ret);
        goto out;
    }
    
    evbuffer_add(ctx.input_body, "", 1);
    response_body = (char *)evbuffer_pullup(ctx.input_body, -1);
    
    if (!response_body || (strlen(response_body) == 0))
    {
        *errmsg = "Empty response from OpenSubsonic server";
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Received empty response from server\n");
        ret = -1;
        goto out;
    }
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Received response: %s\n", response_body);
    
    json_response = json_tokener_parse(response_body);
    if (!json_response)
    {
        *errmsg = "Invalid JSON response from OpenSubsonic server";
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to parse JSON response\n");
        ret = -1;
        goto out;
    }
    
    if (json_object_object_get_ex(json_response, "subsonic-response", &subsonic_response))
    {
        status = jparse_str_from_obj(subsonic_response, "status");
        version = jparse_str_from_obj(subsonic_response, "version");
        
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Server response - status: '%s', version: '%s'\n", 
                status ? status : "null", version ? version : "null");
        
        if (status && strcmp(status, "ok") == 0)
        {
            ret = 0;
            
            CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
            
            free(opensubsonic_credentials.server_url);
            free(opensubsonic_credentials.username);
            free(opensubsonic_credentials.password);
            
            opensubsonic_credentials.server_url = safe_strdup(server_url);
            opensubsonic_credentials.username = safe_strdup(username);
            opensubsonic_credentials.password = safe_strdup(password);
            opensubsonic_credentials.connected = true;
            
            CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
            
            // Clear cache when connecting to a new server
            opensubsonic_cache_clear();
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Cleared OpenSubsonic cache on new connection\n");
            
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Connection successful - server version: %s\n", version ? version : "unknown");
        }
        else
        {
            const char *error_code = jparse_str_from_obj(subsonic_response, "error");
            const char *error_message = jparse_str_from_obj(subsonic_response, "message");
            
            *errmsg = safe_asprintf("OpenSubsonic server error - code: %s, message: %s", 
                                  error_code ? error_code : "unknown", 
                                  error_message ? error_message : "no message");
            
            DPRINTF(E_LOG, L_OPENSUBSONIC, "Server returned error - status: '%s', error: %s\n", 
                    status ? status : "null", *errmsg);
            ret = -1;
        }
    }
    else
    {
        *errmsg = "Invalid OpenSubsonic response format - missing subsonic-response";
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Response missing 'subsonic-response' field\n");
        ret = -1;
    }
    
out:
    if (ctx.input_body)
        evbuffer_free(ctx.input_body);
    free(url);
    free(salt);
    free(token);
    if (json_response)
        json_object_put(json_response);
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Connection attempt completed - result: %s\n", ret == 0 ? "SUCCESS" : "FAILED");
    
    return ret;
}

void
opensubsonic_disconnect(void)
{
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Disconnecting from OpenSubsonic server\n");
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
    opensubsonic_credentials.connected = false;
    
    // Clear cached authentication tokens
    free(opensubsonic_credentials.cached_salt);
    free(opensubsonic_credentials.cached_token);
    opensubsonic_credentials.cached_salt = NULL;
    opensubsonic_credentials.cached_token = NULL;
    opensubsonic_credentials.token_generated_time = 0;
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
    
    // Clear cache when disconnecting
    opensubsonic_cache_clear();
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Cleared OpenSubsonic cache on disconnect\n");
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Disconnection completed\n");
}

void
opensubsonic_webapi_status_get(struct opensubsonic_status *status)
{
    struct opensubsonic_status_info info;
    
    opensubsonic_status_info_get(&info);
    
    memset(status, 0, sizeof(struct opensubsonic_status));
    status->enabled = info.enabled;
    status->connected = info.connected;
    
    if (info.server_url[0])
    {
        strncpy(status->server_url, info.server_url, sizeof(status->server_url) - 1);
        status->server_url[sizeof(status->server_url) - 1] = '\0';
    }
    
    if (info.username[0])
    {
        strncpy(status->username, info.username, sizeof(status->username) - 1);
        status->username[sizeof(status->username) - 1] = '\0';
    }
    
    status->has_connection = info.connected;
}

/* -------------- API Request Functions -------------- */

static struct json_object *
make_api_request(const char *endpoint)
{
    char *url;
    struct http_client_ctx ctx;
    char *response_body;
    struct json_object *json_response = NULL;
    struct json_object *subsonic_response = NULL;
    const char *status;
    int ret;

    DPRINTF(E_LOG, L_OPENSUBSONIC, "Making API request to endpoint: %s\n", endpoint);

    url = build_api_url(endpoint);
    if (!url)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to build API URL for endpoint: %s (not connected?)\n", endpoint);
        return NULL;
    }

    memset(&ctx, 0, sizeof(struct http_client_ctx));
    ctx.url = url;
    ctx.input_body = evbuffer_new();
    ctx.timeout = opensubsonic_credentials.timeout;

    DPRINTF(E_LOG, L_OPENSUBSONIC, "Sending HTTP request to: %s (timeout: %d seconds)\n", url, opensubsonic_credentials.timeout);

    ret = http_client_request(&ctx, NULL);
    DPRINTF(E_LOG, L_OPENSUBSONIC, "HTTP request completed for endpoint '%s' with return code: %d\n", endpoint, ret);
    if (ret < 0)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "HTTP request failed for endpoint '%s' with code: %d\n", endpoint, ret);
        goto out;
    }

    evbuffer_add(ctx.input_body, "", 1);
    response_body = (char *)evbuffer_pullup(ctx.input_body, -1);
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Response body length for endpoint '%s': %zu bytes\n", endpoint, response_body ? strlen(response_body) : 0);
    if (!response_body)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "NULL response body from endpoint '%s'\n", endpoint);
        goto out;
    }
    if (strlen(response_body) == 0)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Empty response body from endpoint '%s'\n", endpoint);
        goto out;
    }
    if (strlen(response_body) == 1 && response_body[0] == '\0')
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Response body contains only null terminator from endpoint '%s'\n", endpoint);
        goto out;
    }

    DPRINTF(E_LOG, L_OPENSUBSONIC, "Received response from '%s': %s\n", endpoint, response_body);

    json_response = json_tokener_parse(response_body);
    if (!json_response)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to parse JSON response from endpoint '%s'\n", endpoint);
        goto out;
    }

    if (!json_object_object_get_ex(json_response, "subsonic-response", &subsonic_response))
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Invalid response format from endpoint '%s' - missing subsonic-response\n", endpoint);
        json_object_put(json_response);
        json_response = NULL;
        goto out;
    }

    // Check response status
    status = jparse_str_from_obj(subsonic_response, "status");
    if (status)
    {
        if (strcmp(status, "ok") == 0)
        {
            DPRINTF(E_LOG, L_OPENSUBSONIC, "API request to '%s' successful\n", endpoint);
        }
        else
        {
            const char *error_code = jparse_str_from_obj(subsonic_response, "error");
            const char *error_message = jparse_str_from_obj(subsonic_response, "message");
            DPRINTF(E_LOG, L_OPENSUBSONIC, "API request to '%s' failed - status: %s, error: %s, message: %s\n", 
                    endpoint, status, error_code ? error_code : "unknown", error_message ? error_message : "no message");
        }
    }
    else
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "API response from '%s' missing status field\n", endpoint);
    }

out:
    if (ctx.input_body)
        evbuffer_free(ctx.input_body);
    free(url);
    return json_response;
}

struct json_object *
opensubsonic_request_artists(void)
{
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Requesting artists list\n");
    return make_api_request("getArtists");
}

struct json_object *
opensubsonic_request_artist(const char *id)
{
    char *endpoint;
    struct json_object *response;

    endpoint = safe_asprintf("getArtist?id=%s", id);
    response = make_api_request(endpoint);
    free(endpoint);

    return response;
}

struct json_object *
opensubsonic_request_albums(void)
{
    struct json_object *cached_data;
    struct json_object *response;
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Requesting albums list\n");
    
    // Try to get from cache first
    cached_data = opensubsonic_cache_get(OPENSUBSONIC_CACHE_ALBUMS, NULL);
    if (cached_data)
    {
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Returning cached albums list\n");
        // Note: cached_data already has incremented reference count from cache
        return cached_data;
    }
    
    // Cache miss or expired, fetch from server
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache miss for albums list, fetching from server\n");
    response = make_api_request("getAlbumList2?type=alphabeticalByName");
    
    // Store in cache
    if (response)
    {
        opensubsonic_cache_put(OPENSUBSONIC_CACHE_ALBUMS, NULL, response);
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Cached albums list\n");
    }
    
    return response;
}

struct json_object *
opensubsonic_request_album(const char *id)
{
    char *endpoint;
    struct json_object *cached_data;
    struct json_object *response;

    if (!id)
        return NULL;
        
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Requesting album details for ID: %s\n", id);
    
    // Try to get from cache first
    cached_data = opensubsonic_cache_get(OPENSUBSONIC_CACHE_ALBUM, id);
    if (cached_data)
    {
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Returning cached album details for ID: %s\n", id);
        return cached_data;
    }
    
    // Cache miss or expired, fetch from server
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache miss for album ID %s, fetching from server\n", id);
    endpoint = safe_asprintf("getAlbum?id=%s", id);
    response = make_api_request(endpoint);
    free(endpoint);
    
    // Store in cache
    if (response)
    {
        opensubsonic_cache_put(OPENSUBSONIC_CACHE_ALBUM, id, response);
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Cached album details for ID: %s\n", id);
    }

    return response;
}

struct json_object *
opensubsonic_request_playlists(void)
{
    struct json_object *cached_data;
    struct json_object *response;
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Requesting playlists list\n");
    
    // Try to get from cache first
    cached_data = opensubsonic_cache_get(OPENSUBSONIC_CACHE_PLAYLISTS, NULL);
    if (cached_data)
    {
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Returning cached playlists list\n");
        return cached_data;
    }
    
    // Cache miss or expired, fetch from server
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache miss for playlists list, fetching from server\n");
    response = make_api_request("getPlaylists");
    
    // Store in cache
    if (response)
    {
        opensubsonic_cache_put(OPENSUBSONIC_CACHE_PLAYLISTS, NULL, response);
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Cached playlists list\n");
    }
    
    return response;
}

struct json_object *
opensubsonic_request_playlist(const char *id)
{
    char *endpoint;
    struct json_object *cached_data;
    struct json_object *response;

    if (!id)
        return NULL;
        
    DPRINTF(E_LOG, L_OPENSUBSONIC, "Requesting playlist details for ID: %s\n", id);
    
    // Try to get from cache first
    cached_data = opensubsonic_cache_get(OPENSUBSONIC_CACHE_PLAYLIST, id);
    if (cached_data)
    {
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Returning cached playlist details for ID: %s\n", id);
        return cached_data;
    }
    
    // Cache miss or expired, fetch from server
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache miss for playlist ID %s, fetching from server\n", id);
    endpoint = safe_asprintf("getPlaylist?id=%s", id);
    response = make_api_request(endpoint);
    free(endpoint);
    
    // Store in cache
    if (response)
    {
        opensubsonic_cache_put(OPENSUBSONIC_CACHE_PLAYLIST, id, response);
        DPRINTF(E_DBG, L_OPENSUBSONIC, "Cached playlist details for ID: %s\n", id);
    }

    return response;
}

struct json_object *
opensubsonic_request_search(const char *query)
{
    char *encoded_query;
    char *endpoint;
    struct json_object *response;

    // URL encode the query parameter to handle Chinese characters and special chars
    encoded_query = evhttp_encode_uri(query);
    if (!encoded_query)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to URL encode search query: %s\n", query);
        return NULL;
    }
    
    endpoint = safe_asprintf("search3?query=%s", encoded_query);
    response = make_api_request(endpoint);

    free(encoded_query);
    free(endpoint);

    return response;
}

char *
opensubsonic_stream_url_get(const char *id)
{
    char *url = build_api_url("stream");
    char *stream_url;
    
    if (!url)
        return NULL;
    
    stream_url = safe_asprintf("%s&id=%s", url, id);
    free(url);
    
    return stream_url;
}

char *
opensubsonic_artwork_url_get(const char *id, int max_w, int max_h)
{
    char *endpoint;
    char *artwork_url;
    
    // Build endpoint with id parameter first
    if (max_w > 0 && max_h > 0)
        endpoint = safe_asprintf("getCoverArt?id=%s&size=%d", id, max_w < max_h ? max_w : max_h);
    else
        endpoint = safe_asprintf("getCoverArt?id=%s", id);
    
    artwork_url = build_api_url(endpoint);
    free(endpoint);
    
    return artwork_url;
}

void
opensubsonic_status_info_get(struct opensubsonic_status_info *info)
{
    cfg_t *opensubsonic_cfg;
    
    memset(info, 0, sizeof(struct opensubsonic_status_info));
    
    opensubsonic_cfg = cfg_getsec(cfg, "opensubsonic");
    if (!opensubsonic_cfg)
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: No opensubsonic config section found\n");
        return;
    }
        
    info->enabled = cfg_getbool(opensubsonic_cfg, "enabled");
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: enabled = %s\n", info->enabled ? "true" : "false");
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&opensubsonic_credentials_lock));
    
    info->connected = opensubsonic_credentials.connected;
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: connected = %s\n", info->connected ? "true" : "false");
    
    if (opensubsonic_credentials.server_url && strlen(opensubsonic_credentials.server_url) > 0)
    {
        strncpy(info->server_url, opensubsonic_credentials.server_url, sizeof(info->server_url) - 1);
        info->server_url[sizeof(info->server_url) - 1] = '\0';
        DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: server_url = '%s'\n", info->server_url);
    }
    else
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: server_url is empty\n");
    }
    
    if (opensubsonic_credentials.username && strlen(opensubsonic_credentials.username) > 0)
    {
        strncpy(info->username, opensubsonic_credentials.username, sizeof(info->username) - 1);
        info->username[sizeof(info->username) - 1] = '\0';
        DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: username = '%s'\n", info->username);
    }
    else
    {
        DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: username is empty\n");
    }
    
    if (opensubsonic_credentials.api_version && strlen(opensubsonic_credentials.api_version) > 0)
    {
        strncpy(info->api_version, opensubsonic_credentials.api_version, sizeof(info->api_version) - 1);
        info->api_version[sizeof(info->api_version) - 1] = '\0';
    }
    else
    {
        // Use default API version if not configured
        strncpy(info->api_version, "1.16.1", sizeof(info->api_version) - 1);
        info->api_version[sizeof(info->api_version) - 1] = '\0';
    }
    
    if (opensubsonic_credentials.client_name && strlen(opensubsonic_credentials.client_name) > 0)
    {
        strncpy(info->client_name, opensubsonic_credentials.client_name, sizeof(info->client_name) - 1);
        info->client_name[sizeof(info->client_name) - 1] = '\0';
    }
    else
    {
        // Use default client name if not configured
        strncpy(info->client_name, "OwnTone", sizeof(info->client_name) - 1);
        info->client_name[sizeof(info->client_name) - 1] = '\0';
    }
    
    info->timeout = opensubsonic_credentials.timeout > 0 ? opensubsonic_credentials.timeout : 30;
    info->artwork_enabled = opensubsonic_credentials.artwork_enabled;
    info->artwork_max_size = opensubsonic_credentials.artwork_max_size > 0 ? opensubsonic_credentials.artwork_max_size : 600;
    
    DPRINTF(E_LOG, L_OPENSUBSONIC, "opensubsonic_status_info_get: api_version = '%s', client_name = '%s', timeout = %d\n",
            info->api_version, info->client_name, info->timeout);
    
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&opensubsonic_credentials_lock));
}

/* --------------------------- Library interface ---------------------------- */
/*                              Thread: library                               */

// Helper function to get song metadata from OpenSubsonic API based on source
static int
opensubsonic_get_song_metadata_from_source(const char *song_id, const char *album_id, const char *playlist_id, 
                                          char **title, char **artist, char **album, int *duration)
{
  struct json_object *response = NULL;
  struct json_object *subsonic_response = NULL;
  struct json_object *album_obj = NULL;
  struct json_object *playlist_obj = NULL;
  struct json_object *songs_array = NULL;
  struct json_object *song_obj = NULL;
  int i, array_len;
  const char *song_title = NULL;
  const char *song_artist = NULL;
  const char *song_album = NULL;
  int song_duration = 0;
  
  *title = NULL;
  *artist = NULL;
  *album = NULL;
  *duration = 0;

  DPRINTF(E_DBG, L_OPENSUBSONIC, "Getting song metadata for ID %s from album %s or playlist %s\n", 
          song_id, album_id ? album_id : "none", playlist_id ? playlist_id : "none");

  // Try to get song info from album first
  if (album_id)
    {
      response = opensubsonic_request_album(album_id);
      if (response && json_object_object_get_ex(response, "subsonic-response", &subsonic_response))
        {
          if (json_object_object_get_ex(subsonic_response, "album", &album_obj))
            {
              if (json_object_object_get_ex(album_obj, "song", &songs_array) && json_object_is_type(songs_array, json_type_array))
                {
                  array_len = json_object_array_length(songs_array);
                  for (i = 0; i < array_len; i++)
                    {
                      song_obj = json_object_array_get_idx(songs_array, i);
                      const char *id = jparse_str_from_obj(song_obj, "id");
                      if (id && strcmp(id, song_id) == 0)
                        {
                          song_title = jparse_str_from_obj(song_obj, "title");
                          song_artist = jparse_str_from_obj(song_obj, "artist");
                          song_album = jparse_str_from_obj(song_obj, "album");
                          song_duration = jparse_int_from_obj(song_obj, "duration");
                          break;
                        }
                    }
                }
            }
        }
      if (response)
        json_object_put(response);
    }
  
  // Try to get song info from playlist if not found in album
  if (!song_title && playlist_id)
    {
      response = opensubsonic_request_playlist(playlist_id);
      if (response && json_object_object_get_ex(response, "subsonic-response", &subsonic_response))
        {
          if (json_object_object_get_ex(subsonic_response, "playlist", &playlist_obj))
            {
              if (json_object_object_get_ex(playlist_obj, "entry", &songs_array) && json_object_is_type(songs_array, json_type_array))
                {
                  array_len = json_object_array_length(songs_array);
                  for (i = 0; i < array_len; i++)
                    {
                      song_obj = json_object_array_get_idx(songs_array, i);
                      const char *id = jparse_str_from_obj(song_obj, "id");
                      if (id && strcmp(id, song_id) == 0)
                        {
                          song_title = jparse_str_from_obj(song_obj, "title");
                          song_artist = jparse_str_from_obj(song_obj, "artist");
                          song_album = jparse_str_from_obj(song_obj, "album");
                          song_duration = jparse_int_from_obj(song_obj, "duration");
                          break;
                        }
                    }
                }
            }
        }
      if (response)
        json_object_put(response);
    }

  // Set the metadata if found
  if (song_title)
    {
      *title = strdup(song_title);
      *artist = song_artist ? strdup(song_artist) : strdup("Unknown Artist");
      *album = song_album ? strdup(song_album) : strdup("Unknown Album");
      *duration = song_duration;
      
      DPRINTF(E_DBG, L_OPENSUBSONIC, "Found song metadata: title='%s', artist='%s', album='%s', duration=%d\n",
              *title, *artist, *album, *duration);
      return 0;
    }

  DPRINTF(E_DBG, L_OPENSUBSONIC, "Song metadata not found for ID %s\n", song_id);
  return -1;
}

static int
opensubsonic_webapi_library_queue_item_add(const char *uri, int position, char reshuffle, uint32_t item_id, int *count, int *new_item_id)
{
  struct db_queue_item qi;
  struct db_queue_add_info queue_add_info;
  char *stream_url;
  char *title = NULL;
  char *artist = NULL;
  char *album = NULL;
  const char *song_id = NULL;
  const char *album_id = NULL;
  const char *playlist_id = NULL;
  int duration = 0;
  int ret;

  DPRINTF(E_DBG, L_OPENSUBSONIC, "Adding opensubsonic item to queue: %s\n", uri);

  // Check if this is an opensubsonic URI with format: os:trackId[:a:albumId|:p:playlistId]
  if (strncmp(uri, "os:", 3) != 0)
    return LIBRARY_PATH_INVALID;

  // Parse format: os:trackId or os:trackId:a:albumId or os:trackId:p:playlistId
  char *uri_copy = strdup(uri + 3); // Skip "os:"
  char *saveptr;
  char *token;
  
  // Get track ID
  token = strtok_r(uri_copy, ":", &saveptr);
  if (token)
    song_id = strdup(token);
  
  // Get source type (a for album, p for playlist) - optional
  token = strtok_r(NULL, ":", &saveptr);
  if (token)
    {
      if (strcmp(token, "a") == 0)
        {
          // Album source
          token = strtok_r(NULL, ":", &saveptr);
          if (token)
            album_id = strdup(token);
        }
      else if (strcmp(token, "p") == 0)
        {
          // Playlist source
          token = strtok_r(NULL, ":", &saveptr);
          if (token)
            playlist_id = strdup(token);
        }
    }
  
  free(uri_copy);
  
  // Try to get song metadata from the source (if source info is available)
  if (song_id && (album_id || playlist_id))
    {
      if (opensubsonic_get_song_metadata_from_source(song_id, album_id, playlist_id, &title, &artist, &album, &duration) < 0)
        {
          DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to get song metadata for ID %s from source\n", song_id);
          // Continue with fallback values
        }
    }
  else if (song_id)
    {
      DPRINTF(E_DBG, L_OPENSUBSONIC, "No source info available for song ID %s, using fallback values\n", song_id);
    }
  
  if (!song_id)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to extract song ID from URI: %s\n", uri);
      return LIBRARY_PATH_INVALID;
    }
  
  // Get stream URL from opensubsonic backend
  stream_url = opensubsonic_stream_url_get(song_id);
  if (!stream_url)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to get stream URL for song ID: %s\n", song_id);
      free((char*)song_id);
      free(title);
      free(artist);
      free(album);
      if (album_id) free((char*)album_id);
      if (playlist_id) free((char*)playlist_id);
      return LIBRARY_ERROR;
    }

  // Create queue item with metadata
  memset(&qi, 0, sizeof(struct db_queue_item));
  
  qi.file_id = DB_MEDIA_FILE_NON_PERSISTENT_ID;
  qi.title = title ? title : safe_asprintf("Track %s", song_id);
  qi.artist = artist ? artist : strdup("OpenSubsonic");
  qi.album = album ? album : strdup("Remote Music");
  qi.data_kind = DATA_KIND_HTTP;
  qi.media_kind = MEDIA_KIND_MUSIC;
  qi.path = strdup(stream_url);
  qi.virtual_path = safe_asprintf("/%s", uri);
  qi.song_length = duration * 1000; // Convert to milliseconds

  ret = db_queue_add_start(&queue_add_info, position);
  if (ret < 0)
    goto error;

  ret = db_queue_add_next(&queue_add_info, &qi);
  ret = db_queue_add_end(&queue_add_info, reshuffle, item_id, ret);
  if (ret < 0)
    goto error;

  if (count)
    *count = queue_add_info.count;
  if (new_item_id)
    *new_item_id = queue_add_info.new_item_id;

  DPRINTF(E_DBG, L_OPENSUBSONIC, "Successfully added opensubsonic item to queue: %s\n", uri);

  free(stream_url);
  free_queue_item(&qi, 1);
  return LIBRARY_OK;

 error:
  free(stream_url);
  free_queue_item(&qi, 1);
  return LIBRARY_ERROR;
}

static int
opensubsonic_webapi_library_init(void)
{
  DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic webapi library source initialized\n");
  return 0;
}

static void
opensubsonic_webapi_library_deinit(void)
{
  DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic webapi library source deinitialized\n");
}

static int
opensubsonic_webapi_library_initscan(void)
{
  // No initial scan needed for opensubsonic
  return 0;
}

static int
opensubsonic_webapi_library_rescan(void)
{
  // No rescan needed for opensubsonic
  return 0;
}

static int
opensubsonic_webapi_library_metarescan(void)
{
  // No meta rescan needed for opensubsonic
  return 0;
}

static int
opensubsonic_webapi_library_fullrescan(void)
{
  // No full rescan needed for opensubsonic
  return 0;
}

struct library_source opensubsonic_webapi_library =
{
  .scan_kind = SCAN_KIND_OPENSUBSONIC,
  .disabled = 0,
  .init = opensubsonic_webapi_library_init,
  .deinit = opensubsonic_webapi_library_deinit,
  .initscan = opensubsonic_webapi_library_initscan,
  .rescan = opensubsonic_webapi_library_rescan,
  .metarescan = opensubsonic_webapi_library_metarescan,
  .fullrescan = opensubsonic_webapi_library_fullrescan,
  .queue_item_add = opensubsonic_webapi_library_queue_item_add,
};
