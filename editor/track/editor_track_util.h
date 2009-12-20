/* editor_track_util.h - misc functions
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

#ifndef INCLUDE__EDITOR_TRACK_UTIL__H
#define INCLUDE__EDITOR_TRACK_UTIL__H

#include "roadmap_navigate.h"
#include "editor_track_main.h"

int editor_track_util_get_distance (const RoadMapPosition *point,
                                    const PluginLine *line,
                                    RoadMapNeighbour *result);

int editor_track_util_find_street
                     (const RoadMapGpsPosition *gps_position,
                      RoadMapTracking *candidate,
                      RoadMapTracking *previous_street,
                      RoadMapNeighbour *previous_line,
                      RoadMapNeighbour *neighbourhood,
                      int max,
                      int *found,
                      RoadMapFuzzy *best,
                      RoadMapFuzzy *second_best);

int editor_track_util_create_line (int gps_first_point,
                                   int gps_last_point,
                                   int from_point,
                                   int to_point,
                                   int cfcc,
                                   int is_new_track);

int editor_track_util_connect_roads (PluginLine *from,
                                     PluginLine *to,
                                     int from_opposite_direction,
                                     int to_opposite_direction,
                                     const RoadMapGpsPosition *gps_position,
                                     int last_point_id);

int editor_track_util_new_road_start (RoadMapNeighbour *line,
                                      const RoadMapPosition *pos,
                                      int last_point_id,
                                      int opposite_direction,
                                      NodeNeighbour *node);

int editor_track_util_new_road_end (RoadMapNeighbour *line,
                                    const RoadMapPosition *pos,
                                    int last_point_id,
                                    int opposite_direction,
                                    NodeNeighbour *node);


int editor_track_util_length (int first, int last);

int editor_track_util_create_trkseg (int square,
												 int line_id,
                                     int plugin_id,
                                     int first_point,
                                     int last_point,
                                     int flags);

void editor_track_add_trkseg
   (PluginLine *line, int trkseg, int direction, int who);
   
int editor_track_util_create_db (const RoadMapPosition *pos);

void editor_track_util_set_focus(const RoadMapPosition *position);
void editor_track_util_release_focus();

int editor_track_util_roadmap_node_to_editor (NodeNeighbour *node);

int editor_track_util_get_line_length (const PluginLine *line);
void editor_track_util_get_line_point_ids (const PluginLine *line, 
														 int reverse, 
														 int *from_id, 
														 int *to_id);

#endif // INCLUDE__EDITOR_TRACK_UTIL__H

