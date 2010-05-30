/* roadmap_dbread.c - a module to read a roadmap database.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * SYNOPSYS:
 *
 *   #include "roadmap_dbread.h"
 *
 *   roadmap_db_model *roadmap_db_register
 *          (char *section, roadmap_db_handler *handler);
 *
 *   int  roadmap_db_open (char *name, roadmap_db_model *model);
 *
 *   void roadmap_db_activate (char *name);
 *
 *   roadmap_db *roadmap_db_get_subsection (roadmap_db *parent, char *path);
 *
 *   roadmap_db *roadmap_db_get_first (roadmap_db *parent);
 *   char       *roadmap_db_get_name  (roadmap_db *section);
 *   unsigned    roadmap_db_get_size  (roadmap_db *section);
 *   int         roadmap_db_get_count (roadmap_db *section);
 *   char       *roadmap_db_get_data  (roadmap_db *section);
 *   roadmap_db *roadmap_db_get_next  (roadmap_db *section);
 *
 *   void roadmap_db_close (char *name);
 *   void roadmap_db_end   (void);
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "roadmap_file.h"

#ifdef RIMAPI
#include "rimapi.h"
#include <device_specific.h>
#elif defined (J2ME)
#include "zlib/zlib.h"
#include <device_specific.h>
#else
#include "zlib/zlib.h"
#endif

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_data_format.h"
#include "roadmap_tile_storage.h"
#include "roadmap_dbread.h"

#ifdef IPHONE
#include "roadmap_main.h"
#endif //IPHONE

typedef struct roadmap_db_context_s {

   roadmap_db_model	*model;
   void					*handler_context;

   struct roadmap_db_context_s *next;
} roadmap_db_context;


typedef struct roadmap_db_database_s {

   int fips;
   int tile_index;

   roadmap_db_data_file data;

   struct roadmap_db_database_s *next;
   struct roadmap_db_database_s *previous;

   roadmap_db_model 		*model;
   roadmap_db_context	*context;

} roadmap_db_database;

static roadmap_db_database *RoadmapDatabaseFirst  = NULL;


static unsigned int roadmap_db_aligned_offset (const roadmap_db_data_file *data, unsigned int unaligned_offset) {

	return (unaligned_offset + data->byte_alignment_add) & data->byte_alignment_mask;
}

static unsigned int roadmap_db_entry_offset (const roadmap_db_data_file *data, unsigned int entry_id) {

	if (entry_id > 0)
		return roadmap_db_aligned_offset (data, data->index[entry_id - 1].end_offset);
	else
		return 0;
}

static unsigned int roadmap_db_size (const roadmap_db_data_file *data, unsigned int from, unsigned int to) {

	return data->index[to].end_offset - roadmap_db_entry_offset (data, from);
}

static unsigned int roadmap_db_sector_size (const roadmap_db_data_file *data, const roadmap_db_sector *sector) {

	return roadmap_db_size (data, sector->first, sector->last);	
}

static unsigned int roadmap_db_entry_size (const roadmap_db_data_file *data, unsigned int entry_id) {

	return roadmap_db_size (data, entry_id, entry_id);	
}


static int roadmap_db_call_map (roadmap_db_database *database) {

   roadmap_db_model *model;
   int res = 1;
   
   for (model = database->model;
   	  model != NULL;
   	  model = model->next) {
   
  		void *handler_context = NULL;
		roadmap_db_context *context;
  		
   	if (roadmap_db_sector_size (&(database->data), &model->sector)) {
   	   		
   		handler_context = model->handler->map (&database->data);
   		if (!handler_context) res = 0;
   	}
   	
		context = (roadmap_db_context *) malloc (sizeof (roadmap_db_context));	
		roadmap_check_allocated (context);
			
		context->model = model;
		context->next = database->context;
		context->handler_context = handler_context;
	
                database->context = context;
   }

   return res;
}


static void roadmap_db_call_activate (const roadmap_db_database *database) {

   roadmap_db_context *context;


   /* Activate each module declared in the model.
    * Modules with no context (context->handler_context is NULL) will be deactivated
    */
   for (context = database->context;
        context != NULL;
        context = context->next) {

		if (context->model &&
			 context->model->handler &&
			 context->model->handler->activate) {

			context->model->handler->activate (context->handler_context);
		}
   }
}   


