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

#define RES_CACHE_SIZE 50

const char *ResourceName[] = {
   "bitmap_res",
   "sound_res"
};

struct resource_slot {
   char *name;
   void *data;
   unsigned int flags;
};

typedef struct resource_cache_entry {
	int key;
	int prev;
	int next;
} ResCacheEntry;


typedef struct roadmap_resource {
   RoadMapHash *hash;

   ResCacheEntry cache[RES_CACHE_SIZE];
   int cache_head;
   int res_type;
   struct resource_slot slots[RES_CACHE_SIZE];
   int count;
   int max;
   int used_mem;
   int max_mem;
} RoadMapResource;


static RoadMapResource Resources[MAX_RESOURCES];
static void roadmap_res_cache_init( RoadMapResource* res );
static int roadmap_res_cache_add( RoadMapResource* res, int hash_key );
static void roadmap_res_cache_set_MRU( RoadMapResource* res, int slot );

static void dbg_cache( RoadMapResource* res, int slot, const char* name );

static void allocate_resource (unsigned int type) {
   RoadMapResource *res = &Resources[type];
   res->res_type = type;

   res->hash = roadmap_hash_new ( ResourceName[type], RES_CACHE_SIZE );

   roadmap_res_cache_init( res );

   res->max = RES_CACHE_SIZE;

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


static void free_resource ( RoadMapResource* res, int slot) {

   void *data = res->slots[slot].data;

   if ( data ) {
      switch ( res->res_type )
      {
         case RES_BITMAP:
            roadmap_canvas_free_image ((RoadMapImage)data);
            break;
         case RES_SOUND:
            roadmap_sound_free ((RoadMapSound)data);
            break;
      }
   }

   free ( res->slots[slot].name);
}


static int find_resource (unsigned int type, const char *name ) {
   int hash;
   int i;
   RoadMapResource *res = &Resources[type];

   if (!res->count) return -1;

   hash = roadmap_hash_string (name);

   for (i = roadmap_hash_get_first (res->hash, hash);
        i >= 0;
        i = roadmap_hash_get_next (res->hash, i)) {

      if (!strcmp(name, res->slots[i].name)) {
         return i;
      }
   }

   return -1;
}


void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name) {

   void *data = NULL;
   int mem;
   RoadMapResource *res = &Resources[type];
   int slot;

   if (name == NULL)
   	return NULL;

   if (Resources[type].hash == NULL) allocate_resource (type);

   if (! (flags & RES_NOCACHE))
   {
      int slot;
      slot = find_resource ( type, name );

      if ( slot > 0 )
	  {
    	  roadmap_res_cache_set_MRU( res, slot );
    	  data = res->slots[slot].data;
    	  return data;
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
#ifdef ANDROID
    	 /* Try BIN */
         if ( !data )
         {
            sprintf( full_name, "%s.bin", name );
            data = load_resource (type, flags, full_name, &mem);
         }
#endif
    	 /* Try PNG */
    	 if ( !data )
    	 {
    		 sprintf( full_name, "%s.png", name );
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

   if ( flags & RES_NOCACHE )
   {
	   if ( type == RES_BITMAP )
	   {
		   roadmap_canvas_unmanaged_list_add( data );
	   }
	   return data;
   }

   slot = roadmap_res_cache_add( res, roadmap_hash_string( name ) );

   roadmap_log( ROADMAP_DEBUG, "Placing the resource at Slot: %d, Flags: %d, ", slot, flags );

   res->slots[slot].data = data;
   res->slots[slot].name = strdup(name);
   res->slots[slot].flags = flags;

   res->used_mem += mem;

   return data;
}

static void roadmap_res_cache_init( RoadMapResource* res )
{
	int i;

	for( i = 1; i < RES_CACHE_SIZE; ++i )
	{
		res->cache[i].key = -1;
		res->cache[i].prev = -1;
		res->cache[i].next = -1;
	}

	res->cache_head = 0;
	res->cache[0].prev = 0;
	res->cache[0].next = 0;

}

static void roadmap_res_cache_set_MRU( RoadMapResource* res, int slot )
{
	ResCacheEntry* cache = res->cache;

	int prev = cache[slot].prev;
	int next = cache[slot].next;
	int last = cache[res->cache_head].prev;

	// Already the head - nothing to do
	if ( slot == res->cache_head )
		return;


	// Set the MRU to be the head
	if ( last != slot )	// If the last element the head and tail are self defined
	{
		cache[slot].prev = cache[res->cache_head].prev;
		cache[slot].next = res->cache_head;
		cache[last].next = slot;

		// Fix the connections
		if ( prev > 0 )
			cache[prev].next = next;
		if ( next > 0 )
			cache[next].prev = prev;

	}

	// Set the current head to be the next
	cache[res->cache_head].prev = slot;

	// Update head pointer
	res->cache_head = slot;

}


static int roadmap_res_cache_add( RoadMapResource* res, int hash_key )
{
	ResCacheEntry* cache = res->cache;
	int slot;


	/*
	 * If there is still available slots just add
	 */

	if ( res->count < RES_CACHE_SIZE  )
	{
		slot = res->count;
		res->count++;
	}
	else
	{
	    /*
	     * Remove and deallocate the LRU element in the cache
	     */
		int non_locked_lru = cache[res->cache_head].prev;
		while ( res->slots[non_locked_lru].flags & RES_LOCK )
		{
			non_locked_lru = cache[non_locked_lru].prev;
		}
		if ( non_locked_lru == res->cache_head )
		{
			roadmap_log( ROADMAP_ERROR, "Cannot find non-locked resource!!! Removing the locked LRU" );
			non_locked_lru = cache[res->cache_head].prev;
			dbg_cache( res, non_locked_lru, "" );
		}

		slot = non_locked_lru;

		roadmap_hash_remove( res->hash, cache[slot].key, slot );
		free_resource( res, slot );
	}

	cache[slot].key = hash_key;
	roadmap_hash_add( res->hash, hash_key, slot );

	/*
	 * Set the element to be MRU
	 */
	if ( res->count > 1 )
	{
		/* For the only element nothing to do */
		roadmap_res_cache_set_MRU( res, slot );
	}
	return slot;
}


void roadmap_res_initialize( void )
{
	int i;

	for( i = 0; i < MAX_RESOURCES; ++i )
	{
		roadmap_res_cache_init( &Resources[i] );
	}
}

void roadmap_res_shutdown ()
{
   int type;
   RoadMapResource *res;
   int i;
   for ( type=0; type<MAX_RESOURCES; type++ )
   {
      res = &Resources[type];
      for ( i=0; i<Resources[type].count; i++ )
      {
         free_resource ( res, i );
      }
      Resources[type].count = 0;
      if ( Resources[type].hash != NULL )
      {
    	  roadmap_hash_free( Resources[type].hash );
      }
   }
}
void roadmap_res_invalidate( int type )
{
   RoadMapResource *res = &Resources[type];
   int i;

   switch( type )
   {
	   case RES_BITMAP:
	   {
		   for ( i = 0; i < Resources[type].count; ++i )
		   {
			   roadmap_canvas_image_invalidate( res->slots[i].data );
		   }
		   break;
	   }
	   default:
	   {
		   roadmap_log( ROADMAP_WARNING, "No appropriate invalidation procedure for resource type: %d", type );
		   break;
	   }
   }	// switch
}



static void dbg_cache( RoadMapResource* res, int slot, const char* name )
{
	int i;
	int next = res->cache_head;
	ResCacheEntry* cache = res->cache;

	roadmap_log( ROADMAP_WARNING, "The cache size exceed - deallocating slot %d. Name %s. Adding: %s", slot, res->slots[slot].name, name );
	for( i = 0; i < RES_CACHE_SIZE; ++i )
	{
		roadmap_log_raw_data_fmt( "Cache snapshot: %d: %d, %s \n", i, next, res->slots[next].name );
		next = cache[next].next;
	}
}
