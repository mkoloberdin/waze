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


#ifndef   __FREEMAP_REALTIMENET_H__
#define   __FREEMAP_REALTIMENET_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "Realtime/RealtimeNetDefs.h"
#include "../editor/track/editor_track_report.h"
#include "../address_search/address_search.h"
#include "roadmap_gps.h"
#include "Realtime/Realtime.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Init();
void  RTNet_Term();

BOOL  RTNet_LoadParams ();
int   RTNet_Send       ( RoadMapSocket socket, const char* szText);
BOOL  RTNet_TransactionQueue_ProcessSingleItem( BOOL* pTransactionStarted);
void  RTNet_TransactionQueue_Clear();

ETransactionStatus   RTNet_GetTransactionState();
void                 RTNet_AbortTransaction( ETransactionStatus* new_state);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Login(      LPRTConnectionInfo   pCI,
                        const char*          szUserName,
                        const char*          szUserPW,
                        const char*          szUserNickname,
                        CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_GuestLogin( LPRTConnectionInfo   pCI,
                        CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_RandomUserRegister(   LPRTConnectionInfo   pCI,
                        CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_Logout(     LPRTConnectionInfo   pCI,
                        CB_OnWSTCompleted pfnOnCompleted);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTNet_Watchdog(LPRTConnectionInfo  pCI);

BOOL  RTNet_SetMyVisability(  LPRTConnectionInfo   pCI,
                              ERTVisabilityGroup   eVisability,
                              ERTVisabilityReport  eVisabilityReport,
                              CB_OnWSTCompleted    pfnOnCompleted,
                              BOOL showWazers,
                              BOOL showReports,
                              BOOL showTraffic,
                              BOOL allowPing,
                              int  eventsRadius,
                              char*                packet_only);

BOOL RTNet_SetMood		   (LPRTConnectionInfo   pCI,
                   			int 					    iMood,
                   			CB_OnWSTCompleted     pfnOnCompleted,
                   			char*                 packet_only);

BOOL RTNet_Location        (LPRTConnectionInfo     pCI,
                           const RoadMapPosition*  location,
                           CB_OnWSTCompleted       pfnOnCompleted,
                           char*                   packet_only);

BOOL RTNet_UserPoints      (LPRTConnectionInfo   pCI,
                           int                   iUserPoints,
                           CB_OnWSTCompleted     pfnOnCompleted,
                           char*                 packet_only);

BOOL  RTNet_At            (LPRTConnectionInfo   pCI,
                           const
                           RoadMapGpsPosition*  pGPSPosition,
                           int                  from_node,
                           int                  to_node,
                           BOOL                 refreshUsers,
                           CB_OnWSTCompleted    pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_NavigateTo(    LPRTConnectionInfo   pCI,
                           const
                           RoadMapPosition*     cordinates,
                           address_info_ptr     ai,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_MapDisplyed ( LPRTConnectionInfo   pCI,
                           const RoadMapArea*   pRoadMapArea,
                           unsigned int         scale,
                           RoadMapPosition      position[5],
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_CreateNewRoads (
                           LPRTConnectionInfo   pCI,
                           int                  nToggles,
                           const time_t*        toggle_time,
                           BOOL                 bStatusFirst,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_StartFollowUsers(
                           LPRTConnectionInfo   pCI,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_StopFollowUsers(LPRTConnectionInfo pCI,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_GPSPath(      LPRTConnectionInfo   pCI,
                           time_t               period_begin,
                           LPGPSPointInTime     points,
                           int                  count,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_NodePath(     LPRTConnectionInfo   pCI,
                           time_t               period_begin,
                           LPNodeInTime         nodes,
                           int                  count,
                           LPUserPointsVer      user_points,
                           int                  num_user_points,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_ExternalPoiDisplayed(     LPRTConnectionInfo   pCI,
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_ReportAlert(   LPRTConnectionInfo   pCI,
                           int                  iType,
                           int                  iSubType,
                           const char*          szDescription,
                           int                  iDirection,
                           const char*          szImageId,
                           const char*          szVoiceId,
                           BOOL						bForwardToTwitter,
                           BOOL                 bForwardToFacebook,
                           const char*          szGroup,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_PinqWazer(     LPRTConnectionInfo   pCI,
                           const RoadMapGpsPosition  *pPosition,
                           int                from_node,
                           int                to_node,
                           int                iUserId,
                           int                iAlertType,
                           const char*        szDescription,
                           const char*        szImageId,
                           const char*        szVoiceId,
                           BOOL               bForwardToTwitter,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_ReportAlertAtPosition(
                        LPRTConnectionInfo         pCI,
                        int                        iType,
                        int                        iSubType,
                        const char*                szDescription,
                        int                        iDirection,
                        const char*                szImageId,
                        const char*                szVoiceId,
                        BOOL								bForwardToTwitter,
                        BOOL                       bForwardToFacebook,
                        const RoadMapGpsPosition*  MyLocation,
                        int 				            from_node,
                        int 				            to_node,
                        const char*                szGroup,
                        CB_OnWSTCompleted       	pfnOnCompleted);

BOOL RTNet_SendSMS (
                LPRTConnectionInfo   pCI,
	            const char*          szPhoneNumber,
	            CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_PostAlertComment(
                        LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        const char*          szDescription,
                        BOOL                 bForwardToTwitter,
                        BOOL                 bForwardToFacebook,
                        CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_RemoveAlert(LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_ReportMarker(  LPRTConnectionInfo   pCI,
                           const char*          szType,
                           int                  iLongitude,
                           int                  iLatitude,
                           int                  iAzimuth,
                           const char*          szDescription,
                           int                  nParams,
                           const char*          szParamKey[],
                           const char*          szParamVal[],
                           CB_OnWSTCompleted pfnOnCompleted,
                           char*                packet_only);

BOOL RTNet_TrafficAlertFeedback( LPRTConnectionInfo   pCI,
                           int                  iTrafficInfoId,
                           int                  iValidity,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_CreateAccount (LPRTConnectionInfo   pCI,
                           const char*          userName,
                           const char*          passWord,
                           const char*          email,
                           BOOL                 send_updates,
                           int                  referrer,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_UpdateProfile (LPRTConnectionInfo   pCI,
                           const char*          userName,
                           const char*          passWord,
                           const char*          email,
                           BOOL                 send_updates,
                           int                  referrer,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_ReportOneSegment_Encode (char *packet, int iSegment);

int   RTNet_ReportOneSegment_MaxLength (int iSegment);

BOOL  RTNet_SendTrafficInfo ( LPRTConnectionInfo   pCI,
                             int mode,
                             CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_GeneralPacket( LPRTConnectionInfo   pCI,
                           const char*          Packet,
                           CB_OnWSTCompleted pfnOnCompleted);

void	RTNet_Auth_BuildCommand (	char*				Command,
											int				ServerId,
											const char*		ServerCookie,
											const char*		UserName,
											int				DeviceId,
											const char*		Version,
                                 int            Protocol);

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
                        CB_OnWSTCompleted		pfnOnCompleted);

BOOL	RTNet_SelectRoute	(LPRTConnectionInfo   	pCI,
								 int							iRoute,
								 int							iAltId,
								 CB_OnWSTCompleted		pfnOnCompleted);

BOOL  RTNet_TwitterConnect (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   BOOL                 bForwardToTwitter,
                   int                  iDeviceId,
                   CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_FoursquareConnect (
                   LPRTConnectionInfo   pCI,
                   const char*          userName,
                   const char*          passWord,
                   BOOL                 bTweetLogin,
                   CB_OnWSTCompleted    pfnOnCompleted);

BOOL  RTNet_FoursquareSearch (
                   LPRTConnectionInfo   pCI,
                   RoadMapPosition*     coordinates,
                   CB_OnWSTCompleted    pfnOnCompleted);

BOOL  RTNet_FoursquareCheckin (
                   LPRTConnectionInfo   pCI,
                   const char*          vid,
                   BOOL                 bTweetBadge,
                   CB_OnWSTCompleted    pfnOnCompleted);

BOOL  RTNet_Scoreboard_getPoints (
                                  LPRTConnectionInfo   pCI,
                                  const char*          period,
                                  const char*          geography,
                                  int                  fromRank,
                                  int                  count,
                                  CB_OnWSTCompleted    pfnOnCompleted);

BOOL  RTNet_TripServer_CreatePOI  (
                   LPRTConnectionInfo   pCI,
                   const char*          name,
                   RoadMapPosition*     coordinates,
                   BOOL					    overide,
                   int                  id,
                   CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_TripServer_DeletePOI  (
                   LPRTConnectionInfo   pCI,
                   const char*          name,
                   CB_OnWSTCompleted pfnOnCompleted);

BOOL RTNet_TripServer_FindTrip  (
						LPRTConnectionInfo   pCI,
						RoadMapPosition*     coordinates,
						CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_TripServer_GetPOIs  (LPRTConnectionInfo   pCI,
                                 CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_TripServer_GetNumPOIs  (LPRTConnectionInfo   pCI,
                                 CB_OnWSTCompleted pfnOnCompleted);

BOOL RTNet_ReportMapProblem(
                  LPRTConnectionInfo   pCI,
                  const char*          szType,
                  const char*          szDescription,
                  const RoadMapGpsPosition   *MyLocation,
                  ESendMapProblemResult * SendMapProblemResult,
                  CB_OnWSTCompleted pfnOnCompleted,
                   char* packet_only);

BOOL RTNet_GetGeoConfig(
                  LPRTConnectionInfo         pCI,
                  wst_handle                 websvc,
                  const RoadMapPosition      *Location,
                  const char                 *name,
                  CB_OnWSTCompleted          pfnOnCompleted);

BOOL RTNet_CollectBonus(
                  LPRTConnectionInfo   pCI,
                  int                  iId,
                  int                  iToken,
                  BOOL                 bForwardToTwitter,
                  BOOL                 bForwardToFacebook,
                  CB_OnWSTCompleted    pfnOnCompleted);

BOOL RTNet_CollectCustomBonus(LPRTConnectionInfo   pCI,
                  int                  iId,
                  BOOL                 bForwardToTwitter,
                  BOOL                 bForwardToFacebook,
                  CB_OnWSTCompleted    pfnOnCompleted);


BOOL RTNet_ReportAbuse (
                  LPRTConnectionInfo   pCI,
                  int                  iAlertID,
                  int                  iCommentID,
                  CB_OnWSTCompleted    pfnOnCompleted);

BOOL RTNet_ThumbsUp (LPRTConnectionInfo   pCI,
                     int                  iAlertID,
                     CB_OnWSTCompleted    pfnOnCompleted);

BOOL RTNet_SetPushNotifications( LPRTConnectionInfo  pCI,
                                const char*          szToken,
                                BOOL                 bScore,
                                BOOL                 bUpdates,
                                BOOL                 bFriends,
                                CB_OnWSTCompleted    pfnOnCompleted,
                                char*                packet_only);

BOOL RTNet_KeepAlive( LPRTConnectionInfo   pCI,
                      CB_OnWSTCompleted pfnOnCompleted);


BOOL  RTNet_FacebookPermissions (LPRTConnectionInfo   pCI,
                                 int                  iShowFacebookName,
                                 int                  iShowFacebookPicture,
                                 int                  iShowFacebookProfile,
                                 int                  iShowTwitterProfile,
                                 CB_OnWSTCompleted    pfnOnCompleted);


BOOL  RTNet_ExternalPoiNotifyOnPopUp (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  int                 iPromotionID,
                                  CB_OnWSTCompleted   pfnOnCompleted);

BOOL  RTNet_ExternalPoiNotifyOnPromotionPopUp (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  int                 iPromotionID,
                                  CB_OnWSTCompleted   pfnOnCompleted);

BOOL  RTNet_ExternalPoiNotifyOnInfoPressed (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  int                 iPromotionID,
                                  CB_OnWSTCompleted   pfnOnCompleted);

BOOL  RTNet_ExternalPoiNotifyOnNavigate (LPRTConnectionInfo   pCI,
                                  int                 iID,
                                  CB_OnWSTCompleted   pfnOnCompleted);

BOOL  RTNet_NotifySplashUpdateTime (LPRTConnectionInfo pCI,
                                  const char *         update_time,
                                  CB_OnWSTCompleted    pfnOnCompleted);

BOOL RTNet_ReportTraffic(  LPRTConnectionInfo   pCI,
                           int traffic_value,
                           CB_OnWSTCompleted pfnOnCompleted);

BOOL  RTNet_Stats(  LPRTConnectionInfo   pCI,
                    const char*          szEventName,
                    int                  nParams,
                    const char*          szParamKey[],
                    const char*          szParamVal[],
                    CB_OnWSTCompleted    pfnOnCompleted,
                    char*                packet_only);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMENET_H__
