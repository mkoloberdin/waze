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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "RealtimeNet.h"
#include "RealtimeAlerts.h"
#include "RealtimeOffline.h"
#include "Realtime.h"
#include "RealtimeSystemMessage.h"
#include "RealtimeExternalPoiNotifier.h"

#include "../roadmap_gps.h"
#include "../roadmap_navigate.h"
#include "../roadmap_trip.h"
#include "../roadmap_net.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_start.h"
#include "../roadmap_layer.h"
#include "../roadmap_main.h"
#include "../roadmap_lang.h"
#include "../roadmap_social.h"
#include "../editor/db/editor_marker.h"
#include "../editor/db/editor_shape.h"
#include "../editor/db/editor_trkseg.h"
#include "../editor/db/editor_line.h"
#include "../editor/db/editor_point.h"
#include "../editor/db/editor_street.h"
#include "../editor/track/editor_track_report.h"
#include "../navigate/navigate_route_trans.h"
#include "roadmap_geo_config.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static char       gs_WebServiceAddress [ WSA_STRING_MAXSIZE       + 1];
static char       gs_WebServerURL      [ WSA_SERVER_URL_MAXSIZE   + 1];
static char       gs_WebServiceName    [ WSA_SERVICE_NAME_MAXSIZE + 1];
static int        gs_WebServerPort;
static BOOL       gs_WebServiceParamsLoaded = FALSE;
static wst_handle gs_WST = NULL;

static wst_parser login_parser[] =
{
   { "SystemMessage",         SystemMessage},
   { "GeoServerConfig",       on_geo_server_config},
   { "ServerConfig",          on_server_config},
   { "UpdateConfig",          on_update_config},
   { "UpgradeClient",         VersionUpgrade},
   { "UserGroups",            UserGroups},
   { NULL, OnLoginResponse},
};

static wst_parser logout_parser[] =
{
   { NULL, OnLogOutResponse}
};

static wst_parser register_parser[] =
{
   { NULL, OnRegisterResponse}
};

static wst_parser geo_config_parser[] =
{
   { "RC",                    VerifyStatus},
   { "GeoServerConfig",       on_geo_server_config},
   { "ServerConfig",          on_server_config},
   { "UpdateConfig",          on_update_config},
};

static wst_parser general_parser[] =
{
   { "RC",              VerifyStatus},
   { "AddUser",         AddUser},
   { "AddAlert",        AddAlert},
   { "AddAlertComment", AddAlertComment},
   { "RmAlert",         RemoveAlert},
   { "SystemMessage",   SystemMessage},
   { "UpgradeClient",   VersionUpgrade},
   { "AddRoadInfo",     AddRoadInfo},
   { "RoadInfoGeom",    RoadInfoGeom},
   { "RoadInfoSegments",RoadInfoSegments},
   { "RmRoadInfo",      RmRoadInfo},
   { "BridgeToRes", 		BridgeToRes},
   { "ReportAlertRes",   ReportAlertRes},
   { "PostAlertCommentRes", PostAlertCommentRes},
   { "MapUpdateTime", 	    MapUpdateTime},
   { "GeoLocation",         GeoLocation},
   { "UpdateUserPoints",    UpdateUserPoints},
   { "RoutingResponseCode",	on_routing_response_code },
   { "RoutingResponse",			on_routing_response },
   { "RoutePoints",				on_route_points },
   { "RouteSegments",			on_route_segments },
   { "SuggestReroute",			on_suggest_reroute },
   { "GeoServerConfig",       on_geo_server_config},
   { "ServerConfig",          on_server_config},
   { "AddCustomBonus",        AddCustomBonus},
   { "AddBonus",              AddBonus},
   { "RmBonus",               RmBonus},
   { "CollectBonusRes",       CollectBonusRes},
   { "OpenMessageTicker",     OpenMessageTicker},
   { "UpdateConfig",          on_update_config},
   { "UserGroups",            UserGroups},
   { "OpenMoodSelection",     OpenMoodSelection},
   { "AddExternalPoiType",    AddExternalPoiType},
   { "AddExternalPoi",        AddExternalPoi},
   { "RmExternalPoi",         RmExternalPoi},
   { "SetExternalPoiDrawOrder",SetExternalPoiDrawOrder},
};

extern const char* RT_GetWebServiceAddress();
BOOL RTNet_LoadParams();
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Init()
{
   assert( NULL == gs_WST);

#ifdef _DEBUG
   assert(RTNet_LoadParams());
#else
   RTNet_LoadParams();
#endif   // _DEBUG

   gs_WST = wst_init( gs_WebServiceAddress, "binary/octet-stream");
   assert( gs_WST);

   return (NULL != gs_WST);
}

void  RTNet_Term()
{
   if( gs_WST)
   {
      wst_term( gs_WST);
      gs_WST = NULL;
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  MEGA               (1000000)
void convert_int_coordinate_to_float_string(char* buffer, int value)
{
   /* Buffer minimum size is COORDINATE_VALUE_STRING_MAXSIZE  */

   int   precision_part;
   int   integer_part;
   BOOL  negative = FALSE;

   if( !value)
   {
      strcpy( buffer, "0");
      return;
   }

   if( value < 0)
   {
      negative = TRUE;
      value   *= -1;
   }

   precision_part= value%MEGA;
   integer_part  = value/MEGA;

   if( negative)
      sprintf( buffer, "-%d.%06d", integer_part, precision_part);
   else
      sprintf( buffer,  "%d.%06d", integer_part, precision_part);
}


void format_RoadMapPosition_string( char* buffer, const RoadMapPosition* position)
{
   char  float_string[COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string, position->longitude);
   sprintf( buffer, "%s,", float_string);
   convert_int_coordinate_to_float_string( float_string, position->latitude);
   strcat( buffer, float_string);
}

void format_RoadMapGpsPosition_string( char* buffer, const RoadMapGpsPosition* position)
{
   /* Buffer minimum size is RoadMapGpsPosition_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_altitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, position->longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , position->latitude);
   convert_int_coordinate_to_float_string( float_string_altitude , position->altitude);
   sprintf( buffer,
            "%s,%s,%s,%d,%d",
            float_string_longitude,
            float_string_latitude,
            float_string_altitude,
            position->steering,
            position->speed);
}

void format_RoadMapGpsPosition_Pos_Azy_Str( char* buffer, const RoadMapGpsPosition* position)
{
   /* Buffer minimum size is RoadMapGpsPosition_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, position->longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , position->latitude);
   sprintf( buffer,
            "%s,%s,%d",
            float_string_longitude,
            float_string_latitude,
            position->steering);
}
void format_DB_point_string( char* buffer, int longitude, int latitude, time_t timestamp, int db_id)
{
   /* Buffer minimum size is DB_Point_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , latitude);

   sprintf( buffer,
            "%d,%s,%s,%d",
            db_id,
            float_string_longitude,
            float_string_latitude,
            (int)timestamp);
}

void format_point_string( char* buffer, int longitude, int latitude, time_t timestamp)
{
   /* Buffer minimum size is Point_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , latitude);

   sprintf( buffer,
            ",%s,%s,%d",
            float_string_longitude,
            float_string_latitude,
            (int)timestamp);
}

void format_RoadMapArea_string( char* buffer, const RoadMapArea* area)
{
   /* Buffer minimum size is RoadMapArea_STRING_MAXSIZE */

   char  float_string_east [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_north[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_west [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_south[COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_east , area->east );
   convert_int_coordinate_to_float_string( float_string_north, area->north);
   convert_int_coordinate_to_float_string( float_string_west , area->west );
   convert_int_coordinate_to_float_string( float_string_south, area->south);

   // Note: Order expected by server:  West, South, East, North
   sprintf( buffer,
            "%s,%s,%s,%s",
            float_string_west ,
            float_string_south,
            float_string_east ,
            float_string_north);
}


BOOL format_ParamPair_string( char*       buffer,
                              int         iBufSize,
                              int         nParams,
                              const char*   szParamKey[],
                              const char*   szParamVal[])
{
   int   iParam;
   int   iBufPos = 0;

   sprintf( buffer, "%d", nParams * 2);
   iBufPos = strlen( buffer );

   for (iParam = 0; iParam < nParams; iParam++)
   {
      if (szParamKey[iParam] && (*szParamKey[iParam]) &&
          szParamVal[iParam] && (*szParamVal[iParam]))
      {
         if (iBufPos == iBufSize)
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Oops", "Sending Report failed");
            return FALSE;
         }
         buffer[iBufPos++] = ',';

         if(!PackNetworkString( szParamKey[iParam], buffer + iBufPos, iBufSize - iBufPos))
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Oops", "Sending Report failed");
            return FALSE;
         }
         iBufPos += strlen (buffer + iBufPos);

         if (iBufPos == iBufSize)
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Oops", "Sending Report failed");
            return FALSE;
         }
         buffer[iBufPos++] = ',';

         if(!PackNetworkString( szParamVal[iParam], buffer + iBufPos, iBufSize - iBufPos))
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Oops", "Sending Report failed");
            return FALSE;
         }
         iBufPos += strlen (buffer + iBufPos);
      }
   }

   return TRUE;
}


BOOL RTNet_LoadParams()
{
   if( !gs_WebServiceParamsLoaded)
   {
      const char*   szWebServiceAddress = RT_GetWebServiceAddress();
      //   Break full name into parameters:
      if(!WSA_ExtractParams(
         szWebServiceAddress, //   IN        -   Web service full address (http://...)
         gs_WebServerURL,     //   OUT,OPT   -   Server URL[:Port]
         &gs_WebServerPort,   //   OUT,OPT   -   Server Port
         gs_WebServiceName))  //   OUT,OPT   -   Web service name
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_LoadParams() - Invalid web-service address (%s)", szWebServiceAddress);

         //   Web-Service address string is invalid...
         return FALSE;
      }

      //   Copy full address:
      strncpy_safe( gs_WebServiceAddress, szWebServiceAddress, sizeof (gs_WebServiceAddress));

      gs_WebServiceParamsLoaded = TRUE;
   }

   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNet_Login( LPRTConnectionInfo   pCI,
                  const char*          szUserName,
                  const char*          szUserPW,
                  const char*          szUserNickname,
                  CB_OnWSTCompleted pfnOnCompleted)
{
   //   Do we have a name/pw
   if( !szUserName || !(*szUserName) || !szUserPW || !(*szUserPW))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_Login() - name and/or password were not supplied");
      return FALSE;
   }

   //   Verify sizes:
   if( (RT_USERNM_MAXSIZE < strlen(szUserName))   ||
       (RT_USERPW_MAXSIZE < strlen(szUserPW)))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_Login() - Size of name/password is bigger then maximum (%d/%d)",
                  RT_USERNM_MAXSIZE,RT_USERPW_MAXSIZE);
      return FALSE;
   }

   //   Copy name/pw
   PackNetworkString( szUserName,      pCI->UserNm, RT_USERNM_MAXSIZE);
   PackNetworkString( szUserPW,        pCI->UserPW, RT_USERPW_MAXSIZE);

   if( szUserNickname && (*szUserNickname))
      PackNetworkString( szUserNickname,  pCI->UserNk, RT_USERNK_MAXSIZE);
