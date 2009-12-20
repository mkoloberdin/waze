/* roadmap_line_route.h - Manage the line route data.
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

#ifndef _ROADMAP_LINE_ROUTE__H_
#define _ROADMAP_LINE_ROUTE__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_db_line_route.h"

#define ROUTE_DIRECTION_NONE         0
#define ROUTE_DIRECTION_WITH_LINE    1
#define ROUTE_DIRECTION_AGAINST_LINE 2
#define ROUTE_DIRECTION_ANY          3

int roadmap_line_route_get_direction (int line, int who);
int roadmap_line_route_is_low_weight (int line);

int roadmap_line_route_get_flags
      (int line, LineRouteFlag *from, LineRouteFlag *to);

int roadmap_line_route_get_speed_limit
      (int line, LineRouteMax *from, LineRouteMax *to);

int roadmap_line_route_get_restrictions (int line, int against_dir);

extern roadmap_db_handler RoadMapLineRouteHandler;

#endif // _ROADMAP_LINE_ROUTE__H_