static void roadmap_db_call_unmap (roadmap_db_database *database) {

   roadmap_db_context *context;
   roadmap_db_context *next;


   /* Activate each module declared in the model.
    * Modules with no context (context->handler_context is NULL) will be deactivated
    */
   for (context = database->context;
        context != NULL;
        context = next) {

		next = context->next;
		
		if (context->model &&
			 context->model->handler &&
			 context->model->handler->unmap &&
			 context->handler_context) {

			context->model->handler->unmap (context->handler_context);
		}
		free (context);
   }
   
   database->context = NULL;
}


static void roadmap_db_close_database (roadmap_db_database *database) {

   roadmap_db_call_unmap (database);

#ifdef NO_MAP_COMPRESSION
	unsigned char *ptr = (unsigned char *)(database->data.header) - sizeof(roadmap_data_file_header);
	free (ptr);
#else
	free (database->data.header);
#endif
	
   if (database->next != NULL) {
      database->next->previous = database->previous;
   }
   if (database->previous == NULL) {
      RoadmapDatabaseFirst = database->next;
   } else {
      database->previous->next = database->next;
   }

   free(database);
}


roadmap_db_model *roadmap_db_register
                      (roadmap_db_model *model,
                       const roadmap_db_sector *sector,
                       roadmap_db_handler *handler) {

   roadmap_db_model *registered;

   registered = malloc (sizeof(roadmap_db_model));

   roadmap_check_allocated(registered);

   registered->sector  = *sector;
   registered->handler = handler;
   registered->next    = model;

   return registered;
}




roadmap_db_database *roadmap_db_find (int fips, int tile_index) {

   roadmap_db_database *database;

   for (database = RoadmapDatabaseFirst;
         database != NULL;
         database = database->next) {

      if ((tile_index == database->tile_index) &&
            (fips == database->fips)) {
      	break;
      }
   }
   
   return database;
}


