/* roadmap_string.c - A garbage-collected abstract type for strings.
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_string.h"


#define ROADMAP_STRING_MODULO  253


struct roadmap_string_descriptor {

   /* The hash collision list: */
   RoadMapDynamicString next;
   RoadMapDynamicString previous;

   unsigned short lock;
   unsigned char  hash;
   char data[1];
};


static RoadMapDynamicString RoadMapStringHeads[ROADMAP_STRING_MODULO];


static int roadmap_string_hash (const char *value, int length) {

   int i;
   unsigned hash = (unsigned)(value[0]);

   for (i = length - 1; i > 0; --i) {
      hash = (9 * hash) + (unsigned)(value[i]);
   }

   return hash % ROADMAP_STRING_MODULO;
}


RoadMapDynamicString roadmap_string_new (const char *value) {

   int length = strlen(value);
   int hash   = roadmap_string_hash(value, length);

   RoadMapDynamicString item = RoadMapStringHeads[hash];


   while (item != NULL) {
      if (strcmp (value, item->data) == 0) break;
      item = item->next;
   }

   if (item != NULL) {

      if (item->lock < 0xffff) item->lock += 1;

   } else {

      int length = strlen(value);

      item = malloc (sizeof(struct roadmap_string_descriptor) + length);
      roadmap_check_allocated(item);

      item->previous = NULL;
      item->next = RoadMapStringHeads[hash];

      RoadMapStringHeads[hash] = item;
      if (item->next != NULL) {
         item->next->previous = item;
      }

      item->lock = 1;
      item->hash = hash;
      memcpy (item->data, value, length+1);
   }

   return item;
}


RoadMapDynamicString roadmap_string_new_in_collection
                          (const char *value,
                           RoadMapDynamicStringCollection *collection) {

   RoadMapDynamicString item = roadmap_string_new (value);

   while (collection->next != NULL) collection = collection->next;

   if (collection->count >= ROADMAP_STRING_COLLECTION_BLOCK) {

      collection->next = malloc (sizeof(RoadMapDynamicStringCollection));
      roadmap_check_allocated(collection->next);

      collection = collection->next;
      collection->count = 0;
   }

   collection->element[collection->count++] = item;

   return item;
}


void roadmap_string_lock (RoadMapDynamicString item) {

   if (item != NULL) {

      if (item->lock < 0xffff) item->lock += 1;
   }
}


void roadmap_string_release (RoadMapDynamicString item) {

   if (item != NULL) {

      if (item->lock < 0xffff) {
         
         if (--item->lock == 0) {

            if (item->previous != NULL) {
               item->previous->next = item->next;
            } else {
               RoadMapStringHeads[item->hash] = item->next;
            }

            if (item->next != NULL) {
               item->next->previous = item->previous;
            }
            free(item);
         }
      }
   }
}


void roadmap_string_release_all (RoadMapDynamicStringCollection *collection) {

   int i;
   RoadMapDynamicString *slot;
   RoadMapDynamicStringCollection *previous = NULL;
   RoadMapDynamicStringCollection *cursor = collection;

   while (cursor != NULL) {

      if (previous != NULL) free (previous);

      slot = cursor->element;
      for (i = cursor->count; i > 0; --i) {
         roadmap_string_release (*slot);
         ++slot;
      }
      previous = cursor;
      cursor = cursor->next;
   }

   /* The first collection block is statically allocated: we must reset it! */
   collection->count = 0;
   collection->next  = NULL;
}


const char *roadmap_string_get (RoadMapDynamicString item) {

   if (item == NULL) return NULL;

   return item->data;
}


int roadmap_string_match (RoadMapDynamicString item, const char *value) {

   if (item == NULL) return 0;

   return (strcmp (item->data, value) == 0);
}


int roadmap_string_is_sub_ignore_case (const char *where, const char *what) {

	const char *start;
	const char *find;
	
	if (!*what) return 1;
	
	for (start = where; *start; start++) {
		const char *curr = start;
		for (find = what; (*find) && *(curr); find++, curr++) {
			if (tolower(*find) != tolower (*curr)) break;
		} 
		if (!*find) return 1;
	}	
	
	return 0;
}


int roadmap_string_compare_ignore_case (const char *str1, const char *str2) {

	while (*str1 == *str2) {
	
		if (!*str1) return 0;
		str1++;
		str2++;	
	}	
	
	return (*str1) - (*str2);
}

