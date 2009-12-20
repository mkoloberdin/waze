/* roadmap_tripserver.c
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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_tripserver.h"
#include "websvc_trans/string_parser.h"
#include "Realtime/RealtimeAltRoutes.h"
#include "roadmap_alternative_routes.h"

const char* on_suggested_trips(int NumParams, const char*  pData);

static TripServeHandlers tripserver_handlers[] =
{ 
   { "SuggestedTrips",   on_suggested_trips }
};



const char* on_suggested_trips(int NumParams,const char*  pData){
   int i;
   int iBufferSize;
   AltRouteTrip route;
   int numRecords = NumParams/7;
   for (i = 0; i < numRecords; i++){
      
      if ( i%7 == 0)
         RealtimeAltRoutes_Init_Record(&route);
      
      //ID
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &route.iTripId,//   [out]      Put it here
                           1);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read ID");
         return NULL;
      }
      
      //Src name
      iBufferSize = sizeof(route.sSrcName);
      pData       = ExtractNetworkString(
                         pData,             // [in]     Source string
                         route.sSrcName,//   [out]   Output buffer
                         &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                         ",",          //   [in]   Array of chars to terminate the copy operation
                          1);   // [in]     Remove additional termination chars
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read SrcName. ID=%d",route.iTripId, route.sDestinationName );
         return NULL;
      }

      //Src Lon
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &route.srcPosition.longitude,//   [out]      Put it here
                           1);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read Src longitude. ID=%d",route.iTripId );
         return NULL;
      }

      //Src Lat
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",\r\n",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &route.srcPosition.latitude,//   [out]      Put it here
                           TRIM_ALL_CHARS);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read Src latitude. ID=%d",route.iTripId );
         return NULL;
      }
      
      //Dest name
      iBufferSize = sizeof(route.sDestinationName);
      pData       = ExtractNetworkString(
                         pData,             // [in]     Source string
                         route.sDestinationName,//   [out]   Output buffer
                         &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                         ",",          //   [in]   Array of chars to terminate the copy operation
                          1);   // [in]     Remove additional termination chars
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read DestName. ID=%d",route.iTripId, route.sDestinationName );
         return NULL;
      }

      //Dest Lon
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &route.destPosition.longitude,//   [out]      Put it here
                           1);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read Destination longitude. ID=%d",route.iTripId );
         return NULL;
      }

      //Dest Lat
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",\r\n",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &route.destPosition.latitude,//   [out]      Put it here
                           TRIM_ALL_CHARS);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_suggested_trips() - Failed to read Destination latitude. ID=%d",route.iTripId );
         return NULL;
      }

      RealtimeAltRoutes_Add_Route(&route);

   }
   roadmap_alternative_routes_suggest_route_dialog(); 
   return pData;
}


const char* roadmap_tripserver_response(int status, int NumParams, const char*  pData){
   
   int num_handlers;
   int i;
   char ResponseName[128];
   int iBufferSize;
   
   iBufferSize =  128;
   pData       = ExtractNetworkString(
                    pData,             // [in]     Source string
                    ResponseName,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars
   
   
   if (status != 200){
      roadmap_log( ROADMAP_ERROR, "roadmap_tripserver_response- %s Command failed (status= %d)",ResponseName, status );
      return pData;
   }
   
   num_handlers = sizeof(tripserver_handlers)/sizeof(TripServeHandlers);
   for( i=0; i<num_handlers; i++){
      if (!strcmp(ResponseName, tripserver_handlers[i].Response)){
         return (*tripserver_handlers[i].handler)(NumParams-1, pData);
      }
   }

   return NULL;   
}
 