static int roadmap_db_fill_data (roadmap_db_database *database, void *base, unsigned int size) {

	roadmap_tile_file_header *tile_header = (roadmap_tile_file_header *) base;
	roadmap_data_file_header *file_header = (roadmap_data_file_header *) tile_header;
	unsigned char *compressed_data = (unsigned char *)(tile_header + 1);
	unsigned char *raw_data;
	unsigned long raw_data_size;
	int status;
	
	if (size < sizeof (roadmap_tile_file_header)) {
	   roadmap_log (ROADMAP_ERROR, "data file open: header size %u too small", size);
	   return 0;
	}
	
	if (memcmp (file_header->signature, ROADMAP_DATA_SIGNATURE, sizeof (file_header->signature))) {
	   roadmap_log (ROADMAP_ERROR, "data file open: invalid signature %c%c%c%c",
	   					file_header->signature[0],
	   					file_header->signature[1],
	   					file_header->signature[2],
	   					file_header->signature[3]);
	   return 0;
	}
	
	if (file_header->endianness != ROADMAP_DATA_ENDIAN_CORRECT) {
	   roadmap_log (ROADMAP_ERROR, "data file open: invalid endianness value %08ux", file_header->endianness);
	   return 0;
	}
	if (file_header->version != ROADMAP_DATA_CURRENT_VERSION) {
	   roadmap_log (ROADMAP_ERROR, "data file open: invalid version 0x%x != 0x%x", file_header->version, ROADMAP_DATA_CURRENT_VERSION);
	   return 0;
	}
	if (tile_header->compressed_data_size != size - sizeof (roadmap_tile_file_header)) {
	   roadmap_log (ROADMAP_ERROR, "data file size mismatch: expecting %d found %d", 
	   				 sizeof (roadmap_tile_file_header) + tile_header->compressed_data_size,
	   				 size);
	   return 0;
		
	}
	
	raw_data_size = tile_header->raw_data_size;

#ifdef NO_MAP_COMPRESSION
	// No compression
	raw_data = (unsigned char *) compressed_data;
#else
	raw_data = malloc (raw_data_size);

#ifdef RIMAPI
	status = RIMAPI_ZLib_uncompress (raw_data, &raw_data_size, compressed_data, tile_header->compressed_data_size);
#else
	status = uncompress (raw_data, &raw_data_size, compressed_data, tile_header->compressed_data_size) == Z_OK;
#endif

	if (!status) {
	   roadmap_log (ROADMAP_ERROR, "data file open: uncompress failed");
		free (raw_data);
		return 0;
	}
	if (raw_data_size != tile_header->raw_data_size) {
	   roadmap_log (ROADMAP_ERROR, "uncompressed data size mismatch: expecting %d found %d", 
	   				 tile_header->raw_data_size, raw_data_size);
		free (raw_data);
		return 0;
	}
#endif

	database->data.header = (roadmap_data_header *) raw_data;
		
	database->data.byte_alignment_add = (1 << database->data.header->byte_alignment_bits) - 1;
	database->data.byte_alignment_mask = ~database->data.byte_alignment_add;
	
	if (raw_data_size < sizeof (roadmap_data_header) + 
				  database->data.header->num_sections * sizeof (roadmap_data_entry)) {
	   roadmap_log (ROADMAP_ERROR, "data file open: size %lu cannot contain index", raw_data_size);
	   return 0;
	}
	database->data.index = (roadmap_data_entry *)(database->data.header + 1);
	
	if (database->data.header->num_sections > 0 &&
		 raw_data_size < sizeof (roadmap_data_header) + 
				  database->data.header->num_sections * sizeof (roadmap_data_entry) +
				  database->data.index[database->data.header->num_sections - 1].end_offset) {
				  	
	   roadmap_log (ROADMAP_ERROR, "data file open: size %lu cannot contain data", raw_data_size);
	   return 0;
	}
	database->data.data = (unsigned char *)(database->data.index + database->data.header->num_sections);
	
	return 1;			
}


static int add_db_and_map (roadmap_db_database *database) {

   if (RoadmapDatabaseFirst != NULL) {
      RoadmapDatabaseFirst->previous = database;
   }
   database->next       = RoadmapDatabaseFirst;
   database->previous   = NULL;
   RoadmapDatabaseFirst = database;

   if (! roadmap_db_call_map  (database)) {
      roadmap_db_close_database (database);
      return 0;
   }

   roadmap_db_call_activate (database);

   return 1;
}


int roadmap_db_open (int fips, int tile_index, roadmap_db_model *model,
                     const char* mode) {

   void *base = NULL;
   size_t size = 0;

   roadmap_db_database *database = roadmap_db_find (fips, tile_index);

   if (database) {

      roadmap_db_call_activate (database);
      return 1; /* Already open. */
   }

   if (roadmap_tile_load(fips, tile_index, &base, &size) != 0) {
   
	  return 0;
   }

   roadmap_log (ROADMAP_INFO, "Opening database file fips:%d, index:%d", fips, tile_index);
   database = malloc(sizeof(*database));
   roadmap_check_allocated(database);

   database->fips = fips;
   database->tile_index = tile_index;
	
	if (!roadmap_db_fill_data (database, base, (unsigned int) size)) {
	      
	   roadmap_log (ROADMAP_INFO, "tile %d (fips %d) has invalid format", tile_index, fips);
	   free (base);
      free (database);
      roadmap_tile_remove (fips, tile_index);
      return 0;
	}
	
#ifndef NO_MAP_COMPRESSION
   // we use the base pointer as there's no compression.
   free(base);
#endif

   database->model = model;
   database->context = NULL;

   return add_db_and_map(database);
}

 
int roadmap_db_open_mem (int fips, int tile_index, roadmap_db_model *model,
                         void *data, size_t size) {

   roadmap_db_database *database = roadmap_db_find (fips, tile_index);
   assert(!database);

   if (database) {
      roadmap_db_close_database (database);
   }

   database = malloc(sizeof(*database));
   roadmap_check_allocated(database);

   database->fips = fips;
   database->tile_index = tile_index;
	
#ifdef NO_MAP_COMPRESSION
   {
      void *orig = data;
      data = malloc(size);
      memcpy(data, orig, size);
   }
#endif

	if (!roadmap_db_fill_data (database, data, (unsigned int) size)) {
	      
	   roadmap_log (ROADMAP_INFO, "tile mem for index:%d (fips %d) has invalid format", tile_index, fips);
#ifdef NO_MAP_COMPRESSION
      free(data);
#endif
      free (database);
      return 0;
	}
	
   database->model = model;
   database->context = NULL;

   return add_db_and_map(database);
}


