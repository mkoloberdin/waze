/* editor_db.c - databse layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See editor_db.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_county.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_messagebox.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_dictionary.h"
#include "editor_marker.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_line.h"
//#include "editor_square.h"
#include "editor_street.h"
//#include "editor_route.h"
#include "editor_override.h"
#include "editor_trkseg.h"
#include "../track/editor_track_report.h"

#define DB_DEFAULT_BLOCK_SIZE 1024
#define DB_DEFAULT_BLOCK_COUNT 10
#define FLUSH_SIZE 300
#define MAX_TYPES 20
#define EDITOR_DB_ALIGN 4

#define TYPE_MULTIPLE_FLAG 	0x80000000
#define TYPE_UPDATE_FLAG   	0x40000000
#define TYPE_COMMITTED_FLAG	0x20000000

static const int DB_SIGNATURE	= 0x3a2e0001; //last 4 digits are incremental version ; don't change the first 4

typedef struct {
	int	generation;
} editor_record_header;

static int EditorActiveMap = -1;
static RoadMapFile EditorDataFile = ROADMAP_INVALID_FILE;
static editor_db_handler *EditorHandlers[MAX_TYPES];
static editor_db_section *EditorActiveSections[MAX_TYPES];

static editor_db_section *editor_db_alloc_section (void) {

   editor_db_section *section = (editor_db_section *) calloc(sizeof(editor_db_section), 1);
   section->max_blocks = DB_DEFAULT_BLOCK_COUNT;
   section->blocks = calloc (DB_DEFAULT_BLOCK_COUNT, sizeof (char *));

   return section;
}

static void editor_db_free_section (editor_db_section *section) {

	int i;
	
	for (i = 0; i < section->max_blocks; i++) {
		if (section->blocks[i]) {
			free (section->blocks[i]);
			section->blocks[i] = NULL;
		}
	}	
	section->num_items = 0;
	section->current_generation = 0;
	section->pending_generation = -1;
	section->committed_generation = -1;
}


static void editor_db_activate_handler (editor_db_handler *handler) {

   editor_db_section *section = editor_db_alloc_section();

   assert(handler->type_id < MAX_TYPES);

   section->type_id = handler->type_id;
   section->item_size = handler->item_size;
   section->flag_committed = handler->flag_committed;
   section->item_offset = section->flag_committed * sizeof (editor_record_header);
   section->record_size = section->item_size + section->item_offset;
   section->items_per_block = DB_DEFAULT_BLOCK_SIZE / section->record_size;

	if (section->flag_committed) {
		section->current_generation = 0;
		section->committed_generation = -1;
		section->pending_generation = -1;
	}
	
   handler->activate(section);
   EditorHandlers[handler->type_id] = handler;
   EditorActiveSections[handler->type_id] = section;

}

static void editor_db_configure (void) {
   
   int i;
   
	for (i = 0; i < MAX_TYPES; i++) {
		EditorActiveSections[i] = NULL;
	}	
	
   editor_db_activate_handler(&EditorDictionaryHandler);
   editor_db_activate_handler(&EditorMarkersHandler);
   editor_db_activate_handler(&EditorShapeHandler);
   editor_db_activate_handler(&EditorPointsHandler);
   editor_db_activate_handler(&EditorLinesHandler);
   editor_db_activate_handler(&EditorStreetHandler);
   editor_db_activate_handler(&EditorOverrideHandler);
   editor_db_activate_handler(&EditorTrksegHandler);
}


static void editor_db_update_generation (editor_db_section *section, int generation) {
	
	if (generation > section->current_generation) {
		section->current_generation = generation;
	}
}


static int editor_db_read_items (char **buffer, size_t size,
                                 editor_db_section **item_section,
                                 int *item_id,
                                 int *error) {

   unsigned int type_id;
   int count = 1;
   editor_db_section *section;
   char *read_buffer;
   int committed = -1;

	*error = 0;
   if (size < sizeof (unsigned int)) return -1;

	type_id = *(unsigned int *)(*buffer);
	read_buffer = (*buffer) + sizeof (unsigned int);
   size -= sizeof (unsigned int);

   if (type_id & TYPE_MULTIPLE_FLAG) {
      if (size < sizeof(int)) return -1;
      count = *(int *)read_buffer;
      read_buffer += sizeof(int);
      size -= sizeof(int);
      type_id &= ~TYPE_MULTIPLE_FLAG;
   }

   if (type_id & TYPE_UPDATE_FLAG) {
      if (size < sizeof(int)) return -1;
      *item_id = *(int *)read_buffer;
      read_buffer += sizeof(int);
      size -= sizeof(int);
      type_id &= ~TYPE_UPDATE_FLAG;
   } else {
      *item_id = -1;
   }

	if (type_id & TYPE_COMMITTED_FLAG) {
      if (size < sizeof(int)) return -1;
		committed = *(int *)read_buffer;
		read_buffer += sizeof (int);
		type_id &= ~TYPE_COMMITTED_FLAG;
		count = 0;
	}
	
   //assert(type_id < MAX_TYPES);
   if (type_id >= MAX_TYPES) {
      editor_log (ROADMAP_ERROR,
                  "editor_db_read_items() - bad type_id.");
   	*error = -1;
   	return -1;
   }
   section = EditorActiveSections[type_id];
   //assert(section);
   if (!section) {
      editor_log (ROADMAP_ERROR,
                  "editor_db_read_items() - invalid section pointer.");
   	*error = -1;
   	return -1;
   }

	if (committed >= 0) {
		section->committed_generation = committed;
	}
	
   if (size < section->record_size * count) return -1;
   *item_section = section;
   *buffer = read_buffer;
   return count;
}


static int editor_db_write_committed (editor_db_section *section, int id) {

	unsigned int type_id = section->type_id | TYPE_COMMITTED_FLAG;
	
   if (roadmap_file_write(EditorDataFile, &type_id, sizeof(type_id)) < 0)
      return -1;

   if (roadmap_file_write(EditorDataFile, &id, sizeof(int)) < 0)
      return -1;

	return 0;	
}


static void *editor_db_get_record (editor_db_section *section, int item_id) {

   int block = item_id / section->items_per_block;
   int block_offset = item_id % section->items_per_block;

   if (section->blocks[block] == NULL) return NULL;

   return section->blocks[block] + block_offset * section->record_size;
}


static int editor_db_allocate_new_block
                     (editor_db_section *section, int block_id) {
   
   if (section->max_blocks == block_id) {
   	int new_size = section->max_blocks * 2;
   	char **more_blocks = (char **)realloc (section->blocks, sizeof (char *) * new_size);
		if (!more_blocks) {
	      editor_log (ROADMAP_ERROR,
	                  "editor_db_allocate_new_block - reached max memory.");
	      return -1;
		}
		
		section->blocks = more_blocks;
		while (section->max_blocks < new_size) {
			section->blocks[section->max_blocks++] = NULL;
		}
   }

   section->blocks[block_id] = malloc(DB_DEFAULT_BLOCK_SIZE);

   if (!section->blocks[block_id]) return -1;

   return 0;
}


static int editor_db_write_record (editor_db_section *section, char *data, int item_id, int count) {

   static int flush_count;
   unsigned int type_id = section->type_id;
   int align;
   char dummy[EDITOR_DB_ALIGN - 1];

   if (item_id != -1) {
      type_id |= TYPE_UPDATE_FLAG;

   } else if (count > 1) {
      type_id |= TYPE_MULTIPLE_FLAG;
   }

   if (roadmap_file_write(EditorDataFile, &type_id, sizeof(type_id)) < 0)
      return -1;

   if ((item_id != -1) &&
         (roadmap_file_write(EditorDataFile, &item_id, sizeof(item_id)) < 0))
      return -1;

   if ((count > 1) &&
         (roadmap_file_write(EditorDataFile, &count, sizeof(count)) < 0))
      return -1;

	if (section->flag_committed) {
	   if (roadmap_file_write(EditorDataFile, data, section->item_offset) < 0)
	         return -1;
	}

   if (roadmap_file_write(EditorDataFile, data + section->item_offset, section->item_size * count) < 0)
         return -1;

   align = (count * section->record_size) % EDITOR_DB_ALIGN;
   if (align) {
   	memset (dummy, 0, EDITOR_DB_ALIGN - align);
   	if (roadmap_file_write (EditorDataFile, dummy, EDITOR_DB_ALIGN - align) < 0) return -1;
   }

   if (++flush_count == FLUSH_SIZE) {
      flush_count = 0;
      //editor_db_sync ();
   }


   return 0;
}


static int editor_db_add_record (editor_db_section *section, void* header, void *data, int write) {
   
   int block = section->num_items / section->items_per_block;
   int block_offset = section->num_items % section->items_per_block;
   char *rec_addr;

   if ((section->num_items % section->items_per_block) == 0) {
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
   }

   rec_addr = section->blocks[block] +
      block_offset * section->record_size;

   if (section->flag_committed) memcpy (rec_addr, header, sizeof (editor_record_header));
   if (data) memcpy (rec_addr + section->item_offset, data, section->item_size);

   if (write) editor_db_write_record(section, rec_addr, -1, 1);

   return section->num_items++;
}


static int editor_db_read (void) {
   char buffer[1024];
   int size = 0;
   editor_db_section *section;
   int error = 0;
   int signature;
   int res;

	res = roadmap_file_read (EditorDataFile, &signature, sizeof (int));
	if (res != sizeof (int) || signature != DB_SIGNATURE) return -1;
	
   while (1) {
      char *head = buffer;
      int count;
      int item_id;
      int align;

      res = roadmap_file_read (EditorDataFile, buffer + size,
                               sizeof(buffer) - size);
      if (res <= 0) return error;
      size += res;

      while ((count = editor_db_read_items(&head, size - (head - buffer),
                                           &section, &item_id, &error)) >= 0) {
         int i;
         for (i=0; i<count; i++) {
            if (item_id != -1) {
               /* Update */
               void *data = editor_db_get_record(section, item_id);
               assert(data);
               if (section->flag_committed) {
               	editor_db_update_generation (section, ((editor_record_header *)data)->generation);
               }
               memcpy(data, head, section->record_size);
            } else {
               /* Add */
               if (editor_db_add_record(section, head, head + section->item_offset, 0) == -1) return -1;
               if (section->flag_committed) {
               	editor_db_update_generation (section, ((editor_record_header *)head)->generation);
               }
            }
            head += section->record_size;
         }
         align = (count * section->record_size) % EDITOR_DB_ALIGN;
         if (align) head += EDITOR_DB_ALIGN - align;
      }

		if (error) break;
		
      size -= (head - buffer);
      if (size > 0) memmove(buffer, head, size);
   }
   
   return error;
}


