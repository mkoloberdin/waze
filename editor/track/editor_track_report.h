/* editor_track_report.h - prepare track data for export
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

#ifndef INCLUDE__EDITOR_TRACK_REPORT__H
#define INCLUDE__EDITOR_TRACK_REPORT__H

#include "roadmap_types.h"
#include "roadmap_gps.h"

#define  INVALID_NODE_ID                     (-1)
#define  INVALID_COORDINATE                  (-1)
#define  INVALID_ROADMAPPOSITION             {INVALID_COORDINATE,INVALID_COORDINATE}
#define  INVALID_POINTINTIME                 {INVALID_ROADMAPPOSITION,0}

#define  ROADMAPPOSITION_IS_INVALID(_pos_)   ((INVALID_COORDINATE==_pos_.longitude)||(INVALID_COORDINATE==_pos_.latitude))
#define  GPSPOINTINTIME_IS_INVALID(_rm_)     ROADMAPPOSITION_IS_INVALID(_rm_.Position)

// A single GPS point
typedef struct tagGPSPointInTime
{
   RoadMapPosition   Position;
   int               altitude;
   time_t            GPS_time;
}  GPSPointInTime, *LPGPSPointInTime;

// A single node
typedef struct tagNodeInTime
{
   int      node;
   time_t   GPS_time;
   
}  NodeInTime, *LPNodeInTime;

// User points
typedef struct tagUserPointsVer
{
   int      points;
   int      version;
   
}  UserPointsVer, *LPUserPointsVer;

// path information
typedef struct
{
	int					max_nodes;
	int					num_nodes;
	LPNodeInTime		nodes;
	int					max_points;
	int					num_points;
	LPGPSPointInTime	points;
	int					max_toggles;
	int					num_update_toggles;
	time_t				*update_toggle_times;
	int					first_update_toggle_state;
   int               max_user_points;
   int               num_user_points;
   LPUserPointsVer   user_points;
}	RTPathInfo;


void editor_track_report_init (void);
void editor_track_report_reset (void);
RTPathInfo *editor_track_report_begin_export (int offline);
void editor_track_report_conclude_export (int success);
int editor_track_report_get_current_position (RoadMapGpsPosition*  GPS_position, 
                               			 		 int*                 from_node,
                               			 		 int*                 to_node,
                               			 		 int*                 direction);

#endif // INCLUDE__EDITOR_TRACK_REPORT__H

