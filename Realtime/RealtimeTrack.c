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


#include <string.h>
#include <time.h>
#include "../roadmap_math.h"
#include "RealtimeMath.h"

#include "RealtimeTrack.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static   time_t   gs_TimeOfLastPoint                        = 0;
static   time_t   gs_TimeOfFirstGPSPointAfterDisconnection  = 0;

void GPSPointInTime_Init( LPGPSPointInTime this)
{ memset( this, 0, sizeof(GPSPointInTime));}

void NodeInTime_Init( LPNodeInTime this)
{ memset( this, 0, sizeof(NodeInTime));}   

void RealtimePathTrack_Init( LPRealtimePathTrack this)
{
   int i;
   memset( this, 0, sizeof(RealtimePathTrack));
   
   for( i=0; i<RTTRK_GPSPATH_MAX_POINTS; i++)
      GPSPointInTime_Init( &(this->points[i]));
   for( i=0; i<RTTRK_NODEPATH_MAX_POINTS; i++)
      NodeInTime_Init( &(this->nodes[i]));
      
   this->last_noded_added = INVALID_NODE_ID;
} 

void RealtimePathTrack_ResetPoints( LPRealtimePathTrack this)
{
   int         i;
   LPTrackInfo pTI = &(this->points_track_info);
   
   roadmap_log( ROADMAP_DEBUG, "RealtimePathTrack_ResetPoints() -\tStatistics: Have %d points; "
                                    "%d were removed by the MIN-DISTANCE threshold; "
                                    "%d were removed by the VARIANT threshold; ",
                                    pTI->count,
                                    this->debug_GPS_points_removed_min_distance,
                                    this->debug_GPS_points_removed_variant_threshold);
                                    

   if( pTI->count)
   {
      GPSPointInTime Last = this->points[pTI->count-1];
      
      for( i=0; i<pTI->count; i++)
         GPSPointInTime_Init( &(this->points[i]));
      
      this->points[0]   = Last;
      pTI->count        = 1;
      pTI->period_begin = Last.GPS_time;
   }
   else
      pTI->period_begin = 0;
      
   pTI->period_end = 0;
   
   this->debug_GPS_points_removed_min_distance       = 0;
   this->debug_GPS_points_removed_variant_threshold  = 0;
}

void RealtimePathTrack_ResetNodes( LPRealtimePathTrack this)
{
   int         i;
   LPTrackInfo pTI = &(this->nodes_track_info);

   for( i=0; i<pTI->count; i++)
      NodeInTime_Init( &(this->nodes[i]));
   pTI->period_begin  = 0;
   pTI->period_end    = 0;
   pTI->count         = 0;
}

int point_distance_from_expected_position(   const GPSPointInTime*   range_begin,
                                             const GPSPointInTime*   range_end,
                                             const GPSPointInTime*   point)
{
   double            factor      = time_relative_part_factor_from_absolute_value(
                                                                        range_begin ->GPS_time, 
                                                                        range_end   ->GPS_time, 
                                                                        point       ->GPS_time);
   RoadMapPosition   expected_pos= RoadMapPosition_from_relative_part_factor(  
                                                                        range_begin ->Position,
                                                                        range_end   ->Position,
                                                                        factor);
   return roadmap_math_distance( &(point->Position), &expected_pos);
}

