/* roadmap_db_line_route.h - the format of a line's route data
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
 *   RoadMap's lines route data is described by the following table:
 *
 *   line_route for each line its direction and cross time and allowed speed are specified
 */

#ifndef _ROADMAP_DB_LINE_ROUTE__H_
#define _ROADMAP_DB_LINE_ROUTE__H_

#include "roadmap_types.h"

#define ROUTE_CAR_ALLOWED 0x1
#define ROUTE_LOW_WEIGHT  0x2

typedef struct {  /* table line_route */

   unsigned char from_flags;
   unsigned char to_flags;
   unsigned char from_turn_res;
   unsigned char to_turn_res;

} RoadMapLineRoute;

#endif // _ROADMAP_DB_LINE_ROUTE__H_

