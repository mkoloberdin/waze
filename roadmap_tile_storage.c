/* roadmap_tile_storage.c - Tile persistency.
 *
 * LICENSE:
 *
 *   Copyright 2009 Ehud Shabtai.
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_tile_storage.h"
#include "roadmap_thread.h"
#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"


typedef enum
{
	_async_store_none = 0,
	_async_store_same_thread,
	_async_store_separate_thread
} RMAsyncStoreType;

#ifdef __SYMBIAN32__
RMAsyncStoreType sgAsyncStoreType = _async_store_same_thread;
#else
RMAsyncStoreType sgAsyncStoreType = _async_store_none;
#endif

static const char * get_tile_filename (int fips, int tile_index, int create_path) {

   const char *map_path = roadmap_db_map_path ();
   char name[30];
   char path[512];
   static char filename[512];

   if (tile_index == -1) {
      /* Global square id */

      const char *suffix = "index";
      static char name[512];

      snprintf (name, sizeof (name), "%05d_%s%s", fips, suffix,
            ROADMAP_DATA_TYPE);
	   roadmap_path_format (filename, sizeof (filename), map_path, name);
      return filename;
   }

#ifdef J2ME
   snprintf (filename, sizeof (filename), "recordstore://map%05d.%d:1", fips, tile_index);
#else
	snprintf (path, sizeof (path), "%05d", fips);
	roadmap_path_format (path, sizeof (path), map_path, path);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", tile_index >> 24);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", (tile_index >> 16) & 255);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", (tile_index >> 8) & 255);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%05d_%08x%s", fips, tile_index,
         ROADMAP_DATA_TYPE);
	roadmap_path_format (filename, sizeof (filename), path, name);
#endif

   return filename;
}


int roadmap_tile_store( int fips, int tile_index, void *data, size_t size )
{
	int res = 0;
    char thread_name[RM_THREAD_MAX_THREAD_NAME];
	const char* full_path = get_tile_filename( fips, tile_index, 0 );
	TileContext *ctx = NULL;
	if ( sgAsyncStoreType != _async_store_none )
	{
		BOOL separate_thread = ( sgAsyncStoreType == _async_store_separate_thread );
		/*
		 * Prepare context. Duplicate data to prevent syncronization
		 */
	    ctx = malloc ( sizeof( TileContext ) );
		ctx->data = malloc( size );
		memcpy( ctx->data, data, size );
		ctx->free_memory = TRUE;
		strncpy( ctx->full_path, full_path, sizeof( ctx->full_path ) );
		ctx->size = size;

		/*
		 * Unique thread name
		 */
		snprintf( thread_name, RM_THREAD_MAX_THREAD_NAME, "TileStorageThread %d", tile_index );
		/* Run it up ... */
		if ( roadmap_thread_run( (RMThreadFunc) roadmap_tile_store_context, ctx, _priority_idle, thread_name, separate_thread ) == FALSE )
		{
			res = -1;
		}
	}
	else
	{
		/*
		 * Prepare context - just fill the fields
		 */
		TileContext stack_context;
		ctx = &stack_context;
		ctx->data = data;
		ctx->free_memory = FALSE;
		strncpy( ctx->full_path, full_path, sizeof( ctx->full_path ) );
		ctx->size = size;

		res = roadmap_tile_store_context( ctx );
	}
	return res;
}

/*
 * This function must be thread safe in order to be used as a thread body!
 * Please check OS specific implementations before using it
 */
int roadmap_tile_store_context( TileContext* context )
{
   int res = 0;
   RoadMapFile file = NULL;
   char *parent = roadmap_path_parent( context->full_path, NULL );
   roadmap_path_create( parent );
   free(parent);

   file = roadmap_file_open( context->full_path, "w" );

   if ( ROADMAP_FILE_IS_VALID( file ) )
   {
	   res = (roadmap_file_write( file, context->data, context->size ) != (int) context->size );
       roadmap_file_close(file);
   }
   else
   {
      res = -1;
      if ( sgAsyncStoreType != _async_store_separate_thread )
	  {
		  /* Preserve thread safety - roadmap_log still not thread safe */
    	  roadmap_log( ROADMAP_ERROR, "Can't save tile data to %s", context->full_path );
	  }
   }
   /*
    * Frees it up
    */
   if ( context->free_memory )
   {
		free( context->data );
		free( context );
   }
   return res;
}

void roadmap_tile_remove (int fips, int tile_index) {

   roadmap_file_remove(NULL, get_tile_filename(fips, tile_index, 0));
}

int roadmap_tile_load (int fips, int tile_index, void **base, size_t *size) {

   RoadMapFile		file;
   int				res;

   const char		*full_name = get_tile_filename(fips, tile_index, 0);

   file = roadmap_file_open (full_name, "r");

   if (!ROADMAP_FILE_IS_VALID(file)) {
      return -1;
   }

#ifdef J2ME
   *size = favail(file);
#else
   *size = roadmap_file_length (NULL, full_name);
#endif
   *base = malloc (*size);

	   res = roadmap_file_read (file, *base, *size);
	   roadmap_file_close (file);

   if (res != (int)*size) {
      free (*base);
      return -1;
   }

   return 0;
}


