/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
XXX THIS FILE IS OBSOLETE XXX

#ifndef  __FREEMAP_REALTIMETRACK_H__
#define  __FREEMAP_REALTIMETRACK_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include "RealtimeNetDefs.h"

#define  INVALID_NODE_ID                     (-1)
#define  INVALID_COORDINATE                  (-1)
#define  INVALID_ROADMAPPOSITION             {INVALID_COORDINATE,INVALID_COORDINATE}
#define  INVALID_POINTINTIME                 {INVALID_ROADMAPPOSITION,0,TRUE}

#define  ROADMAPPOSITION_IS_INVALID(_pos_)   ((INVALID_COORDINATE==_pos_.longitude)||(INVALID_COORDINATE==_pos_.latitude))
#define  GPSPOINTINTIME_IS_INVALID(_rm_)     ROADMAPPOSITION_IS_INVALID(_rm_.Position)

// A single GPS point
typedef struct tagGPSPointInTime
{
   RoadMapPosition   Position;
   time_t            GPS_time;
   BOOL              ToBeSaved;
   
}  GPSPointInTime, *LPGPSPointInTime;

void GPSPointInTime_Init( LPGPSPointInTime this);

// A single node
typedef struct tagNodeInTime
{
   int      node;
   time_t   GPS_time;
   
}  NodeInTime, *LPNodeInTime;

void NodeInTime_Init( LPNodeInTime this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagTrackInfo
{
   int      count;
   time_t   period_begin;
   time_t   period_end;

}  TrackInfo, *LPTrackInfo;

typedef struct tagRealtimePathTrack
{
   GPSPointInTime points[RTTRK_GPSPATH_MAX_POINTS];
   NodeInTime     nodes [RTTRK_NODEPATH_MAX_POINTS];
   TrackInfo      points_track_info;
   TrackInfo      nodes_track_info;
   int            last_noded_added;
   
   int            debug_GPS_points_removed_min_distance;
   int            debug_GPS_points_removed_variant_threshold;

}  RealtimePathTrack, *LPRealtimePathTrack;

void  RealtimePathTrack_Init        ( LPRealtimePathTrack this);
void  RealtimePathTrack_ResetPoints ( LPRealtimePathTrack this);
void  RealtimePathTrack_ResetNodes  ( LPRealtimePathTrack this);
BOOL  RealtimePathTrack_AddPoint    ( LPRealtimePathTrack this, const int longtitude, const int latitude, time_t GPS_time);
BOOL  RealtimePathTrack_AddNode     ( LPRealtimePathTrack this, const int node, time_t GPS_time);
void  RealtimePathTrack_ErasePoint  ( LPRealtimePathTrack this, int index);
void  RealtimePathTrack_EraseNode   ( LPRealtimePathTrack this, int index);
void  RealtimePathTrack_Compress    ( LPRealtimePathTrack this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   // __FREEMAP_REALTIMETRACK_H__
