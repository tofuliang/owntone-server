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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <json-c/json.h>

#include "logger.h"
#include "misc.h"
#include "opensubsonic_cache.h"

// Global cache hash table
static struct opensubsonic_cache_entry *cache_table[OPENSUBSONIC_CACHE_HASH_SIZE];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool cache_initialized = false;

// Hash function for cache keys
static unsigned int
hash_key(const char *key)
{
  unsigned int hash = 5381;
  int c;
  
  while ((c = *key++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
    
  return hash % OPENSUBSONIC_CACHE_HASH_SIZE;
}

// Generate cache key based on type and id
static char *
make_cache_key(opensubsonic_cache_type_t type, const char *id)
{
  const char *type_str;
  
  switch (type) {
    case OPENSUBSONIC_CACHE_ALBUMS:
      type_str = "albums";
      break;
    case OPENSUBSONIC_CACHE_ALBUM:
      type_str = "album";
      break;
    case OPENSUBSONIC_CACHE_PLAYLISTS:
      type_str = "playlists";
      break;
    case OPENSUBSONIC_CACHE_PLAYLIST:
      type_str = "playlist";
      break;
    default:
      return NULL;
  }
  
  if (id && strlen(id) > 0)
    return safe_asprintf("%s:%s", type_str, id);
  else
    return safe_strdup(type_str);
}

// Get TTL for cache type
static time_t
get_ttl_for_type(opensubsonic_cache_type_t type)
{
  switch (type) {
    case OPENSUBSONIC_CACHE_ALBUMS:
      return OPENSUBSONIC_CACHE_TTL_ALBUMS;
    case OPENSUBSONIC_CACHE_ALBUM:
      return OPENSUBSONIC_CACHE_TTL_ALBUM;
    case OPENSUBSONIC_CACHE_PLAYLISTS:
      return OPENSUBSONIC_CACHE_TTL_PLAYLISTS;
    case OPENSUBSONIC_CACHE_PLAYLIST:
      return OPENSUBSONIC_CACHE_TTL_PLAYLIST;
    default:
      return 300; // Default 5 minutes
  }
}

// Find cache entry by key (must be called with cache_mutex held)
static struct opensubsonic_cache_entry *
find_cache_entry(const char *key)
{
  unsigned int hash = hash_key(key);
  struct opensubsonic_cache_entry *entry = cache_table[hash];
  
  while (entry) {
    if (strcmp(entry->key, key) == 0)
      return entry;
    entry = entry->next;
  }
  
  return NULL;
}

// Create new cache entry (must be called with cache_mutex held)
static struct opensubsonic_cache_entry *
create_cache_entry(const char *key, opensubsonic_cache_type_t type)
{
  struct opensubsonic_cache_entry *entry;
  unsigned int hash;
  
  entry = calloc(1, sizeof(struct opensubsonic_cache_entry));
  if (!entry)
    return NULL;
    
  entry->key = safe_strdup(key);
  entry->type = type;
  entry->json_string = NULL;
  entry->timestamp = time(NULL);
  entry->expires_at = entry->timestamp + get_ttl_for_type(type);
  entry->is_loading = false;
  
  if (pthread_mutex_init(&entry->mutex, NULL) != 0) {
    free(entry->key);
    free(entry);
    return NULL;
  }
  
  if (pthread_cond_init(&entry->cond, NULL) != 0) {
    pthread_mutex_destroy(&entry->mutex);
    free(entry->key);
    free(entry);
    return NULL;
  }
  
  // Add to hash table
  hash = hash_key(key);
  entry->next = cache_table[hash];
  cache_table[hash] = entry;
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Created cache entry for key: %s\n", key);
  
  return entry;
}

// Remove cache entry from hash table (must be called with cache_mutex held)
static void
remove_cache_entry(struct opensubsonic_cache_entry *entry)
{
  unsigned int hash = hash_key(entry->key);
  struct opensubsonic_cache_entry *current = cache_table[hash];
  struct opensubsonic_cache_entry *prev = NULL;
  
  while (current) {
    if (current == entry) {
      if (prev)
        prev->next = current->next;
      else
        cache_table[hash] = current->next;
      break;
    }
    prev = current;
    current = current->next;
  }
}

// Free cache entry (must be called with cache_mutex held and entry->ref_count == 0)
static void
free_cache_entry(struct opensubsonic_cache_entry *entry)
{
  if (!entry)
    return;
    
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Freeing cache entry for key: %s\n", entry->key);
  
  remove_cache_entry(entry);
  
  if (entry->json_string)
    free(entry->json_string);
    
  pthread_cond_destroy(&entry->cond);
  pthread_mutex_destroy(&entry->mutex);
  free(entry->key);
  free(entry);
}

// Initialize the cache system
int
opensubsonic_cache_init(void)
{
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  if (cache_initialized) {
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
    return 0;
  }
  
  memset(cache_table, 0, sizeof(cache_table));
  cache_initialized = true;
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic cache initialized\n");
  return 0;
}

// Cleanup the cache system
void
opensubsonic_cache_deinit(void)
{
  int i;
  struct opensubsonic_cache_entry *entry, *next;
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  if (!cache_initialized) {
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
    return;
  }
  
  // Free all cache entries
  for (i = 0; i < OPENSUBSONIC_CACHE_HASH_SIZE; i++) {
    entry = cache_table[i];
    while (entry) {
      next = entry->next;
      
      // Wait for any ongoing operations to complete
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
      while (entry->is_loading) {
        CHECK_ERR(L_OPENSUBSONIC, pthread_cond_wait(&entry->cond, &entry->mutex));
      }
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
      
      free_cache_entry(entry);
      entry = next;
    }
    cache_table[i] = NULL;
  }
  
  cache_initialized = false;
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic cache deinitialized\n");
}

// Get cached data or return NULL if not available/expired
json_object *
opensubsonic_cache_get(opensubsonic_cache_type_t type, const char *id)
{
  char *key;
  struct opensubsonic_cache_entry *entry;
  json_object *result = NULL;
  time_t now = time(NULL);
  
  if (!cache_initialized)
    return NULL;
    
  key = make_cache_key(type, id);
  if (!key)
    return NULL;
    
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  entry = find_cache_entry(key);
  if (!entry) {
    // Create new entry and mark as loading
    entry = create_cache_entry(key, type);
    if (entry) {
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
      entry->is_loading = true;
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
    }
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
    
    free(key);
    return NULL; // Caller should load data and call opensubsonic_cache_put
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  // Wait if another thread is loading this entry
  while (entry->is_loading) {
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Waiting for cache entry to be loaded: %s\n", key);
    CHECK_ERR(L_OPENSUBSONIC, pthread_cond_wait(&entry->cond, &entry->mutex));
  }
  
  // Check if entry is expired
  if (now >= entry->expires_at) {
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache entry expired: %s\n", key);
    
    // Mark as loading again
    entry->is_loading = true;
    CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
    
    free(key);
    return NULL; // Caller should load fresh data
  }
  
  // Return cached data
  if (entry->json_string) {
    // Parse JSON string to create new object for caller
    result = json_tokener_parse(entry->json_string);
    DPRINTF(E_DBG, L_OPENSUBSONIC, "Cache hit for key: %s\n", key);
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
  
  free(key);
  return result;
}

// Store data in cache
void
opensubsonic_cache_put(opensubsonic_cache_type_t type, const char *id, json_object *data)
{
  char *key;
  struct opensubsonic_cache_entry *entry;
  time_t now = time(NULL);
  
  if (!cache_initialized)
    return;
    
  key = make_cache_key(type, id);
  if (!key)
    return;
    
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  entry = find_cache_entry(key);
  if (!entry) {
    // This shouldn't happen if cache_get was called first
    entry = create_cache_entry(key, type);
    if (!entry) {
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
      free(key);
      return;
    }
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  // Update entry data
  if (entry->json_string)
    free(entry->json_string);
    
  entry->json_string = data ? strdup(json_object_to_json_string(data)) : NULL; // Store JSON string
  entry->timestamp = now;
  entry->expires_at = now + get_ttl_for_type(type);
  entry->is_loading = false;
  
  // Wake up waiting threads
  CHECK_ERR(L_OPENSUBSONIC, pthread_cond_broadcast(&entry->cond));
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Cached data for key: %s\n", key);
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
  
  free(key);
}

// Clear all cache entries
void
opensubsonic_cache_clear(void)
{
  int i;
  struct opensubsonic_cache_entry *entry, *next;
  
  if (!cache_initialized)
    return;
    
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  for (i = 0; i < OPENSUBSONIC_CACHE_HASH_SIZE; i++) {
    entry = cache_table[i];
    while (entry) {
      next = entry->next;
      
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
      
      // Wait for ongoing operations
      while (entry->is_loading) {
        CHECK_ERR(L_OPENSUBSONIC, pthread_cond_wait(&entry->cond, &entry->mutex));
      }
      
      CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
      
      free_cache_entry(entry);
      entry = next;
    }
    cache_table[i] = NULL;
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic cache cleared\n");
}

// Clear cache entries of specific type
void
opensubsonic_cache_clear_type(opensubsonic_cache_type_t type)
{
  int i;
  struct opensubsonic_cache_entry *entry, *next, *prev;
  
  if (!cache_initialized)
    return;
    
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  for (i = 0; i < OPENSUBSONIC_CACHE_HASH_SIZE; i++) {
    prev = NULL;
    entry = cache_table[i];
    
    while (entry) {
      next = entry->next;
      
      if (entry->type == type) {
        CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
        
        // Wait for ongoing operations
        while (entry->is_loading) {
          CHECK_ERR(L_OPENSUBSONIC, pthread_cond_wait(&entry->cond, &entry->mutex));
        }
        
        CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
        
        // Remove from chain
        if (prev)
          prev->next = next;
        else
          cache_table[i] = next;
          
        free_cache_entry(entry);
      } else {
        prev = entry;
      }
      
      entry = next;
    }
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Cleared cache entries of type: %d\n", type);
}

// Remove expired entries
void
opensubsonic_cache_cleanup_expired(void)
{
  int i;
  struct opensubsonic_cache_entry *entry, *next, *prev;
  time_t now = time(NULL);
  
  if (!cache_initialized)
    return;
    
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&cache_mutex));
  
  for (i = 0; i < OPENSUBSONIC_CACHE_HASH_SIZE; i++) {
    prev = NULL;
    entry = cache_table[i];
    
    while (entry) {
      next = entry->next;
      
      if (now >= entry->expires_at) {
        CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_lock(&entry->mutex));
        
        // Only remove if not in use
        if (!entry->is_loading) {
          CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
          
          // Remove from chain
          if (prev)
            prev->next = next;
          else
            cache_table[i] = next;
            
          free_cache_entry(entry);
        } else {
          CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&entry->mutex));
          prev = entry;
        }
      } else {
        prev = entry;
      }
      
      entry = next;
    }
  }
  
  CHECK_ERR(L_OPENSUBSONIC, pthread_mutex_unlock(&cache_mutex));
}