/* roadmap_trip.h - Manage a trip: destination & waypoints.
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
 */

#ifndef INCLUDE__ROADMAP_TRIP__H
#define INCLUDE__ROADMAP_TRIP__H

#include "roadmap_types.h"
#include "roadmap_list.h"
#include "roadmap_gps.h"

#define ROADMAP_DEFAULT_LOC_LONGITUDE 34794810
#define ROADMAP_DEFAULT_LOC_LATITUDE   32106010

#define IS_DEFAULT_LOCATION( loc_ptr ) ( ( (loc_ptr)->longitude == ROADMAP_DEFAULT_LOC_LONGITUDE ) && \
									 ( (loc_ptr)->latitude == ROADMAP_DEFAULT_LOC_LATITUDE ) )
enum { TRIP_FOCUS_GPS = -1,
       TRIP_FOCUS_NO_GPS = 0};

void  roadmap_trip_set_point (const char *name,
                              const RoadMapPosition *position);

void  roadmap_trip_set_mobile (const char *name,
                               const RoadMapGpsPosition *gps_position);

void roadmap_trip_set_gps_position (const char *name, const char*sprite, const char* image,
                              const RoadMapGpsPosition *gps_position);

void roadmap_trip_set_gps_and_nodes_position (const char *name, const char*sprite, const char* image,
							  const RoadMapGpsPosition *gps_position, int from_node, int to_node );

void  roadmap_trip_copy_focus (const char *name);

void  roadmap_trip_set_selection_as (const char *name);

void  roadmap_trip_remove_point (const char *name);


void  roadmap_trip_restore_focus (void);
void  roadmap_trip_set_focus (const char *name);

int   roadmap_trip_is_focus_changed  (void);
int   roadmap_trip_is_focus_moved    (void);
int   roadmap_trip_is_refresh_needed (void);

int   roadmap_trip_get_orientation (void);
const char *roadmap_trip_get_focus_name (void);

const RoadMapPosition *roadmap_trip_get_focus_position (void);
const RoadMapPosition *roadmap_trip_get_position (const char *name);
const RoadMapGpsPosition *roadmap_trip_get_gps_position(const char*name);

void roadmap_trip_get_nodes(const char *name, int *from_node, int *to_node );

void  roadmap_trip_start   (void);
void  roadmap_trip_resume  (void);
void  roadmap_trip_stop    (void);
void  roadmap_trip_reverse (void);

void  roadmap_trip_display (void);

void  roadmap_trip_new (void);

void  roadmap_trip_initialize (void);

const char *roadmap_trip_current (void);

/* In the two primitives that follow, the name is either NULL (i.e.
 * open a dialog to let the user enter one), or an explicit name.
 */
int   roadmap_trip_load (const char *name, int silent);
void  roadmap_trip_save (const char *name);

void roadmap_trip_save_screenshot (void);

int roadmap_trip_gps_state (void) ;

#endif // INCLUDE__ROADMAP_TRIP__H

