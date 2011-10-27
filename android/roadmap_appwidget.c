/* roadmap_appwidget.c - Android application widget functionality module
 *
 * LICENSE:
 *
  *   Copyright 2011 Alex Agranovich (AGA), Waze Ltd
 *
 *   This file is part of RoadMap.
 *
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
 *
 *   SYNOPSYS:
 *
 *          See FreeMapJNI.h
 *              roadmap_appwidget.h
 */

#include "JNI/WazeAppWidgetManager_JNI.h"
#include "roadmap_appwidget.h"
#include "roadmap.h"
#include "roadmap_gps.h"
#include "navigate/navigate_route_trans.h"
#include "roadmap_social.h"
#include "roadmap_line_route.h"
#include "roadmap_trip.h"
#include "roadmap_history.h"
#include "roadmap_lang.h"
#include <string.h>
#include <time.h>
#include "Realtime/Realtime.h"

//======================== Local defines ========================
#define DESTINATIONS_NUMBER            2
#define AT_DESTINATION_RADIUS          1000 //(meters)
#define WORK_ARRIVAL_START_TIME        4
#define WORK_ARRIVAL_END_TIME          11

#define MAX_HISTORY_ENTRIES            100

//======================== Globals ========================
static BOOL sgRequestActive = FALSE;
static const char* sgDestDescription = NULL;
static RoadMapCallback sgNextLoginCb = NULL;

extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;

//======================== Local Declarations ========================
static void nav_cb_on_rc ( NavigateRouteRC rc, int protocol_rc, const char *description );
static void nav_cb_on_segments ( NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments );
static void nav_cb_update_route ( int num_instrumented );
static void nav_cb_on_reroute ( int reroute_segment, int time_before, int time_after );
static void nav_cb_on_square_ver_mismatch ( void );
static void get_current_position( RoadMapPosition *pos, PluginLine* line, int* from_point, int* direction );
static const char* get_destination( const RoadMapPosition* current_pos, RoadMapPosition* dest_pos, BOOL* found );
static void request_route_on_login( void );


/***********************************************************/
/*  Name        : roadmap_appwidget_request_route
 *  Purpose     : Request route for the current departure to the work/home
 *  Params      : void
 *              :
 *  Returns     :
 *              :
 */
void roadmap_appwidget_request_route()
{
   const char* user = RealTime_GetUserName();
   if ( user && user[0] )
   {
      roadmap_log( ROADMAP_WARNING, "Starting widget request" );
      sgNextLoginCb = Realtime_NotifyOnLogin ( request_route_on_login );
      sgRequestActive = TRUE;
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "No valid login. Cannot proceed with request" );
//      WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_NO_LOGIN,
//                                 roadmap_lang_get( "No valid login" ), NULL, -1 );
   }
}

int roadmap_appwidget_request_active( void )
{
   return sgRequestActive;
}

void roadmap_appwidget_request_refresh( void )
{
   WazeAppWidgetManager_RequestRefresh();
}

static void request_route_on_login( void )
{
   BOOL found;
   static NavigateRouteCallbacks cb = {
      nav_cb_on_rc,
      NULL,
      nav_cb_on_segments,
      nav_cb_update_route,
      NULL,
      nav_cb_on_reroute,
      nav_cb_on_square_ver_mismatch
   };

   RoadMapPosition pos = {-1,-1}, dest_pos;
   int from_point, direction;
   PluginLine line;

   // Call previous callback
   if ( sgNextLoginCb )
      sgNextLoginCb();

   // Step 1. Determine the current position
   get_current_position( &pos, &line, &from_point, &direction );
   if ( from_point == -1 && pos.latitude == -1 )
   {
//      WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_NO_LOCATION,
//                           roadmap_lang_get( "Current location cannot be determined!" ), NULL, -1 );
      return;
   }

   // Step 2. Determine the destination
   sgDestDescription = get_destination( &pos, &dest_pos, &found );
   roadmap_log( ROADMAP_DEBUG, "Destination for route: %s (%d, %d)", SAFE_STR( sgDestDescription ), dest_pos.latitude, dest_pos.longitude );
   if ( !found )
   {
      roadmap_log( ROADMAP_ERROR, "Can't find destination for route" );
//      WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_NO_DESTINATION,
//                     roadmap_lang_get( "Destination is not defined!" ), sgDestDescription, -1 );
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "Departure point for route: %s (%d, %d)", SAFE_STR( sgDestDescription ), pos.latitude, pos.longitude );

   // Step 3. Route request
   navigate_route_cancel_request();
   navigate_main_prepare_for_request();
   navigate_route_request ( &line,
                           from_point,
                           NULL,
                           &pos,
                           &dest_pos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           ROADMAP_SOCIAL_DESTINATION_MODE_DISABLED,
                           0,
                           -1,
                           1,
                           &cb );
}