// Implementation note: This method can add up to n items
//                      Once n was reached, any new item will override last item
BOOL  RealtimePathTrack_AddPoint(LPRealtimePathTrack  this, 
                                 const int            longtitude, 
                                 const int            latitude, 
                                 time_t               GPS_time)
{
   RoadMapPosition   new_position;
   LPTrackInfo       pTI = &(this->points_track_info);

   new_position.longitude  = longtitude;
   new_position.latitude   = latitude;      
   pTI->period_end         = GPS_time;

   // Initialize value:
   if( !gs_TimeOfFirstGPSPointAfterDisconnection)
      gs_TimeOfFirstGPSPointAfterDisconnection = GPS_time;
   
   // IF GPS WAS DISCONNECTED FOR A WHILE - INSERT A DUMMY POINT (INVALID_COORDINATE,INVALID_COORDINATE,0)   
   if( gs_TimeOfLastPoint)
   {
      time_t   SecondsPassedSinceLastGPSPoint = GPS_time - gs_TimeOfLastPoint;
      
      if( RTTRK_MAX_SECONDS_BETWEEN_GPS_POINTS < SecondsPassedSinceLastGPSPoint)
      {
         gs_TimeOfLastPoint                        = 0;
         gs_TimeOfFirstGPSPointAfterDisconnection  = GPS_time;
         
         roadmap_log(ROADMAP_DEBUG, 
                     "RealtimePathTrack_AddPoint(GPS-DISCONNECTION TAG) - %d seconds passed lince last GPS point - INSERTING A NULL POINT AS A DELIMITER (pos: %d)",
                     SecondsPassedSinceLastGPSPoint, pTI->count);
         
         // RECURSION //
         RealtimePathTrack_AddPoint(this,INVALID_COORDINATE,INVALID_COORDINATE,0);
      }
   }
   
   if( INVALID_COORDINATE != longtitude)
   {
      gs_TimeOfLastPoint = GPS_time;

      if( pTI->count)
      {
         RoadMapPosition   last_position              = this->points[pTI->count-1].Position;
         int               distance_from_last_position= roadmap_math_distance( &last_position, &new_position);
         if( distance_from_last_position < RTTRK_MIN_DISTANCE_BETWEEN_POINTS)
         {
            this->debug_GPS_points_removed_min_distance++;
            return TRUE;
         }
      }
   }

   this->points[pTI->count].Position  = new_position;
   this->points[pTI->count].GPS_time  = GPS_time;
   
   if( 0 == pTI->period_begin)
      pTI->period_begin = GPS_time;

   if(pTI->count < (RTTRK_GPSPATH_MAX_POINTS-1))
      pTI->count++;
   else
   {
      int   size_before = pTI->count;
      int   size_after;
      float ratio;
   
      RealtimePathTrack_Compress( this);
      size_after  = pTI->count;
      ratio       = (float)size_after/(float)size_before;
      
      if( RTTRK_COMPRESSION_RATIO_THRESHOLD < ratio)
      {
         roadmap_log( ROADMAP_WARNING, "RealtimePathTrack_AddPoint() - Queue is full and compression ratio reached %f; Emptying both queues (GPS points and line-id)", ratio);
         RealtimePathTrack_ResetPoints ( this);
         RealtimePathTrack_ResetNodes  ( this);
      }
   }
   
   return TRUE;
}

// Implementation note: This method can add up to n items
//                      Once n was reached, any new item will override last item
BOOL  RealtimePathTrack_AddNode( LPRealtimePathTrack  this, 
                                 const int            node, 
                                 time_t               GPS_time)
{
   LPTrackInfo pTI = &(this->nodes_track_info);
   pTI->period_end = GPS_time;
   
   if( gs_TimeOfFirstGPSPointAfterDisconnection == GPS_time)
   {
      roadmap_log(ROADMAP_DEBUG, 
                  "RealtimePathTrack_AddNode() - This GPS_time is a first node after a GPS disconnection - IGNORING NODE %d !!!",
                  node);
      return TRUE;
   }
   
   if( pTI->count)
   {
      if( node == this->nodes[pTI->count-1].node)
         return TRUE;   // Node did not change...
   }
   else
   {
      if( node == this->last_noded_added)
         return TRUE;   // This first-item is equal to previous send's last-item
   }

   this->nodes[pTI->count].node     = node;
   this->nodes[pTI->count].GPS_time = GPS_time;
   this->last_noded_added           = node;
   
   if( 0 == pTI->period_begin)
      pTI->period_begin = GPS_time;

   if(pTI->count < (RTTRK_NODEPATH_MAX_POINTS-1))
      pTI->count++;
#ifdef _DEBUG
   else
      assert(0);
#endif   // _DEBUG      
   
   return TRUE;
}

void  RealtimePathTrack_ErasePoint( LPRealtimePathTrack this, int index)
{
   int         i;
   LPTrackInfo pTI = &(this->points_track_info);

   if( (index < 0) || (pTI->count <= index))
      return;

   for( i=index; i<(pTI->count-1); i++)
      this->points[i] = this->points[i+1];
   GPSPointInTime_Init( &(this->points[i]));
   pTI->count--;
}

