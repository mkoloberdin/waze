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

#ifndef   __FREEMAP_REALTIME_H__
#define   __FREEMAP_REALTIME_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "../Realtime/LMap_Base.h"
#include "../roadmap_gps.h"
#include "../address_search/address_search.h"
#include "../websvc_trans/websvc_trans.h"
#include "../roadmap.h"
#include "../roadmap_login.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*PFN_LOGINTESTRES)( BOOL bDetailsVerified, roadmap_result rc);

// Module initialization/termination   - Called once, when the process starts/terminates
BOOL  Realtime_Initialize();
void  Realtime_Terminate();

//   Module start/stop                 - Can be called many times during the process lifetime
BOOL  Realtime_Start();
void  Realtime_Stop( BOOL bEnableLogout);

void  Realtime_AbortTransaction( RoadMapCallback pfnOnTransAborted);
void  Realtime_NotifyOnIdle    ( RoadMapCallback pfnOnTransIdle);
//   Register for notification. Callback must call next in chain (in return value).
RoadMapCallback  Realtime_NotifyOnLogin   ( RoadMapCallback pfnOnLogin);

BOOL  Realtime_ServiceIsActive();
BOOL  Realtime_IsInTransaction();

BOOL  Realtime_IsEnabled();

BOOL  Realtime_SendCurrentViewDimentions();
BOOL Realtime_VerifyLoginDetails( PFN_LOGINTESTRES pfn );

BOOL  Realtime_ReportOnNavigation( const RoadMapPosition* cordinates, address_info_ptr ai);
BOOL  Realtime_RequestRoute(int						iRoute,
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
									BOOL						bReRoute);
BOOL	Realtime_SelectRoute	(int                 iRoute,
								 	 int                 AltId);

BOOL  Realtime_CollectBonus(int                 iId,
                            int                 iToken,
                            BOOL                bForwardToTwitter,
                            BOOL                bForwardToFacebook);

BOOL Realtime_ReportAbuse (int                  iAlertID,
                           int                  iCommentID);

BOOL  Realtime_Report_Alert(int iAlertType, const char * szDescription, int iDirection, const char* szImageId, BOOL bForwardToTwitter, BOOL bForwardToFacebook );
BOOL  Realtime_Alert_ReportAtLocation(int iAlertType, const char * szDescription, int iDirection, RoadMapGpsPosition   MyLocation);
BOOL  Realtime_Remove_Alert(int iAlertId);
BOOL  Realtime_Post_Alert_Comment(int iAlertId, const char * szCommentText, BOOL bForwardToTwitter, BOOL bForwardToFacebook);
BOOL  Realtime_StartSendingTrafficInfo(void);
BOOL  Realtime_SendTrafficInfo(int mode);
BOOL  Realtime_SendSMS(const char * szPhoneNumber);
BOOL  Realtime_TwitterConnect(const char * userName, const char *passWord);
BOOL  Realtime_FoursquareConnect(const char * userName, const char *passWord, BOOL bTweetLogin);
BOOL  Realtime_FoursquareSearch(RoadMapPosition* coordinates);
BOOL  Realtime_FoursquareCheckin(const char* vid, BOOL bTweetBadge);
BOOL Realtime_Scoreboard_getPoints(const char *period, const char *geography, int fromRank, int count);
BOOL  Realtime_Editor_ExportMarkers(PFN_LOGINTESTRES editor_cb);
BOOL  Realtime_Editor_ExportSegments(PFN_LOGINTESTRES editor_cb);

BOOL Realtime_TripServer_CreatePOI  (const char* name, RoadMapPosition* coordinates, BOOL overide);
BOOL Realtime_TripServer_DeletePOI(const char * name);
BOOL Realtime_TripServer_FindTrip (RoadMapPosition*     coordinates);

BOOL Realtime_ReportMapProblem(const char*  szType, const char* szDescription, const RoadMapGpsPosition *MyLocation);
BOOL Realtime_TrafficAlertFeedback(int iTrafficInfoId, int iValidity);
BOOL Realtime_CreateAccount(const char* userName, const char* passWord,const char* email,BOOL send_updates);
BOOL Realtime_UpdateProfile(const char* userName, const char* passWord,const char* email,BOOL send_updates);
BOOL Realtime_is_random_user();
void Realtime_set_random_user(BOOL is_random);
void OnSettingsChanged_VisabilityGroup(void);
BOOL Realtime_RandomUserRegister();
void OnMoodChanged(void);
void Realtime_SetLoginUsername( const char* username );
void Realtime_SetLoginPassword( const char* pwd );
void Realtime_SetLoginNickname( const char* nickname );
void Realtime_LoginDetailsReset( void );
void Realtime_LoginDetailsInit( void );
void Realtime_SaveLoginInfo( void );
BOOL Realtime_IsLoggedIn( void );
void Realtime_Relogin(void);
int  Realtime_AddonState(void);
void Realtime_ShowWazerNearby (void);
int  Realtime_WazerNearbyState (void);

void	RealTime_Auth (void);
void RecommentToFriend(void);

int         RealTimeLoginState(void);
const char* RealTime_GetUserName();
const char* Realtime_GetNickName();

int RealTime_GetMyTotalPoints();
int RealTime_GetMyRanking();

void RealTime_SetMapDisplayed(BOOL should_send);

BOOL Realtime_GetGeoConfig(const RoadMapPosition *pGPSPosition, const char *name, wst_handle websvc);

char* Realtime_GetServerCookie(void);
BOOL Realtime_PinqWazer(const RoadMapGpsPosition *pPosition, int from_node, int to_node, int iUserId, int iAlertType, const char * szDescription, const char* szImageId, BOOL bForwardToTwitter );
BOOL Realtime_AllowPing(void);
void Realtime_Set_AllowPing (BOOL AllowPing);
int Realtime_GetServerId(void);
void RT_SetWebServiceAddress(const char* address);
void Realtime_CheckDumpOfflineAfterCrash(void);
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum tagEnumSendMapProblemResult
{
	SendMapProblemValidityFailure=0,
	SendMapProblemValidityOK,
}	ESendMapProblemResult;

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIME_H__

