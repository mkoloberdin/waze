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

#ifndef   __FREEMAP_REALTIMENETDEFS_H__
#define   __FREEMAP_REALTIMENETDEFS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include "Realtime/RealtimeDefs.h"
#include "roadmap_net.h"
#include "../roadmap_gps.h"

#include "websvc_trans/websvc_trans.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RTNET_SESSION_TIME                     (75)  /* seconds */
#define  RTNET_SERVERCOOKIE_MAXSIZE             (63)
#define  RTNET_WEBSERVICE_ADDRESS               ("")
#define  RTNET_PROTOCOL_VERSION                 (135)
#define  RTNET_PACKET_MAXSIZE                   MESSAGE_MAX_SIZE__AllTogether
#define  RTNET_PACKET_MAXSIZE__dynamic(_GPSPointsCount_,_NodesPointsCount_)      \
               MESSAGE_MAX_SIZE__AllTogether__dynamic(_GPSPointsCount_,_NodesPointsCount_)

//efine  RTNET_HTTP_STATUS_STRING_MAXSIZE       (63)
#define  RTTRK_GPSPATH_MAX_POINTS               (100)
#define  RTTRK_NODEPATH_MAX_POINTS              (60)
#define  RTTRK_CREATENEWROADS_MAX_TOGGLES       (40)
#define  RTTRK_MIN_VARIANT_THRESHOLD            (5)
#define  RTTRK_MIN_DISTANCE_BETWEEN_POINTS      (2)
#define  RTTRK_MAX_DISTANCE_BETWEEN_POINTS      (1000)
#define  RTTRK_COMPRESSION_RATIO_THRESHOLD      (0.8F)
#define  RTTRK_MAX_SECONDS_BETWEEN_GPS_POINTS   (2)

#define  RTNET_SYSTEMMESSAGE_TITLE_MAXSIZE      (64)
#define  RTNET_SYSTEMMESSAGE_TEXT_MAXSIZE       (512)
#define  RTNET_SYSTEMMESSAGE_ICON_MAXSIZE       (20)
#define  RTNET_SYSTEMMESSAGE_TEXT_COLOR_MAXSIZE (16)

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// BUFFER SIZES   //
////////////////////

// RTNet_HttpAsyncTransaction:
//    Q: How big should be the buffer to hold the formatted input string?
//    A: strlen(format) + (maximum number of place-holders (%) * size of its data)
//    The command with the highest number of place-holders is 'At' (RTNet_At)
//    with the following string:    "At,%f,%f,%f,%d,%f,%d,%d"
//    thus maximum additional size is:  20 20 20 10 20 10 10      (110)
//    To be on the safe-side, call it: 256
#define  HttpAsyncTransaction_MAXIMUM_SIZE_NEEDED_FOR_ADDITIONAL_PARAMS    (256)

#define  CUSTOM_HEADER_MAX_SIZE              (20 + RTNET_SERVERCOOKIE_MAXSIZE)
//             "UID,<%ddddddddd>,<RTNET_SERVERCOOKIE_MAXSIZE>\r\n"


//                                             4 float strings (coordinate)           ','
#define  RoadMapArea_STRING_MAXSIZE          ((4 * COORDINATE_VALUE_STRING_MAXSIZE) +  3)

//                                             3 float strings (coordinate)           speed/dir     ','
#define  RoadMapGpsPosition_STRING_MAXSIZE   ((3 * COORDINATE_VALUE_STRING_MAXSIZE) + (2 * 10)   +   4)

//                                             2 float strings (coordinate)          ','
#define  RoadMapPosition_STRING_MAXSIZE      ((2 * COORDINATE_VALUE_STRING_MAXSIZE) + 2 + 1)

#define  DB_Point_STRING_MAXSIZE             (49)
#define  Point_STRING_MAXSIZE                (37)

