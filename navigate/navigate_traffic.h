/* navigate_traffic.h - MAnage traffic statistics
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#ifndef INCLUDE__NAVIGATE_TRAFFIC__H
#define INCLUDE__NAVIGATE_TRAFFIC__H

#include "roadmap_canvas.h"

void navigate_traffic_initialize (void);
void navigate_traffic_adjust_layer (int layer, int thickness, int pen_count);
void navigate_traffic_display (int status);
int navigate_traffic_override_pen (int line, int cfcc, int fips, int pen_type,
                                   RoadMapPen *override_pen);

void navigate_traffic_refresh (void);

#endif // INCLUDE__NAVIGATE_TRAFFIC__H

