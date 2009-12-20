/* editor_point.c - point databse layer
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
 *   See editor_point.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"

#include "editor_db.h"
#include "editor_point.h"

static editor_db_section *ActivePointsDB;

static void editor_points_activate (editor_db_section *context) {
   ActivePointsDB = context;
}

editor_db_handler EditorPointsHandler = {
   EDITOR_DB_POINTS,
   sizeof (editor_db_point),
   0,
   editor_points_activate
};

int editor_point_add (RoadMapPosition *position, int db_id) {
   
   editor_db_point point;
   int id;

   point.longitude = position->longitude;
   point.latitude = position->latitude;
   point.db_id = db_id;

   id = editor_db_add_item (ActivePointsDB, &point, 1);
   return id;
}


void editor_point_position (int point, RoadMapPosition *position) {

   editor_db_point *point_st = editor_db_get_item (ActivePointsDB, point, 0, 0);
   assert(point_st != NULL);

   position->longitude = point_st->longitude;
   position->latitude = point_st->latitude;
}


int editor_point_db_id  (int point) {
	
   editor_db_point *point_st = editor_db_get_item (ActivePointsDB, point, 0, 0);
   assert(point_st != NULL);
   
   return point_st->db_id;
}


int editor_point_set_pos (int point, RoadMapPosition *position) {

   editor_db_point *point_st = editor_db_get_item (ActivePointsDB, point, 0, 0);
   assert(point_st != NULL);

   point_st->longitude = position->longitude;
   point_st->latitude = position->latitude;
   point_st->db_id = -1;

   return editor_db_update_item (ActivePointsDB, point);
}
