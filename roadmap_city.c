/* roadmap_city.c - city list and search tools
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
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "roadmap.h"
#include "roadmap_list.h"
#include "roadmap_hash.h"
#include "roadmap_dictionary.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_string.h"

#include "roadmap_city.h"

typedef struct {
	RoadMapList list;
	char *name;
} RoadMapCityData;

struct RoadMapCityItem {
	RoadMapListItem current;
	RoadMapCityId id;
	int cell_id;
};

#define CITY_LIST_SIZE		4096

static RoadMapHash *RoadMapCityHash = NULL;
static int RoadMapCityCount = 0;
static int RoadMapCityChanged = 0;

void roadmap_city_init (void) {

	roadmap_city_free ();
	RoadMapCityCount =  0;
	RoadMapCityHash = roadmap_hash_new ("city_list", CITY_LIST_SIZE);
	RoadMapCityChanged = 1; 
}



void roadmap_city_free (void) {

	int i;
	RoadMapListItem *ptr;
	RoadMapListItem *tmp;
	
	if (RoadMapCityHash) {
		for (i = 0; i < RoadMapCityCount; i++) {
			RoadMapCityData *city = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, i);
			if (city) {
				ROADMAP_LIST_FOR_EACH (&city->list, ptr, tmp) {
					roadmap_list_remove (ptr);
					free (ptr);
				}
				free (city->name);
				free (city);
			}
		}
		roadmap_hash_free (RoadMapCityHash);
		RoadMapCityHash = NULL;	
	}
	RoadMapCityCount = 0;	
}


static int roadmap_city_hash_key (const char *name) {
	
	int key = 0;
	
	while (*name) {
		key = key * 256 + *name++;
	}
	
	return key;
}


int roadmap_city_add (const char *name, int square_id, int city_id) {
	
	int index = roadmap_city_find (name);
	RoadMapCityEntry ptr;
	RoadMapCityData *data;
	
	if (index == -1) {
		if (RoadMapCityCount && (RoadMapCityCount % CITY_LIST_SIZE == 0)) {
			roadmap_hash_resize (RoadMapCityHash, RoadMapCityCount + CITY_LIST_SIZE);			
		}
		index = RoadMapCityCount++;
		data = malloc (sizeof (RoadMapCityData));
		data->name = strdup (name);
		ROADMAP_LIST_INIT (&data->list);
		roadmap_hash_add (RoadMapCityHash, roadmap_city_hash_key (name), index);
		roadmap_hash_set_value (RoadMapCityHash, index, data);
		RoadMapCityChanged++;
	} else {
		RoadMapListItem *item;
		RoadMapListItem *tmp;
		
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		ROADMAP_LIST_FOR_EACH (&data->list, item, tmp) {
			ptr = (RoadMapCityEntry)item;
			if (ptr->id.square_id == square_id) {
				if (ptr->id.city_id != city_id) {
					ptr->id.city_id = city_id;
					RoadMapCityChanged++;
				}
				return index;
			}
		}
	}
	
	ptr = malloc (sizeof (struct RoadMapCityItem));
	ptr->id.square_id = square_id;
	ptr->id.city_id = city_id;
	ptr->cell_id = index;
	
	roadmap_list_append (&data->list, &ptr->current);
	RoadMapCityChanged++;
	
	return index;
}


void roadmap_city_remove (const char *name, int square_id) {
	
	int index = roadmap_city_find (name);
	RoadMapListItem *ptr;
	RoadMapListItem *tmp;
	RoadMapCityData *data;
	
	if (index == -1) {
		return;
	}
	
	data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
	ROADMAP_LIST_FOR_EACH (&data->list, ptr, tmp) {
		if (((RoadMapCityEntry)ptr)->id.square_id == square_id) {
			roadmap_list_remove (ptr);
			free (ptr);
			RoadMapCityChanged++;
			break;
		}
	}
}

int roadmap_city_find (const char *name) {
	
	int key;
	int index;
	RoadMapCityData *data;
	
	if (!name) return -1;
	
	if (!*name) return -1;
	
	key = roadmap_city_hash_key (name);
	index = roadmap_hash_get_first (RoadMapCityHash, key);
	
	while (index != -1) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		if (!strcmp (data->name, name)) break;
		index = roadmap_hash_get_next (RoadMapCityHash, index);
	}
	
	return index;
}


const RoadMapCityId *roadmap_city_first (int index, RoadMapCityEntry *entry) {
	
	int cell_id = index;
	RoadMapCityData *data;
	
	if (index == -1) cell_id = 0;
		
	if (cell_id < 0 || cell_id >= RoadMapCityCount) {
		return NULL;
	}
	
	while (index == -1 && cell_id < RoadMapCityCount) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, cell_id);
		if (!ROADMAP_LIST_EMPTY (&data->list)) break;
		cell_id++;
	}
	
	if (cell_id >= RoadMapCityCount) {
		return NULL;
	}
	data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, cell_id);
	if (ROADMAP_LIST_EMPTY (&data->list)) return NULL;
	
	*entry = (RoadMapCityEntry) ROADMAP_LIST_FIRST (&data->list);	
	if (!*entry || 
		 ((RoadMapListItem *)(*entry) == &data->list)) {
		return NULL;	
	}

	return &(*entry)->id;
}

const RoadMapCityId *roadmap_city_next (int index, RoadMapCityEntry *entry) {

	int cell_id;
	RoadMapCityData *data;
	
	if (!*entry) {
		return NULL;	
	}

	cell_id = (*entry)->cell_id;
	data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, cell_id);
	
	*entry = (RoadMapCityEntry)ROADMAP_LIST_NEXT (&(*entry)->current);	
	
	if ((RoadMapListItem *)(*entry) == &data->list) {
		if (index >= 0) return NULL;
		
		while (index == -1 && ++cell_id < RoadMapCityCount) {
			data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, cell_id);
			if (!ROADMAP_LIST_EMPTY (&data->list)) break;
		} 
		
		if (cell_id == RoadMapCityCount) return NULL; 
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, cell_id);
		*entry = (RoadMapCityEntry)ROADMAP_LIST_FIRST (&data->list);	
	}

	
	return &(*entry)->id;
}

int roadmap_city_count (void) {

	return RoadMapCityCount;
}

const char *roadmap_city_name (int index) {
	
	return ((RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index))->name;
}

void roadmap_city_list_all (FILE *f) {

	int index;
	RoadMapCityEntry entry;
	const RoadMapCityId *id;
	RoadMapCityData *data;
	
	fprintf (f, "====CITY LIST====\n");
	for (index = 0; index < RoadMapCityCount; index++) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		if (data->name) {
			fprintf (f, "City name is %s:\n", data->name);
			id = roadmap_city_first (index, &entry);
			while (id) {
				fprintf (f, "   Sq = %d ID = %d\n", id->square_id, id->city_id);
				id = roadmap_city_next (index, &entry);
			}
		}
	}	
}

void roadmap_city_unload (int square) {

	int index;
	RoadMapListItem *ptr;
	RoadMapListItem *tmp;
	RoadMapCityData *data;
	
	for (index = 0; index < RoadMapCityCount; index++) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		if (data->name) {
			ROADMAP_LIST_FOR_EACH (&data->list, ptr, tmp) {
				if (((RoadMapCityEntry)ptr)->id.square_id == square) {
					roadmap_list_remove (ptr);
					free (ptr);
					RoadMapCityChanged++;
				}
			}
		}
	}
}


int roadmap_city_search (const char *str, RoadMapDictionaryCB cb, void *context) {
	
	int index;
	int count = 0;
	RoadMapCityData *data;
	
	for (index = 0; index < RoadMapCityCount; index++) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		if (data->name &&
		    (!str || roadmap_string_is_sub_ignore_case (data->name, str))) {
		   count++;
		   if (cb) {
		   	if (!cb (index, data->name, context)) {
		   		break;
		   	}
		   }
		}
	}	
	
	return count;
}


static int roadmap_city_write_int (RoadMapFile file, int val, int switch_endian) {

	if (switch_endian) {
	   unsigned char *b = (unsigned char *) &val;
	   val = (unsigned int) (b[0]<<24 | b[1]<<16 | b[2]<<8 | b[3]);
	}
	
	return roadmap_file_write (file, &val, sizeof (int));	
}


int roadmap_city_write_file (const char *path, const char *name, int switch_endian) {
	
	RoadMapFile file;
	int index;
	RoadMapCityEntry entry;
	const RoadMapCityId *id;
	RoadMapCityData *data;
	int length;
	int count;
	char *full_name;
	
	if (!RoadMapCityChanged) return 0;
	
	if (path) {
		full_name = roadmap_path_join (path, name);
	} else {
		full_name = roadmap_path_join (roadmap_db_map_path(), name);
	}
	file = roadmap_file_open (full_name, "w");
	roadmap_path_free (full_name);
	
	if (!ROADMAP_FILE_IS_VALID (file)) return -1;
	
	roadmap_file_write (file, &RoadMapCityCount, sizeof (int));
	for (index = 0; index < RoadMapCityCount; index++) {
		data = (RoadMapCityData *)roadmap_hash_get_value (RoadMapCityHash, index);
		if (data->name) {
			length = strlen (data->name);
			roadmap_city_write_int (file, length, switch_endian); 
			roadmap_file_write (file, data->name, length);
			count = 0;
			id = roadmap_city_first (index, &entry);
			while (id) {
				count++;
				id = roadmap_city_next (index, &entry);
			}
			roadmap_city_write_int (file, count, switch_endian); 
			id = roadmap_city_first (index, &entry);
			while (id) {
				roadmap_city_write_int (file, id->square_id, switch_endian); 
				roadmap_city_write_int (file, id->city_id, switch_endian); 
				id = roadmap_city_next (index, &entry);
			}
		} else {
			length = 0;
			count = 0;
			roadmap_file_write (file, &length, sizeof (int)); 
			roadmap_file_write (file, &count, sizeof (int)); 
		}
	}
	
	roadmap_file_close (file);
	RoadMapCityChanged = 0;	
	return 0;
}


int roadmap_city_read_file (const char *name) {

	const char *map_path;
	char *full_path;
	RoadMapFile file;
	int num_entries;
	int length;
	int count;
	RoadMapCityId id;
	char *city = NULL;
	int rc = -1;

	/* make sure index is clean */
	roadmap_city_init ();
	
	/* find the file in maps folders */
	map_path = roadmap_path_first ("maps");
	file = ROADMAP_INVALID_FILE;
	
	while (map_path && !ROADMAP_FILE_IS_VALID (file)) {
		
		full_path = roadmap_path_join (map_path, name);
		file = roadmap_file_open (full_path, "r");
		roadmap_path_free (full_path);

		map_path = roadmap_path_next ("maps", map_path);
	}
	
	if (!ROADMAP_FILE_IS_VALID (file)) {
		roadmap_log (ROADMAP_INFO, "failed to open city index file %s", name);
		goto exit;
	}
	
	if (!roadmap_file_read (file, &num_entries, sizeof (int))) goto exit;
	
	while (num_entries-- > 0) {
		if (!roadmap_file_read (file, &length, sizeof (int))) goto exit;
		city = (char *)malloc (length + 1);
		if (roadmap_file_read (file, city, length) != length) goto exit;
		city[length] = '\0';
		
		if (!roadmap_file_read (file, &count, sizeof (int))) goto exit;
		while (count-- > 0) {
			if (!roadmap_file_read (file, &id, sizeof (RoadMapCityId))) goto exit;
			roadmap_city_add (city, id.square_id, id.city_id);
		}
		free (city);
		city = NULL;
	}

	RoadMapCityChanged = 0;
	rc = 0;
		
exit:
	if (city) free (city);
	if (ROADMAP_FILE_IS_VALID (file)) roadmap_file_close (file);
	return rc;
}

