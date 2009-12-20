/* roadmap_tile_status.c - Manage tile cache.
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik.
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_tile_status.h"

#include "roadmap.h"
#include "roadmap_hash.h"
#include "roadmap_square.h"

#define	TS_BLOCK_SIZE	4096
#define	TS_MAX_BLOCKS	16
#define	TS_MAX_SIZE		(TS_BLOCK_SIZE * TS_MAX_BLOCKS)

typedef struct {
	
	int tile_index;
	int status;
} TileStatus;
	

static int	NumTiles = 0;

static TileStatus *Tiles[TS_MAX_BLOCKS];
static RoadMapHash *TileHash = NULL;

static TileStatus *tile_status (int position) {
	
	assert (position >= 0 && position < NumTiles);
	return Tiles[position / TS_BLOCK_SIZE] + (position % TS_BLOCK_SIZE);
}

static int *roadmap_tile_status_add (int index) {
	
	TileStatus *tile;
	
	if (NumTiles >= TS_MAX_SIZE) {
		roadmap_log (ROADMAP_ERROR, "Tile queue is full");
		return NULL;
	}
	
	if (NumTiles % TS_BLOCK_SIZE == 0) {
		Tiles[NumTiles / TS_BLOCK_SIZE] = (TileStatus *)malloc (TS_BLOCK_SIZE * sizeof (TileStatus));
		if (Tiles[NumTiles / TS_BLOCK_SIZE] == NULL) {
			roadmap_log (ROADMAP_ERROR, "Failed allocation for tile queue");
			return NULL;
		}
		if (TileHash == NULL) {
			TileHash = roadmap_hash_new ("tile_loader", NumTiles + TS_BLOCK_SIZE);	
		} else {
			roadmap_hash_resize (TileHash, NumTiles + TS_BLOCK_SIZE); 
		}	
	}
	
	tile = Tiles[NumTiles / TS_BLOCK_SIZE] + NumTiles % TS_BLOCK_SIZE;
	tile->status = 0;
	tile->tile_index = index;
	
	roadmap_hash_add (TileHash, index, NumTiles);
	
	NumTiles++;
	return &tile->status;
}

int *roadmap_tile_status_get (int index) {

	TileStatus *tile;
	int i;
	
	if (NumTiles == 0) {
		return roadmap_tile_status_add (index);
	}
	
	i = roadmap_hash_get_first (TileHash, index);
	while (i >= 0) {
		tile = tile_status (i);
		if (tile->tile_index == index) {
			return &tile->status;
		}
		i = roadmap_hash_get_next (TileHash, i);	
	}
	
	return roadmap_tile_status_add (index);
}

