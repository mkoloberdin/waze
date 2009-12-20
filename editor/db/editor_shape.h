/* editor_shape.h - database layer
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

#ifndef INCLUDE__EDITOR_SHAPE__H
#define INCLUDE__EDITOR_SHAPE__H

#include <time.h>
#include "roadmap_types.h"
#include "editor/db/editor_db.h"


typedef struct editor_db_shape_s {
	int ordinal;
   short delta_longitude;
   short delta_latitude;
   short delta_time;
   short filler;
} editor_db_shape;


int editor_shape_add (int ordinal,
							 short delta_longitude,
                      short delta_latitude,
                      short delta_time);

void editor_shape_position (int shape, RoadMapPosition *position);
void editor_shape_time (int shape, time_t *time);
int editor_shape_ordinal (int shape);

void editor_shape_adjust_point (int shape,
                                int lon_diff,
                                int lat_diff,
                                int time_diff);

void editor_shape_set_point (int shape,
                             int lon_diff,
                             int lat_diff,
                             int time_diff);

extern editor_db_handler EditorShapeHandler;

#endif // INCLUDE__EDITOR_SHAPE__H