#define  MESSAGE_MAX_SIZE__At                (32 + RoadMapGpsPosition_STRING_MAXSIZE)
#define  MESSAGE_MAX_SIZE__MapDisplyed       (100)
#define  MESSAGE_MAX_SIZE__SeeMe             (20)
#define  MESSAGE_MAX_SIZE__SetMood           (12)
#define  MESSAGE_MAX_SIZE__Location          (28)
#define  MESSAGE_MAX_SIZE__UserPoints        (10 + 1 + 10)
#define  MESSAGE_MAX_SIZE__CreateNewRoads    (14 + 1 + 10 + 1 + 1 + 1)
#define	MESSAGE_MAX_SIZE__Auth					(4 + 1 + \
															 10 + 1 + \
															 RTNET_SERVERCOOKIE_MAXSIZE + 1 + \
															 RT_USERNM_MAXSIZE * 2 + 1 + \
															 10 + 1 + \
															 64)


#define  MESSAGE_MAX_SIZE__AllTogether             \
         (  HTTP_HEADER_MAX_SIZE                +  \
            CUSTOM_HEADER_MAX_SIZE              +  \
            MESSAGE_MAX_SIZE__At                +  \
            MESSAGE_MAX_SIZE__MapDisplyed       +  \
            MESSAGE_MAX_SIZE__CreateNewRoads    +  \
            RTNET_GPSPATH_BUFFERSIZE            +  \
            RTNET_NODEPATH_BUFFERSIZE           +  \
            RTNET_CREATENEWROADS_BUFFERSIZE     +  \
            MESSAGE_MAX_SIZE__SeeMe             +  \
            MESSAGE_MAX_SIZE__UserPoints)
#define  MESSAGE_MAX_SIZE__AllTogether__dynamic(_GPSPointsCount_,_NodesPointsCount_,_AllowNewRoadCount_) \
         (  HTTP_HEADER_MAX_SIZE                                           +  \
            CUSTOM_HEADER_MAX_SIZE                                         +  \
            MESSAGE_MAX_SIZE__At                                           +  \
            MESSAGE_MAX_SIZE__MapDisplyed                                  +  \
            MESSAGE_MAX_SIZE__CreateNewRoads                               +  \
            RTNET_GPSPATH_BUFFERSIZE__dynamic(_GPSPointsCount_)            +  \
            RTNET_NODEPATH_BUFFERSIZE__dynamic(_NodesPointsCount_)         +  \
            RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(_AllowNewRoadCount_)  +  \
            MESSAGE_MAX_SIZE__SeeMe                                        +  \
            MESSAGE_MAX_SIZE__SetMood                                      +  \
            MESSAGE_MAX_SIZE__Location                                     +  \
            MESSAGE_MAX_SIZE__UserPoints)

// Buffer sizes for GPS path string
// --------------------------------
//    Command name (GPSPath)                             ==  7 chars
//    Time offset:                  <32>,                == 33 chars
//    First row (row count)         <10>,                == 11 chars
//    A single row:                 <12>,<12>,<12>,<10>,      == 46 chars
//		Optional GPSDisconnect directive		<15>
//    First row (usr point type)    <10>,                == 11 chars
//    A single row (user points)    <10>,<10>,           == 22 chars
#define  RTNET_GPSPATH_BUFFERSIZE_command_name           ( 7)
#define  RTNET_GPSPATH_BUFFERSIZE_time_offset            (33)
#define  RTNET_GPSPATH_BUFFERSIZE_row_count              (11)
#define  RTNET_GPSPATH_BUFFERSIZE_point_type             (11)
#define  RTNET_GPSPATH_BUFFERSIZE_single_row             (46+4+15+22)
#define  RTNET_GPSPATH_BUFFERSIZE_rows                                  \
                           (RTNET_GPSPATH_BUFFERSIZE_single_row * RTTRK_GPSPATH_MAX_POINTS)
#define  RTNET_GPSPATH_BUFFERSIZE_rows__dynamic(_n_)                    \
                           (RTNET_GPSPATH_BUFFERSIZE_single_row * _n_)