#ifdef IPHONE
   else if (!Realtime_is_random_user())
      PackNetworkString( szUserName,  pCI->UserNk, RT_USERNK_MAXSIZE);
#endif //IPHONE
   else
      pCI->UserNk[0] = '\0';


  // Perform WebService Transaction:
   if( wst_start_trans( gs_WST,
                        "login",
                        login_parser,
                        sizeof(login_parser)/sizeof(wst_parser),
                        pfnOnCompleted,
                        pCI,
                        RTNET_FORMAT_NETPACKET_9Login,// Custom data for the HTTP request
                        RTNET_PROTOCOL_VERSION,
                        pCI->UserNm,
                        pCI->UserPW,
                        RT_DEVICE_ID,
                        roadmap_start_version(),
                        pCI->UserNk,
                        roadmap_geo_config_get_version(),
                        RTSystemMessagesGetLastMessageDisplayed(),
                        roadmap_lang_get_system_lang()))
      return TRUE;

   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   return FALSE;
}

BOOL RTNet_RandomUserRegister( LPRTConnectionInfo pCI, CB_OnWSTCompleted pfnOnCompleted)
{
   //   Verify identity is reset:
   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   memset( pCI->UserNk, 0, sizeof(pCI->UserNk));

   // Perform WebService Transaction:
   return wst_start_trans( gs_WST,
                           "static",
                           register_parser,
                           sizeof(register_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_3Register,
                              RTNET_PROTOCOL_VERSION,
                              RT_DEVICE_ID,
                              roadmap_start_version());
}

BOOL RTNet_GuestLogin( LPRTConnectionInfo pCI, CB_OnWSTCompleted pfnOnCompleted)
{
   //   Verify identity is reset:
   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   memset( pCI->UserNk, 0, sizeof(pCI->UserNk));

   // Perform WebService Transaction:
   return wst_start_trans( gs_WST,
                           "login",
                           login_parser,
                           sizeof(login_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_3GuestLogin,
                              RTNET_PROTOCOL_VERSION,
                              RT_DEVICE_ID,
                              roadmap_start_version());
}

/*    Method:  wst_start_session_trans()

      Wraps    [websvc_trans]  wst_start_trans()

      Additional task:

      Prefix packet with session-ID:
         "UID,123,abc\r\n"...                                     */
BOOL wst_start_session_trans( const wst_parser_ptr parsers,       // Array of 1..n data parsers
                              int                  parsers_count, // Parsers count
                              CB_OnWSTCompleted    cbOnCompleted, // Callback for transaction completion
                              LPRTConnectionInfo   pCI,           // Connection info
                              const char*          szFormat,      // Custom data for the HTTP request
                              ...)                                // Parameters
{
   char     Header[CUSTOM_HEADER_MAX_SIZE+1];
   va_list  vl;
   int      i;
   ebuffer  Packet;
   char*    Data;
   int      SizeNeeded;
   BOOL     bRes;



   if( !pCI || !parsers || !parsers_count || !cbOnCompleted || !szFormat || !(*szFormat))
   {
      assert(0);  // Invalid argument(s)
      return FALSE;
   }

   ebuffer_init( &Packet);

   SizeNeeded  =  strlen(szFormat)        +
                  CUSTOM_HEADER_MAX_SIZE  +
                  HttpAsyncTransaction_MAXIMUM_SIZE_NEEDED_FOR_ADDITIONAL_PARAMS;
   Data        = ebuffer_alloc( &Packet, SizeNeeded);

   va_start(vl, szFormat);
   i = vsnprintf( Data, SizeNeeded, szFormat, vl);
   va_end(vl);

   if( i < 0)
   {
      roadmap_log( ROADMAP_ERROR, "RT::wst_start_session_trans() - Failed to format command '%s' (buffer size too small?)", szFormat);
      ebuffer_free( &Packet);
      return FALSE;
   }

   snprintf(Header,
            CUSTOM_HEADER_MAX_SIZE,
            "UID,%d,%s\r\n",
            pCI->iServerID, pCI->ServerCookie);

   if( SizeNeeded <= (int)(strlen(Header) + strlen(Data)))
   {
      roadmap_log( ROADMAP_ERROR, "RT::wst_start_session_trans() - Insufficient allocation size:  'Packet-data' + 'Packet-header' does not fit in buffer");
      ebuffer_free( &Packet);
      return FALSE;
   }

   Data = AppendPrefix_ShiftOriginalRight( Header, Data);

   bRes = wst_start_trans( gs_WST,
                           "command",
                           parsers,
                           parsers_count,
                           cbOnCompleted,
                           pCI,
                           Data);

   ebuffer_free( &Packet);

   return bRes;
}

BOOL RTNet_Logout( LPRTConnectionInfo pCI, CB_OnWSTCompleted pfnOnCompleted)
{
   return wst_start_session_trans(  logout_parser,
                                    sizeof(logout_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    "Logout");
}

BOOL RTNet_At( LPRTConnectionInfo   pCI,
               const
               RoadMapGpsPosition*  pGPSPosition,
               int                  from_node,
               int                  to_node,
               BOOL                 refreshUsers,
               CB_OnWSTCompleted pfnOnCompleted,
               char*                packet_only)
{
   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];

   format_RoadMapGpsPosition_string( GPSPosString, pGPSPosition);

   if( packet_only)
   {
      sprintf( packet_only,
               RTNET_FORMAT_NETPACKET_4At,
                  GPSPosString,
                  from_node,
                  to_node,
                  refreshUsers ? "T" : "F");
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(  general_parser,
                                    sizeof(general_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    RTNET_FORMAT_NETPACKET_4At,// Custom data for the HTTP request
                                       GPSPosString,
                                       from_node,
                                       to_node,
                                       refreshUsers ? "T" : "F");
}

BOOL RTNet_KeepAlive( LPRTConnectionInfo   pCI,
                      CB_OnWSTCompleted pfnOnCompleted)
{
   return wst_start_session_trans(  general_parser,
                                    sizeof(general_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    "KeepAlive\n");
}
BOOL RTNet_NavigateTo(  LPRTConnectionInfo   pCI,
                        const
                        RoadMapPosition*     cordinates,
                        address_info_ptr     ai,
                        CB_OnWSTCompleted pfnOnCompleted)
{
   char  gps_point[RoadMapPosition_STRING_MAXSIZE+1];

   format_RoadMapPosition_string( gps_point, cordinates);

   return wst_start_session_trans(  general_parser,
                                    sizeof(general_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    RTNET_FORMAT_NETPACKET_4NavigateTo, // Custom data for the HTTP request
                                       gps_point,
                                       ai->city,
                                       ai->street,
                                       ""); // Avi removed ai->house
}
BOOL RTNet_MapDisplyed( LPRTConnectionInfo   pCI,
                        const RoadMapArea*   pRoadMapArea,
                        unsigned int         scale,
                        CB_OnWSTCompleted pfnOnCompleted,
                        char*                packet_only)
{
   char  MapArea[RoadMapArea_STRING_MAXSIZE+1];

   format_RoadMapArea_string( MapArea, pRoadMapArea);

   if( packet_only)
   {
      sprintf( packet_only, RTNET_FORMAT_NETPACKET_2MapDisplayed, MapArea, scale);
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(
               general_parser,
               sizeof(general_parser)/sizeof(wst_parser),
               pfnOnCompleted,
               pCI,
               RTNET_FORMAT_NETPACKET_2MapDisplayed,
                  MapArea, scale);
}

BOOL RTNet_CreateNewRoads( LPRTConnectionInfo   pCI,
                           int                  nToggles,
                           const time_t*        toggle_time,
                           BOOL                 bStatusFirst,
                           CB_OnWSTCompleted    pfnOnCompleted,
                           char*                packet_only)
{
   ebuffer  Packet;
   char*    CreateNewRoadsBuffer = NULL;
   int      i;
   BOOL     bStatus;
   BOOL     bRes;

   ebuffer_init( &Packet);

   CreateNewRoadsBuffer = ebuffer_alloc( &Packet, RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(nToggles));
   memset( CreateNewRoadsBuffer, 0, RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(nToggles));

   bStatus = bStatusFirst;
   for (i = 0; i < nToggles; i++)
   {
      char *buffer = CreateNewRoadsBuffer + strlen (CreateNewRoadsBuffer);
      sprintf (buffer, RTNET_FORMAT_NETPACKET_2CreateNewRoads,
               (unsigned int)toggle_time[i],
               bStatus ? "T" : "F");
      bStatus = !bStatus;
   }

   assert(*CreateNewRoadsBuffer);
   roadmap_log(ROADMAP_DEBUG, "RTNet_CreateNewRoads() - Output command: '%s'", CreateNewRoadsBuffer);

   if( packet_only)
   {
      strcpy( packet_only, CreateNewRoadsBuffer);
      bRes = TRUE;
   }
   else
   {
      bRes = wst_start_session_trans(
                              general_parser,
                              sizeof(general_parser)/sizeof(wst_parser),
                              pfnOnCompleted,
                              pCI,
                              CreateNewRoadsBuffer);      //   Custom data
   }

   ebuffer_free( &Packet);
   return bRes;
}


void RTNet_GPSPath_BuildCommand( char*             Packet,
                                 LPGPSPointInTime  points,
                                 int               count,
                                 BOOL					end_track)
{
   int      i;
   char     temp[RTNET_GPSPATH_BUFFERSIZE_single_row+1];

   if( (count >= 2) && (RTTRK_GPSPATH_MAX_POINTS >= count))
   {
	   sprintf( Packet, "GPSPath,%u,%u", (uint32_t)points->GPS_time, (3 * count));

	   for( i=0; i<count; i++)
	   {
	      char  gps_point[RoadMapPosition_STRING_MAXSIZE+1];
	      int   seconds_gap = 0;

	      if( i)
	         seconds_gap = (int)(points[i].GPS_time - points[i-1].GPS_time);

	      assert( !GPSPOINTINTIME_IS_INVALID(points[i]));
/*
	      if (points[i].Position.longitude < 20000000 ||
	      	 points[i].Position.longitude > 40000000 ||
	      	 points[i].Position.latitude < 20000000 ||
	      	 points[i].Position.latitude > 40000000 ||
	      	 seconds_gap < 0 ||
	      	 seconds_gap > 1000) {

	      	roadmap_log (ROADMAP_ERROR, "Invalid GPS sequence: %d,%d,%d",
	      					 points[i].Position.longitude,
	      					 points[i].Position.latitude,
	      					 seconds_gap);
	      }
*/
	      format_RoadMapPosition_string( gps_point, &(points[i].Position));
	      sprintf( temp, ",%s,%d,%d", gps_point, points[i].altitude, seconds_gap);
	      strcat( Packet, temp);
	   }
	   strcat( Packet, "\n");
   }

   if (end_track)
   {
   	strcat( Packet, "GPSDisconnect\n");
   }
}

BOOL RTNet_GPSPath(  LPRTConnectionInfo   pCI,
                     time_t               period_begin,
                     LPGPSPointInTime     points,
                     int                  count,
                     CB_OnWSTCompleted    pfnOnCompleted,
                     char*                packet_only)
{
   ebuffer Packet;
   char*    GPSPathBuffer = NULL;
   int      iRangeBegin;
   BOOL     bRes;
   int      i;

   if( count < 2)
      return FALSE;

   ebuffer_init( &Packet);

   if( RTTRK_GPSPATH_MAX_POINTS < count) {
      roadmap_log (ROADMAP_ERROR, "GPSPath too long, dropping first %d points", count - RTTRK_GPSPATH_MAX_POINTS);
      points += count - RTTRK_GPSPATH_MAX_POINTS;
   	points[0].Position.longitude = INVALID_COORDINATE;
   	points[0].Position.latitude = INVALID_COORDINATE;
   	points[0].GPS_time = 0;
      count = RTTRK_GPSPATH_MAX_POINTS;
   }

   GPSPathBuffer = ebuffer_alloc( &Packet, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));
   memset( GPSPathBuffer, 0, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));

   iRangeBegin = 0;
   for( i=0; i<count; i++)
   {
      if( GPSPOINTINTIME_IS_INVALID( points[i]))
      {
         int               iPointsCount= i - iRangeBegin;
         LPGPSPointInTime  FirstPoint  = points + iRangeBegin;
         char* Buffer = GPSPathBuffer + strlen(GPSPathBuffer);

         roadmap_log(ROADMAP_DEBUG,
                     "RTNet_GPSPath(GPS-DISCONNECTION TAG) - Adding %d points to packet. Range offset: %d",
                     iPointsCount, iRangeBegin);
         RTNet_GPSPath_BuildCommand( Buffer, FirstPoint, iPointsCount, TRUE);
         iRangeBegin = i+1;
      }
   }

   if( iRangeBegin < (count - 1))
   {
      LPGPSPointInTime  FirstPoint  = points + iRangeBegin;
      int               iPointsCount= count - iRangeBegin;
      char*             Buffer      = GPSPathBuffer + strlen(GPSPathBuffer);

      roadmap_log(ROADMAP_DEBUG,
                  "RTNet_GPSPath() - Adding range to packet. Range begin: %d; Range end: %d (count-1)",
                  iRangeBegin, (count - 1));
      RTNet_GPSPath_BuildCommand( Buffer, FirstPoint, iPointsCount, FALSE);
   }

   assert(*GPSPathBuffer);
   roadmap_log(ROADMAP_DEBUG, "RTNet_GPSPath() - Output command: '%s'", GPSPathBuffer);

   if( packet_only)
   {
      sprintf( packet_only, "%s", GPSPathBuffer);
      bRes = TRUE;
   }
   else
      bRes = wst_start_session_trans(
                              general_parser,
                              sizeof(general_parser)/sizeof(wst_parser),
                              pfnOnCompleted,
                              pCI,
                              GPSPathBuffer);      //   Custom data

   ebuffer_free( &Packet);
   return bRes;
}

BOOL RTNet_NodePath( LPRTConnectionInfo   pCI,
                     time_t               period_begin,
                     LPNodeInTime         nodes,
                     int                  count,
                     LPUserPointsVer      user_points,
                     int                  num_user_points,
                     CB_OnWSTCompleted    pfnOnCompleted,
                     char*                packet_only)
{
   ebuffer Packet;
   char*    NodePathBuffer = NULL;
   int      i;
   char     temp[RTNET_NODEPATH_BUFFERSIZE_temp+1];
   BOOL     bRes;
   BOOL     bAddUserPoints = FALSE;

   if( count < 1)
      return FALSE;

   if (num_user_points > 0 && num_user_points != count) {
      roadmap_log (ROADMAP_ERROR, "Number of user points (%d) does not equal nodes count (%d) ; dropping user points", num_user_points, count);
   } else if (num_user_points == count) {
      for( i=0; i<count; i++)
      {
         if (user_points[i].points > 0) {
            bAddUserPoints = TRUE;
            break;
         }
      }
   }

   ebuffer_init( &Packet);

   if( RTTRK_NODEPATH_MAX_POINTS < count) {
      // patch by SRUL
      //return FALSE;
      nodes += count - RTTRK_NODEPATH_MAX_POINTS;
      count = RTTRK_NODEPATH_MAX_POINTS;
   }

   NodePathBuffer = ebuffer_alloc( &Packet, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));

   memset( NodePathBuffer, 0, sizeof(NodePathBuffer));
   sprintf( NodePathBuffer, "NodePath,%d,", (unsigned int)period_begin);//(period_end-period_begin));
   sprintf( temp, "%d", 2 * count);
   strcat( NodePathBuffer, temp);

   for( i=0; i<count; i++)
   {
      int   seconds_gap = 0;

      if( i)
         seconds_gap = (int)(nodes[i].GPS_time - nodes[i-1].GPS_time);

      sprintf( temp, ",%d,%d", nodes[i].node, seconds_gap);
      strcat( NodePathBuffer, temp);
   }

   if (bAddUserPoints) {
      sprintf( temp, ",%d", EDITOR_POINT_TYPE_MUNCHING);
      strcat( NodePathBuffer, temp);

      for( i=0; i<count; i++)
      {
         int version_gap = user_points[i].version;

         if( i)
            version_gap = user_points[i].version - user_points[i-1].version;

         sprintf( temp, ",%d,%d", user_points[i].points, version_gap);
         strcat( NodePathBuffer, temp);
      }
   }

   if( packet_only)
   {
      sprintf( packet_only, "%s\n", NodePathBuffer);
      bRes = TRUE;
   }
   else
      bRes = wst_start_session_trans(
                              general_parser,
                              sizeof(general_parser)/sizeof(wst_parser),
                              pfnOnCompleted,
                              pCI,
                              NodePathBuffer);     //   Custom data

   ebuffer_free( &Packet);
   return bRes;
}

BOOL  RTNet_ExternalPoiDisplayed(     LPRTConnectionInfo   pCI,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only)
{
   int i;
   ebuffer Packet ;
   BOOL     bRes;
   int iPoiCount = RealtimeExternalPoiNotifier_DisplayedList_Count();
   char*    PoisBuffer = NULL;

   ebuffer_init(&Packet);
   if (iPoiCount == 0)
      return FALSE;

   PoisBuffer = ebuffer_alloc( &Packet, RTNET_EXTERNALPOIDISPLAYED_BUFFERSIZE__dynamic(iPoiCount));
   memset( PoisBuffer, 0, sizeof(PoisBuffer));
   sprintf(PoisBuffer,"NotifyExternalPoiDisplayed,%d", iPoiCount);

   for (i = 0; i < iPoiCount; i++){
       char temp[20];
       int iID = RealtimeExternalPoi_DisplayedList_get_ID(i);
       sprintf( temp, ",%d", iID);
       strcat( PoisBuffer, temp);
   }

   if( packet_only)
   {
      sprintf( packet_only, "%s\n", PoisBuffer);
      bRes = TRUE;
   }
   else
      bRes = wst_start_session_trans(
                              general_parser,
                              sizeof(general_parser)/sizeof(wst_parser),
                              pfnOnCompleted,
                              pCI,
                              PoisBuffer);     //   Custom data
   RealtimeExternalPoiNotifier_DisplayedList_clear();
   ebuffer_free( &Packet);
   return bRes;
}

BOOL RTNet_ReportAlert( LPRTConnectionInfo   pCI,
                        int                  iType,
                        const char*          szDescription,
                        int                  iDirection,
                        const char*          szImageId,
                        BOOL						bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        const char*          szGroup,
                        CB_OnWSTCompleted    pfnOnCompleted)
{
   const RoadMapGpsPosition   *TripLocation;
   int from_node, to_node;

   TripLocation = roadmap_trip_get_gps_position("AlertSelection");

   if (TripLocation == NULL)
   {
 		roadmap_messagebox ("Oops", "Can't find alert position.");
 		return FALSE;
   }
   else
   {
	   roadmap_trip_get_nodes( "AlertSelection", &from_node, &to_node );
      return RTNet_ReportAlertAtPosition(pCI, iType, szDescription, iDirection, szImageId, bForwardToTwitter,
                                          bForwardToFacebook, TripLocation, from_node, to_node, szGroup, pfnOnCompleted );
   }
}

BOOL RTNet_SendSMS( LPRTConnectionInfo   pCI,
	                const char*          szPhoneNumber,
	                CB_OnWSTCompleted pfnOnCompleted)
{

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_1SendSMS,// Custom data for the HTTP request
                           szPhoneNumber);
}


BOOL  RTNet_TwitterConnect (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   BOOL                 bForwardToTwitter,
                   int                  iDeviceId,
                   CB_OnWSTCompleted pfnOnCompleted){

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_5TwitterConnect,// Custom data for the HTTP request
                           userName,
                           passWord,
                           "true",
                           bForwardToTwitter ? "true" : "false",
                           iDeviceId);

}

BOOL  RTNet_FoursquareConnect (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   BOOL                 bTweetLogin,
                   CB_OnWSTCompleted    pfnOnCompleted){

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_4FoursquareConnect,// Custom data for the HTTP request
                           userName,
                           passWord,
                           "T",
                           bTweetLogin ? "T" : "F");

}

BOOL  RTNet_FoursquareSearch (
                   LPRTConnectionInfo   pCI,
                   RoadMapPosition*     coordinates,
                   CB_OnWSTCompleted    pfnOnCompleted){

   char  lat[RoadMapPosition_STRING_MAXSIZE+1];
   char  lon[RoadMapPosition_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string(lat, coordinates->latitude);
   convert_int_coordinate_to_float_string(lon, coordinates->longitude);

   return wst_start_session_trans(  general_parser,
                                    sizeof(general_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    RTNET_FORMAT_NETPACKET_2FoursquareSearch, // Custom data for the HTTP request
                                    lat,
                                    lon);
}

BOOL  RTNet_FoursquareCheckin (
                   LPRTConnectionInfo   pCI,
                   const char*          vid,
                   BOOL                 bTweetBadge,
                   CB_OnWSTCompleted    pfnOnCompleted){

   return wst_start_session_trans(  general_parser,
                                    sizeof(general_parser)/sizeof(wst_parser),
                                    pfnOnCompleted,
                                    pCI,
                                    RTNET_FORMAT_NETPACKET_2FoursquareCheckin, // Custom data for the HTTP request
                                    vid,
                                    bTweetBadge ? "T" : "F");
}

BOOL  RTNet_Scoreboard_getPoints (
                                  LPRTConnectionInfo   pCI,
                                  const char*          period,
                                  const char*          geography,
                                  int                  fromRank,
                                  int                  count,
                                  CB_OnWSTCompleted    pfnOnCompleted){

   char  sFromRank[20];
   char  sCount[20];

   snprintf(sFromRank, sizeof(sFromRank), "%d", fromRank);
   snprintf(sCount, sizeof(sCount), "%d", count);

   return wst_start_session_trans(  general_parser,
                                  sizeof(general_parser)/sizeof(wst_parser),
                                  pfnOnCompleted,
                                  pCI,
                                  RTNET_FORMAT_NETPACKET_4ScoreboardGetPoints, // Custom data for the HTTP request
                                  period,
                                  geography,
                                  sFromRank,
                                  sCount);
}

BOOL RTNet_PostAlertComment( LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        const char*          szDescription,
                        BOOL                 bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        CB_OnWSTCompleted pfnOnCompleted)
{

   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_PostAlertComment() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Sending Comment failed");
         return FALSE;
      }

      szPackedString = PackedString;
   }

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "PostAlertComment,%d,%s,%s,%s", // Custom data for the HTTP request
                           iAlertId,
                           szPackedString,
                           bForwardToTwitter ? "T" : "F",
                           bForwardToFacebook ? "T" : "F");
}


