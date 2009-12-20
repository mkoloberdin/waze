/* roadmap_gzm.c - Open and read a gzm compressed map file.
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
 *   Copyright 2008 Ehud Shabtai
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

#include <string.h>
#include <stdlib.h>

#ifdef RIMAPI
#include "rimapi.h"
#include <device_specific.h>
#elif defined (J2ME)
#include "zlib/zlib.h"
#include <device_specific.h>
#else
#include "zlib.h"
#endif

#include "roadmap_gzm.h"
#include "roadmap.h"
#include "roadmap_data_format.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_tile.h"

typedef struct gzm_file_s {

	char *name;
	roadmap_map_file_header header;
	roadmap_map_entry *index;
	int ref_count;
	
	RoadMapFile file;
	
} gzm_file;

#define MAX_GZM	3

static gzm_file GzmFile[MAX_GZM];
static int GzmMax = 0;

int roadmap_gzm_open (const char *name) {

	int id;
	const char *map_path;
	char *full_path;
	int index_size;
	
	/* look for already open file */
	for (id = 0; id < GzmMax; id++) {
		if (GzmFile[id].name && !strcmp (name, GzmFile[id].name)) {
			GzmFile[id].ref_count++;
			return id;
		}
	}
	
	/* find an empty slot */
	for (id = 0; id < GzmMax; id++) {
		if (!GzmFile[id].name) break;
	}
	if (id >= MAX_GZM) return -1;
	
	/* find the file in maps folders */
	map_path = roadmap_path_first ("maps");
	GzmFile[id].file = ROADMAP_INVALID_FILE;
	
	while (map_path && !ROADMAP_FILE_IS_VALID (GzmFile[id].file)) {
		
		full_path = roadmap_path_join (map_path, name);
		GzmFile[id].file = roadmap_file_open (full_path, "r");
		roadmap_path_free (full_path);

		map_path = roadmap_path_next ("maps", map_path);
	}
	
	if (!ROADMAP_FILE_IS_VALID (GzmFile[id].file)) {
		roadmap_log (ROADMAP_ERROR, "failed to open map file %s", name);
		return -1;
	}
	
	/* read and confirm file header */
	if (roadmap_file_read (GzmFile[id].file, &GzmFile[id].header, sizeof (roadmap_map_file_header)) !=  sizeof (roadmap_map_file_header)) {
		roadmap_log (ROADMAP_ERROR, "bad map file format of %s", name);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	if (memcmp (GzmFile[id].header.map_general_header.signature, ROADMAP_MAP_SIGNATURE, sizeof (GzmFile[id].header.map_general_header.signature))) {
		roadmap_log (ROADMAP_ERROR, "bad map file format of %s: invalid signature %.4s", name, GzmFile[id].header.map_general_header.signature);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	if (GzmFile[id].header.map_general_header.endianness != ROADMAP_DATA_ENDIAN_CORRECT) {
		roadmap_log (ROADMAP_ERROR, "bad map file format of %s: endianness is %x", name, GzmFile[id].header.map_general_header.endianness);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	if (GzmFile[id].header.map_general_header.version != ROADMAP_MAP_CURRENT_VERSION) {
		roadmap_log (ROADMAP_ERROR, "bad map file format of %s: version is %x", name, GzmFile[id].header.map_general_header.version);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	
	index_size = GzmFile[id].header.num_tiles * sizeof (roadmap_map_entry);
	GzmFile[id].index = malloc (index_size);

	if (roadmap_file_read (GzmFile[id].file, GzmFile[id].index, index_size) != index_size) {
		roadmap_log (ROADMAP_ERROR, "Cannot read index from map file %s", name);
		free (GzmFile[id].index);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	
	GzmFile[id].name = strdup (name);
	GzmFile[id].ref_count = 1;
	if (id >= GzmMax) GzmMax = id + 1;
	return id;
}


void roadmap_gzm_close (int gzm_id) {
	
	if (GzmFile[gzm_id].name) {
		if (--GzmFile[gzm_id].ref_count == 0) {
			if (ROADMAP_FILE_IS_VALID (GzmFile[gzm_id].file)) roadmap_file_close (GzmFile[gzm_id].file);
			if (GzmFile[gzm_id].index) free (GzmFile[gzm_id].index);
			if (GzmFile[gzm_id].name) free (GzmFile[gzm_id].name);
			GzmFile[gzm_id].name = NULL;
		}
	}
}


int roadmap_map_get_num_tiles (int gzm_id) {

	return GzmFile[gzm_id].header.num_tiles;	
}


int roadmap_map_get_tile_id (int gzm_id, int tile_no) {

	return GzmFile[gzm_id].index[tile_no].tile_id;	
}


static roadmap_map_entry *roadmap_gzm_locate_entry (int gzm_id, int tile_id) {

	int hi = GzmFile[gzm_id].header.num_tiles - 1;
	int lo = 0;
	roadmap_map_entry *index = GzmFile[gzm_id].index;
	int west, east, south, north;
	
	roadmap_tile_edges (tile_id, &west, &east, &south, &north);
	if (west > GzmFile[gzm_id].header.max_lon ||
		 east < GzmFile[gzm_id].header.min_lon ||
		 south > GzmFile[gzm_id].header.max_lat ||
		 north < GzmFile[gzm_id].header.min_lat) {
		return NULL;
	}
	
	while (hi >= lo) {
	
		int mid = (hi + lo) / 2;
		int dir = tile_id - index[mid].tile_id;
		
		if (dir < 0) {
			hi = mid - 1;
		} else if (dir > 0) {
			lo = mid + 1;
		} else {
			return index + mid;
		}
	}
	
	return NULL;
}


int roadmap_gzm_get_section (int gzm_id, int tile_id,
									  void **section, int *length) {
									  	
	roadmap_map_entry *entry = roadmap_gzm_locate_entry (gzm_id, tile_id);
	int success;
	unsigned long ul;
	roadmap_tile_file_header *tile;
	
	if (!entry) {
		roadmap_log (ROADMAP_INFO, "failed to find tile %d", tile_id);
		return -1;
	}

	ul = *length = entry->compressed_size + sizeof (roadmap_tile_file_header);

#if defined(J2ME) && defined(CIBYL_MEM_POOLS)
   tile = (roadmap_tile_file_header *)NOPH_DeviceSpecific_allocChunk(*length);
#else
	tile = malloc (*length);
#endif
	
	success = 
		roadmap_file_seek (GzmFile[gzm_id].file, entry->offset, ROADMAP_SEEK_START) > 0 &&
		roadmap_file_read (GzmFile[gzm_id].file, (tile + 1), entry->compressed_size) == (int)entry->compressed_size;

	if (!success) {
		roadmap_log (ROADMAP_ERROR, "failed to load tile %d", tile_id);

#if defined(J2ME) && defined(CIBYL_MEM_POOLS)
                NOPH_DeviceSpecific_freeChunk((int)tile);
#else
		free (tile);
#endif
		return -1;
	} 

	memcpy (&tile->general_header, &GzmFile[gzm_id].header.tile_general_header, sizeof (roadmap_data_file_header));
	tile->compressed_data_size = entry->compressed_size;
	tile->raw_data_size = entry->raw_size;	

	*section = (void *)tile;
	return 0;
}


void roadmap_gzm_free_section (int gzm_id, void *section) {

#if defined(J2ME) && defined(CIBYL_MEM_POOLS)
        NOPH_DeviceSpecific_freeChunk((int)section);
#else
	free (section);	
#endif
}

