/* roadmap_adjust.h - Convert a GPS position into a map position.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module adjusts a raw GPS position to fit the RoadMap map.
 *
 *   There are several reason why we may want to convert what should
 *   be the same geographic coordinates:
 *
 *   * because the map is not accurate enough. The map should be fixed,
 *     really, to avoid any overhead. Fixing a map is a huge undertaking
 *     and in the mean time adjusting is the quick solution. In addition
 *     the patching mechanism may also become the way to gather and share
 *     fixes.
 *
 *   * because the GPS datum is not the same as the map's one. We should
 *     really change the GPS datum, or else do this in gpsd. Oh well..
 */

#ifndef _ROADMAP_ADJUST__H_
#define _ROADMAP_ADJUST__H_

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gps.h"

int  roadmap_adjust_offset_latitude (void);
int  roadmap_adjust_offset_longitude (void);

void roadmap_adjust_position
         (const RoadMapGpsPosition *gps, RoadMapPosition *map);

void roadmap_adjust_initialize (void);

#endif // _ROADMAP_ADJUST__H_