BOOL RTNet_ReportAlertAtPosition( LPRTConnectionInfo   pCI,
                        int                  iType,
                        const char*          szDescription,
                        int                  iDirection,
                        const char*          szImageId,
                        BOOL						bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        const RoadMapGpsPosition   *MyLocation,
                        int 				      from_node,
                        int 				      to_node,
                        const char*          szGroup,
                        CB_OnWSTCompleted pfnOnCompleted)
{
   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];
   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportAlertAtPosition() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Sending Report failed");
         return FALSE;
      }
      szPackedString = PackedString;
   }
   // Image ID - packing unnecessary
   if ( !szImageId )
       szImageId = "";

   if (!szGroup)
      szGroup = "";

   format_RoadMapGpsPosition_string( GPSPosString, MyLocation);

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "At,%s,%d,%d\nReportAlert,%d,%s,%d,%s,%s,%s,%s", // Custom data for the HTTP request
                              GPSPosString,
                              from_node,
                              to_node,
                              iType,
                              szPackedString,
                              iDirection,
                              szImageId,
                              bForwardToTwitter ? "T" : "F",
                              bForwardToFacebook ? "T" : "F",
                              szGroup);
}

BOOL RTNet_PinqWazer( LPRTConnectionInfo   pCI,
                      const RoadMapGpsPosition   *pPosition,
                      int                from_node,
                      int                to_node,
                      int                iUserId,
                      int                iAlertType,
                      const char*        szDescription,
                      const char*        szImageId,
                      BOOL               bForwardToTwitter,
                      CB_OnWSTCompleted pfnOnCompleted)
{
   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];
   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_PinqWazer() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Sending ping failed. Please try again later");
         return FALSE;
      }
      szPackedString = PackedString;
   }

   // Image ID - packing unnecessary
   if ( !szImageId )
       szImageId = "";

   format_RoadMapGpsPosition_Pos_Azy_Str( GPSPosString, pPosition);

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "PingWazer,%s,%d,%d,%d,%d,%s,%s,%s", // Custom data for the HTTP request
                              GPSPosString,
                              from_node,
                              to_node,
                              iUserId,
                              iAlertType,
                              szPackedString,
                              szImageId,
                              bForwardToTwitter ? "T" : "F");
}

