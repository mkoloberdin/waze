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
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_tripserver.h"
#include "websvc_trans/string_parser.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAltRoutes.h"
#include "roadmap_alternative_routes.h"
#include "roadmap_history.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_dialog.h"

#define MAX_POI_NAME 250
static RoadMapCallback gLoginCallBack;
static RoadMapCallback gLoginChangedCallBack;

const char* on_suggested_trips(int NumParams, const char*  pData);
const char* on_get_pois_res(int NumParams, const char*  pData);
const char* on_get_num_pois_res(int NumParams, const char*  pData);
const char* on_create_poi_res(int NumParams, const char*  pData);
void roadmap_trip_server_restore_favorites(void);

static TripServeHandlers tripserver_handlers[] =
{
   { "SuggestedTrips",   on_suggested_trips },
   { "GetPOIsRes",       on_get_pois_res},
   { "GetNumPOIsRes",       on_get_num_pois_res},
   { "CreatePOIRes",     on_create_poi_res}

};

#define MAX_REQUESTS 100
void  *g_request_cursors[MAX_REQUESTS];
static int g_counter = 0;

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
   roadmap_alternative_routes_suggested_trip();
   return pData;
}

static void purge_old_favorites(void){
   void *cursor;

   roadmap_log( ROADMAP_WARNING, "Tripserver::purge_old_favorites");
   cursor = roadmap_history_latest (ADDRESS_FAVORITE_CATEGORY);
   if( !cursor)
         return;

   while (cursor){
      roadmap_history_delete_entry(cursor);
      cursor = roadmap_history_latest (ADDRESS_FAVORITE_CATEGORY);
   }

}

const char* on_create_poi_res(int NumParams,const char*  pData){
   char *argv[ahi__count];
   void * history;
   int id;

   //ID
   pData = ReadIntFromString(
                         pData,         //   [in]      Source string
                         ",\r\n",           //   [in,opt]   Value termination
                         NULL,          //   [in,opt]   Allowed padding
                         &id,    //   [out]      Put it here
                         TRIM_ALL_CHARS);            //   [in]      Remove
    if (!pData){
       roadmap_log( ROADMAP_ERROR, "Tripserver::on_create_poi_res() - Failed to read ID");
       return NULL;
    }

    if (id > MAX_REQUESTS)
       return pData;
    roadmap_log( ROADMAP_DEBUG, "on_create_poi_res - id=%d",id );
    history = g_request_cursors[id];
    if (history == NULL)
       return pData;

    roadmap_history_get (ADDRESS_FAVORITE_CATEGORY, history, argv);
    roadmap_log( ROADMAP_DEBUG, "on_create_poi_res updating favorite id=%d name=%s",id, argv[ahi_name] );
    argv[ahi_synced] = "true";
    roadmap_history_update(history, ADDRESS_FAVORITE_CATEGORY, argv);
    roadmap_history_save();
    g_request_cursors[id] = NULL;
    return pData;
}
const char* on_get_num_pois_res(int NumParams,const char*  pData){
   int num_Pois;

   roadmap_log( ROADMAP_WARNING, "roadmap_tripserver_response- got  GetNumPOIsRes (Num parameters= %d )",NumParams );

   //NumPois
   pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",\r\n",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &num_Pois,    //   [out]      Put it here
                           1);            //   [in]      Remove
   if (!pData){
      roadmap_log( ROADMAP_ERROR, "Tripserver::on_get_num_pois_res() - Failed to read num pois" );
      return NULL;
   }

   if (num_Pois > 0){
      roadmap_trip_server_restore_favorites();
   }
   return pData;
}

