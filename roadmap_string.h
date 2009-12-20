/* roadmap_string.h - A garbage-collected abstract type for strings.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef INCLUDED__ROADMAP_STRING__H
#define INCLUDED__ROADMAP_STRING__H


struct roadmap_string_descriptor;
typedef struct roadmap_string_descriptor *RoadMapDynamicString;

#define ROADMAP_STRING_COLLECTION_BLOCK  16

struct roadmap_string_collection {

   struct roadmap_string_collection *next;
   int count;

   RoadMapDynamicString element[ROADMAP_STRING_COLLECTION_BLOCK];
};
typedef struct roadmap_string_collection RoadMapDynamicStringCollection;

RoadMapDynamicString roadmap_string_new (const char *value);

RoadMapDynamicString roadmap_string_new_in_collection
                          (const char *value,
                           RoadMapDynamicStringCollection *collection);

void roadmap_string_lock (RoadMapDynamicString item);
void roadmap_string_release (RoadMapDynamicString item);
void roadmap_string_release_all (RoadMapDynamicStringCollection *collection);

const char *roadmap_string_get (RoadMapDynamicString item);

int roadmap_string_match (RoadMapDynamicString item, const char *value);

int roadmap_string_is_sub_ignore_case (const char *where, const char *what);
int roadmap_string_compare_ignore_case (const char *str1, const char *str2);
#endif // INCLUDED__ROADMAP_STRING__H