BOOL RTNet_RemoveAlert( LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        CB_OnWSTCompleted pfnOnCompleted)
{
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "ReportRmAlert,%d",  // Custom data for the HTTP request
                              iAlertId);
}

BOOL RTNet_ReportMapProblem( LPRTConnectionInfo   pCI,
                             const char*          szType,
                             const char*          szDescription,
                             const RoadMapGpsPosition   *MyLocation,
                             ESendMapProblemResult*  SendMapProblemResult,
                             CB_OnWSTCompleted pfnOnCompleted,
                             char* packet_only)
{
   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";


   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportAlertAtPosition() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Sending Report failed");
         (*SendMapProblemResult) = SendMapProblemValidityFailure;
         return FALSE;
      }
      szPackedString = PackedString;
   }

   if (MyLocation == NULL){
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportMapProblem() - Coordinates are null");
         (*SendMapProblemResult) = SendMapProblemValidityFailure;
         return FALSE;
   }
   (*SendMapProblemResult) = SendMapProblemValidityOK;
   if (packet_only)
   {
      sprintf( packet_only,
               RTNET_FORMAT_NETPACKET_4ReportMapError, // Custom data for the HTTP request
               MyLocation->longitude, //x
               MyLocation->latitude, // y
               szType,
               szPackedString );
      return TRUE;
   }


   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_4ReportMapError,// Custom data for the HTTP request
                           MyLocation->longitude, //x
                           MyLocation->latitude, //y
                           szType,
                           szPackedString);
}

