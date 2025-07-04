/*
 * OpenSubsonic scanner integration
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

#include "opensubsonic_scanner.h"
#include "opensubsonic_api.h" // For API calls and structs
#include "logger.h"
#include "db.h"             // For db_queue_add_start etc.
#include "library.h"        // For LIBRARY_OK, etc.
#include "misc.h"           // For safe_strdup, etc.
#include "misc_json.h"      // For jparse_*, json_object types

// Helper to parse OpenSubsonic URI. Example: opensubsonic:song:123
// Returns type (e.g., "song", "album") and id.
// Caller must free *type and *id if not NULL.
static int parse_opensubsonic_uri(const char *uri_str, char **type, char **id) {
    if (!uri_str || !type || !id) return -1;

    *type = NULL;
    *id = NULL;

    if (strncmp(uri_str, "opensubsonic:", 13) != 0) {
        return -1; // Not an OpenSubsonic URI
    }

    const char *p = uri_str + 13;
    const char *colon1 = strchr(p, ':');
    if (!colon1) return -1; // Malformed

    *type = strndup(p, colon1 - p);
    if (!*type) return -1;

    p = colon1 + 1;
    *id = strdup(p);
    if (!*id) {
        free(*type);
        *type = NULL;
        return -1;
    }
    return 0;
}


static int opensubsonic_scanner_init(void) {
    opensubsonic_api_init(); // Initializes the API client and loads config
    if (!opensubsonic_api_is_enabled()) {
        DPRINTF(E_INFO, L_SCAN, "OpenSubsonic scanner disabled by configuration.\n");
        // Set the scanner to disabled so it's not used by library functions
        opensubsonic_scanner.disabled = 1;
        return 0; // Not an error, just disabled
    }
    opensubsonic_scanner.disabled = 0;

    const char *errmsg = NULL;
    if (opensubsonic_api_ping(&errmsg) != 0) {
        DPRINTF(E_WARN, L_SCAN, "OpenSubsonic server ping failed: %s. Scanner will be disabled.\n", errmsg ? errmsg : "Unknown error");
        // opensubsonic_scanner.disabled = 1; // Optionally disable if ping fails at init
    } else {
        DPRINTF(E_INFO, L_SCAN, "OpenSubsonic scanner initialized and server ping successful.\n");
    }
    return 0;
}

static void opensubsonic_scanner_deinit(void) {
    opensubsonic_api_deinit();
    DPRINTF(E_INFO, L_SCAN, "OpenSubsonic scanner deinitialized.\n");
}

// Minimal implementation for scan functions as we don't store the library locally.
// They can be used to refresh configuration or check server status if needed.
static int opensubsonic_scanner_initscan(void) {
    DPRINTF(E_DBG, L_SCAN, "OpenSubsonic initscan called.\n");
    if (!opensubsonic_api_is_enabled()) return 0;
    // Could potentially do a ping or fetch basic server info
    return 0;
}

static int opensubsonic_scanner_rescan(void) {
    DPRINTF(E_DBG, L_SCAN, "OpenSubsonic rescan called.\n");
    if (!opensubsonic_api_is_enabled()) return 0;
    // Reload config in case it changed via API, or re-ping
    opensubsonic_api_load_config();
    const char *errmsg = NULL;
    opensubsonic_api_ping(&errmsg); // Check connectivity
    return 0;
}

static int opensubsonic_scanner_metarescan(void) {
    DPRINTF(E_DBG, L_SCAN, "OpenSubsonic metarescan called.\n");
    // Similar to rescan, could refresh config or check server status
    return opensubsonic_scanner_rescan();
}

static int opensubsonic_scanner_fullrescan(void) {
    DPRINTF(E_DBG, L_SCAN, "OpenSubsonic fullrescan called.\n");
    // Similar to rescan
    return opensubsonic_scanner_rescan();
}


static int opensubsonic_scanner_queue_item_add(const char *uri_str, int position, char reshuffle, uint32_t item_id, int *count, int *new_item_id) {
    if (!opensubsonic_api_is_enabled()) {
        return LIBRARY_SERVICE_UNAVAILABLE;
    }

    char *type = NULL;
    char *id = NULL;
    int ret = LIBRARY_ERROR;

    if (parse_opensubsonic_uri(uri_str, &type, &id) != 0) {
        DPRINTF(E_WARN, L_SCAN, "Failed to parse OpenSubsonic URI: %s\n", uri_str);
        return LIBRARY_PATH_INVALID;
    }

    DPRINTF(E_DBG, L_SCAN, "Queue add request for OpenSubsonic item: type='%s', id='%s'\n", type, id);

    if (strcmp(type, "song") == 0) {
        struct opensubsonic_track os_track;
        const char *fetch_errmsg = NULL;
        // Use the new public API function to fetch and parse song details
        if (opensubsonic_api_fetch_and_parse_song_details(id, &os_track, &fetch_errmsg) == 0) {
            struct db_queue_item queue_item = {0};
            if (opensubsonic_track_to_queue_item(&os_track, &queue_item) == 0) {
                struct db_queue_add_info queue_add_info;
                ret = db_queue_add_start(&queue_add_info, position);
                if (ret >= 0) {
                    ret = db_queue_add_next(&queue_add_info, &queue_item);
                    if (ret >= 0) {
                         if (new_item_id) *new_item_id = ret; // db_queue_add_next returns new item id
                         ret = db_queue_add_end(&queue_add_info, reshuffle, item_id, 1); // 1 item added
                         if (ret >= 0) {
                            if (count) *count = 1;
                            ret = LIBRARY_OK;
                         } else {
                            DPRINTF(E_LOG, L_SCAN, "Failed to finalize adding OpenSubsonic track to queue.\n");
                            ret = LIBRARY_ERROR;
                         }
                    } else {
                        DPRINTF(E_LOG, L_SCAN, "Failed to add OpenSubsonic track to queue.\n");
                        ret = LIBRARY_ERROR;
                    }
                } else {
                     DPRINTF(E_LOG, L_SCAN, "Failed to start adding OpenSubsonic track to queue.\n");
                     ret = LIBRARY_ERROR;
                }
                free_queue_item(&queue_item, 1); // Free contents if strduped
            } else {
                DPRINTF(E_LOG, L_SCAN, "Failed to convert OpenSubsonic track to queue item.\n");
                ret = LIBRARY_ERROR;
            }
            opensubsonic_free_track(&os_track);
        } else {
            DPRINTF(E_LOG, L_SCAN, "Failed to fetch OpenSubsonic track details for ID: %s\n", id);
            ret = LIBRARY_ERROR;
        }
    } else if (strcmp(type, "album") == 0) {
        // TODO: Implement adding all tracks from an album
        struct opensubsonic_album_with_entries album_data;
        const char *fetch_album_errmsg = NULL;
        memset(&album_data, 0, sizeof(struct opensubsonic_album_with_entries));

        if (opensubsonic_api_get_parsed_album(id, &album_data, &fetch_album_errmsg) == 0) {
            if (album_data.num_songs > 0) {
                struct db_queue_add_info queue_add_info;
                ret = db_queue_add_start(&queue_add_info, position);
                if (ret >= 0) {
                    int items_added_count = 0;
                    for (int i = 0; i < album_data.num_songs; i++) {
                        struct db_queue_item temp_queue_item = {0};
                        // Ensure album tracks have necessary info, or enrich if needed
                        // For now, assume songs in album_data.songs are complete enough
                        if (opensubsonic_track_to_queue_item(&album_data.songs[i], &temp_queue_item) == 0) {
                            int current_new_id = db_queue_add_next(&queue_add_info, &temp_queue_item);
                             if (current_new_id >= 0) {
                                items_added_count++;
                                if (new_item_id && *new_item_id == 0 && items_added_count == 1) {
                                     *new_item_id = current_new_id;
                                }
                            } else {
                                DPRINTF(E_WARN, L_SCAN, "Failed to add track %s from album %s to queue.\n", album_data.songs[i].id, id);
                            }
                            free_queue_item(&temp_queue_item, 1);
                        }
                    }
                    ret = db_queue_add_end(&queue_add_info, reshuffle, item_id, items_added_count);
                    if (ret >= 0) {
                        if (count) *count = items_added_count;
                        ret = LIBRARY_OK;
                    } else {
                         DPRINTF(E_LOG, L_SCAN, "Failed to finalize adding OpenSubsonic album %s to queue.\n", id);
                         ret = LIBRARY_ERROR;
                    }
                } else {
                     DPRINTF(E_LOG, L_SCAN, "Failed to start adding OpenSubsonic album %s to queue.\n", id);
                     ret = LIBRARY_ERROR;
                }
            } else {
                DPRINTF(E_INFO, L_SCAN, "OpenSubsonic album %s is empty or has no tracks listed.\n", id);
                if(count) *count = 0;
                if(new_item_id) *new_item_id = 0;
                ret = LIBRARY_OK;
            }
            opensubsonic_free_album_with_entries(&album_data);
        } else {
            DPRINTF(E_LOG, L_SCAN, "Failed to fetch/parse OpenSubsonic album %s: %s\n", id, fetch_album_errmsg ? fetch_album_errmsg : "Unknown error");
            ret = LIBRARY_ERROR;
        }
    } else if (strcmp(type, "playlist") == 0) {
        struct opensubsonic_playlist_with_entries playlist_data;
        const char *fetch_errmsg = NULL;
        memset(&playlist_data, 0, sizeof(struct opensubsonic_playlist_with_entries));

        if (opensubsonic_api_get_parsed_playlist(id, &playlist_data, &fetch_errmsg) == 0) {
            if (playlist_data.num_entries > 0) {
                struct db_queue_add_info queue_add_info;
                ret = db_queue_add_start(&queue_add_info, position);
                if (ret >= 0) {
                    int items_added_count = 0;
                    for (int i = 0; i < playlist_data.num_entries; i++) {
                        struct db_queue_item temp_queue_item = {0};
                        if (opensubsonic_track_to_queue_item(&playlist_data.entries[i], &temp_queue_item) == 0) {
                            int current_new_id = db_queue_add_next(&queue_add_info, &temp_queue_item);
                            if (current_new_id >= 0) {
                                items_added_count++;
                                if (new_item_id && *new_item_id == 0 && items_added_count == 1) {
                                     *new_item_id = current_new_id;
                                }
                            } else {
                                DPRINTF(E_WARN, L_SCAN, "Failed to add track %s from playlist %s to queue.\n", playlist_data.entries[i].id, id);
                            }
                            free_queue_item(&temp_queue_item, 1);
                        }
                    }
                    ret = db_queue_add_end(&queue_add_info, reshuffle, item_id, items_added_count);
                    if (ret >= 0) {
                        if (count) *count = items_added_count;
                        // new_item_id is set on the first successful add inside the loop
                        ret = LIBRARY_OK;
                    } else {
                         DPRINTF(E_LOG, L_SCAN, "Failed to finalize adding OpenSubsonic playlist %s to queue.\n", id);
                         ret = LIBRARY_ERROR;
                    }
                } else {
                    DPRINTF(E_LOG, L_SCAN, "Failed to start adding OpenSubsonic playlist %s to queue.\n", id);
                    ret = LIBRARY_ERROR;
                }
            } else {
                DPRINTF(E_INFO, L_SCAN, "OpenSubsonic playlist %s is empty.\n", id);
                if(count) *count = 0;
                if(new_item_id) *new_item_id = 0; // No items added
                ret = LIBRARY_OK; // Empty playlist is not an error
            }
            opensubsonic_free_playlist_with_entries(&playlist_data);
        } else {
            DPRINTF(E_LOG, L_SCAN, "Failed to fetch/parse OpenSubsonic playlist %s: %s\n", id, fetch_errmsg ? fetch_errmsg : "Unknown error");
            ret = LIBRARY_ERROR;
        }
    } else {
        DPRINTF(E_WARN, L_SCAN, "Unsupported OpenSubsonic URI type: %s\n", type);
        ret = LIBRARY_PATH_INVALID;
    }

    free(type);
    free(id);
    return ret;
}


struct library_source opensubsonic_scanner = {
    .scan_kind = SCAN_KIND_OPENSUBSONIC, // Needs to be defined in db.h
    .disabled = 1, // Initially disabled, enabled by init if config is okay
    .name = "OpenSubsonic", // Name for logging or UI if ever displayed
    .init = opensubsonic_scanner_init,
    .deinit = opensubsonic_scanner_deinit,
    .initscan = opensubsonic_scanner_initscan,
    .rescan = opensubsonic_scanner_rescan,
    .metarescan = opensubsonic_scanner_metarescan,
    .fullrescan = opensubsonic_scanner_fullrescan,
    .queue_item_add = opensubsonic_scanner_queue_item_add,
    // .artwork_get = opensubsonic_scanner_artwork_get, // Optional: if we need specific artwork handling
};