#define  RTNET_GPSPATH_BUFFERSIZE                                       \
                           (RTNET_GPSPATH_BUFFERSIZE_command_name    +  \
                            RTNET_GPSPATH_BUFFERSIZE_time_offset     +  \
                            RTNET_GPSPATH_BUFFERSIZE_row_count       +  \
                            RTNET_GPSPATH_BUFFERSIZE_point_type      +  \
                            RTNET_GPSPATH_BUFFERSIZE_rows)

#define  RTNET_GPSPATH_BUFFERSIZE__dynamic(_n_)                         \
                           (RTNET_GPSPATH_BUFFERSIZE_command_name    +  \
                            RTNET_GPSPATH_BUFFERSIZE_time_offset     +  \
                            RTNET_GPSPATH_BUFFERSIZE_row_count       +  \
                            RTNET_GPSPATH_BUFFERSIZE_point_type      +  \
                            RTNET_GPSPATH_BUFFERSIZE_rows__dynamic(_n_))

#define   RTNET_CREATENEWROADS_BUFFERSIZE                                  \
                           (MESSAGE_MAX_SIZE__CreateNewRoads * RTTRK_CREATENEWROADS_MAX_TOGGLES)

#define   RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(_n_)                    \
                           (MESSAGE_MAX_SIZE__CreateNewRoads * (_n_))


// Buffer sizes for Node path string
// ---------------------------------
//    Command name (NodePath)                       ==  8 chars
//    Time offset:                  <32>,           == 33 chars
//    First row (row count)         <10>            == 11 chars
//    A single row:                 <10>,<10>,      == 22 chars
#define  RTNET_NODEPATH_BUFFERSIZE_command_name           ( 7)
#define  RTNET_NODEPATH_BUFFERSIZE_time_offset            (33)
#define  RTNET_NODEPATH_BUFFERSIZE_row_count              (11)
#define  RTNET_NODEPATH_BUFFERSIZE_single_row             (22)
#define  RTNET_NODEPATH_BUFFERSIZE_temp                   (33)
#define  RTNET_NODEPATH_BUFFERSIZE_rows                   \
                           (RTNET_NODEPATH_BUFFERSIZE_single_row * RTTRK_NODEPATH_MAX_POINTS)
#define  RTNET_NODEPATH_BUFFERSIZE_rows__dynamic(_n_)                    \
                           (RTNET_NODEPATH_BUFFERSIZE_single_row * _n_)

#define  RTNET_NODEPATH_BUFFERSIZE                                      \
                           (RTNET_NODEPATH_BUFFERSIZE_command_name    + \
                            RTNET_NODEPATH_BUFFERSIZE_time_offset     + \
                            RTNET_NODEPATH_BUFFERSIZE_row_count       + \
                            RTNET_NODEPATH_BUFFERSIZE_rows)

#define  RTNET_NODEPATH_BUFFERSIZE__dynamic(_n_)                        \
                           (RTNET_NODEPATH_BUFFERSIZE_command_name    + \
                            RTNET_NODEPATH_BUFFERSIZE_time_offset     + \
                            RTNET_NODEPATH_BUFFERSIZE_row_count       + \
                            RTNET_NODEPATH_BUFFERSIZE_rows__dynamic(_n_))
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Connection status information
typedef struct tagRTConnectionInfo
{
/* 1*/   char                 UserNm      [RT_USERNM_MAXSIZE         +1];
/* 2*/   char                 UserPW      [RT_USERPW_MAXSIZE         +1];
/* 3*/   char                 UserNk      [RT_USERNK_MAXSIZE         +1];
/* 4*/   char                 ServerCookie[RTNET_SERVERCOOKIE_MAXSIZE+1];
/* 5*/   BOOL                 bLoggedIn;
/* 6*/   int                  iServerID;
/* 7*/   RoadMapArea          LastMapPosSent;
/* 8*/   RTUsers              Users;
/* 9*/   ETransactionStatus   eTransactionStatus;
/*10*/   BOOL                 bStatusVerified;
/*11*/   roadmap_result       LastError;
/*12*/   int                  iMyRanking;
/*13*/   int                  iMyTotalPoints;
/*14*/   int                  iMyRating;
/*15*/   int                  iMyPreviousRanking;
/*16*/   int                  iMyAddon;
/*17*/   int                  iPointsTimeStamp;
/*18*/   int                  iExclusiveMoods;

}  RTConnectionInfo, *LPRTConnectionInfo;