BOOL  RTNet_ReportMarker(  LPRTConnectionInfo   pCI,
                           const char*          szType,
                           int                  iLongitude,
                           int                  iLatitude,
                           int                  iAzimuth,
                           const char*          szDescription,
                           int                  nParams,
                           const char*            szParamKey[],
                           const char*            szParamVal[],
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                  packet_only)
{
   char        PackedString[(ED_MARKER_MAX_STRING_SIZE*2)+1];
   char         AttrString[ED_MARKER_MAX_ATTRS * 2 * (ED_MARKER_MAX_STRING_SIZE * 2 + 1) + 4];
   const char* szPackedString = "";
   char        float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char        float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, iLongitude);
   convert_int_coordinate_to_float_string( float_string_latitude , iLatitude);

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportMarker() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Sending Report failed");
         return FALSE;
      }

      szPackedString = PackedString;
   }

   if (!format_ParamPair_string(AttrString,
                                sizeof(AttrString),
                                nParams,
                                szParamKey,
                                szParamVal))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_ReportMarker() - Failed to serialize attributes");
      roadmap_messagebox ("Oops", "Sending Report failed");
      return FALSE;
   }

   if (packet_only)
   {
      sprintf( packet_only,
               RTNET_FORMAT_NETPACKET_6ReportMarker, // Custom data for the HTTP request
               szType,
               float_string_longitude,
               float_string_latitude,
               iAzimuth,
               szPackedString,
               AttrString);
      return TRUE;
   }

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_6ReportMarker, // Custom data for the HTTP request
                              szType,
                              float_string_longitude,
                              float_string_latitude,
                              iAzimuth,
                              szPackedString,
                              AttrString);
}

BOOL RTNet_StartFollowUsers(  LPRTConnectionInfo   pCI,
                              CB_OnWSTCompleted pfnOnCompleted,
                              char*                packet_only)
{ return FALSE;}

BOOL RTNet_StopFollowUsers(   LPRTConnectionInfo   pCI,
                              CB_OnWSTCompleted pfnOnCompleted,
                              char*                packet_only)
{ return FALSE;}

BOOL RTNet_SetMyVisability(LPRTConnectionInfo   pCI,
                           ERTVisabilityGroup   eVisability,
                           ERTVisabilityReport  eVisabilityReport,
                           CB_OnWSTCompleted pfnOnCompleted,
                           BOOL downloadWazers,
                           BOOL downloadReports,
                           BOOL downloadTraffic,
                           BOOL allowPing,
                           int  eventsRadius,
                           char*                packet_only)
{
   if( packet_only)
   {
      sprintf( packet_only, "SeeMe,%d,%d,%s,%s,%s,%s,%d\n", eVisability,eVisabilityReport,
      												downloadWazers ? "T" : "F",
      												downloadReports ? "T" : "F",
      												downloadTraffic ? "T" : "F",
      												allowPing ? "1" : "2",
      											   eventsRadius);
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "SeeMe,%d,%d,%s,%s,%s,%s,%d",          //   Custom data for the HTTP request
                            eVisability,eVisabilityReport,
                            downloadWazers ? "T" : "F",
                            downloadReports ? "T" : "F",
      						    downloadTraffic ? "T" : "F",
      						    allowPing ? "1" : "2",
      						    eventsRadius);
}

BOOL RTNet_SetMood(LPRTConnectionInfo   pCI,
                   int 					iMood,
                   CB_OnWSTCompleted pfnOnCompleted,
                   char*                packet_only)
{
   if( packet_only)
   {
      sprintf( packet_only, "SetMood,%d\n", iMood);
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "SetMood,%d",          //   Custom data for the HTTP request
                           iMood);
}

BOOL RTNet_Location        (LPRTConnectionInfo     pCI,
                           const RoadMapPosition*  location,
                           CB_OnWSTCompleted       pfnOnCompleted,
                           char*                   packet_only)
{
   char  location_string[RoadMapPosition_STRING_MAXSIZE+1];

   format_RoadMapPosition_string( location_string, location);

   if( packet_only)
   {
      sprintf( packet_only, "Location,%s\n", location_string);
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "Location,%s\n",          //   Custom data for the HTTP request
                           location_string);
}

