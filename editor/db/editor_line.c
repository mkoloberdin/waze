/* editor_line.c - line databse layer
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
 *   See editor_line.h
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_square.h"
#include "roadmap_shape.h"
#include "roadmap_street.h"
#include "roadmap_math.h"
#include "roadmap_line_route.h"
#include "roadmap_navigate.h"
#include "roadmap_tile.h"
#include "roadmap_tile_manager.h"

#include "../editor_log.h"
#include "../editor_main.h"
#include "../editor_bar.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_line.h"
#include "editor_trkseg.h"
#include "editor_shape.h"
#include "editor_square.h"
#include "editor_street.h"
#include "editor_override.h"

typedef struct {
	RoadMapPosition	last_position;
	int					first_shape;
	int					last_shape;
} editor_line_copy_points;


static editor_db_section *ActiveLinesDB;

static void editor_lines_activate (editor_db_section *context) {
   ActiveLinesDB = context;
}

editor_db_handler EditorLinesHandler = {
   EDITOR_DB_LINES,
   sizeof (editor_db_line),
   1,
   editor_lines_activate
};


static void editor_line_set_modification_time (editor_db_line *line_db) {

	int time_now = (int)roadmap_navigate_get_time ();
	if (time_now > line_db->update_timestamp)
		line_db->update_timestamp = (time_t)time_now;	
}


int editor_line_add
         (int p_from,
          int p_to,
          int trkseg,
          int cfcc,
          int direction,
          int street,
          int flags) {

   editor_db_line line;
   int id;
   time_t update_time = roadmap_navigate_get_time ();

	line.update_timestamp = (int)update_time;
   line.point_from = p_from;
   line.point_to = p_to;

   line.trkseg = trkseg;

   line.cfcc = cfcc;
   line.flags = flags;

   line.street = street;
   line.direction = direction;

   id = editor_db_add_item (ActiveLinesDB, &line, 1);

   if (id == -1) return -1;
   assert (editor_line_length (id) > 0);

   if (editor_line_length (id) == 0) return -1;

   editor_bar_set_temp_length(0);
   editor_bar_set_length(editor_line_length (id));
   
   editor_log
      (ROADMAP_INFO,
       "Adding new line - from:%d, to:%d, trkseg:%d, cfcc:%d, flags:%d",
       p_from, p_to, trkseg, cfcc, flags);

   if (id == -1) {
      editor_log (ROADMAP_ERROR, "Can't add new line.");
   }

   return id;
}


void editor_line_get (int line,
                      RoadMapPosition *from,
                      RoadMapPosition *to,
                      int *trkseg,
                      int *cfcc,
                      int *flags) {
   
   editor_db_line *line_db;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

	assert (line_db);
	if (!line_db) return;
	
   if (from) editor_point_position (line_db->point_from, from);
   if (to) editor_point_position (line_db->point_to, to);

   if (trkseg) *trkseg = line_db->trkseg;
   if (cfcc) *cfcc = line_db->cfcc;
   if (flags) *flags = line_db->flags;
}


void editor_line_get_points (int line, int *from, int *to) {

   editor_db_line *line_db;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

	assert (line_db);
	if (!line_db) return;

   if (from) *from = line_db->point_from;
   if (to) *to = line_db->point_to;
}


void editor_line_modify_properties (int line,
                                    int cfcc,
                                    int flags) {
   
   editor_db_line *line_db;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

   assert (line_db != NULL);

   if (line_db == NULL) return;

   line_db->cfcc = cfcc;
   line_db->flags = flags;
   
	editor_line_set_modification_time (line_db);
   editor_db_update_item (ActiveLinesDB, line);
}


void editor_line_set_flag (int line, int flag) {

   editor_db_line *line_db;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

   assert (line_db != NULL);

   if (line_db == NULL) return;

	line_db->flags = line_db->flags | flag;
	editor_line_set_modification_time (line_db);
   editor_db_update_item (ActiveLinesDB, line);
}


void editor_line_reset_flag (int line, int flag) {
	
   editor_db_line *line_db;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

   assert (line_db != NULL);

   if (line_db == NULL) return;

	line_db->flags = line_db->flags & ~flag;
	editor_line_set_modification_time (line_db);
   editor_db_update_item (ActiveLinesDB, line);
}


int editor_line_length (int line) {

   RoadMapPosition p1;
   RoadMapPosition p2;
   editor_db_line *line_db;
   int length = 0;
   int trk_from;
   int first_shape;
   int last_shape;
   int i;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

   editor_trkseg_get (line_db->trkseg,
                      &trk_from, &first_shape, &last_shape, NULL);

   editor_point_position (line_db->point_from, &p1);

   if (first_shape > -1) {
      editor_point_position (trk_from, &p2);

      for (i=first_shape; i<=last_shape; i++) {

         editor_shape_position (i, &p2);
         length += roadmap_math_distance (&p1, &p2);
         p1 = p2;
      }
   }

   editor_point_position (line_db->point_to, &p2);
   length += roadmap_math_distance (&p1, &p2);

   return length;
}


int editor_line_get_street (int line,
                            int *street) {

   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line, 0, NULL);

   if (line_db == NULL) return -1;

   if (street != NULL) *street = line_db->street;

   return 0;
}


int editor_line_set_street (int line,
                            int street) {

   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line, 0, NULL);

   if (line_db == NULL) return -1;

   line_db->street = street;

	editor_line_set_modification_time (line_db);
   return editor_db_update_item (ActiveLinesDB, line);
}


int editor_line_get_direction (int line, int *direction) {

   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line, 0, NULL);

   if (line_db == NULL) return -1;

   if (direction != NULL) *direction = line_db->direction;

   return 0;
}


int editor_line_set_direction (int line, int direction) {

   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line, 0, NULL);

   if (line_db == NULL) return -1;

   line_db->direction = direction;

	editor_line_set_modification_time (line_db);
   return editor_db_update_item (ActiveLinesDB, line);
}


int editor_line_get_cross_time (int line, int direction) {

   editor_db_line *line_db;
   time_t start_time;
   time_t end_time;

   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line, 0, NULL);

   editor_trkseg_get_time
      (line_db->trkseg, &start_time, &end_time);

   if (start_time == end_time) return -1;

   return  (int) (end_time - start_time);
}


int editor_line_get_timestamp (int line_id) {
	
   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line_id, 0, NULL);

   if (line_db == NULL) return -1;

   return line_db->update_timestamp;
}


int editor_line_is_valid (int line_id) {
	
   editor_db_line *line_db;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line_id, 0, NULL);

   if (line_db == NULL) return 0;

   return !(line_db->flags & ED_LINE_DELETED);
}


void editor_line_invalidate (int line_id) {

	editor_line_set_flag (line_id, ED_LINE_DELETED);
}


static int editor_line_interpolate_request_tiles (int from_tile, int to_tile, 
																	RoadMapPosition *from_pos, RoadMapPosition *to_pos,
																	int min_update) {

	int mid_tile;
	RoadMapPosition mid_pos;
   int tile_update;
	
	if (roadmap_tile_is_adjacent (from_tile, to_tile)) {
		return min_update;
	}

	mid_pos.longitude = (from_pos->longitude + to_pos->longitude) / 2;
	mid_pos.latitude = (from_pos->latitude + to_pos->latitude) / 2;
	if ((mid_pos.longitude == from_pos->longitude && mid_pos.latitude == from_pos->latitude) ||
		 (mid_pos.longitude == to_pos->longitude && mid_pos.latitude == to_pos->latitude)) {
		 return min_update;
	}
	mid_tile = roadmap_tile_get_id_from_position (0, &mid_pos);
	if (mid_tile != from_tile && mid_tile != to_tile) {
		roadmap_tile_request (mid_tile, 0, 1, NULL);
		tile_update = (int)roadmap_square_timestamp (mid_tile);
		if (tile_update < min_update) {
			min_update = tile_update;
		}
	}
	if (mid_tile != from_tile) {
		min_update = editor_line_interpolate_request_tiles (from_tile, mid_tile, from_pos, &mid_pos, min_update);
	}																	
	if (mid_tile != to_tile) {
		min_update = editor_line_interpolate_request_tiles (mid_tile, to_tile, &mid_pos, to_pos, min_update);
	}
	
	return min_update;
}


int editor_line_get_update_time (int line_id) {

   RoadMapPosition pos;
   RoadMapPosition last_pos;
   int shape;
   int last_tile = -1;
   int tile;
   editor_db_line *line_db;
   int min_update = 0x7fffffff;
   int tile_update;
   int first_shape;
   int last_shape;
   int from_point;

   line_db =
      (editor_db_line *) editor_db_get_item
         (ActiveLinesDB, line_id, 0, NULL);

   if (line_db == NULL) return min_update;
	
	editor_trkseg_get (line_db->trkseg, &from_point, &first_shape, &last_shape, NULL);
	if (first_shape == -1) {
		last_shape = -1;
	}
	for (shape = first_shape - 1; shape <= last_shape; shape++) {
		if (shape == -2) {
			editor_point_position (line_db->point_from, &pos);
		} else if (shape < first_shape) {
			editor_point_position (from_point, &pos);
		} else if (shape >= 0) {
			editor_shape_position (shape, &pos);
		} else {
			editor_point_position (line_db->point_to, &pos);
		}
		
		tile = roadmap_tile_get_id_from_position (0, &pos);
		if (tile != last_tile) {
			roadmap_tile_request (tile, 0, 1, NULL);
			tile_update = (int)roadmap_square_timestamp (tile);
			if (tile_update < min_update) {
				min_update = tile_update;
			}
			if (last_tile != -1) {
				min_update = editor_line_interpolate_request_tiles (last_tile, tile, &last_pos, &pos, min_update);
			}
			last_tile = tile;
		}
		last_pos = pos;
	}
	
	return min_update;
}


int editor_line_get_count (void) {
   
   return editor_db_get_item_count (ActiveLinesDB);
}


int editor_line_mark_dirty (int line_id) {
   
   editor_db_line *line_db;
   
   line_db = (editor_db_line *) editor_db_get_item
                           (ActiveLinesDB, line_id, 0, NULL);

   if (line_db == NULL) return -1;

   line_db->flags |= ED_LINE_DIRTY;

	editor_line_set_modification_time (line_db);
   return editor_db_update_item (ActiveLinesDB, line_id);
}


int editor_line_committed (int line_id) {
	
	return editor_db_item_committed (ActiveLinesDB, line_id);	
}


int editor_line_begin_commit (void) {

	return editor_db_begin_commit (ActiveLinesDB);	
}


void editor_line_confirm_commit (int id) {

	editor_db_confirm_commit (ActiveLinesDB, id);	
}


int editor_line_items_pending (void) {

	return editor_db_items_pending (ActiveLinesDB);	
}


static void add_point (editor_line_copy_points *copy_points, RoadMapPosition *position) {

	int delta_lon = position->longitude - copy_points->last_position.longitude;
	int delta_lat = position->latitude - copy_points->last_position.latitude;
	int shape_id;
	
	if (delta_lon == 0 && delta_lat == 0) return;
	
	shape_id = editor_shape_add (0, delta_lon, delta_lat, -1);
	
	if (copy_points->first_shape == -1) {
		copy_points->first_shape = shape_id;
	}
	copy_points->last_shape = shape_id;
	copy_points->last_position = *position;
}


static int handle_segment (const PluginLine *line, void *context, int extend_flags) {
	
	editor_line_copy_points		*copy_points = (editor_line_copy_points *)context;
	RoadMapPosition				from;
	RoadMapPosition				to;
	int								first;
	int								last;
	RoadMapShapeItr				itr;
	int								shape;
	
	roadmap_plugin_get_line_points (line, &from, &to, &first, &last, &itr);
	add_point (copy_points, &from);

	if (first >= 0) {
		for (shape = first; shape <= last; shape++) {
			if (itr)
				itr (shape, &from);
			else
				roadmap_shape_get_position (shape, &from);
			add_point (copy_points, &from);
		}
	} 
	
	return 0;
} 


int editor_line_copy (PluginLine *line, int street) {

	editor_line_copy_points		copy_points;
	RoadMapPosition				from_pos;
	RoadMapPosition				to_pos;
	int								trkseg;
	int								from_point;
	int								to_point;
	int								line_id;
	int								direction;
	int								id_from;
	int								id_to;
		
	// collect shape points for trkseg
	roadmap_plugin_line_from (line, &copy_points.last_position);
	copy_points.first_shape = -1;
	copy_points.last_shape = -1; 
	
	// we assume that in case of a line broken because of tiles,
	// we begin with the original "from" segment
	handle_segment (line, &copy_points, FLAG_EXTEND_TO);
	roadmap_street_extend_line_ends (line, &from_pos, &to_pos, FLAG_EXTEND_TO, 
												  handle_segment, &copy_points);
												  
	if (line->plugin_id == EditorPluginID) {

		editor_line_get_points (line->line_id, &from_point, &to_point); 
	} else {
	
		roadmap_square_set_current (line->square);
		roadmap_line_point_ids (line->line_id, &id_from, &id_to);
		
		from_point = editor_point_add (&from_pos, id_from);
		to_point = editor_point_add (&to_pos, id_to);
	}
	
	trkseg = editor_trkseg_add (-1, -1, from_point, 
										 copy_points.first_shape, copy_points.last_shape,
										 -1, -1, 0, 0, ED_TRKSEG_FAKE);
										 
	direction = roadmap_plugin_get_direction (line, ROUTE_DIRECTION_ANY);									 
	line_id = editor_line_add (from_point, to_point, trkseg, line->cfcc, direction, street, ED_LINE_DIRTY);	
		
	return line_id;	
}