int editor_db_write_item (editor_db_section *section, int item_id, int count) {
	
	return editor_db_write_record (section, editor_db_get_record (section, item_id), -1, count);
}


int editor_db_create (int map_id) {
   assert(0);
   return -1;
}

static void editor_db_free (void) {

	int i;
	
	for (i = 0; i < MAX_TYPES; i++) {
		if (EditorActiveSections[i]) editor_db_free_section (EditorActiveSections[i]);
	}
}


int editor_db_open (int map_id) {

   char name[100];
   const char *map_path;
   char file_name[512];
   int do_read = 0;

   editor_log_push ("editor_db_open");
	
#ifndef IPHONE
   map_path = roadmap_db_map_path();
#else
	map_path = roadmap_path_preferred("maps");
#endif //IPHONE
	
	if (!map_path) {
      editor_log (ROADMAP_ERROR, "Can't find editor path");
      editor_log_pop ();
      return -1;
	}

   snprintf (name, sizeof(name), "edt%05d.dat", map_id);

   roadmap_path_format (file_name, sizeof (file_name), map_path, name);

   if (roadmap_file_exists (map_path, name)) {
      EditorDataFile = roadmap_file_open(file_name, "rw");  
      do_read = 1;
   } else {
      roadmap_path_create (map_path);
      EditorDataFile = roadmap_file_open(file_name, "w");
      roadmap_file_write (EditorDataFile, &DB_SIGNATURE, sizeof (int));
   }

	do {
	   if (!ROADMAP_FILE_IS_VALID(EditorDataFile)) {
	      editor_log (ROADMAP_ERROR, "Can't open/create new database: %s/%s",
	            map_path, name);
	      editor_log_pop ();
	      return -1;
	   }
	
	   if (do_read) {
   		do_read = 0;
	   	if (editor_db_read () == -1) {
	   		editor_db_free ();
	   		//roadmap_messagebox("Error", "Offline data file is currupt: Re-Initializing data");
	   		roadmap_log (ROADMAP_ERROR, "Offline data file is currupt: Re-Initializing data");
	   		roadmap_file_close (EditorDataFile);
	   		roadmap_file_remove (NULL, file_name);
		      EditorDataFile = roadmap_file_open(file_name, "w");
      		roadmap_file_write (EditorDataFile, &DB_SIGNATURE, sizeof (int));
	   	}
	   }
	} while (do_read);

   EditorActiveMap = map_id;
   editor_log_pop ();
   return 0;
}


