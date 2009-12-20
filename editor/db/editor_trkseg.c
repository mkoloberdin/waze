/* editor_trkseg.c - point databse layer
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
 *   See editor_trkseg.h
 */

#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_street.h"
#include "roadmap_navigate.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_trkseg.h"

static editor_db_section *ActiveTrksegDB;

static void editor_trkseg_activate (editor_db_section *context) {
   ActiveTrksegDB = context;
}

editor_db_handler EditorTrksegHandler = {
   EDITOR_DB_TRKSEGS,
   sizeof (editor_db_trkseg),
   1,
   editor_trkseg_activate
};


void editor_trkseg_get_time (int trkseg,
                             time_t *start_time,
                             time_t *end_time) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);

   assert (track != NULL);
   if (track == NULL) return;

   *start_time = track->gps_start_time;
   *end_time = track->gps_end_time;
}


int editor_trkseg_add (int from_point_id,
							  int to_point_id,
                       int p_from,
                       int first_shape,
                       int last_shape,
                       time_t gps_start_time,
                       time_t gps_end_time,
                       int user_points,
                       int square_ver,
                       int flags) {

   editor_db_trkseg track;
   int id;

	track.from_point_id		 = from_point_id;
   track.to_point_id        = to_point_id;
   track.point_from         = p_from;
   track.first_shape        = first_shape;
   track.last_shape         = last_shape;
   track.gps_start_time     = (int)gps_start_time;
   track.gps_end_time       = (int)gps_end_time;
   track.user_points        = user_points;
   track.square_ver         = square_ver;
   track.flags              = flags;

   id = editor_db_add_item (ActiveTrksegDB, &track, 1);

   return id;
}


void editor_trkseg_get (int trkseg,
                        int *p_from,
                        int *first_shape,
                        int *last_shape,
                        int *flags) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);

   if (p_from != NULL) *p_from = track->point_from;
   if (first_shape != NULL) *first_shape = track->first_shape;
   if (last_shape != NULL) *last_shape = track->last_shape;
   if (flags != NULL) *flags = track->flags;
}


void editor_trkseg_get_points (int trkseg, int *from_point_id, int *to_point_id, int *user_points, int *square_ver) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                                 (ActiveTrksegDB, trkseg, 0, NULL);

   assert (track != NULL);

	*from_point_id = track->from_point_id;
	*to_point_id = track->to_point_id;
   *user_points = track->user_points;
   *square_ver = track->square_ver;
}


int editor_trkseg_get_count (void) {
   
   return editor_db_get_item_count (ActiveTrksegDB);
}


int editor_trkseg_begin_commit (void) {

	return editor_db_begin_commit (ActiveTrksegDB);	
}


void editor_trkseg_confirm_commit (int id) {

	editor_db_confirm_commit (ActiveTrksegDB, id);	
}


int editor_trkseg_item_committed (int item_id) {

	return editor_db_item_committed (ActiveTrksegDB, item_id);	
}


int editor_trkseg_items_pending (void) {

	return editor_db_items_pending (ActiveTrksegDB);	
}
