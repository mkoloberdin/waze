/* editor_override.c - line databse layer
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
 *   See editor_override.h
 */

#include <assert.h>
#include "stdlib.h"
#include "string.h"

#include "roadmap.h"
#include "roadmap_locator.h"
#include "roadmap_line_route.h"
#include "roadmap_square.h"

#include "editor_db.h"
#include "editor_override.h"

static editor_db_section *ActiveOverridesDB;


static void editor_override_activate (editor_db_section *context) {

   ActiveOverridesDB = context;
}

editor_db_handler EditorOverrideHandler = {
   EDITOR_DB_OVERRIDES,
   sizeof (editor_db_override),
   0,
   editor_override_activate
};

int editor_override_get_count (void) {

   return editor_db_get_item_count (ActiveOverridesDB);
}

int editor_override_get (int index, int *line_id, int *square_id, int *direction, int *flags){
   editor_db_override *rec;

   rec = (editor_db_override *) editor_db_get_item
                           (ActiveOverridesDB, index, 0, NULL);

   assert (rec);
   if (!rec) return 0;

   if (line_id)   *line_id = rec->line;
   if (square_id) *square_id = rec->square;
   if (direction) *direction = rec->direction;
   if (flags)     *flags = rec->flags;

   if (rec->timestamp != (int)roadmap_square_timestamp (rec->square)) {
   	return 0;
   }

   return 1;
}

static int editor_override_find (int line, int square, editor_db_override **data, int *create) {

   int id;
   int count;
   int curr_timestamp = (int)roadmap_square_timestamp (square);

   editor_db_override *rec = NULL;

   count = editor_db_get_item_count (ActiveOverridesDB);

	for (id = 0; id < count; id++) {

		rec = (editor_db_override *)editor_db_get_item (ActiveOverridesDB, id, 0, NULL);
		if (rec->square == square && rec->line == line &&
			 rec->timestamp == curr_timestamp) {
			if (create) *create = 0;
			break;
		}
	}

	if (id == count) {
		if (!create) return -1;

		id = editor_db_add_item (ActiveOverridesDB, NULL, 0);
		if (id >= 0) {
			rec = (editor_db_override *)editor_db_get_item (ActiveOverridesDB, id, 0, NULL);
			rec->square = square;
			rec->timestamp = curr_timestamp;
			rec->line = line;
			rec->flags = 0;
			rec->direction = roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED);
			*create = 1;
		}
	}

	if (data) *data = rec;
	return id;
}


int editor_override_line_get_flags (int line, int square, int *flags) {

   editor_db_override *data;
   int id = editor_override_find (line, square, &data, NULL);

   if (id < 0)
      return -1;
	if (flags) *flags = data->flags;

   return 0;
}


int editor_override_line_set_flag (int line, int square, int flags) {

   editor_db_override *data;
   int created;
   int id = editor_override_find (line,square,  &data, &created);
   int rc;

   if (id < 0) return 0;

   data->flags = data->flags | flags;

	if (created) {
		rc = editor_db_write_item (ActiveOverridesDB, id, 1);
	} else {
   	rc = editor_db_update_item (ActiveOverridesDB, id);
	}

	return rc;
}


int editor_override_line_reset_flag (int line, int square, int flags) {

   editor_db_override *data;
   int created;
   int id = editor_override_find (line, square, &data, &created);
   int rc;

   if (id < 0) return 0;

   data->flags = data->flags & ~flags;

	if (created) {
		rc = editor_db_write_item (ActiveOverridesDB, id, 1);
	} else {
   	rc = editor_db_update_item (ActiveOverridesDB, id);
	}

	return rc;
}


int editor_override_line_get_direction (int line, int square, int *direction) {

   editor_db_override *data;
   int id = editor_override_find (line, square,  &data, NULL);

   if (id < 0) return -1;
   if (direction) *direction = data->direction;

   return 0;
}


int editor_override_line_set_direction (int line, int square, int direction) {

   editor_db_override *data;
   int created;
   int id = editor_override_find (line, square,  &data, &created);
   int rc;

   if (id < 0) return 0;

   data->direction = direction;

	if (created) {
		rc = editor_db_write_item (ActiveOverridesDB, id, 1);
	} else {
   	rc = editor_db_update_item (ActiveOverridesDB, id);
	}

	return rc;
}

int editor_override_exists (int line, int square) {
	int id = editor_override_find (line, square,  NULL, NULL);
	if (id == -1)
		return FALSE;
	else
		return TRUE;
}