void editor_db_sync (int map_id) {
   assert(map_id == EditorActiveMap);
   //roadmap_file_flush(EditorDataFile);
}


int editor_db_activate (int map_id) {
   int res;

   if (EditorActiveMap == map_id) {
       return 0;
   }

   assert(EditorActiveMap == -1);

   if (map_id == -1) return -1;

   editor_db_configure();

   res = editor_db_open (map_id);

   return res;
}


void editor_db_close (int map_id) {

   if (EditorActiveMap == -1) return;

   assert(map_id == EditorActiveMap);
   editor_db_free ();
   roadmap_file_close(EditorDataFile);
   EditorDataFile = ROADMAP_INVALID_FILE;
   EditorActiveMap = -1;
}


void editor_db_delete (int map_id) {

   char name[100];
  	const char *map_path;

   map_path = roadmap_db_map_path();
   snprintf (name, sizeof(name), "edt%05d.dat", map_id);
   
   if (roadmap_file_exists (map_path, name)) {

      char **files;
      char **cursor;
      char directory[512];
      char full_name[512];

      /* Delete notes wav files */
      /* FIXME this is broken for multiple counties */
      roadmap_path_format (directory, sizeof (directory), roadmap_path_user (), "markers");
      files = roadmap_path_list (directory, ".wav");

      for (cursor = files; *cursor != NULL; ++cursor) {

         roadmap_path_format (full_name, sizeof (full_name), directory, *cursor);
         roadmap_file_remove (NULL, full_name);
      }

      /* Remove the actual editor file */
      roadmap_file_remove (map_path, name);
   }
}


