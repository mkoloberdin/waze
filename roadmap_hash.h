/* roadmap_hash.h - the API for the roadmap hash index.
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

#ifndef _ROADMAP_HASH__H_
#define _ROADMAP_HASH__H_

#ifndef J2ME
#define ROADMAP_HASH_MODULO 4093
#else
#define ROADMAP_HASH_MODULO 397
#endif

struct roadmap_hash_struct {

   const char *name;

   struct roadmap_hash_struct *next_hash;
   struct roadmap_hash_struct *prev_hash;

   int head[ROADMAP_HASH_MODULO];

   int    size;
   int   *next;
   void **values;

   /* Statistics: */
   int count_add_first;
   int count_add_next;
   int count_get_first;
   int count_get_next;

};

typedef struct roadmap_hash_struct RoadMapHash;


RoadMapHash *roadmap_hash_new (const char *name, int size);

void roadmap_hash_add       (RoadMapHash *hash, int key, int index);
int  roadmap_hash_get_first (RoadMapHash *hash, int key);
int  roadmap_hash_get_next  (RoadMapHash *hash, int index);
void roadmap_hash_resize    (RoadMapHash *hash, int size);
int  roadmap_hash_remove    (RoadMapHash *hash, int key, int index);

void roadmap_hash_free (RoadMapHash *hash);

void  roadmap_hash_set_value (RoadMapHash *hash, int index, void *value);
void *roadmap_hash_get_value (RoadMapHash *hash, int index);

void  roadmap_hash_summary (void);

int roadmap_hash_string (const char *str);

#endif // _ROADMAP_HASH__H_