void RTConnectionInfo_Init             ( LPRTConnectionInfo this, PFN_ONUSER pfnOnAddUser, PFN_ONUSER pfnOnMoveUser, PFN_ONUSER pfnOnRemoveUser);
void RTConnectionInfo_FullReset        ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetLogin       ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetTransaction ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetParser      ( LPRTConnectionInfo this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define   RTNET_ONRESPONSE_BEGIN(_method_,_expectedTag_)                \
   LPRTConnectionInfo   pCI = (LPRTConnectionInfo)pContext;             \
                                                                        \
   /* Verify status (OK), and verify the expected tag             */    \
   if( !pCI->bStatusVerified)                                           \
   {                                                                    \
      /* Verify we have any data:                                 */    \
      pNext = VerifyStatusAndTag(/* IN  */   pNext,                     \
                                 /* IN  */   pContext,                  \
                                 /* IN  */   _expectedTag_,             \
                                 /* OUT */   more_data_needed,          \
                                 /* OUT */   rc);                       \
                                                                        \
      if( !pNext || (*more_data_needed))                                \
      {                                                                 \
         if( !pNext)                                                    \
            roadmap_log( ROADMAP_ERROR, "RTNet::%s() - 'VerifyStatusAndTag()' had failed", _method_);   \
/*       Else                                                   */      \
/*          trans_in_progress  (Keep on reading data...)  */            \
                                                                        \
         return pNext;                                                  \
      }                                                                 \
                                                                        \
      pCI->bStatusVerified = TRUE;                                      \
   }

#define  RTNET_FORMAT_NETPACKET_9Login                ("Login,%d,%s,%s,%d,%s,%s,%s,%d,%s")
#define  RTNET_FORMAT_NETPACKET_3Register             ("Register,%d,%d,%s")
#define  RTNET_FORMAT_NETPACKET_3GuestLogin           ("GuestLogin,%d,%d,%s")
#define  RTNET_FORMAT_NETPACKET_4At                   ("At,%s,%d,%d,%s\n")
#define  RTNET_FORMAT_NETPACKET_2MapDisplayed         ("MapDisplayed,%s,%u\n")
#define  RTNET_FORMAT_NETPACKET_6ReportMarker         ("SubmitMarker,%s,%s,%s,%d,%s,%s\n")
#define  RTNET_FORMAT_NETPACKET_5ReportSegment        ("SubmitSegment,%s,%s,%s,%s,%d")
#define  RTNET_FORMAT_NETPACKET_5ReportSegmentAttr    (",%d,road_type,%s,street_name,%s,test2speech,%s,city_name,%s\n")
#define  RTNET_FORMAT_NETPACKET_1ReportSegmentNoAttr  (",%d\n")
#define  RTNET_FORMAT_NETPACKET_2CreateNewRoads       ("CreateNewRoads,%u,%s\n")
#define  RTNET_FORMAT_NETPACKET_4NavigateTo           ("NavigateTo,%s,,%s,%s,%s\n")
#define  RTNET_FORMAT_NETPACKET_5Auth		            ("Auth,%d,%s,%s,%d,%s\n")
#define  RTNET_FORMAT_NETPACKET_1SendSMS	            ("BridgeTo,DOWNLOADSMS,send_download,2,phone_number,%s\n")
#define  RTNET_FORMAT_NETPACKET_3TwitterConnect       ("BridgeTo,TWITTER,twitter_connect,6,twitter_username,%s,twitter_password,%s,connect,%s\n")
#define  RTNET_FORMAT_NETPACKET_4FoursquareConnect    ("BridgeTo,FOURSQUARE,signin,8,username,%s,password,%s,connect,%s,tweet_login,%s\n")
#define  RTNET_FORMAT_NETPACKET_2FoursquareSearch     ("BridgeTo,FOURSQUARE,getNearbyPlaces,4,lat,%s,lon,%s\n")
#define  RTNET_FORMAT_NETPACKET_2FoursquareCheckin    ("BridgeTo,FOURSQUARE,checkin,4,vid,%s,tweet_badge,%s\n")
#define	RTNET_FORMAT_NETPACKET_2SelectRoute				("SelectRoute,%d,%d\n")
#define  RTNET_FORMAT_NETPACKET_4CreateAccount        ("BridgeTo,CREATEACCOUNT,signup_mobile,6,user_name,%s,password,%s,email,%s,receive_mails,%s\n")
#define  RTNET_FORMAT_NETPACKET_4UpdateProfile        ("BridgeTo,UPDATEPROFILE,update_profile_mobile,8,user_name,%s,password,%s,email,%s,receive_mails,%s\n")

#define  RTNET_FORMAT_NETPACKET_4TripServerCreatePOI   		("BridgeTo,TRIPSERVER,CreatePOI,8,x,%d,y,%d,name,%s,override,%s\n")
#define  RTNET_FORMAT_NETPACKET_1TripServerDeletePOI   		("BridgeTo,TRIPSERVER,DeletePOI,2,name,%s\n")
#define  RTNET_FORMAT_NETPACKET_2TripServerFindTrip    		("BridgeTo,TRIPSERVER,FindTrip,4,x,%d,y,%d\n")
#define  RTNET_FORMAT_NETPACKET_1TripServerGetTripRoutes   	("BridgeTo,TRIPSERVER,GetTripRoutes,2,tripId,%d\n")
#define  RTNET_FORMAT_NETPACKET_1TripServerGetRouteSegments ("BridgeTo,TRIPSERVER,GetRouteSegments,2,tripId,%d\n")
#define  RTNET_FORMAT_NETPACKET_4ReportMapError             ("BridgeTo,UPDATEMAP,send_update_request_mobile,8,lon,%d,lat,%d,type,%s,description,%s\n")
#define  RTNET_FORMAT_NETPACKET_5GetGeoConfig               ("GetGeoServerConfig,%d,%s,%d,%s,%s")
#define  RTNET_FORMAT_NETPACKET_4ScoreboardGetPoints        ("BridgeTo,SCOREBOARD,getPoints,8,period,%s,geography,%s,from_rank,%s,count,%s\n")

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Realtime data parsers:
DECLARE_WEBSVC_PARSER(VerifyStatus)
DECLARE_WEBSVC_PARSER(AddUser)
DECLARE_WEBSVC_PARSER(AddAlert)
DECLARE_WEBSVC_PARSER(AddAlertComment)
DECLARE_WEBSVC_PARSER(RemoveAlert)
DECLARE_WEBSVC_PARSER(SystemMessage)
DECLARE_WEBSVC_PARSER(VersionUpgrade)
DECLARE_WEBSVC_PARSER(AddRoadInfo)
DECLARE_WEBSVC_PARSER(RoadInfoGeom)
DECLARE_WEBSVC_PARSER(RoadInfoSegments)
DECLARE_WEBSVC_PARSER(RmRoadInfo)
DECLARE_WEBSVC_PARSER(OnLoginResponse)
DECLARE_WEBSVC_PARSER(OnRegisterResponse)
DECLARE_WEBSVC_PARSER(OnLogOutResponse)
DECLARE_WEBSVC_PARSER(BridgeToRes)
DECLARE_WEBSVC_PARSER(ReportAlertRes)
DECLARE_WEBSVC_PARSER(PostAlertCommentRes)
DECLARE_WEBSVC_PARSER(MapUpdateTime)
DECLARE_WEBSVC_PARSER(GeoLocation)
DECLARE_WEBSVC_PARSER(onGeoConfigResponse)
DECLARE_WEBSVC_PARSER(UpdateUserPoints)
DECLARE_WEBSVC_PARSER(AddBonus)
DECLARE_WEBSVC_PARSER(RmBonus)
DECLARE_WEBSVC_PARSER(CollectBonusRes)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMENETDEFS_H__
