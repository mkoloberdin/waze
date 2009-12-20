/* editor_point.h - database layer
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

#ifndef INCLUDE__EDITOR_POINT__H
#define INCLUDE__EDITOR_POINT__H

#include "roadmap_types.h"
#include "editor/db/editor_db.h"


typedef struct editor_db_point_s {
   int longitude;
   int latitude;
   int db_id;
} editor_db_point;

#define EDITOR_POINT_TYPE_MUNCHING 1

int editor_point_add (RoadMapPosition *position, int db_id);
void editor_point_position  (int point, RoadMapPosition *position);
int editor_point_db_id  (int point);
int editor_point_set_pos (int point, RoadMapPosition *position);


extern editor_db_handler EditorPointsHandler;

#endif // INCLUDE__EDITOR_POINT__H

