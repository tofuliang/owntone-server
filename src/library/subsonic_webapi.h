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

#ifndef SRC_SUBSONIC_WEBAPI_H_
#define SRC_SUBSONIC_WEBAPI_H_

#include <event2/event.h>
#include <stdbool.h>

#include "http.h"
#include "library.h"

struct subsonicwebapi_status_info
{
  bool token_valid;
  char username[100];
  char server_url[256];
  char api_version[32];
  char granted_scope[250];
  char required_scope[250];
};

struct subsonicwebapi_auth_info
{
  char username[100];
  char password[100];
  char salt[32];
  char token[64];
  bool use_token_auth;
};

/* Library scanning functions */
int
subsonicwebapi_fullrescan(void);

int
subsonicwebapi_rescan(void);

void
subsonicwebapi_purge(void);

/* Status functions */
void
subsonicwebapi_status_info_get(struct subsonicwebapi_status_info *info);

void
subsonicwebapi_auth_info_get(struct subsonicwebapi_auth_info *info);

/* Authentication functions */
int
subsonicwebapi_login(const char *username, const char *password, const char *server_url);

void
subsonicwebapi_logout(void);

/* Artwork functions */
char *
subsonicwebapi_artwork_url_get(const char *id, int max_w, int max_h);

/* Library source */
extern struct library_source subsonicsource;

#endif /* SRC_SUBSONIC_WEBAPI_H_ */ 