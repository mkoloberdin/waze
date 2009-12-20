/* roadmap_dbread.h - General API for accessing a RoadMap map file.
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

#ifndef _ROADMAP_DBREAD__H_
#define _ROADMAP_DBREAD__H_

#include "roadmap_data_format.h"

typedef struct {
	roadmap_data_header		*header;
	roadmap_data_entry		*index;
	unsigned char				*data;
	
	unsigned int 				byte_alignment_add;
	unsigned int 				byte_alignment_mask;
} roadmap_db_data_file;


typedef void * (*roadmap_db_mapper)  (const roadmap_db_data_file *file);
typedef void (*roadmap_db_activator) (void *context);
typedef void (*roadmap_db_unmapper)  (void *context);

typedef struct {

   char *name;

   roadmap_db_mapper    map;
   roadmap_db_activator activate;
   roadmap_db_unmapper  unmap;

} roadmap_db_handler;

typedef struct {
	int	first;
	int	last;
} roadmap_db_sector;


typedef struct roadmap_db_model_s {

   roadmap_db_handler	*handler;
   roadmap_db_sector		sector;

   struct roadmap_db_model_s *next;
} roadmap_db_model;

roadmap_db_model *roadmap_db_register
     (roadmap_db_model *model, const roadmap_db_sector *sector, roadmap_db_handler *handler);


int  roadmap_db_open (int fips, int tile_index, roadmap_db_model *model,
                     const char* mode);

int  roadmap_db_open_mem (int fips, int tile_index, roadmap_db_model *model,
                         void *data, size_t size);

void roadmap_db_activate (int fips, int tile_index);

int	roadmap_db_exists (const roadmap_db_data_file *file, const roadmap_db_sector *sector);

int	roadmap_db_get_data (const roadmap_db_data_file *file, 
									unsigned int data_id,
									unsigned int item_size, 
									void 			 **data, 
									int 			 *num_items);

int  roadmap_db_close (int fips, int tile_index);
void roadmap_db_remove (int fips, int tile_index);
void roadmap_db_end   (void);

const char *roadmap_db_map_path (void);

#endif // _ROADMAP_DBREAD__H_

