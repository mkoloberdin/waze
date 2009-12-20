/* editor_trkseg.h - database layer
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

#ifndef INCLUDE__EDITOR_TRKSEG__H
#define INCLUDE__EDITOR_TRKSEG__H

#include <time.h>
#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "editor/db/editor_db.h"

/* flags */
#define ED_TRKSEG_FAKE         0x1
#define ED_TRKSEG_IGNORE       0x2
#define ED_TRKSEG_END_TRACK    0x4
#define ED_TRKSEG_NEW_TRACK    0x8
#define ED_TRKSEG_OPPOSITE_DIR 0x10
#define ED_TRKSEG_NO_GLOBAL    0x20
#define ED_TRKSEG_LOW_CONFID   0x40
#define ED_TRKSEG_RECORDING_ON 0x80


typedef struct editor_db_trkseg_s {
   int gps_start_time;
   int gps_end_time;
   int from_point_id;
   int to_point_id;
   int point_from;
   int first_shape;
   int last_shape;
   int user_points;
   int square_ver;
   int flags;
} editor_db_trkseg;

//void editor_trkseg_set_line (int trkseg, int line_id, int plugin_id);

int editor_trkseg_add (int from_point_id,
							  int to_point_id,
                       int p_from,
                       int first_shape,
                       int last_shape,
                       time_t gps_start_time,
                       time_t gps_end_time,
                       int user_points,
                       int square_ver,
                       int flags);

void editor_trkseg_get (int trkseg,
                        int *p_from,
                        int *first_shape,
                        int *last_shape,
                        int *flags);

void editor_trkseg_get_points (int trkseg, int *from_point_id, int *to_point_id, int *user_points, int *square_ver);

void editor_trkseg_get_time (int trkseg,
                             time_t *start_time,
                             time_t *end_time);

/*
void editor_trkseg_connect_roads (int previous, int next);
void editor_trkseg_connect_global (int previous, int next);
int editor_trkseg_next_in_road (int trkseg);
int editor_trkseg_next_in_global (int trkseg);

int editor_trkseg_split (int trkseg,
                         RoadMapPosition *line_from,
                         RoadMapPosition *line_to,
                         RoadMapPosition *split_pos);

int editor_trkseg_dup (int source_id);

int editor_trkseg_get_current_trkseg (void);
void editor_trkseg_reset_next_export (void);
int editor_trkseg_get_next_export (void);
void editor_trkseg_set_next_export (int id);
*/

int editor_trkseg_get_count (void);

int editor_trkseg_begin_commit (void);
void editor_trkseg_confirm_commit (int id);
int editor_trkseg_item_committed (int item_id);
int editor_trkseg_items_pending (void);

extern editor_db_handler EditorTrksegHandler;

#endif // INCLUDE__EDITOR_TRKSEG__H

