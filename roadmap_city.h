/* roadmap_city.h - city list and search tools
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

#ifndef _ROADMAP_CITY_H_
#define _ROADMAP_CITY_H_

#include "roadmap_dictionary.h"

struct RoadMapCityItem;
typedef struct RoadMapCityItem *RoadMapCityEntry;

typedef struct {
	int square_id;
	int city_id;
} RoadMapCityId;

void roadmap_city_init (void);
void roadmap_city_free (void);
int roadmap_city_add (const char *name, int square_id, int city_id);
void roadmap_city_remove (const char *name, int square_id);
int roadmap_city_find (const char *name);
const RoadMapCityId *roadmap_city_first (int index, RoadMapCityEntry *entry);
const RoadMapCityId *roadmap_city_next (int index, RoadMapCityEntry *entry);
int roadmap_city_count (void);
const char *roadmap_city_name (int index);
void roadmap_city_list_all (FILE *f);
void roadmap_city_unload (int square);
int roadmap_city_search (const char *str, RoadMapDictionaryCB cb, void *context);

int roadmap_city_write_file (const char *path, const char *name, int switch_endian);
int roadmap_city_read_file (const char *name);

#endif // _ROADMAP_CITY_H