BOOL RTNet_UserPoints(LPRTConnectionInfo   pCI,
                   int              iUserPoints,
                   CB_OnWSTCompleted pfnOnCompleted,
                   char*                packet_only)
{
   if( packet_only)
   {
      sprintf( packet_only, "UserPoints,%d\n", iUserPoints);
      return TRUE;
   }

   // Else:
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "UserPoints,%d",          //   Custom data for the HTTP request
                           iUserPoints);
}

BOOL RTNet_SendTrafficInfo ( LPRTConnectionInfo   pCI,
                                                              int mode,
                                                               CB_OnWSTCompleted pfnOnCompleted){

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "SendRoadInfo,%d",   // Custom data for the HTTP request
                              mode);
}

BOOL RTNet_TrafficAlertFeedback( LPRTConnectionInfo   pCI,
                        int                  iTrafficInfoId,
                        int                  iValidity,
                        CB_OnWSTCompleted pfnOnCompleted)
{
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "TrafficAlertFeedback,%d,%d",  // Custom data for the HTTP request
                           iTrafficInfoId, iValidity);
}

BOOL  RTNet_CreateAccount (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   const char*          email,
                   BOOL                 send_updates,
                   CB_OnWSTCompleted pfnOnCompleted){

   return wst_start_trans( gs_WST,
            "createaccount",
            general_parser,
            sizeof(general_parser)/sizeof(wst_parser),
            pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_4CreateAccount,// Custom data for the HTTP request
                           userName,
                           passWord,
                           email,
                           send_updates ? "true" : "false");

}

BOOL  RTNet_UpdateProfile (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   const char*          email,
                   BOOL                 send_updates,
                   CB_OnWSTCompleted pfnOnCompleted){

   roadmap_log( ROADMAP_DEBUG, "RTNet_UpdateProfile() - username=%s, password=%s,email=%s, send_updates=%d", userName, passWord,email, send_updates);

   return wst_start_session_trans( general_parser,
                                   sizeof(general_parser)/sizeof(wst_parser),
                                   pfnOnCompleted,
                                   pCI,
                                   RTNET_FORMAT_NETPACKET_4UpdateProfile,// Custom data for the HTTP request
                                   userName,
                                   passWord,
                                   email,
                                   send_updates ? "true" : "false");

}
BOOL RTNet_GeneralPacket(  LPRTConnectionInfo   pCI,
                           const char*          Packet,
                           CB_OnWSTCompleted    pfnOnCompleted)
{
	//Realtime_OfflineWrite (Packet);
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           Packet);             //   Custom data for the HTTP request
}

BOOL RTNet_CollectBonus(LPRTConnectionInfo   pCI,
                        int                  iId,
                        int                  iToken,
                        BOOL                 bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        CB_OnWSTCompleted    pfnOnCompleted){

   char AtStr[MESSAGE_MAX_SIZE__At];
   SendMessage_At( AtStr, FALSE);

   return wst_start_session_trans(
                             general_parser,
                             sizeof(general_parser)/sizeof(wst_parser),
                             pfnOnCompleted,
                             pCI,
                             "%sCollectBonus,%d,%d,%s,%s",  // Custom data for the HTTP request
                             AtStr,
                             iId,
                             iToken,
                             bForwardToTwitter ? "T" : "F",
                             bForwardToFacebook ? "T" : "F");

}

BOOL RTNet_CollectCustomBonus(LPRTConnectionInfo   pCI,
                        int                  iId,
                        BOOL                 bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        CB_OnWSTCompleted    pfnOnCompleted){

    BOOL success;
    char AtStr[MESSAGE_MAX_SIZE__At];
    success = SendMessage_At( AtStr, FALSE);
    if (!success)
       AtStr[0] = '\0';
    return wst_start_session_trans(
                             general_parser,
                             sizeof(general_parser)/sizeof(wst_parser),
                             pfnOnCompleted,
                             pCI,
                             "%sCollectCustomBonus,%d,%s,%s",  // Custom data for the HTTP request
                             AtStr,
                             iId,
                             bForwardToTwitter ? "T" : "F",
                             bForwardToFacebook ? "T" : "F");

}

BOOL RTNet_ReportAbuse (LPRTConnectionInfo   pCI,
                           int                  iAlertID,
                           int                  iCommentID,
                           CB_OnWSTCompleted    pfnOnCompleted){
   return wst_start_session_trans(
                             general_parser,
                             sizeof(general_parser)/sizeof(wst_parser),
                             pfnOnCompleted,
                             pCI,
                             "ReportAbuse,%d,%d",  // Custom data for the HTTP request
                             iAlertID,
                             iCommentID);
}

BOOL  RTNet_FacebookPermissions (LPRTConnectionInfo   pCI,
                                 int                  iShowFacebookName,
                                 int                  iShowFacebookPicture,
                                 int                  iShowFacebookProfile,
                                 int                  iShowTwitterProfile,
                                 CB_OnWSTCompleted    pfnOnCompleted){
   const char *show_name, *show_picture, *show_facebook_profile, *show_twitter_profile;

   switch (iShowFacebookName) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED:
         show_name = "false";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         show_name = "true";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         show_name = "friends";
         break;
      default:
         show_name = "";
         break;
   }

   switch (iShowFacebookPicture) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED:
         show_picture = "false";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         show_picture = "true";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         show_picture = "friends";
         break;
      default:
         show_name = "";
         break;
   }

   switch (iShowFacebookProfile) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED:
         show_facebook_profile = "false";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         show_facebook_profile = "true";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         show_facebook_profile = "friends";
         break;
      default:
         show_facebook_profile = "";
         break;
   }

   switch (iShowTwitterProfile) {
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_DISABLED:
         show_twitter_profile = "false";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_ENABLED:
         show_twitter_profile = "true";
         break;
      case ROADMAP_SOCIAL_SHOW_DETAILS_MODE_FRIENDS:
         show_twitter_profile = "friends";
         break;
      default:
         show_twitter_profile = "";
         break;
   }

   return wst_start_session_trans(
                                  general_parser,
                                  sizeof(general_parser)/sizeof(wst_parser),
                                  pfnOnCompleted,
                                  pCI,
                                  RTNET_FORMAT_NETPACKET_4FacebookPermissions,// Custom data for the HTTP request
                                  show_name,
                                  show_picture,
                                  show_facebook_profile,
                                  show_twitter_profile);

}


BOOL  RTNet_ExternalPoiNotifyOnPopUp (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  CB_OnWSTCompleted   pfnOnCompleted){

    return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           "NotifyExternalPoiViewed,%d",  // Custom data for the HTTP request
                           iID);
}

BOOL  RTNet_ExternalPoiNotifyOnNavigate (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  CB_OnWSTCompleted   pfnOnCompleted){
     return wst_start_session_trans(
                            general_parser,
                            sizeof(general_parser)/sizeof(wst_parser),
                            pfnOnCompleted,
                            pCI,
                            "NotifyExternalPoiNavigation,%d",  // Custom data for the HTTP request
                            iID);

}