void  RealtimePathTrack_EraseNode( LPRealtimePathTrack this, int index)
{
   int         i;
   LPTrackInfo pTI = &(this->nodes_track_info);
   
   if( (index < 0) || (pTI->count <= index))
      return;
   
   for( i=index; i<(pTI->count-1); i++)
      this->nodes[i] = this->nodes[i+1];
   NodeInTime_Init( &(this->nodes[i]));
   pTI->count--;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RealtimePathTrack_CompressRange( LPRealtimePathTrack this, const int from, const int to)
{
   int               i;
   LPGPSPointInTime  range_begin = &(this->points[from]);
   LPGPSPointInTime  range_end   = &(this->points[to]);
   int               distance    =  0;
   int               farest      = -1;    // Index of the most distanced point
   
   range_begin->ToBeSaved = TRUE;
   range_end  ->ToBeSaved = TRUE;
   
   if( (to - from) < 2)
      return;

   for( i=(from+1); i<to; i++)
   {
      LPGPSPointInTime  point       = &(this->points[i]);
      int               cur_distance= point_distance_from_expected_position( 
                                                         range_begin,   // Range begin
                                                         range_end,     // Range end
                                                         point);        // Point to test

      if(distance < cur_distance)
      {
         distance = cur_distance;
         farest   = i;
      }
   }
   
   if( (-1 == farest) || (distance < RTTRK_MIN_VARIANT_THRESHOLD))
      return;

   RealtimePathTrack_CompressRange( this, from,   farest);
   RealtimePathTrack_CompressRange( this, farest, to);
}

void  RealtimePathTrack_Compress( LPRealtimePathTrack this)
{
   int         i;
   int         iRangeBegin;
   int         iGPSDisconnectionTagsCount = 0;
   LPTrackInfo pTI = &(this->points_track_info);

   if( pTI->count < 3)
      return;
   
   for( i=0; i<pTI->count; i++)
      this->points[i].ToBeSaved = FALSE;

   iRangeBegin = 0;
   for( i=0; i<pTI->count; i++)
   {
      LPGPSPointInTime  point = &(this->points[i]);
      if( ROADMAPPOSITION_IS_INVALID(point->Position))
      {
         int iRangeEnd     = i-1;
         point->ToBeSaved  = TRUE;
         
         roadmap_log(ROADMAP_DEBUG, 
                     "RealtimePathTrack_Compress(GPS-DISCONNECTION TAG) - Found tag (%d) at position %d", ++iGPSDisconnectionTagsCount, i);

         if( iRangeBegin < iRangeEnd)
         {
            roadmap_log(ROADMAP_DEBUG, 
                        "RealtimePathTrack_Compress(GPS-DISCONNECTION TAG) - Compressing between %d to %d", 
                        iRangeBegin, iRangeEnd);
            RealtimePathTrack_CompressRange( this, iRangeBegin, iRangeEnd);
         }
         else
            roadmap_log(ROADMAP_DEBUG, 
                        "RealtimePathTrack_Compress(GPS-DISCONNECTION TAG) - Do not have enough items to compres!");

         iRangeBegin = i+1;
      }
   }
   
   if( iRangeBegin < (pTI->count - 1))
   {
      roadmap_log(ROADMAP_DEBUG, 
                  "RealtimePathTrack_Compress() - Compressing between %d to %d (count-1)", 
                  iRangeBegin, (pTI->count - 1));
      RealtimePathTrack_CompressRange( this, iRangeBegin, (pTI->count - 1));
   }
   
   // Remove single points:
   for( i=0; i<pTI->count; i++)
   {
      if(   ((i == 0)            || GPSPOINTINTIME_IS_INVALID(this->points[i-1])) &&
            (((i+1)==pTI->count) || GPSPOINTINTIME_IS_INVALID(this->points[i+1])))
      {
         int iTagToRemove;
         
         if( 0==i)
            iTagToRemove = i+1;
         else
            iTagToRemove = i-1;

         roadmap_log(ROADMAP_DEBUG, 
                     "RealtimePathTrack_Compress(GPS-DISCONNECTION TAG) - Removing the single GPSpoint[%d] and the neigbour disconnection-tag point GPSpoint[%d]",
                     i, iTagToRemove);

         this->points[i]            .ToBeSaved = FALSE;
         this->points[iTagToRemove] .ToBeSaved = FALSE;
      }
   }

   for( i=0; i<pTI->count; i++)
   {
   
      if( FALSE == this->points[i].ToBeSaved)
      {
         RealtimePathTrack_ErasePoint( this, i);
         i--;
         this->debug_GPS_points_removed_variant_threshold++;
      }
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