int editor_db_add_item (editor_db_section *section, void *data, int write) {
   
   editor_record_header header;

	if (section->flag_committed) {
		editor_db_update_generation (section, section->committed_generation + 1);
		editor_db_update_generation (section, section->pending_generation + 1);
		header.generation = section->current_generation;
	}
	
	return editor_db_add_record (section, &header, data, write);
}


int editor_db_get_item_count (editor_db_section *section) {
   return section->num_items;
}


void *editor_db_get_item (editor_db_section *section,
                           int item_id, int create, editor_item_init init) {

   int block = item_id / section->items_per_block;
   int block_offset = item_id % section->items_per_block;

   if (section->blocks[block] == NULL) {
      
      if (!create) return NULL;
      
      if (editor_db_allocate_new_block (section, block) == -1) {
         return NULL;
      }
      
      if (init != NULL) {
         int i;
         char *addr = section->blocks[block];

         for (i=0; i<section->items_per_block; i++) {
            (*init) (addr + i * section->record_size + section->item_offset);
         }
      }
   }

   return section->blocks[block] + block_offset * section->record_size + section->item_offset;
}


int editor_db_get_block_size (void) {
   return DB_DEFAULT_BLOCK_SIZE;
}

int editor_db_allocate_items (editor_db_section *section, int count) {

   int block = section->num_items / section->items_per_block;
   int block_offset = section->num_items % section->items_per_block;
   int first_item_id;

   if (count > section->items_per_block) return -1;

   if ((section->num_items == 0) || (block_offset == 0)) {
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
   }

   if ((block_offset + count * section->item_size) >
         DB_DEFAULT_BLOCK_SIZE) {
      block++;
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
      section->num_items = block * section->items_per_block;
      block_offset = 0;
   }

   first_item_id = section->num_items;
   section->num_items += count;

   return first_item_id;
}


int editor_db_update_item (editor_db_section *section, int item_id) {

   void *rec = editor_db_get_record(section, item_id);

   assert(rec != NULL);

	if (section->flag_committed) {
		editor_db_update_generation (section, section->committed_generation + 1);
		editor_db_update_generation (section, section->pending_generation + 1);
		((editor_record_header *)rec)->generation = section->current_generation;
	}
	
   return editor_db_write_record(section, rec, item_id, 1);
}


int editor_db_begin_commit (editor_db_section *section) {

	section->pending_generation = section->current_generation;	
	return section->current_generation;
}


void editor_db_confirm_commit (editor_db_section *section, int id) {

	if (id > section->committed_generation) {
		section->committed_generation = id;
		if (editor_db_write_committed (section, id) != 0) {
	      editor_log (ROADMAP_ERROR,
	                  "editor_db_confirm_commit - editor_db_write_committed failed.");
		}
	}	
}


int editor_db_item_committed (editor_db_section *section, int item_id) {

	editor_record_header *rec;

	assert (section->flag_committed);
	
   rec = editor_db_get_record (section, item_id);

   assert (section->flag_committed);
	
   return rec->generation <= section->committed_generation;
}


int editor_db_items_pending (editor_db_section *section) {

	editor_db_update_generation (section, section->committed_generation);
	return section->current_generation - section->committed_generation;	
}