void roadmap_db_activate (int fips, int tile_index) {

   roadmap_db_database *database = roadmap_db_find (fips, tile_index);

   if (database) {

      roadmap_log (ROADMAP_DEBUG, "Activating tile %d (fips %d)", tile_index, fips);

      roadmap_db_call_activate (database);

      return;
   }

   roadmap_log
      (ROADMAP_ERROR, "cannot activate tile %d (fips %d) (not found)", tile_index, fips);
}


int	roadmap_db_exists (const roadmap_db_data_file *file, const roadmap_db_sector *sector) {
	
	return roadmap_db_sector_size (file, sector) > 0;
}


int	roadmap_db_get_data (const roadmap_db_data_file *file, 
									unsigned int data_id,
									unsigned int item_size, 
									void 			 **data, 
									int 			 *num_items) {

	unsigned int offset = roadmap_db_entry_offset (file, data_id);
	unsigned int size = roadmap_db_entry_size (file, data_id);

	assert (item_size > 0);
		
	if (size % item_size) {
      roadmap_log (ROADMAP_WARNING, "Invalid data size - item size %u data size %u", item_size, size);
      return 0;
	}		
	
	if (data) {
		if (size)	*data = file->data + offset;
		else			*data = NULL;
	}
	if (num_items) *num_items = size / item_size;
	return 1;								
}


int roadmap_db_close (int fips, int tile_index) {

   roadmap_db_database *database = roadmap_db_find (fips, tile_index);

   if (database) { 
   	roadmap_db_close_database (database);
   }
   
   return database != NULL;
}


void roadmap_db_remove (int fips, int tile_index) {

	roadmap_db_close (fips, tile_index);
	roadmap_tile_remove (fips, tile_index);	
}


void roadmap_db_end (void) {

   roadmap_db_database *database;
   roadmap_db_database *next;

   for (database = RoadmapDatabaseFirst; database != NULL; ) {

      next = database->next;
      roadmap_db_close_database (database);
      database = next;
   }
}

const char *roadmap_db_map_path (void) {

   const char *map_path;
   static char map_path_static[512];
   static int map_path_initialized = 0;
   
   if (!map_path_initialized) { 
   
	#ifdef J2ME
      map_path = "recordstore:/";
		//strncpy_safe (map_path_static, map_path, sizeof (map_path_static));
	#elif defined (WIN32)
		map_path = roadmap_path_join (roadmap_path_user(), "maps");
		strncpy_safe (map_path_static, map_path, sizeof (map_path_static));
		roadmap_path_free (map_path);
	#elif IPHONE
	   map_path = roadmap_path_join (roadmap_main_bundle_path(), "maps");
	   strncpy_safe (map_path_static, map_path, sizeof (map_path_static));
	   roadmap_path_free (map_path);
	#else
	   map_path = roadmap_path_first ("maps");
	   while (map_path && !roadmap_file_exists (map_path,"")) {
	   	map_path = roadmap_path_next ("maps", map_path);
	   }
	   if (map_path) {
			strncpy_safe (map_path_static, map_path, sizeof (map_path_static));
	   } else {
	   	map_path_static[0] = '\0';
	   }
	#endif
		map_path_initialized = 1;
   }
   	
   return map_path_static;
}