/***********************************************************/
/*  Name        : nav_cb_on_rc
 *  Purpose     : on_rc callback (see NavigateRouteCallbacks)
 */
static void nav_cb_on_rc ( NavigateRouteRC rc, int protocol_rc, const char *description ) {

   // AGA DEBUG
   roadmap_log( ROADMAP_WARNING, "Request RC %d", rc );

   if (rc == route_succeeded) {
   }
   else
   {
      navigate_route_cancel_request();
      sgRequestActive = FALSE;
//      WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_ROUTE_SERVER,
//                           roadmap_lang_get( "Cannot calculate route" ), NULL, -1 );
   }
}
/***********************************************************/
/*  Name        : nav_cb_on_segments
 *  Purpose     : on_segments callback (see NavigateRouteCallbacks)
 */
static void nav_cb_on_segments ( NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments ) {

   char msg[128];

   // AGA DEBUG
   roadmap_log( ROADMAP_WARNING, "On segments RC %d", rc );

   if ( rc != route_succeeded ) {
      roadmap_log (ROADMAP_ERROR, "The service failed to provide a valid route rc=%d", rc);
//      WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_ROUTE_SERVER,
//                           roadmap_lang_get( "Cannot calculate route" ), NULL, -1 );
   }
   else // rc == route_succeeded
   {
      if ( res )
      {
         if ( res->route_status == ROUTE_ORIGINAL )
         {
            int time_to_dest = ( int ) ( res->total_time / 60.0 );
//            WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_SUCCESS,
//                                 "", sgDestDescription, time_to_dest );


//          char text[256];
//            navigate_main_on_route ( res->flags, res->total_length, res->total_time, segments->segments,
//                                      segments->num_segments, segments->num_instrumented,
//                                      res->geometry.points, res->geometry.num_points,res->description, FALSE );
//            if ( roadmap_message_format (text, sizeof(text), "%A|%T") )
//            {
//               char numText[256];
//               char *p;
//               p = strtok (text," ");
//               sprintf( numText, "%s", p );
//               time_to_dest = atoi( numText );
//               WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_SUCCESS,
//                                    "", sgDestDescription, time_to_dest );
//            }
//            else
//            {
//               roadmap_log( ROADMAP_ERROR, "Unable to get time to destination estimation" );
//               WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_ROUTE_SERVER,
//                                          roadmap_lang_get( "Cannot calculate time to destination" ), NULL, -1 );
//            }

         }
      }
      else
      {
         roadmap_log( ROADMAP_WARNING, "Navigation status was not supplied!" );
//         WazeAppWidgetManager_RouteRequestCallback( com_waze_widget_WazeAppWidgetManager_STATUS_ERR_ROUTE_SERVER,
//                                    roadmap_lang_get( "Cannot calculate route" ), NULL, -1 );
      }
   }
   sgRequestActive = FALSE;
   // Cancel request anyway we need only time-to-destination
   navigate_route_cancel_request();
}
/***********************************************************/
/*  Name        : nav_cb_on_segments
 *  Purpose     : on_instrumented callback (see NavigateRouteCallbacks)
 */
static void nav_cb_update_route ( int num_instrumented ) {
   // Do nothing ...
   // AGA DEBUG
   roadmap_log( ROADMAP_WARNING, "nav_cb_update_route" );
}
/***********************************************************/
/*  Name        : nav_cb_on_reroute
 *  Purpose     : on_reroute callback (see NavigateRouteCallbacks)
 */
static void nav_cb_on_reroute ( int reroute_segment, int time_before, int time_after ) {
   // Do nothing ...
   // AGA DEBUG
   roadmap_log( ROADMAP_WARNING, "nav_cb_on_reroute" );

}
/***********************************************************/
/*  Name        : nav_cb_on_square_ver_mismatch
 *  Purpose     : on_square_ver_mismatch callback (see NavigateRouteCallbacks)
 */
static void nav_cb_on_square_ver_mismatch ( void ) {
   // Do nothing ...
   // AGA DEBUG
   roadmap_log( ROADMAP_WARNING, "nav_cb_on_square_ver_mismatch" );
}


