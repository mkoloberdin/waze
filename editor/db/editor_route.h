/* editor_route.h - database layer
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

#ifndef INCLUDE__EDITOR_ROUTE__H
#define INCLUDE__EDITOR_ROUTE__H

typedef struct editor_route_segment_s {
   LineRouteFlag from_flags;
   LineRouteFlag to_flags;
   LineRouteMax from_speed_limit;
   LineRouteMax to_speed_limit;
} editor_db_route_segment;


int editor_route_segment_add
      (LineRouteFlag from_flags,
       LineRouteFlag to_flags,
       LineRouteMax from_speed_limit,
       LineRouteMax to_speed_limit);

void editor_route_segment_get
      (int route,
       LineRouteFlag *from_flags,
       LineRouteFlag *to_flags,
       LineRouteMax *from_speed_limit,
       LineRouteMax *to_speed_limit);

void editor_route_segment_set
      (int route,
       LineRouteFlag from_flags,
       LineRouteFlag to_flags,
       LineRouteMax from_speed_limit,
       LineRouteMax to_speed_limit);

void editor_route_segment_copy (int source_line, int plugin_id, int dest_line);

int editor_route_get_direction (int route_id, int who);

extern editor_db_handler EditorRouteHandler;

#endif // INCLUDE__EDITOR_ROUTE__H