const char* on_get_pois_res(int NumParams,const char*  pData){
   int i;
   int iBufferSize;
   int longitude;
   int latitude;
   char poiName[MAX_POI_NAME];
   char *argv[ahi__count];
   char temp[20];
   char msg[100];

   int numRecords = NumParams/3;

   roadmap_log( ROADMAP_WARNING, "roadmap_tripserver_response- got  GetPOIsRes (Num parameters= %d )",NumParams );

   if (numRecords > 0)
      purge_old_favorites();

   for (i = 0; i < numRecords; i++){

      //POI name
      iBufferSize = sizeof(poiName);
      pData       = ExtractNetworkString(
                         pData,           // [in]     Source string
                         poiName,         //   [out]   Output buffer
                         &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                         ",",             //   [in]   Array of chars to terminate the copy operation
                          1);             // [in]     Remove additional termination chars
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_get_pois_res() - Failed to read POI name");
         return NULL;
      }

      //POI Lon
      pData = ReadIntFromString(
                           pData,         //   [in]      Source string
                           ",",           //   [in,opt]   Value termination
                           NULL,          //   [in,opt]   Allowed padding
                           &longitude,    //   [out]      Put it here
                           1);            //   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_get_pois_res() - Failed to read Destination longitude. POI Name=%s",poiName );
         return NULL;
      }

      //POI Lat
      pData = ReadIntFromString(
                           pData,          //   [in]      Source string
                           ",\r\n",        //   [in,opt]   Value termination
                           NULL,           //   [in,opt]   Allowed padding
                           &latitude,      //   [out]      Put it here
                           TRIM_ALL_CHARS);//   [in]      Remove
      if (!pData){
         roadmap_log( ROADMAP_ERROR, "Tripserver::on_get_pois_res() - Failed to read Destination latitude. POI Name=%s",poiName );
         return NULL;
      }

      argv[ahi_house_number] = "";
      argv[ahi_street] = "";
      argv[ahi_city] = "";
      argv[ahi_state] = "";
      argv[ahi_name] = (char *)poiName;
      sprintf(temp, "%d", latitude);
      argv[ahi_latitude] = strdup(temp);
      sprintf(temp, "%d", longitude);
      argv[ahi_longtitude] = strdup(temp);
      argv[ahi_synced] = "true";
      roadmap_log( ROADMAP_WARNING, "roadmap_tripserver_response- GetPOIsRes Adding favorite (name=%s, lat=%s, lon=%s )", argv[ahi_name], argv[ahi_latitude], argv[ahi_longtitude]);
      roadmap_history_add (ADDRESS_FAVORITE_CATEGORY, (const char **)argv);
   }

   ssd_progress_msg_dialog_hide();

   if (numRecords == 0)
      sprintf(msg, "%s", roadmap_lang_get("No favorite destinations were found"));
   else if (numRecords == 1)
      sprintf(msg, "%s", roadmap_lang_get("1 destination was restored to your favorites"));
   else
      sprintf(msg, "%d %s", numRecords, roadmap_lang_get("destinations were restored to your favorites"));

   roadmap_messagebox_timeout("Favorites", msg, 5);
   roadmap_history_save();

   /*
    * Request refresh of widget upon saving the favorites
    */
#ifdef ANDROID_WIDGET
      roadmap_appwidget_request_refresh();
#endif

   return pData;
}