static void get_current_position( RoadMapPosition *pos, PluginLine* line, int* from_point, int* direction )
{
   int from_tmp;
   int to_tmp;
   RoadMapGpsPosition gps_pos;
   const RoadMapPosition *pos_ptr;

   if ( ( roadmap_navigate_get_current ( &gps_pos, line, direction ) != -1 ) &&
            ( roadmap_plugin_get_id( line ) == ROADMAP_PLUGIN_ID ) )
   {
      roadmap_square_set_current ( line->square );
      roadmap_line_points ( line->line_id, &from_tmp, &to_tmp );

      if ( *direction == ROUTE_DIRECTION_WITH_LINE ) {
        *from_point = to_tmp;
      } else {
        *from_point = from_tmp;
      }
      pos->latitude = gps_pos.latitude;
      pos->longitude = gps_pos.longitude;
   }
   else
   {
      if ( roadmap_gps_have_reception() ) {
         pos_ptr = roadmap_trip_get_position ( "GPS" );
         *pos = *pos_ptr;
      }
      else {
         pos_ptr = roadmap_trip_get_position ( "Location" );
         *pos = *pos_ptr;
      }
      *direction = ROUTE_DIRECTION_NONE;
      *from_point = -1;
      line->line_id = -1;
   }
}


/***********************************************************/
/*  Name        : get_destination
 *  Purpose     : Determines the target destination according to the criteria.
 *                Looks for the favorite in the history. Returns name of favorite or NULL if not found
 *
 *  Params      : position [out] - destination position
 *              :
 *  Returns     : Destination name ( Not translated )
 *              :
 */
static const char* get_destination( const RoadMapPosition* current_pos, RoadMapPosition* dest_pos, BOOL* found )
{
#define HOME_INDEX  0
#define WORK_INDEX  1
   const char* res = NULL;
   static const char* destinations[DESTINATIONS_NUMBER] ={ "Home", "Work" };
   static const char* destinations_secondary[DESTINATIONS_NUMBER] = { "home", "office"};
   RoadMapPosition dest_positions[DESTINATIONS_NUMBER] = {0};

   int i, count = 0;
   char str[512];
   void* history;
   void* prev;
   int distance;
   time_t time_now = time( NULL );
   struct tm* local_time = localtime( &time_now );

   *found = FALSE;
   // Look for the destination in the favorites
   history = roadmap_history_latest ( ADDRESS_FAVORITE_CATEGORY );
   for ( ; history && (count < MAX_HISTORY_ENTRIES); ++count )
   {
      char *argv[ahi__count];

      roadmap_history_get( ADDRESS_FAVORITE_CATEGORY, history, argv );
      prev = history;
      snprintf( str, sizeof(str), "%s", argv[4] );
      for ( i = 0; i < DESTINATIONS_NUMBER; ++i )
      {
         if ( !strcasecmp( destinations[i], str ) || !strcasecmp( roadmap_lang_get( destinations[i] ), str )
               || !strcasecmp( destinations_secondary[i], str ) || !strcasecmp( roadmap_lang_get( destinations_secondary[i] ), str ) )
         {
            dest_positions[i].latitude = atoi( argv[5] );
            dest_positions[i].longitude = atoi( argv[6] );
            break;
         }
      }
      // Next entry
      history = roadmap_history_before ( ADDRESS_FAVORITE_CATEGORY, history);
      if (history == prev) break;
   }

   if ( ( local_time->tm_hour > WORK_ARRIVAL_START_TIME &&
            local_time->tm_hour < WORK_ARRIVAL_END_TIME ) )
   {
      if ( dest_positions[WORK_INDEX].latitude > 0 ) // Found in history
      {
         *found = TRUE;
         distance = roadmap_math_distance( &dest_positions[WORK_INDEX], current_pos );
         // Check if already at work - set to home
         if ( ( distance < AT_DESTINATION_RADIUS ) && ( dest_positions[HOME_INDEX].latitude > 0 ) )
         {
            res = destinations[HOME_INDEX];
            *dest_pos = dest_positions[HOME_INDEX];
         }
         else
         {
            res = destinations[WORK_INDEX];
            *dest_pos = dest_positions[WORK_INDEX];
         }

      }
      else
      {
         res = destinations[WORK_INDEX];
      }
   }
   else
   {
      if ( dest_positions[HOME_INDEX].latitude > 0 ) // Found in history
      {
         *found = TRUE;
         distance = roadmap_math_distance( &dest_positions[HOME_INDEX], current_pos );
         // Check if already at home - set to work
         if ( ( distance < AT_DESTINATION_RADIUS ) && ( dest_positions[WORK_INDEX].latitude > 0 ) )
         {
            res = destinations[WORK_INDEX];
            *dest_pos = dest_positions[WORK_INDEX];
         }
         else
         {
            res = destinations[HOME_INDEX];
            *dest_pos = dest_positions[HOME_INDEX];
         }

      }
      else
      {
         res = destinations[HOME_INDEX];
      }
   }

   return res;
}


