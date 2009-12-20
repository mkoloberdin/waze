/* editor_db.h - database layer
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
 */

#ifndef INCLUDE__EDITOR_DB__H
#define INCLUDE__EDITOR_DB__H

#include "roadmap_types.h"

#define EDITOR_DB_MARKERS		1
#define EDITOR_DB_DICTIONARY	2
#define EDITOR_DB_SHAPES		3
#define EDITOR_DB_POINTS		4
#define EDITOR_DB_TRKSEGS		5
#define EDITOR_DB_STREETS		6
#define EDITOR_DB_LINES			7
#define EDITOR_DB_OVERRIDES	    8

typedef struct editor_db_section_s {
   unsigned int type_id;
   int num_items;
   int max_blocks;
   int flag_committed;
   size_t item_offset;
   size_t item_size;
   size_t record_size;
   int items_per_block;
   char **blocks; /* dynamic */
   int current_generation;
   int committed_generation;
   int pending_generation;
} editor_db_section;

typedef void (*editor_item_init)  (void *item);

typedef void (*editor_db_activator) (editor_db_section *section);

typedef struct {

   unsigned int type_id;
   size_t item_size;
   int flag_committed;

   editor_db_activator activate;

} editor_db_handler;


int editor_db_create (int fips);
int editor_db_activate (int fips);
void editor_db_sync (int fips);
void editor_db_close (int fips);
void editor_db_delete (int fips);

int editor_db_add_item (editor_db_section *section, void *data, int write);
int editor_db_get_item_count (editor_db_section *section);
void *editor_db_get_item (editor_db_section *section, int item_id, int create, editor_item_init init);
int editor_db_get_block_size (void);
void *editor_db_get_last_item (editor_db_section *section);
int editor_db_allocate_items (editor_db_section *section, int count);
int editor_db_update_item (editor_db_section *section, int item_id);
int editor_db_write_item (editor_db_section *section, int item_id, int count);
int editor_db_begin_commit (editor_db_section *section);
void editor_db_confirm_commit (editor_db_section *section, int id);
int editor_db_item_committed (editor_db_section *section, int item_id);
int editor_db_items_pending (editor_db_section *section);

#endif // INCLUDE__EDITOR_DB__H

