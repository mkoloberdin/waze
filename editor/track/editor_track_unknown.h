/* editor_track_unknown.h - handle unknown points
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

#ifndef INCLUDE__EDITOR_TRACK_UNKNOWN__H
#define INCLUDE__EDITOR_TRACK_UNKNOWN__H

#include "roadmap_navigate.h"
#include "editor_track_main.h"

int editor_track_unknown_locate_point (int point_id,
                                       const RoadMapGpsPosition *gps_position,
                                       RoadMapTracking *confirmed_street,
                                       RoadMapNeighbour *confirmed_line,
                                       TrackNewSegment *new_segment,
                                       int max_segments,
                                       int force);


#endif // INCLUDE__EDITOR_TRACK_UNKNOWN__H

