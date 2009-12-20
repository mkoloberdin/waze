/* roadmap_res.c - Resources manager (Bitmap, voices, etc')
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See roadmap_res.h
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "roadmap_canvas.h"
#include "roadmap_sound.h"
#include "roadmap_hash.h"
#include "roadmap_list.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_prompts.h"

#include "roadmap_res.h"

#define BLOCK_SIZE 300

const char *ResourceName[] = {
   "bitmap_res",
   "sound_res"
};

struct resource_slot {
   char *name;
   void *data;
};

typedef struct roadmap_resource {
   RoadMapHash *hash;
   struct resource_slot *slots;
   int count;
   int max;
   int used_mem;
   int max_mem;
} RoadMapResource;


static RoadMapResource Resources[MAX_RESOURCES];

static void allocate_resource (unsigned int type) {
   RoadMapResource *res = &Resources[type];

   res->hash = roadmap_hash_new (ResourceName[type], BLOCK_SIZE);

   res->slots = malloc (BLOCK_SIZE * sizeof (*res->slots));
   res->max = BLOCK_SIZE;
}


static void *load_resource (unsigned int type, unsigned int flags,
                            const char *name, int *mem) {

   const char *cursor;
   void *data = NULL;

   if (flags & RES_SKIN) {
      for (cursor = roadmap_path_first ("skin");
            cursor != NULL;
            cursor = roadmap_path_next ("skin", cursor)) {
         switch (type) {
            case RES_BITMAP:
               *mem = 0;

               data = roadmap_canvas_load_image (cursor, name);
               break;
            case RES_SOUND:
               data = roadmap_sound_load (cursor, name, mem);
               break;
         }

         if (data) break;
      }

   } else {

      const char *user_path = roadmap_path_user ();
      char path[512];
      switch (type) {
         case RES_BITMAP:
            *mem = 0;
            roadmap_path_format (path, sizeof (path), user_path, "icons");
            data = roadmap_canvas_load_image (path, name);
            break;
         case RES_SOUND:
            roadmap_path_format (path, sizeof (path), roadmap_path_downloads(), "sound");
            roadmap_path_format (path, sizeof (path), path, roadmap_prompts_get_name());
            data = roadmap_sound_load (path, name, mem);
            break;
      }
   }

   return data;
}


static void free_resource (unsigned int type, int slot) {

   void *data = Resources[type].slots[slot].data;

   if (data) {
      switch (type) {
         case RES_BITMAP:
            roadmap_canvas_free_image ((RoadMapImage)data);
            break;
         case RES_SOUND:
            roadmap_sound_free ((RoadMapSound)data);
            break;
      }
   }

   free (Resources[type].slots[slot].name);
}


static void *find_resource (unsigned int type, const char *name, int* found) {
   int hash;
   int i;
   RoadMapResource *res = &Resources[type];

   *found = FALSE;

   if (!res->count) return NULL;

   hash = roadmap_hash_string (name);

   for (i = roadmap_hash_get_first (res->hash, hash);
        i >= 0;
        i = roadmap_hash_get_next (res->hash, i)) {

      if (!strcmp(name, res->slots[i].name)) {
         *found = TRUE;
         return res->slots[i].data;
      }
   }

   return NULL;
}


void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name) {

   void *data = NULL;
   int mem;
   RoadMapResource *res = &Resources[type];

   if (name == NULL)
   	return NULL;

   if (! (flags & RES_NOCACHE)) {
      int found;
      data = find_resource (type, name, &found);

      if (found) return data;
      if (Resources[type].slots == NULL) allocate_resource (type);

      //TODO implement grow (or old deletion)
      if (Resources[type].count == Resources[type].max){
       	roadmap_log (ROADMAP_ERROR, "roadmap_res_get:%s table is full",name);
      	return NULL;
      }
   }

   if (flags & RES_NOCREATE) return NULL;

   switch (type) {
   case RES_BITMAP:
      if ( strchr (name, '.') )
      {
     	 if ( !data )
     	 {
     		 data = load_resource( type, flags, name, &mem );
     	 }
      }
      else
      {
    	 char *full_name = malloc (strlen (name) + 5);
    	 data = NULL;
    	 /* Try PNG */
    	 if ( !data )
    	 {
    		 sprintf( full_name, "%s.png", name );
    		 data = load_resource (type, flags, full_name, &mem);
    	 }
    	 /* Try BIN */
         if ( !data )
         {
            sprintf( full_name, "%s.bin", name );
            data = load_resource (type, flags, full_name, &mem);
         }

         free (full_name);
      }
      break;

   default:
      data = load_resource (type, flags, name, &mem);
   }

   if (!data) {
   	if (type != RES_SOUND)
   		roadmap_log (ROADMAP_ERROR, "roadmap_res_get - resource %s type=%d not found.", name, type);
   	return NULL;
   }
   if (flags & RES_NOCACHE) return data;

   res->slots[res->count].data = data;
   res->slots[res->count].name = strdup(name);

   res->used_mem += mem;

   roadmap_hash_add (res->hash, roadmap_hash_string (name), res->count);
   res->count++;

   return data;
}


void roadmap_res_shutdown (void) {
   int type;

   for (type=0; type<MAX_RESOURCES; type++) {
      RoadMapResource *res = &Resources[type];
      int i;

      for (i=0; i<Resources[type].count; i++) {

         free_resource (type, i);
      }
      
      if (res->slots)
         free(res->slots);
   }
   
}


