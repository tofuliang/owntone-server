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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

#include <event2/event.h>
#include <event2/buffer.h>

#include "input.h"
#include "logger.h"
#include "misc.h"
#include "http.h"
#include "opensubsonic.h"
#include "library/opensubsonic_webapi.h"

struct opensubsonic_session
{
  struct http_client_session session;
  char *stream_url;
  bool is_streaming;
};

static int
setup(struct input_source *source)
{
  struct opensubsonic_session *os_session;
  char *stream_url;
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Setting up OpenSubsonic input for '%s'\n", source->path);
  
  // Extract song ID from OpenSubsonic URI (format: os:trackId[:a:albumId|:p:playlistId])
  if (strncmp(source->path, "os:", 3) != 0)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Invalid OpenSubsonic URI format: %s\n", source->path);
      return -1;
    }

  // Parse the URI to extract track ID
  char *uri_copy = strdup(source->path + 3); // Skip "os:"
  char *saveptr;
  char *track_id = strtok_r(uri_copy, ":", &saveptr);
  
  if (!track_id)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to extract track ID from URI: %s\n", source->path);
      free(uri_copy);
      return -1;
    }

  stream_url = opensubsonic_stream_url_get(track_id);
  free(uri_copy);
  if (!stream_url)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "Could not get stream URL for: %s\n", source->path);
      return -1;
    }
  
  CHECK_NULL(L_OPENSUBSONIC, os_session = calloc(1, sizeof(struct opensubsonic_session)));
  
  os_session->stream_url = stream_url;
  os_session->is_streaming = false;
  
  http_client_session_init(&os_session->session);
  
  source->input_ctx = os_session;
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "OpenSubsonic input setup complete for '%s'\n", source->path);
  
  return 0;
}

static int
play(struct input_source *source)
{
  struct opensubsonic_session *os_session = source->input_ctx;
  struct http_client_ctx ctx;
  int ret;
  
  if (!os_session)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic session not initialized\n");
      return -1;
    }
  
  if (!os_session->is_streaming)
    {
      // Start streaming
      memset(&ctx, 0, sizeof(struct http_client_ctx));
      ctx.url = os_session->stream_url;
      ctx.input_body = evbuffer_new();
      
      DPRINTF(E_DBG, L_OPENSUBSONIC, "Starting OpenSubsonic stream: %s\n", os_session->stream_url);
      
      ret = http_client_request(&ctx, &os_session->session);
      if (ret < 0)
        {
          DPRINTF(E_LOG, L_OPENSUBSONIC, "Failed to start OpenSubsonic stream\n");
          evbuffer_free(ctx.input_body);
          return -1;
        }
      
      os_session->is_streaming = true;
      
      // For now, we'll just return the raw stream data
      // In a full implementation, this would need to be decoded to PCM
      if (evbuffer_get_length(ctx.input_body) > 0)
        {
          // Set up a basic quality - this should be determined from the stream
          source->quality.sample_rate = 44100;
          source->quality.bits_per_sample = 16;
          source->quality.channels = 2;
          
          ret = input_write(ctx.input_body, &source->quality, 0);
          if (ret < 0)
            {
              DPRINTF(E_LOG, L_OPENSUBSONIC, "Error writing OpenSubsonic stream data\n");
              evbuffer_free(ctx.input_body);
              return -1;
            }
        }
      
      evbuffer_free(ctx.input_body);
    }
  
  // Continue reading from stream
  // This is a simplified implementation - a full version would handle
  // continuous streaming, buffering, and proper audio decoding
  
  return 0;
}

static int
stop(struct input_source *source)
{
  struct opensubsonic_session *os_session = source->input_ctx;
  
  if (!os_session)
    return 0;
  
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Stopping OpenSubsonic input for '%s'\n", source->path);
  
  http_client_session_deinit(&os_session->session);
  
  free(os_session->stream_url);
  free(os_session);
  
  source->input_ctx = NULL;
  
  return 0;
}

static int
seek(struct input_source *source, int seek_ms)
{
  // OpenSubsonic seeking would require special handling
  // For now, we don't support seeking
  DPRINTF(E_DBG, L_OPENSUBSONIC, "Seek not supported for OpenSubsonic streams\n");
  return 0;
}

static int
metadata_get(struct input_metadata *metadata, struct input_source *source)
{
  // Metadata would be retrieved from OpenSubsonic API
  // For now, return empty metadata
  return 0;
}

static int
init(void)
{
  struct opensubsonic_status_info info;
  int ret;
  
  DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic input init() called\n");
  
  // First initialize the OpenSubsonic backend
  ret = opensubsonic_init();
  if (ret < 0)
    {
      DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic backend initialization failed\n");
      return -1;
    }
  
  // Check status after initialization
  opensubsonic_status_info_get(&info);
  
  if (!info.enabled)
    {
      DPRINTF(E_INFO, L_OPENSUBSONIC, "OpenSubsonic input disabled in configuration\n");
      return -1;
    }
  
  DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic input initialized - enabled: %s, connected: %s\n",
          info.enabled ? "true" : "false", info.connected ? "true" : "false");
  
  return 0;
}

static void
deinit(void)
{
  DPRINTF(E_LOG, L_OPENSUBSONIC, "OpenSubsonic input deinit() called\n");
  opensubsonic_deinit();
  DPRINTF(E_DBG, L_OPENSUBSONIC, "OpenSubsonic input deinitialized\n");
}

struct input_definition input_opensubsonic =
{
  .name = "opensubsonic",
  .type = INPUT_TYPE_OPENSUBSONIC,
  .disabled = 0,
  .setup = setup,
  .play = play,
  .stop = stop,
  .seek = seek,
  .metadata_get = metadata_get,
  .init = init,
  .deinit = deinit,
};