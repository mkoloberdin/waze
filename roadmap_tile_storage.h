/* roadmap_tile_storage.h - persistency for tiles
 *
 * LICENSE:
 *
 *   Copyright 2009 Ehud Shabtai
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef ROADMAP_TILE_STORAGE_H_
#define ROADMAP_TILE_STORAGE_H_

#include <stdlib.h>
#include "roadmap.h"

typedef struct
{
	char full_path[512];
	size_t size;
	void* data;
	BOOL free_memory;
} TileContext;

typedef void (*roadmap_tile_enum_cb) (int tile_index);

int roadmap_tile_enumerate (int fips, roadmap_tile_enum_cb cb);

int roadmap_tile_store (int fips, int tile_index, void *data, size_t size);

void roadmap_tile_remove (int fips, int tile_index);

void roadmap_tile_remove_all ( int fips );

int roadmap_tile_store_context( TileContext* context );

int roadmap_tile_load (int fips, int tile_index, void **data, size_t *size);

#endif /*ROADMAP_TILE_STORAGE_H_*/