const char* roadmap_tripserver_response(int status, int NumParams, const char*  pData){

   int num_handlers;
   int i;
   char ResponseName[128];
   int iBufferSize;

   iBufferSize =  128;
   if (status != 200){
      if (NumParams){

         pData       = ExtractNetworkString(
                        pData,             // [in]     Source string
                        ResponseName,//   [out]   Output buffer
                        &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                        ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                        TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

         if (status != 500)
            roadmap_log( ROADMAP_ERROR, "roadmap_tripserver_response- Command failed (status= %d,%s )",status,ResponseName );

         for( i=0; i<NumParams -1; i++){
            iBufferSize =  128;
            ResponseName[0] = 0;
            pData       = ExtractNetworkString(
                           pData,             // [in]     Source string
                           ResponseName,//   [out]   Output buffer
                           &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                           ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                           TRIM_ALL_CHARS);   // [in]     Remove additional termination chars
         }
      }
      return pData;
   }

   if (NumParams){
      pData       = ExtractNetworkString(
                     pData,             // [in]     Source string
                     ResponseName,//   [out]   Output buffer
                     &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                     ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                     1);   // [in]     Remove additional termination chars

      num_handlers = sizeof(tripserver_handlers)/sizeof(TripServeHandlers);
      for( i=0; i<num_handlers; i++){
         if (!strcmp(ResponseName, tripserver_handlers[i].Response)){
            return (*tripserver_handlers[i].handler)(NumParams-1, pData);
         }
      }
   }

   return pData;
}

static void roadmap_trip_server_restore_favorites_cb(int exit_code, void *context){
   if (exit_code != dec_yes){
      if (gLoginChangedCallBack)
          (*gLoginChangedCallBack) ();
      return;
   }
   ssd_progress_msg_dialog_show(roadmap_lang_get("Restoring favorites..."));
   Realtime_TripServer_GetPOIs();
   if (gLoginChangedCallBack)
       (*gLoginChangedCallBack) ();
}

void roadmap_trip_server_restore_favorites(void){
   ssd_confirm_dialog (roadmap_lang_get("Welcome back"), roadmap_lang_get("Would you like to restore your favorites?"), TRUE, roadmap_trip_server_restore_favorites_cb , NULL);
}

void roadmap_trip_get_num_Pois(void){
   Realtime_TripServer_GetNumPOIs();
}
static int get_next_id(){
   int i;
   if (g_counter == 0){
      g_counter++;
      return 0;
   }

   if (g_counter< MAX_REQUESTS-1){
      g_counter++;
      return g_counter;
   }

   for (i = 0; i<MAX_REQUESTS; i++){
       if (g_request_cursors[i] == NULL)
          return i;
   }

   roadmap_log( ROADMAP_ERROR, "roadmap_trip_server_create_poi - table is full" );
   return -1;
}

static BOOL create_poi (const char* name, RoadMapPosition* coordinates, BOOL overide, void * cursor){
   int id = get_next_id();
   if (id < 0)
      return FALSE;
   g_request_cursors[id] = cursor;
   roadmap_log( ROADMAP_DEBUG, "create_poi -  name = %s,lat-%d, lon=%d, id=%d",name, coordinates->latitude, coordinates->longitude, id );
   return Realtime_TripServer_CreatePOI(name, coordinates, overide,id);
}

BOOL roadmap_trip_server_create_poi  (const char* name, RoadMapPosition* coordinates, BOOL overide){
   void * cursor = roadmap_history_latest(ADDRESS_FAVORITE_CATEGORY);
   return create_poi(name, coordinates, overide, cursor);
}


static void roadmap_trip_server_after_login_delayed (void) {

   void *cursor;
   void *prev;
   char *argv[ahi__count];
   static int count = 0;

   roadmap_main_remove_periodic(roadmap_trip_server_after_login_delayed);
   roadmap_log( ROADMAP_DEBUG, "roadmap_trip_server_after_login" );
   cursor = roadmap_history_latest (ADDRESS_FAVORITE_CATEGORY);
   if( !cursor){
      return;
   }

   while (cursor){
      roadmap_history_get (ADDRESS_FAVORITE_CATEGORY, cursor, argv);
      prev = cursor;
      if (strcmp(argv[ahi_synced], "true")){
         RoadMapPosition coordinates;
         count++;
         if (count > 3) //limit syncing to 3
            return;

         roadmap_log( ROADMAP_DEBUG, "roadmap_trip_server_after_login_delayed - Syncing favorite (%s) with server",argv[ahi_name] );
         coordinates.latitude = atoi(argv[ahi_latitude]);
         coordinates.longitude = atoi(argv[ahi_longtitude]);
         create_poi(argv[ahi_name], &coordinates, TRUE, cursor);

      }
      else{
         roadmap_log( ROADMAP_DEBUG, "roadmap_trip_server_after_login_delayed - Favorite (%s) already synced with server",argv[ahi_name] );
      }

      cursor = roadmap_history_before (ADDRESS_FAVORITE_CATEGORY, cursor);
      if (cursor == prev) break;
   }

   roadmap_log( ROADMAP_DEBUG, "roadmap_trip_server_after_login_delayed end" );

}

static void roadmap_trip_server_after_login (void) {
   roadmap_main_set_periodic(1000*30, roadmap_trip_server_after_login_delayed);
   if (gLoginCallBack)
       (*gLoginCallBack) ();

}

void roadmap_trip_server_init(void){
   int i;

   for (i = 0; i<MAX_REQUESTS; i++){
      g_request_cursors[i] = NULL;
   }

   roadmap_history_declare (ADDRESS_FAVORITE_CATEGORY, ahi__count);
   gLoginCallBack = Realtime_NotifyOnLogin (roadmap_trip_server_after_login);
   gLoginChangedCallBack = Realtime_NotifyOnLoginChanged (roadmap_trip_get_num_Pois);
}

