/*
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
#include <stdint.h>
#include <string.h>

#include "logger.h"
#include "opensubsonic.h"

// OpenSubsonic backend implementation - similar to Spotify's abstraction
// Currently only one backend, but keeping the abstraction for future extensibility

extern struct opensubsonic_backend opensubsonic_webapi;

static struct opensubsonic_backend *
backend_set(void)
{
  return &opensubsonic_webapi;
}


/* -------------- Dispatches functions exposed via opensubsonic.h ----------- */
/*             Called from other threads than the input thread                */

int
opensubsonic_init(void)
{
  struct opensubsonic_backend *backend = backend_set();

  if (!backend || !backend->init)
    return 0; // Just a no-op

  return backend->init();
}

void
opensubsonic_deinit(void)
{
  struct opensubsonic_backend *backend = backend_set();

  if (!backend || !backend->deinit)
    return;

  backend->deinit();
}

// Note: opensubsonic_connect and opensubsonic_disconnect are implemented in opensubsonic_webapi.c

void
opensubsonic_status_get(struct opensubsonic_status *status)
{
  struct opensubsonic_backend *backend = backend_set();

  memset(status, 0, sizeof(struct opensubsonic_status));

  if (!backend || !backend->status_get)
    return;

  backend->status_get(status);
}