BOOL  RTNet_NotifySplashUpdateTime (LPRTConnectionInfo   pCI,
                                  const char *                 update_time,
                                  CB_OnWSTCompleted   pfnOnCompleted){
   char        PackedString[50];
   if( update_time && (*update_time))
   {
      if(PackNetworkString( update_time, PackedString, sizeof(PackedString)))
         return wst_start_session_trans(
                            general_parser,
                            sizeof(general_parser)/sizeof(wst_parser),
                            pfnOnCompleted,
                            pCI,
                            "NotifySplashUpdateTime,%s",  // Custom data for the HTTP request
                            PackedString);
      else
         return FALSE;
   }
   else
      return FALSE;

}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   RTNet_ReportOneSegment_Encode (char *packet, int iSegment)
{
   int trkseg;
   int line_flags;
   int num_shapes;
   int first_shape;
   int last_shape;
   const char *action;
   const char *status;
   RoadMapPosition from;
   RoadMapPosition to;
   int cfcc;
   int trkseg_flags;
   int from_point;
   int to_point;
   int from_id;
   int to_id;
   char from_string[DB_Point_STRING_MAXSIZE];
   char to_string[DB_Point_STRING_MAXSIZE];
   time_t start_time;
   time_t end_time;
   int num_attr;
   int street = -1;
   char name_string[513];
   char t2s_string[513];
   char city_string[513];

   if (editor_line_committed (iSegment))
   {
      *packet = '\0';
      return TRUE;
   }

   editor_line_get (iSegment, &from, &to, &trkseg, &cfcc, &line_flags);
   if (!(line_flags & ED_LINE_DIRTY))
   {
      *packet = '\0';
      return TRUE;
   }

   editor_trkseg_get (trkseg, NULL, &first_shape, &last_shape, &trkseg_flags);
   if (first_shape >= 0) num_shapes = last_shape - first_shape + 1;
   else                   num_shapes = 0;

   editor_trkseg_get_time (trkseg, &start_time, &end_time);

   editor_line_get_points (iSegment, &from_point, &to_point);
   from_id = editor_point_db_id (from_point);
   to_id = editor_point_db_id (to_point);

   format_DB_point_string (from_string, from.longitude, from.latitude, start_time, from_id);
   format_DB_point_string (to_string, to.longitude, to.latitude, end_time, to_id);

   if (line_flags & ED_LINE_DELETED) action = "delete";
   else                               action = "update";

   status = "fake"; // should be omitted from command line

   sprintf (packet,
            RTNET_FORMAT_NETPACKET_5ReportSegment,
            action,
            status,
            from_string,
            to_string,
            num_shapes * 3);

   if (num_shapes)
   {
      char shape_string[Point_STRING_MAXSIZE];
      int shape;

      RoadMapPosition curr_position = from;
      time_t curr_time = start_time;

      for (shape = first_shape; shape <= last_shape; shape++)
      {
         editor_shape_position (shape, &curr_position);
         editor_shape_time (shape, &curr_time);
         format_point_string (shape_string, curr_position.longitude, curr_position.latitude, curr_time);
         strcat (packet, shape_string);
      }
   }

   num_attr = 0;
   if ((line_flags & ED_LINE_DELETED) == 0)
   {
      editor_line_get_street (iSegment, &street);
      if (street >= 0)
      {
         num_attr = 4;

         PackNetworkString (editor_street_get_street_name (street), name_string, sizeof (name_string) - 1);
         PackNetworkString (editor_street_get_street_t2s (street),  t2s_string,  sizeof (t2s_string)  - 1);
         PackNetworkString (editor_street_get_street_city (street), city_string, sizeof (city_string) - 1);

         sprintf (packet + strlen (packet),
                  RTNET_FORMAT_NETPACKET_5ReportSegmentAttr,
                  num_attr * 2,
                  roadmap_layer_cfcc2type (cfcc),
                  name_string,
                  t2s_string,
                  city_string);
      }
   }

   if (num_attr == 0)
   {
      sprintf (packet + strlen (packet),
               RTNET_FORMAT_NETPACKET_1ReportSegmentNoAttr,
               num_attr * 2);
   }


   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int   RTNet_ReportOneSegment_MaxLength (int iSegment)
{
   int trkseg;
   int flags;
   int num_shapes;
   int first_shape;
   int last_shape;
   int num_bytes;

   if (editor_line_committed (iSegment)) return 0;

   editor_line_get (iSegment, NULL, NULL, &trkseg, NULL, &flags);
   if (!(flags & ED_LINE_DIRTY)) return 0;

   editor_trkseg_get (trkseg, NULL, &first_shape, &last_shape, NULL);
   if (first_shape >= 0) num_shapes = last_shape - first_shape + 1;
   else                   num_shapes = 0;

   num_bytes =
      + 14                           // command
      + 7                           // action
      + 13                           // status
      + (12 + 12 + 12 + 12) * 2      // from,to
      + 12                           // num points
      + (12 + 12 + 12) * num_shapes // shape points
      + 12;                           // num attributes

   if ((flags & ED_LINE_DELETED) == 0)
   {
      num_bytes +=
         4 * (256 * 2 + 2);         // street attributes
   }

   num_bytes += 1024;               // just in case

   return num_bytes;
}

void	RTNet_Auth_BuildCommand (	char*				Command,
											int				ServerId,
											const char*		ServerCookie,
											const char*		UserName,
											int				DeviceId,
											const char*		Version)
{

	sprintf (Command, RTNET_FORMAT_NETPACKET_5Auth,
				ServerId, ServerCookie, UserName, DeviceId, Version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_TripServer_CreatePOI  (LPRTConnectionInfo   pCI,
											 const char*           name,
											 RoadMapPosition*      coordinates,
											 BOOL					     overide,
											 int                   id,
											 CB_OnWSTCompleted pfnOnCompleted){

   if (name == NULL){
   		roadmap_log( ROADMAP_ERROR, "RTNet_TripServerCreatePOI() - Name is null");
   		return FALSE;
   }

   if (coordinates == NULL){
   		roadmap_log( ROADMAP_ERROR, "RTNet_TripServerCreatePOI() - Coordinates are null");
   		return FALSE;
   }

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_5TripServerCreatePOI,// Custom data for the HTTP request
                           coordinates->longitude, //x
                           coordinates->latitude, //y
                           name,
                           "true",
                           id);

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_TripServer_GetPOIs  (LPRTConnectionInfo   pCI,
                                 CB_OnWSTCompleted pfnOnCompleted){

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_0TripServerGetPOIs,// Custom data for the HTTP request
                           "true");

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_TripServer_GetNumPOIs  (LPRTConnectionInfo   pCI,
                                 CB_OnWSTCompleted pfnOnCompleted){

   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_0TripServerGetNumPOIs,// Custom data for the HTTP request
                           "true");

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_TripServer_DeletePOI  (LPRTConnectionInfo   pCI,
									 const char*          name,
									 CB_OnWSTCompleted pfnOnCompleted){

   if (name == NULL){
   		roadmap_log( ROADMAP_ERROR, "RTNet_TripDeletePOI() - Name is null");
   		return FALSE;
   }


   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_1TripServerDeletePOI,// Custom data for the HTTP request
                           name);

}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNet_TripServer_FindTrip  (
								LPRTConnectionInfo   pCI,
								RoadMapPosition*     coordinates,
								CB_OnWSTCompleted pfnOnCompleted){

	//time_t current_time = time(NULL);
   roadmap_log (ROADMAP_DEBUG, "RTNet_TripServer_FindTrip() - lat=%d, lon=%d",coordinates->latitude,coordinates->longitude );

	return wst_start_session_trans(
                general_parser,
                sizeof(general_parser)/sizeof(wst_parser),
                pfnOnCompleted,
                pCI,
                RTNET_FORMAT_NETPACKET_2TripServerFindTrip,// Custom data for the HTTP request
                coordinates->longitude, //x
                coordinates->latitude
                );
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNET_TripServer_GetTripRoutes(
								LPRTConnectionInfo pCI,
								int routId,
								CB_OnWSTCompleted pfnOnCompleted){

	return wst_start_session_trans(
                general_parser,
                sizeof(general_parser)/sizeof(wst_parser),
                pfnOnCompleted,
                pCI,
                RTNET_FORMAT_NETPACKET_1TripServerGetTripRoutes,// Custom data for the HTTP request
                routId);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNET_TripServer_GetRouteSegments(LPRTConnectionInfo
									  pCI, int routId,
									  CB_OnWSTCompleted pfnOnCompleted){

	return wst_start_session_trans(
                general_parser,
                sizeof(general_parser)/sizeof(wst_parser),
                pfnOnCompleted,
                pCI,
                RTNET_FORMAT_NETPACKET_1TripServerGetRouteSegments,// Custom data for the HTTP request
                routId);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void RTNet_TransactionQueue_Clear()
{
   if( gs_WST)
      wst_queue_clear( gs_WST);
#ifdef   _DEBUG
   else
      assert(0);
#endif   // _DEBUG
}

BOOL RTNet_TransactionQueue_ProcessSingleItem( BOOL* pTransactionStarted)
{
   if( gs_WST)
      return wst_process_queue_item( gs_WST, pTransactionStarted);

   assert(0);
   return FALSE;
}

void RTNet_Watchdog(LPRTConnectionInfo  pCI)
{
   if( gs_WST)
      wst_watchdog( gs_WST);
#ifdef   _DEBUG
   else
      assert(0);
#endif   // _DEBUG
}

ETransactionStatus RTNet_GetTransactionState()
{
   transaction_state state;

   if( !gs_WST)
   {
      assert(0);
      return TS__invalid;
   }

   state = wst_get_trans_state( gs_WST);

   switch(state)
   {
      case trans_idle:     return TS_Idle;
      case trans_active:   return TS_Active;
      case trans_stopping: return TS_Stopping;
   }

   assert(0);
   return TS__invalid;
}

void RTNet_AbortTransaction( ETransactionStatus* new_state)
{
   ETransactionStatus State = RTNet_GetTransactionState();

   (*new_state) = TS__invalid;

   if( TS__invalid == State)
      return;  // assert was already raised...

   if( TS_Idle == State)
      (*new_state) = TS_Idle;
   else
   {
      wst_stop_trans( gs_WST);
      (*new_state) = RTNet_GetTransactionState();
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNet_RequestRoute(LPRTConnectionInfo   pCI,
								int						iRoute,
								int						iType,
								int						iTripId,
								int						iAltId,
								int						nMaxRoutes,
								int						nMaxSegments,
								int						nMaxPoints,
								RoadMapPosition		posFrom,
								int						iFrSegmentId,
								int						iFrNodeId[2],
								const char*				szFrStreet,
								BOOL						bFrAllowBidi,
								RoadMapPosition		posTo,
								int						iToSegmentId,
								int						iToNodeId[2],
								const char*				szToStreet,
								const char*          szToStreetNumber,
								const char*          szToCity,
								const char*          szToState,
								BOOL						bToAllowBidi,
								int						nOptions,
								const int*				iOptionNumeral,
								const BOOL*				bOptionValue,
								int                  iTwitterLevel,
								int                  iFacebookLevel,
								BOOL						bReRoute,
                        CB_OnWSTCompleted		pfnOnCompleted)
{
   char        *PackedFrStreet = NULL;
   char        *PackedToStreet = NULL;
   const char* szPackedFrStreet = "";
   const char* szPackedToStreet = "";
   char			*szPacket = NULL;
   int			iPacketSize;
   BOOL			rc = FALSE;
   int			iOption;
   int         len;
   time_t      now;
   struct tm   *current_time;
   long        lTimeOfDay;

   if (szFrStreet && (*szFrStreet))
   {
   	int iPackedSize = strlen (szFrStreet) * 2 + 1;
   	PackedFrStreet = malloc (iPackedSize);
      if (!PackNetworkString (szFrStreet, PackedFrStreet, iPackedSize))
      {
         roadmap_log (ROADMAP_ERROR, "RTNet_RequestRoute() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Routing Request Failed");
         goto Exit;
      }

      szPackedFrStreet = PackedFrStreet;
   }

   if (szToStreet && (*szToStreet))
   {
   	int iPackedSize = strlen (szToStreet) * 2 + 1;
   	PackedToStreet = malloc (iPackedSize);
      if (!PackNetworkString (szToStreet, PackedToStreet, iPackedSize))
      {
         roadmap_log (ROADMAP_ERROR, "RTNet_RequestRoute() - Failed to pack network string");
         roadmap_messagebox ("Oops", "Routing Request Failed");
         goto Exit;
      }

      szPackedToStreet = PackedToStreet;
   }

   if (!szToStreetNumber)
      szToStreetNumber = "";
   if (!szToCity)
      szToCity = "";
   if (!szToState)
      szToState = "";

   iPacketSize = 15 + // RoutingRequest,
   				  11 + // route_id,
   				  11 + // type,
   				  11 + // trip_id,
   				  11 + // max_routes,
   				  11 + // max segments,
   				  11 + // max points,
   				  2 * (12 + 12 + 11 + 11 + 11 + 1 + 2) + // from / to
   				  strlen (szPackedFrStreet) + 1 +
   				  strlen (szPackedToStreet) + 1 +
   				  2 + 2 + // with geometries, with instructions
   				  11 + nOptions * (11 + 2) + // options
                 2 + strlen(szToStreetNumber) + 1 + strlen(szToCity) + 1 + strlen(szToState) + 1 + 11 + 2 + 2
                                    // iTwitterLevel, szToStreetNumber, szToCity, szToState, lTimeOfDay, iFacebookLevel, bReroute
   				  ;

  	szPacket = malloc (iPacketSize);

   snprintf (szPacket, iPacketSize,
   			 "RoutingRequest,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%d,%d,%d,%d,%d,%s,%s,T,T,%d",
   			 iRoute, iType, iTripId, nMaxRoutes, nMaxSegments, nMaxPoints,
   			 posFrom.longitude, posFrom.latitude, iFrSegmentId, iFrNodeId[0], iFrNodeId[1],
   			 szPackedFrStreet, bFrAllowBidi ? "T" : "F",
   			 posTo.longitude, posTo.latitude, iToSegmentId, iToNodeId[0], iToNodeId[1],
   			 szPackedToStreet, bToAllowBidi ? "T" : "F",
   			 nOptions*2);

   for (iOption = 0; iOption < nOptions; iOption++)
   {
   	len = strlen (szPacket);
   	snprintf (szPacket + len, iPacketSize - len, ",%d,%s",
   				 iOptionNumeral[iOption], bOptionValue[iOption] ? "T" : "F");
   }

   now = time(NULL);
   current_time = localtime(&now);
   lTimeOfDay = current_time->tm_hour *60*60 + current_time->tm_min *60 + current_time->tm_sec;
   len = strlen (szPacket);
   snprintf (szPacket + len, iPacketSize - len, ",%d,%s,%s,%s,%ld,%d,%s",
             iTwitterLevel, szToStreetNumber, szToCity, szToState, lTimeOfDay, iFacebookLevel,
             bReRoute ? "T" : "F");
   roadmap_log (ROADMAP_ERROR, szPacket);

   rc = wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           szPacket);

Exit:
	if (szPacket) free (szPacket);
	if (PackedToStreet) free (PackedToStreet);
	if (PackedFrStreet) free (PackedFrStreet);
	return rc;
}

BOOL	RTNet_SelectRoute	(LPRTConnectionInfo   	pCI,
								 int							iRoute,
								 int							iAltId,
								 CB_OnWSTCompleted		pfnOnCompleted)
{
   return wst_start_session_trans(
                           general_parser,
                           sizeof(general_parser)/sizeof(wst_parser),
                           pfnOnCompleted,
                           pCI,
                           RTNET_FORMAT_NETPACKET_2SelectRoute,// Custom data for the HTTP request
                           iRoute,
                           iAltId);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL RTNet_GetGeoConfig(
                  LPRTConnectionInfo         pCI,
                  wst_handle                 websvc,
                  const RoadMapPosition      *pGPSPosition,
                  const char                 *name,
                  CB_OnWSTCompleted          pfnOnCompleted)
{

   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];

   format_RoadMapPosition_string( GPSPosString, pGPSPosition);

   // Perform WebService Transaction:
    if( wst_start_trans( websvc,
                         "login",
                         geo_config_parser,
                         sizeof(geo_config_parser)/sizeof(wst_parser),
                         pfnOnCompleted,
                         pCI,
                         RTNET_FORMAT_NETPACKET_5GetGeoConfig,// Custom data for the HTTP request
                         RTNET_PROTOCOL_VERSION,
                         GPSPosString,
                         RT_DEVICE_ID,
                         roadmap_start_version(),
                         name))

       return TRUE;
    else
       return FALSE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef IPHONE
BOOL RTNet_SetPushNotifications( LPRTConnectionInfo  pCI,
                                const char*          szToken,    //max 64
                                BOOL                 bScore,
                                BOOL                 bUpdates,
                                BOOL                 bFriends,
                                CB_OnWSTCompleted    pfnOnCompleted,
                                char*                packet_only)
{

   if (packet_only)
   {
      sprintf( packet_only,
              RTNET_FORMAT_NETPACKET_4PushNotificationsSet, // Custom data for the HTTP request
              szToken,
              bScore ? "T" : "F",
              bUpdates ? "T" : "F",
              bFriends ? "T" : "F");
      return TRUE;
   }


   return wst_start_session_trans(
                                  general_parser,
                                  sizeof(general_parser)/sizeof(wst_parser),
                                  pfnOnCompleted,
                                  pCI,
                                  RTNET_FORMAT_NETPACKET_4PushNotificationsSet,// Custom data for the HTTP request
                                  szToken,
                                  bScore ? "T" : "F",
                                  bUpdates ? "T" : "F",
                                  bFriends ? "T" : "F");
}
#endif //IPHONE
