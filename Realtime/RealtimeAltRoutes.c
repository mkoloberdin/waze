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
#include "../roadmap_twitter.h"
#include "RealtimeAltRoutes.h"
#include "../roadmap_alternative_routes.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
   AltRouteTrip altRoutTrip[MAX_ALT_ROUTES];
   int iCount;
} AltRoutesTripsTable;

static AltRoutesTripsTable altRoutesTrips;
static int gTripId;


//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAltRoutes_Count() {
   return altRoutesTrips.iCount;
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
static BOOL RealtimeAltRoutes_Route_Exist(int iTripId) {
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

   if (RealtimeAltRoutes_Route_Exist(pRoute->iTripId)) {
      roadmap_log( ROADMAP_ERROR,"RealtimeAltRoutes_Add_Route - cannot add route (id=%d) already exist", pRoute->iTripId);
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
   altRoutesTrips.iCount++;
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnRouteResults (NavigateRouteRC rc, int num_res, const NavigateRouteResult *res){
   int i;
   if (num_res > MAX_ROUTES)
      num_res = MAX_ROUTES;
   
   altRoutesTrips.altRoutTrip[gTripId].iNumRoutes = num_res;
   if (rc != route_succeeded){
      
      return;
   }
   for (i = 0; i < num_res ; i++){
      altRoutesTrips.altRoutTrip[gTripId].pRouteResults[i] = *(res+i);//todo check
   }  
   roadmap_alternative_routes_routes_dialog();
   roadmap_screen_refresh();
}



//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAltRoutes_OnRouteSegments (NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments){
   navigate_main_on_route (res->flags, res->total_length, res->total_time, segments->segments,
                             segments->num_segments, segments->num_instrumented,
                             res->geometry.points, res->geometry.num_points);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
NavigateRouteResult *RealtimeAltRoutes_Get_Route_Result(int index){
   return &altRoutesTrips.altRoutTrip[gTripId].pRouteResults[index];
}


int RealtimeAltRoutes_Get_Num_Routes(){
   return altRoutesTrips.altRoutTrip[gTripId].iNumRoutes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeAltRoutes_Route_Request(int iTripId, const RoadMapPosition *from_pos, const RoadMapPosition *to_pos){
	static NavigateRouteCallbacks cb = {
		NULL,
		RealtimeAltRoutes_OnRouteResults,
      RealtimeAltRoutes_OnRouteSegments,
      navigate_main_update_route,
      navigate_main_on_suggest_reroute,
      navigate_main_on_segment_ver_mismatch
	};
   gTripId = 0;
   navigate_route_request (NULL, 
                           -1,
                           NULL,
                           from_pos,
                           to_pos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           ROADMAP_TWITTER_DESTINATION_MODE_DISABLED,
                           0,
                           iTripId,
                           MAX_ROUTES,
                           &cb);
   return TRUE;
}
