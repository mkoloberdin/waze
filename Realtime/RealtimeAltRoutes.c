/* RealtimeAltRoutes.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
 *
 *   This file is part of RoadMap.
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
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_social.h"
#include "../roadmap_messagebox.h"
#include "RealtimeAltRoutes.h"
#include "../roadmap_alternative_routes.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "../roadmap_trip.h"
#include "../roadmap_navigate.h"
#include "../roadmap_line_route.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
   AltRouteTrip altRoutTrip[MAX_ALT_ROUTES];
   int iCount;
} AltRoutesTripsTable;

static AltRoutesTripsTable altRoutesTrips;

static BOOL cancelled = FALSE;

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAltRoutes_Count() {
   return altRoutesTrips.iCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_Clear(void) {
   altRoutesTrips.iCount = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_Init_Record(AltRouteTrip *route) {
   int i;

   route->iTripId = -1;
   route->sTripName[0] = 0;
   route->srcPosition.latitude = -1;
   route->srcPosition.longitude = -1;
   route->destPosition.latitude = -1;
   route->destPosition.longitude = -1;
   route->sDestinationName[0] = 0;
   route->sSrcName[0] = 0;
   route->iNumRoutes = 0;
   for (i=0; i<MAX_ROUTES;i++){
      route->pRouteResults[i].alt_id = -1;
      route->pRouteResults[i].description = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
AltRouteTrip *RealtimeAltRoutes_Get_Record(int index) {
   return &altRoutesTrips.altRoutTrip[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeAltRoutes_Route_Exist(int iTripId) {
   int i;
   for (i = 0; i < altRoutesTrips.iCount; i++) {
      if (altRoutesTrips.altRoutTrip[i].iTripId == iTripId)
         return TRUE;
   }
   return FALSE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeAltRoutes_Add_Route(AltRouteTrip *pRoute) {
   if (altRoutesTrips.iCount == MAX_ALT_ROUTES) {
      roadmap_log(ROADMAP_ERROR,"RealtimeAltRoutes_Add_Route - cannot add route. table is full" );
      return FALSE;
   }

   if (!pRoute){
      roadmap_log(ROADMAP_ERROR,"RealtimeAltRoutes_Add_Route - cannot add route. route is NULL" );
      return FALSE;
   }

   roadmap_log(ROADMAP_DEBUG,"RealtimeAltRoutes_Add_Route - id=%d, name=%s", pRoute->iTripId, pRoute->sTripName );
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].iTripId = pRoute->iTripId;

   strncpy(altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].sDestinationName , pRoute->sDestinationName, sizeof(pRoute->sDestinationName));
   strncpy(altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].sSrcName , pRoute->sSrcName, sizeof(pRoute->sSrcName));
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].srcPosition.latitude = pRoute->srcPosition.latitude;
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].srcPosition.longitude = pRoute->srcPosition.longitude;
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].destPosition.latitude = pRoute->destPosition.latitude;
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].destPosition.longitude = pRoute->destPosition.longitude;

   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].iTripDistance = pRoute->iTripDistance;
   altRoutesTrips.altRoutTrip[altRoutesTrips.iCount].iTripLenght = pRoute->iTripLenght;
   altRoutesTrips.iCount++;
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnRouteResults (NavigateRouteRC rc, int num_res, const NavigateRouteResult *res){
   int i;
   if (num_res > MAX_ROUTES)
      num_res = MAX_ROUTES;


   if (rc != route_succeeded){
      ssd_progress_msg_dialog_hide ();
      roadmap_log(ROADMAP_ERROR,"RealtimeAltRoutes_OnRouteResults failed rc=%d", rc );
      return;
   }

   roadmap_log (ROADMAP_DEBUG,"RealtimeAltRoutes_OnRouteResults %d", num_res);
   altRoutesTrips.altRoutTrip[0].iNumRoutes = num_res;
   for (i = 0; i < num_res ; i++){
      altRoutesTrips.altRoutTrip[0].pRouteResults[i] = *(res+i);//todo check
   }

   altRoutesTrips.altRoutTrip[0].iTripLenght = altRoutesTrips.altRoutTrip[0].pRouteResults[0].total_time;
   altRoutesTrips.altRoutTrip[0].iTripDistance = altRoutesTrips.altRoutTrip[0].pRouteResults[0].total_length;
   roadmap_alternative_routes_routes_dialog();

   roadmap_screen_refresh();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnTripRouteResults (NavigateRouteRC rc, int num_res, const NavigateRouteResult *res){
   int i;
   if (num_res > MAX_ROUTES)
      num_res = MAX_ROUTES;


   if (rc != route_succeeded){
      roadmap_log(ROADMAP_ERROR,"RealtimeAltRoutes_OnTripRouteResults failed rc=%d", rc );
      return;
   }

   roadmap_log (ROADMAP_DEBUG,"RealtimeAltRoutes_OnRouteResults %d", num_res);
   altRoutesTrips.altRoutTrip[0].iNumRoutes = num_res;
   for (i = 0; i < num_res ; i++){
      altRoutesTrips.altRoutTrip[0].pRouteResults[i] = *(res+i);
   }

   altRoutesTrips.altRoutTrip[0].iTripLenght = altRoutesTrips.altRoutTrip[0].pRouteResults[0].total_time;
   altRoutesTrips.altRoutTrip[0].iTripDistance = altRoutesTrips.altRoutTrip[0].pRouteResults[0].total_length;

   roadmap_trip_set_point ("Destination", &altRoutesTrips.altRoutTrip[0].destPosition);
   roadmap_trip_set_point ("Departure", &altRoutesTrips.altRoutTrip[0].srcPosition);

   roadmap_alternative_routes_suggest_route_dialog();

   roadmap_screen_refresh();
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnRouteSegments (NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments){
   roadmap_log (ROADMAP_DEBUG,"RealtimeAltRoutes_OnRouteSegments");
   if (cancelled){
      roadmap_log (ROADMAP_DEBUG,"RealtimeAltRoutes_OnRouteSegments - Navigation cancelled");
      return;
   }

   navigate_main_on_route (res->flags, res->total_length, res->total_time, segments->segments,
                             segments->num_segments, segments->num_instrumented,
                             res->geometry.points, res->geometry.num_points, res->description, FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
NavigateRouteResult *RealtimeAltRoutes_Get_Route_Result(int index){
   return &altRoutesTrips.altRoutTrip[0].pRouteResults[index];
}


int RealtimeAltRoutes_Get_Num_Routes(){
   return altRoutesTrips.altRoutTrip[0].iNumRoutes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_Route_CancelRequest(void){
   cancelled = TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL RealtimeAltRoutes_GetOrigin(RoadMapGpsPosition *pos, PluginLine *line, int *fromPoint){
   int direction;

   if ((roadmap_navigate_get_current (pos, line, &direction) != -1) &&
       (roadmap_plugin_get_id(line) == ROADMAP_PLUGIN_ID)){
      int from;
      int to;
      roadmap_square_set_current (line->square);
      roadmap_line_points (line->line_id, &from, &to);
      if (direction == ROUTE_DIRECTION_WITH_LINE){
         *fromPoint = to;
      } else{
         *fromPoint = from;
      }
      return TRUE;
   }
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeAltRoutes_Route_Request(int iTripId, const RoadMapPosition *from_pos, const RoadMapPosition *to_pos, int max_routes){
	static NavigateRouteCallbacks cb = {
		NULL,
		RealtimeAltRoutes_OnRouteResults,
      RealtimeAltRoutes_OnRouteSegments,
      navigate_main_update_route,
      navigate_main_on_suggest_reroute,
      navigate_main_on_segment_ver_mismatch
	};
   RoadMapGpsPosition position;
   PluginLine fromLine;
   int fromPoint;
   cancelled = FALSE;

   if (navigate_main_get_follow_gps()){
      if (RealtimeAltRoutes_GetOrigin (&position, &fromLine, &fromPoint)){
         from_pos = (RoadMapPosition*)&position;
      }else{
         fromLine.line_id = -1;
         fromPoint = -1;
         if (roadmap_gps_have_reception()){
            from_pos = roadmap_trip_get_position ("GPS");
         }
      }
   }else{
      fromLine.line_id = -1;
      fromPoint = -1;
   }

   navigate_main_prepare_for_request();
   navigate_route_request (&fromLine,
                           fromPoint,
                           NULL,
                           from_pos,
                           to_pos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           0,
                           iTripId,
                           max_routes,
                           &cb);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnTripRouteRC (NavigateRouteRC rc, int protocol_rc, const char *description){
   if ((protocol_rc != 200) || (rc != route_succeeded)){
      roadmap_log (ROADMAP_ERROR,"RealtimeAltRoutes_OnTripRouteRC - %s (rc=%d)", description, rc);
      if (roadmap_alternative_routes_suggest_dlg_active()){
         ssd_dialog_hide_all(dec_close);
         if (!roadmap_screen_refresh())
           roadmap_screen_redraw();
         roadmap_messagebox_timeout("Oops", description, 5);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeAltRoutes_TripRoute_Request(int iTripId, const RoadMapPosition *from_pos, const RoadMapPosition *to_pos, int max_routes){
   static NavigateRouteCallbacks cb = {
      RealtimeAltRoutes_OnTripRouteRC,
      RealtimeAltRoutes_OnTripRouteResults,
      RealtimeAltRoutes_OnRouteSegments,
      navigate_main_update_route,
      navigate_main_on_suggest_reroute,
      navigate_main_on_segment_ver_mismatch
   };

   RoadMapGpsPosition position;
   PluginLine fromLine;
   int fromPoint;

   if (RealtimeAltRoutes_GetOrigin (&position, &fromLine, &fromPoint)){
      from_pos = (RoadMapPosition*)&position;
   }else{
      fromLine.line_id = -1;
      fromPoint = -1;
      if (roadmap_gps_have_reception()){
         from_pos = roadmap_trip_get_position ("GPS");
      }
   }
   navigate_main_prepare_for_request();
   navigate_route_request (&fromLine,
                           fromPoint,
                           NULL,
                           from_pos,
                           to_pos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           0,
                           iTripId,
                           max_routes,
                           &cb);
   return TRUE;
}
