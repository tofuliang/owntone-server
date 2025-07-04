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

#ifndef OPENSUBSONIC_SCANNER_H
#define OPENSUBSONIC_SCANNER_H

#include "library.h" // For struct library_source

extern struct library_source opensubsonic_scanner;

// Potentially other functions to be exposed by opensubsonic_scanner.c if needed,
// for example, functions that might be called directly by httpd_jsonapi.c
// for on-demand fetching if not going through the generic library_source paths.

#endif // OPENSUBSONIC_SCANNER_H
