/* editor_track_known.h - handle known points
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

#ifndef INCLUDE__EDITOR_TRACK_KNOWN__H
#define INCLUDE__EDITOR_TRACK_KNOWN__H

#include "roadmap_navigate.h"
#include "editor_track_main.h"

int editor_track_known_num_candidates (void);
int editor_track_known_resolve (void);
void editor_track_known_reset_resolve (void);

int editor_track_known_locate_point (int point_id,
                                     const RoadMapGpsPosition *gps_position,
                                     RoadMapTracking *confirmed_street,
                                     RoadMapNeighbour *confirmed_line,
                                     RoadMapTracking *new_street,
                                     RoadMapNeighbour *new_line);

int editor_track_known_end_segment (PluginLine *previous_line,
                                    int last_point_id,
                                    PluginLine *line,
                                    RoadMapTracking *street,
                                    int is_new_track);

#endif // INCLUDE__EDITOR_TRACK_KNOWN__H

