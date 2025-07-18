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

#ifndef SRC_OPENSUBSONIC_CACHE_H_
#define SRC_OPENSUBSONIC_CACHE_H_

#include <json-c/json.h>
#include <pthread.h>
#include <time.h>

// Cache entry types
typedef enum {
  OPENSUBSONIC_CACHE_ALBUMS,
  OPENSUBSONIC_CACHE_ALBUM,
  OPENSUBSONIC_CACHE_PLAYLISTS,
  OPENSUBSONIC_CACHE_PLAYLIST
} opensubsonic_cache_type_t;

// Cache entry structure
struct opensubsonic_cache_entry {
  char *key;                           // Cache key (e.g., "albums" or "album:123")
  opensubsonic_cache_type_t type;      // Type of cached data
  char *json_string;                   // Cached JSON response as string
  time_t timestamp;                    // When the entry was cached
  time_t expires_at;                   // When the entry expires
  pthread_mutex_t mutex;               // Mutex for this entry
  pthread_cond_t cond;                 // Condition variable for waiting threads
  bool is_loading;                     // True if data is currently being loaded
  struct opensubsonic_cache_entry *next; // Next entry in hash table chain
};

// Cache configuration
#define OPENSUBSONIC_CACHE_TTL_ALBUMS     300    // 5 minutes for albums list
#define OPENSUBSONIC_CACHE_TTL_ALBUM      600    // 10 minutes for individual album
#define OPENSUBSONIC_CACHE_TTL_PLAYLISTS  180    // 3 minutes for playlists list
#define OPENSUBSONIC_CACHE_TTL_PLAYLIST   600    // 10 minutes for individual playlist
#define OPENSUBSONIC_CACHE_HASH_SIZE      256    // Hash table size

// Initialize the cache system
int opensubsonic_cache_init(void);

// Cleanup the cache system
void opensubsonic_cache_deinit(void);

// Get cached data or trigger loading if not available
// Returns cached json_object* or NULL if not available
// If data is being loaded by another thread, this will wait
json_object *opensubsonic_cache_get(opensubsonic_cache_type_t type, const char *id);

// Store data in cache
void opensubsonic_cache_put(opensubsonic_cache_type_t type, const char *id, json_object *data);

// Clear all cache entries
void opensubsonic_cache_clear(void);

// Clear cache entries of specific type
void opensubsonic_cache_clear_type(opensubsonic_cache_type_t type);

// Remove expired entries
void opensubsonic_cache_cleanup_expired(void);

#endif /* SRC_OPENSUBSONIC_CACHE_H_ */