/* roadmap_hash.c - A simple hash index mechanism for RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   RoadMapHash *roadmap_hash_new (int size);
 *
 *   void roadmap_hash_add       (RoadMapHash *hash, int key, int index);
 *   int  roadmap_hash_get_first (RoadMapHash *hash, int key);
 *   int  roadmap_hash_get_next  (RoadMapHash *hash, int index);
 *   void roadmap_hash_resize    (RoadMapHash *hash, int size);
 *
 *   void  roadmap_hash_summary (void);
 *   void  roadmap_hash_reset   (void);
 *
 * These functions are used to build a hash index. The idea is to
 * accelerate scanning a BuildMap table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_hash.h"


static RoadMapHash *HashLast = NULL;


RoadMapHash *roadmap_hash_new (const char *name, int size) {

   int i;
   RoadMapHash *hash = malloc (sizeof(RoadMapHash));

   roadmap_check_allocated(hash);

   hash->name = name;

   for (i = 0; i < ROADMAP_HASH_MODULO; i++) {
      hash->head[i] = -1;
   }

   hash->size = size;
   hash->next = malloc (size * sizeof(int));
   hash->values = NULL;

   roadmap_check_allocated(hash->next);

   for (i = 0; i < size; i++) {
      hash->next[i] = -1;
   }

	if (HashLast) {
		HashLast->prev_hash = hash;
	}
   hash->next_hash = HashLast;
   hash->prev_hash = NULL;
   HashLast = hash;

   return hash;
}


void roadmap_hash_add (RoadMapHash *hash, int key, int index) {

   int hash_code = abs(key % ROADMAP_HASH_MODULO);

   if ((index < 0) || (index > hash->size)) {
      roadmap_log (ROADMAP_FATAL, "invalid index %d in hash table %s",
                         index, hash->name);
   }

   if (hash->head[hash_code] < 0) {
      hash->count_add_first += 1;
   } else {
      hash->count_add_next += 1;
   }

   hash->next[index] = hash->head[hash_code];
   hash->head[hash_code] = index;
}


int  roadmap_hash_get_first (RoadMapHash *hash, int key) {

   int hash_code = abs(key % ROADMAP_HASH_MODULO);

   hash->count_get_first += 1;

   return hash->head[hash_code];
}


int  roadmap_hash_get_next  (RoadMapHash *hash, int index) {

   if ((index < 0) || (index >= hash->size)) {
      roadmap_log (ROADMAP_FATAL, "invalid index %d in hash table %s",
                         index, hash->name);
   }

   hash->count_get_next += 1;

   return hash->next[index];
}


int	roadmap_hash_remove	(RoadMapHash *hash, int key, int index) {

   int hash_code = abs(key % ROADMAP_HASH_MODULO);
   int *slot;

   if ((index < 0) || (index > hash->size)) {
      roadmap_log (ROADMAP_FATAL, "invalid index %d in hash table %s",
                         index, hash->name);
   }

	slot = &(hash->head[hash_code]);
	
	while (*slot >= 0) {
	
		if (*slot == index) {

			*slot = hash->next[index];
			return 1;			
		}	
		slot = &(hash->next[*slot]);
	}

	return 0;	
}


void roadmap_hash_resize (RoadMapHash *hash, int size) {

   int i;

   hash->next = realloc (hash->next, size * sizeof(int));
   roadmap_check_allocated(hash->next);

   if (hash->values != NULL) {
      hash->values = realloc (hash->values, size * sizeof(void *));
      roadmap_check_allocated(hash->values);
   }

   for (i = hash->size; i < size; i++) {
      hash->next[i] = -1;
   }
   hash->size = size;
}


void roadmap_hash_free (RoadMapHash *hash) {

	RoadMapHash *prev = hash->prev_hash;
	RoadMapHash *next = hash->next_hash;

	if (hash == HashLast) {
		HashLast = hash->next_hash;
	}
	if (hash->prev_hash) {
		hash->prev_hash->next_hash = next;
	}
	if (hash->next_hash) {
		hash->next_hash->prev_hash = prev;
	}
	
   if (hash->values != NULL) {
      free (hash->values);
   }
   free (hash->next);
   free (hash);
}


void  roadmap_hash_set_value (RoadMapHash *hash, int index, void *value) {

   if ((index < 0) || (index > hash->size)) {
      roadmap_log (ROADMAP_FATAL, "invalid index %d in hash table %s",
                         index, hash->name);
   }

   if (hash->values == NULL) {
      hash->values = calloc (hash->size, sizeof(void *));
      roadmap_check_allocated(hash->values);
   }

   hash->values[index] = value;
}


void *roadmap_hash_get_value (RoadMapHash *hash, int index) {

   if ((index < 0) || (index > hash->size)) {
      roadmap_log (ROADMAP_FATAL, "invalid index %d in hash table %s",
                         index, hash->name);
   }

   if (hash->values == NULL) return NULL;

   return hash->values[index];
}


#ifndef J2ME
void  roadmap_hash_summary (void) {

   RoadMapHash *hash;

   for (hash = HashLast; hash != NULL; hash = hash->next_hash) {

      fprintf (stderr, "-- hash table %s:", hash->name);

      fprintf (stderr,
               "\n--      %d lists, %d items",
               hash->count_add_first,
               hash->count_add_first + hash->count_add_next);
      if (hash->count_add_first > 0) {
         fprintf (stderr,
                  " (%d items/list)",
                  (hash->count_add_first + hash->count_add_next)
                      / hash->count_add_first);
      }

      fprintf (stderr,
               "\n--      %d get first, %d get",
               hash->count_get_first,
               hash->count_get_first + hash->count_get_next);
      if (hash->count_get_first > 0) {
         fprintf (stderr,
                  " (%d loops/search)",
                  (hash->count_get_first + hash->count_get_next)
                      / hash->count_get_first);
      }
      fprintf (stderr, "\n");
   }
}

#endif


int roadmap_hash_string (const char *str) {

   int hash = 0;
   unsigned int i;
   
   for (i=0; i<strlen(str); i++) {
      hash += str[i]*(i+1);
   }

   return hash;
}

