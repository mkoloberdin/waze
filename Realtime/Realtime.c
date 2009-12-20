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

#include <stdlib.h>
#include <string.h>

#include "../roadmap_main.h"
#include "../roadmap_config.h"
#include "../roadmap_navigate.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_trip.h"
#include "../roadmap_line.h"
#include "../roadmap_point.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_line_route.h"
#include "../roadmap_net_mon.h"
#include "../roadmap_screen.h"
#include "../roadmap_internet.h"
#include "../roadmap_square.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../roadmap_mood.h"
#include "../roadmap_device_events.h"
#include "../roadmap_state.h"
#include "../roadmap_twitter.h"
#include "../roadmap_foursquare.h"
#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_keyboard_dialog.h"
#include "../ssd/ssd_confirm_dialog.h"
#include "../editor/editor_points.h"
#include "../editor/db/editor_marker.h"
#include "../editor/db/editor_line.h"
#include "../editor/export/editor_report.h"
#include "../editor/export/editor_sync.h"
#include "../ssd/ssd_progress_msg_dialog.h"
#include "roadmap_message.h"
#include "RealtimeNet.h"
#include "RealtimeAlerts.h"
#include "RealtimeTrafficInfo.h"
#include "roadmap_warning.h"
#include "RealtimePrivacy.h"
#include "RealtimeOffline.h"
#include "RealtimeBonus.h"
#include "RealtimeSystemMessage.h"
#include "roadmap_geo_config.h"
#include "../roadmap_download_settings.h"
#include "../websvc_trans/string_parser.h"
#include "Realtime.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static   BOOL                 gs_bInitialized               = FALSE;
static   BOOL                 gs_bRunning                   = FALSE;
static   BOOL                 gs_bSaveLoginInfoOnSuccess    = FALSE;
static   BOOL                 gs_bShouldSendMyVisability    = TRUE;
static   BOOL                 gs_bShouldSendMapDisplayed    = TRUE;
static   BOOL                 gs_bShouldSendSetMood         = TRUE;
static   BOOL                 gs_bShouldSendUserPoints      = TRUE;
static   BOOL                 gs_bHadAtleastOneGoodSession  = FALSE;
static   BOOL                 gs_bWasStoppedAutoamatically  = FALSE;
static   BOOL                 gs_bQuiteErrorMode            = FALSE;
static   RTConnectionInfo     gs_CI;
         VersionUpgradeInfo   gs_VU;
static   StatusStatistics     gs_ST;
static   EConnectionStatus    gs_eConnectionStatus          = CS_Unknown;
static   PFN_LOGINTESTRES     gs_pfnOnLoginTestResult       = NULL;
static   RoadMapCallback      gs_pfnOnSystemIsIdle          = NULL;
static   RoadMapCallback      gs_pfnOnUserLoggedIn          = NULL;
static   PFN_LOGINTESTRES     gs_pfnOnExportMarkersResult   = NULL;
static   PFN_LOGINTESTRES     gs_pfnOnExportSegmentsResult  = NULL;
static   CB_OnWSTCompleted    gs_pfnOnLoginAfterRegister    = NULL;
static   BOOL                 gs_bWritingOffline            = FALSE;
static   RTPathInfo*          gs_pPI;
static	int					  	gs_iCycleTimeSeconds			= 0;
static	int					  	gs_iCycleRoundoffSeconds	= 0;
static	int					  	gs_iMaxCommCheckSeconds		= 0;
static	int						gs_iSummaryCycleSeconds		= 0;
static   LoginDetails         gs_LoginDetails;
static   BOOL                 gs_bRTWarningInit	= TRUE;	/* Initial state before the actual */
																	/* network information is available */
static BOOL 				      gs_bFirstLoginFailure = TRUE;
static int                    gs_WazerNearbyID           = -1;
static int                    gs_iWazerNearbyState       = 0;
static time_t                 gs_WazerNearbyLastShown    = 0;
static RoadMapScreenSubscriber realtime_prev_after_refresh = NULL;
#define WAZER_NEARBY_TIME        15
#define WAZER_NEARBY_SLEEP_TIME  300

static void Realtime_SessionDetailsInit( void );
static void Realtime_SessionDetailsSave( void );
static void Realtime_SessionDetailsClear( void );

static void Realtime_FullReset( BOOL bRedraw)
{
   RTConnectionInfo_FullReset    ( &gs_CI);
   Realtime_SessionDetailsInit();
   VersionUpgradeInfo_Init       ( &gs_VU);
   StatusStatistics_Reset        ( &gs_ST);
   RTNet_TransactionQueue_Clear  ();
   //RTSystemMessageQueue_Empty    ();

   gs_pfnOnLoginTestResult       = NULL;
   gs_bHadAtleastOneGoodSession  = FALSE;
   gs_bQuiteErrorMode            = FALSE;

   if(bRedraw)
      roadmap_screen_redraw();
}

static void Realtime_ResetTransactionState( BOOL bRedraw)
{
   RTConnectionInfo_ResetTransaction( &gs_CI);
//   VersionUpgradeInfo_Init          ( &gs_VU); this was already initialized in the realtime_init func
//   RTSystemMessageQueue_Empty       ();

   if(bRedraw)
      roadmap_screen_redraw();
}

static void Realtime_ResetLoginState( BOOL bRedraw)
{
   RTConnectionInfo_ResetLogin   ( &gs_CI);
   VersionUpgradeInfo_Init       ( &gs_VU);
   StatusStatistics_Reset        ( &gs_ST);
   RTNet_TransactionQueue_Clear  ();
   RTSystemMessageQueue_Empty    ();
   Realtime_SessionDetailsClear();

   if( bRedraw )
      roadmap_screen_redraw();
}

static void RealTime_WarningInit( void )
{
	roadmap_main_remove_periodic( RealTime_WarningInit );
	gs_bRTWarningInit = FALSE;
}

static BOOL RealTime_Warning( char* dest_string )
{
	BOOL res = FALSE;

	if ( !RealTimeLoginState() )
	{
		if ( gs_bRTWarningInit )
		{
			strncpy( dest_string, roadmap_lang_get("Searching network . . .    "), ROADMAP_WARNING_MAX_LEN );
			res = TRUE;
		}
		else
		{
			strncpy( dest_string, roadmap_lang_get("No network, unable to display live data"), ROADMAP_WARNING_MAX_LEN );
			res = TRUE;
		}
	}

	return res;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void     OnTimer_Realtime                 (void);
void     OnSettingsChanged_EnableDisable  (void);
void     OnSettingsChanged_VisabilityGroup(void);

void     OnAddUser      (LPRTUserLocation pUI);
void     OnMoveUser     (LPRTUserLocation pUI);
void     OnRemoveUser   (LPRTUserLocation pUI);
void     OnMapMoved     (void);

void     OnDeviceEvent  ( device_event event, void* context);
static  BOOL     TestLoginDetails();

static BOOL     Login( CB_OnWSTCompleted callback /* Callback on login transaction */,
					BOOL bShowRegistration /* Show registration dialog if no user&pwd */ );

static void OnTransactionCompleted_TestLoginDetails_Login( void* ctx, roadmap_result rc );
static BOOL TestLoginMain();
static void RealTime_WarningInit( void );
void     Realtime_DumpOffline (void);

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////

extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;

//   Enable / Disable service
static RoadMapConfigDescriptor RT_CFG_PRM_STATUS_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_STATUS_Name);

//   Enable / Disable auto registration
static RoadMapConfigDescriptor RT_CFG_PRM_AUTOREG_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_AUTOREG_Name);

//   Refresh rate
static RoadMapConfigDescriptor RT_CFG_PRM_REFRAT_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_REFRAT_Name);

//   Hi-Res Refresh rate
static RoadMapConfigDescriptor RT_CFG_PRM_HIRESREFRAT_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_HIRESREFRAT_Name);

//   Summary Refresh rate
static RoadMapConfigDescriptor RT_CFG_PRM_SUMMARY_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_SUMMARY_Name);

//   Commcheck time
static RoadMapConfigDescriptor RT_CFG_PRM_COMMCHECK_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_COMMCHECK_Name);

//   Web-service address
static RoadMapConfigDescriptor RT_CFG_PRM_WEBSRV_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_WEBSRV_Name);

//   Web-service address
static RoadMapConfigDescriptor RT_CFG_PRM_RANDOM_USER_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_RANDOM_USER_Name);

/*   My visability status
static RoadMapConfigDescriptor RT_CFG_PRM_MYVSBL_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_MYVSBL_Name);

//   My ROI - What people do I want to see
static RoadMapConfigDescriptor RT_CFG_PRM_INTRST_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_INTRST_Name);*/

//   Visability group:
RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var =
                                ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_VISGRP_Name);

//   Visability Report:
RoadMapConfigDescriptor RT_CFG_PRM_VISREP_Var =
                                ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_VISREP_Name);

//   Server ID
static RoadMapConfigDescriptor RT_CFG_PRM_SERVER_ID_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_SERVER_ID_Name);

//   Server Cookie
static RoadMapConfigDescriptor RT_CFG_PRM_SERVER_COOKIE_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_SERVER_COOKIE_Name);

BOOL GetCurrentDirectionPoints(  RoadMapGpsPosition*  GPS_position,
                                 int*                 from_node,
                                 int*                 to_node,
                                 int*                 direction);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsImportantError( roadmap_result res)
{ return (res != succeeded) && (res != err_rt_no_data_to_send);}

BOOL IsTransactionError( roadmap_result res)
{ return is_network_error(res) || is_general_error(res);}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Expose configuration property - web-service address - to external modules (C files)
const char* RT_GetWebServiceAddress()
{ return roadmap_config_get( &RT_CFG_PRM_WEBSRV_Var);}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static int Realtime_DecodeRefreshRateMilliseconds( RoadMapConfigDescriptor *pDesc,
																	float							fDefVal,
												  				 	float							fMinVal,
												  				 	float							fMaxVal)
{
   const char* szRefreshRate  	 = NULL;
   float       fRefreshRate   	 = 0.F;

   //   Calculate refresh rate:
   szRefreshRate = roadmap_config_get( pDesc);
   if( szRefreshRate && (*szRefreshRate))
      fRefreshRate = (float) atof(szRefreshRate);

   //   Fix refresh rate:
   if( !fRefreshRate)
      fRefreshRate = fDefVal;
   else
   {
	  BOOL bLowLimExceed = ( fRefreshRate < fMinVal );
	  BOOL bUpLimExceed = ( fRefreshRate > fMaxVal );
      if ( bLowLimExceed )
         fRefreshRate = fMinVal;
 	  if ( bUpLimExceed )
		 fRefreshRate = fMaxVal;
   }

   //   Refresh-rate in milliseconds
   return (int)(fRefreshRate * 60 * 1000.F);
}


int Realtime_GetRefreshRateinMilliseconds()
{
	int iRefreshRate;

   gs_iCycleTimeSeconds = Realtime_DecodeRefreshRateMilliseconds ( &RT_CFG_PRM_REFRAT_Var,
   																					 RT_CFG_PRM_REFRAT_iDef,
   																					 RT_CFG_PRM_REFRAT_iMin,
   																					 RT_CFG_PRM_REFRAT_iMax) / 1000;

   roadmap_log( ROADMAP_DEBUG, "Current network cycle time: %d seconds.", gs_iCycleTimeSeconds );

   iRefreshRate = Realtime_DecodeRefreshRateMilliseconds ( &RT_CFG_PRM_HIRESREFRAT_Var,
   																			RT_CFG_PRM_HIRESREFRAT_iDef,
   																			RT_CFG_PRM_HIRESREFRAT_iMin,
   																			gs_iCycleTimeSeconds / 60.0);

   roadmap_log( ROADMAP_DEBUG, "Current network refresh rate: %d seconds.", (int)(iRefreshRate/1000) );
   gs_iCycleRoundoffSeconds = (int)(iRefreshRate/2000);

   gs_iSummaryCycleSeconds = Realtime_DecodeRefreshRateMilliseconds ( &RT_CFG_PRM_SUMMARY_Var,
   																					 RT_CFG_PRM_SUMMARY_iDef,
   																					 iRefreshRate / 60000.0,
   																					 gs_iCycleTimeSeconds / 60.0) / 1000;

   roadmap_log( ROADMAP_DEBUG, "Current summary cycle time: %d seconds.", gs_iSummaryCycleSeconds );

   gs_iMaxCommCheckSeconds = Realtime_DecodeRefreshRateMilliseconds ( &RT_CFG_PRM_COMMCHECK_Var,
   																			RT_CFG_PRM_COMMCHECK_iDef,
   																			iRefreshRate / 60000.0,
   																			RT_CFG_PRM_COMMCHECK_iMax) / 1000;

   roadmap_log( ROADMAP_DEBUG, "Current comm retry time: %d seconds.", gs_iMaxCommCheckSeconds );

   return iRefreshRate;
}

//   Enabled / Disabled:
BOOL GetEnableDisableState()
{
   if( 0 == strcmp( roadmap_config_get( &RT_CFG_PRM_STATUS_Var), RT_CFG_PRM_STATUS_Enabled))
      return TRUE;
   return FALSE;
}

//   Get 'Auto Registration' state:
BOOL IsAutoRegEnabled()
{
   const char* val = roadmap_config_get( &RT_CFG_PRM_AUTOREG_Var);
   if( 0 == strcmp( val, RT_CFG_PRM_AUTOREG_Enabled))
      return TRUE;
   return FALSE;
}

BOOL Realtime_IsEnabled()
{ return GetEnableDisableState();}

//   Module initialization/termination - Called once, when the process starts/terminates
BOOL Realtime_Initialize()
{
   if( gs_bInitialized)
      return TRUE;

   // Nickname:
   roadmap_config_declare( RT_USER_TYPE,
                           &RT_CFG_PRM_NKNM_Var,
                           RT_CFG_PRM_NKNM_Default,
                           NULL);


   // Password:
   roadmap_config_declare_password (
                           RT_USER_TYPE,
                           &RT_CFG_PRM_PASSWORD_Var,
                           RT_CFG_PRM_PASSWORD_Default);

   // Name:
   roadmap_config_declare( RT_USER_TYPE,
                           &RT_CFG_PRM_NAME_Var,
                           RT_CFG_PRM_NAME_Default,
                           NULL);

   //   Enable / Disable service
   roadmap_config_declare_enumeration( RT_CFG_TYPE,
                                       &RT_CFG_PRM_STATUS_Var,
                                       OnSettingsChanged_EnableDisable,
                                       RT_CFG_PRM_STATUS_Enabled,
                                       RT_CFG_PRM_STATUS_Disabled,
                                       NULL);

   //   Enable / Disable auto registration
   roadmap_config_declare_enumeration( RT_CFG_TYPE,
                                       &RT_CFG_PRM_AUTOREG_Var,
                                       NULL,
                                       RT_CFG_PRM_AUTOREG_Disabled,
                                       RT_CFG_PRM_AUTOREG_Enabled,
                                       NULL);

   //   Refresh rate
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_REFRAT_Var,
                           RT_CFG_PRM_REFRAT_Default,
                           NULL);

   //   Hi-Res Refresh rate
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_HIRESREFRAT_Var,
                           RT_CFG_PRM_HIRESREFRAT_Default,
                           NULL);

   //   Summary Refresh rate
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_SUMMARY_Var,
                           RT_CFG_PRM_SUMMARY_Default,
                           NULL);

   //   Comm-check max time
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_COMMCHECK_Var,
                           RT_CFG_PRM_COMMCHECK_Default,
                           NULL);

   //   Web-service address
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_WEBSRV_Var,
                           RT_CFG_PRM_WEBSRV_Default,
                           NULL);

   // Visability group:
   roadmap_config_declare_enumeration( RT_USER_TYPE,
                                       &RT_CFG_PRM_VISGRP_Var,
                                       OnSettingsChanged_VisabilityGroup,
                                       RT_CFG_PRM_VISGRP_Anonymous,
                                       RT_CFG_PRM_VISGRP_Invisible,
                                       RT_CFG_PRM_VISGRP_Nickname,
                                       NULL);


   // Visability report:
   roadmap_config_declare_enumeration( RT_USER_TYPE,
                                       &RT_CFG_PRM_VISREP_Var,
                                       OnSettingsChanged_VisabilityGroup,
                                       RT_CFG_PRM_VISREP_Nickname,
                                       RT_CFG_PRM_VISREP_Anonymous,
                                       NULL);

   //Random User
   roadmap_config_declare( RT_USER_TYPE,
                           &RT_CFG_PRM_RANDOM_USER_Var,
                           RT_CFG_PRM_RANDOM_USERT_Default,
                           NULL);

   // Session server id:
   roadmap_config_declare( RT_SESSION_TYPE,
                           &RT_CFG_PRM_SERVER_ID_Var,
                           RT_CFG_PRM_SERVER_ID_Default,
                           NULL);
                           
   // Session server coockie:
   roadmap_config_declare( RT_SESSION_TYPE,
                           &RT_CFG_PRM_SERVER_COOKIE_Var,
                           RT_CFG_PRM_SERVER_COOKIE_Default,
                           NULL);


   RealtimePrivacyInit();
   RTConnectionInfo_Init   ( &gs_CI, OnAddUser, OnMoveUser, OnRemoveUser);
   VersionUpgradeInfo_Init ( &gs_VU);
   StatusStatistics_Init   ( &gs_ST);
   RTAlerts_Init();
   RealtimeBonus_Init();
   RTNet_Init();
   Realtime_LoginDetailsInit();
   Realtime_SessionDetailsInit();
   RTSystemMessagesInit();

   // This will disable the auto-detection of large time gap between good sessions:
   gs_ST.timeLastGoodSession = time(NULL);

   roadmap_device_events_register( OnDeviceEvent, NULL);

   gs_bInitialized = TRUE;

   // dump data from previous run to offline file
   Realtime_DumpOffline ();

   //   If needed - start service:
   OnSettingsChanged_EnableDisable();

   roadmap_main_set_periodic( RT_WARNING_INIT_TO, RealTime_WarningInit );
   roadmap_warning_register( (RoadMapWarningFn) RealTime_Warning, "RealTime" );

   return TRUE;
}

BOOL Realtime_is_random_user(){
   int is_random = roadmap_config_get_integer(&RT_CFG_PRM_RANDOM_USER_Var);
   return (BOOL)is_random;
}

void Realtime_set_random_user(BOOL is_random){
   roadmap_config_set_integer(&RT_CFG_PRM_RANDOM_USER_Var, (int)is_random);
   roadmap_config_save(0);
}

void Realtime_Terminate()
{
   if( !gs_bInitialized)
      return;

   roadmap_device_events_unregister( OnDeviceEvent);


   Realtime_Stop( FALSE /* Enable Logout? */);
   RTNet_Term();
   RealtimeBonus_Term();
   RTAlerts_Term();
   RTTrafficInfo_Term();

   gs_bInitialized = FALSE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Module start/stop - Can be called many times during the process lifetime
BOOL Realtime_Start()
{
   if( !gs_bInitialized)
      return FALSE;

   if( gs_bRunning)
      return FALSE;

   //   Initialize all:
   Realtime_FullReset( FALSE /* Redraw */);
   RTAlerts_Term();  // 'Term' is not called from 'Stop', thus here we do ('Termp' + 'Init').
                     // 'Init' was called from 'Realtime_Initialize()', thus it is always
                     // called before 'Term'...
   RTAlerts_Init();

   gs_pfnOnSystemIsIdle = NULL;

   gs_bRunning = TRUE;
   roadmap_main_set_periodic( Realtime_GetRefreshRateinMilliseconds(), OnTimer_Realtime);

   roadmap_screen_subscribe_after_flow_control_refresh( OnMapMoved);

   OnTimer_Realtime();

   return gs_bRunning;
}

void OnTransactionCompleted_LogoutAndStop( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_LogoutAndStop() - 'Logout' failed");

   gs_CI.eTransactionStatus = TS_Idle;
   Realtime_FullReset( TRUE /* Redraw */);
}

void Realtime_Stop(BOOL bEnableLogout)
{
   BOOL bLogoutInProcess = FALSE;

   if( !gs_bRunning)
      return;

   roadmap_screen_subscribe_after_flow_control_refresh( NULL);

   roadmap_main_remove_periodic( OnTimer_Realtime);

   if( gs_CI.bLoggedIn)
   {
      VersionUpgradeInfo_Init       ( &gs_VU);
      RTNet_TransactionQueue_Clear  ();
      RTSystemMessageQueue_Empty    ();

      if( bEnableLogout)
      {
         if( RTNet_Logout( &gs_CI, OnTransactionCompleted_LogoutAndStop))
            bLogoutInProcess = TRUE;
         else
            roadmap_log( ROADMAP_ERROR, "Realtime_Stop() - 'RTNet_Logout()' had failed");
      }
   }

   if( bLogoutInProcess)
      return;

   if( TS_Idle == gs_CI.eTransactionStatus)
      Realtime_FullReset(TRUE /* Redraw */);
   else
      RTNet_AbortTransaction( &gs_CI.eTransactionStatus);

   gs_bRunning = FALSE;
}

BOOL Realtime_ServiceIsActive()
{ return gs_bInitialized && gs_bRunning;}

BOOL Realtime_IsInTransaction()
{ return (Realtime_ServiceIsActive() && (TS_Idle != gs_CI.eTransactionStatus));}

void  Realtime_NotifyOnIdle( RoadMapCallback pfnOnSystemIsIdle)
{
   assert(    pfnOnSystemIsIdle);
   assert(!gs_pfnOnSystemIsIdle);

   gs_pfnOnSystemIsIdle = NULL;

   if( TS_Idle == gs_CI.eTransactionStatus)
   {
      pfnOnSystemIsIdle();
      return;
   }

   gs_pfnOnSystemIsIdle = pfnOnSystemIsIdle;
}

static void Realtime_LoginCallback( void)
{
   RoadMapCallback   pPrev = gs_pfnOnUserLoggedIn;
   
	roadmap_main_remove_periodic( Realtime_LoginCallback);
   gs_pfnOnUserLoggedIn = NULL;

   if ( gs_bSaveLoginInfoOnSuccess )
   {
	   Realtime_SaveLoginInfo();
	   gs_bSaveLoginInfoOnSuccess = FALSE;
   }

	if( pPrev )
	{
		pPrev();
	}

	roadmap_screen_refresh();
}

RoadMapCallback  Realtime_NotifyOnLogin( RoadMapCallback pfnOnLogin)
{
	RoadMapCallback	pPrev	= gs_pfnOnUserLoggedIn;

   assert(    pfnOnLogin);

	if( gs_pfnOnUserLoggedIn == pfnOnLogin)
	{
		roadmap_log( ROADMAP_ERROR, "Login callback 0x%lx already registered", (long)pfnOnLogin);
		return NULL;
	}

   gs_pfnOnUserLoggedIn = pfnOnLogin;

   if( (TS_Idle == gs_CI.eTransactionStatus) && gs_CI.bLoggedIn)
   {
      roadmap_main_set_periodic( 10, Realtime_LoginCallback);
   }

   return pPrev;
}

void Realtime_AbortTransaction( RoadMapCallback pfnOnSystemIsIdle)
{
   assert(    pfnOnSystemIsIdle);
   assert(!gs_pfnOnSystemIsIdle);

   gs_pfnOnSystemIsIdle = NULL;

   RTNet_AbortTransaction( &gs_CI.eTransactionStatus);

   if( TS_Idle == gs_CI.eTransactionStatus)
   {
      pfnOnSystemIsIdle();
      return;
   }

   gs_pfnOnSystemIsIdle    = pfnOnSystemIsIdle;
   gs_CI.eTransactionStatus= TS_Stopping;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnMsgboxClosed_ShowSystemMessage( int exit_code)
{
   RTSystemMessage Message;

   if( RTSystemMessageQueue_IsEmpty())
      return;

   if( RTSystemMessageQueue_Pop( &Message))
   {
      roadmap_messagebox_cb( Message.Title, Message.Text, OnMsgboxClosed_ShowSystemMessage);
      RTSystemMessage_Free( &Message);
   }
}

void ShowSystemMessages()
{ OnMsgboxClosed_ShowSystemMessage( -1);}

void PerformVersionUpgrade( int exit_code, void *context)
{
//   assert( gs_VU.NewVersion[0]);
//   assert( gs_VU.URL[0]);

#if defined (_WIN32) || defined (__SYMBIAN32__)
   if( dec_yes == exit_code)
      roadmap_internet_open_browser( gs_VU.URL);
#endif

   VersionUpgradeInfo_Init ( &gs_VU);
}

#ifdef IPHONE
void UpgradeVersion()
{
   const char* szTitle  = "Software Update";
   const char* szText   = "A newer version of Waze is available. Please go to App Store to perform an upgrade.";

   roadmap_messagebox(szTitle, szText);
   VersionUpgradeInfo_Init ( &gs_VU);
}
#elif ANDROID
void UpgradeVersion()
{
   const char* szTitle  = "Software Update";
   const char* szText   = "A newer version of Waze is available. Please go to Android Market to perform an upgrade.";

   roadmap_messagebox(szTitle, szText);
   VersionUpgradeInfo_Init ( &gs_VU);
}
#else
void UpgradeVersion()
{
   const char* szTitle  = "Software Update";
   const char* szText   = "";

   switch( gs_VU.eSeverity)
   {
      case VUS_NA:
         return;

      case VUS_Low:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      case VUS_Medium:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      case VUS_Hi:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      default:
      {
         assert(0);
         return;
      }
   }

   ssd_confirm_dialog      (  szTitle,
                              szText,
                              TRUE,
                              PerformVersionUpgrade,
                              NULL);
}
#endif

BOOL ThereAreTooManyNetworkErrors(/*?*/)
{
   int iSecondsPassedFromLastGoodSession;

   if( (gs_ST.iNetworkErrorsSuccessive < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS_SUCCESSIVE)
         &&
       (gs_ST.iNetworkErrors           < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS))
      return FALSE;


   iSecondsPassedFromLastGoodSession = (int)(time(NULL) - gs_ST.timeLastGoodSession);
   if( iSecondsPassedFromLastGoodSession < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_SECONDS_FROM_LAST_SESSION)
      return FALSE;

   roadmap_log(ROADMAP_WARNING,
               "There Are Too Many Network Errors(!) - %d network errors occurred. %d of them successive. %d seconds passed from last good session!",
               gs_ST.iNetworkErrors,
               gs_ST.iNetworkErrorsSuccessive,
               iSecondsPassedFromLastGoodSession);

   return TRUE;
}

static void HandleNetworkErrors()
{
   if( IsTransactionError( gs_CI.LastError) || (err_timed_out == gs_CI.LastError))
   {
      gs_ST.iNetworkErrors++;
      gs_ST.iNetworkErrorsSuccessive++;

      if( ThereAreTooManyNetworkErrors())
      {
         // TURNNING OFF REALTIME
         roadmap_log( ROADMAP_WARNING, "HandleNetworkErrors() - STOPPING SERVICE - Too many network errors occurred; Disabling service for a while");
         Realtime_Stop( FALSE /* Enable Logout? */);
      }
      else
      {
         if(!gs_bQuiteErrorMode &&
            (RT_THRESHOLD_TO_ENTER_SILENT_MODE__MAX_NETWORK_ERRORS_SUCCESSIVE <
               gs_ST.iNetworkErrorsSuccessive))
         {
            roadmap_log( ROADMAP_INFO, "HandleNetworkErrors() - Entering the 'quite error mode' state");
            gs_bQuiteErrorMode = TRUE;
         }
      }
   }
   else
   {
      gs_ST.iNetworkErrorsSuccessive= 0;

      if( gs_bQuiteErrorMode)
      {
         gs_bQuiteErrorMode = FALSE;
         roadmap_log( ROADMAP_INFO, "HandleNetworkErrors() - Exiting the 'quite error mode' state");
      }
   }
}

void OnTransactionCompleted( void* ctx, roadmap_result rc)
{
   BOOL bNewTransactionStarted = FALSE;

   //if( succeeded == gs_CI.LastError)
      gs_CI.LastError = rc;

   // Were we asked to abort?
   if( TS_Stopping == gs_CI.eTransactionStatus)
   {
      Realtime_ResetTransactionState(TRUE /* Redraw */);
      RTNet_TransactionQueue_Clear();

      if( gs_pfnOnSystemIsIdle)
      {
         gs_pfnOnSystemIsIdle();
         gs_pfnOnSystemIsIdle = NULL;
      }

      return;
   }

   if( !RTUsers_IsEmpty( &gs_CI.Users))
   {
      int   iUpdatedUsersCount;
      int   iRemovedUsersCount;

      RTUsers_RemoveUnupdatedUsers( &gs_CI.Users, &iUpdatedUsersCount, &iRemovedUsersCount);

      if( iUpdatedUsersCount || iRemovedUsersCount)
         roadmap_screen_redraw();
   }

   if( RTUsers_IsEmpty( &gs_CI.Users))
      roadmap_log( ROADMAP_DEBUG, "OnTransactionCompleted() - No users where found");
   else
      roadmap_log( ROADMAP_DEBUG, "OnTransactionCompleted() - Have %d users", RTUsers_Count( &gs_CI.Users));

   if( succeeded != gs_CI.LastError)
   {
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted() - Last operation ended with error '%s'", roadmap_result_string(gs_CI.LastError));
   }

   switch( gs_CI.LastError)
   {
      case err_rt_unknown_login_id:
         roadmap_net_mon_error(roadmap_result_string(gs_CI.LastError));
            
         Realtime_ResetLoginState( TRUE);
         break;

      case err_rt_login_failed:
      case err_rt_wrong_name_or_password:
      {

    	 if ( gs_bFirstLoginFailure )
    	 {
    		 roadmap_login_new_existing_dlg();
    		 gs_bFirstLoginFailure = FALSE;
    	 }
    	 else
    	 {
    		 roadmap_messagebox( "Login Failed: Wrong login details", "Please verify login details are accurate" );
    		 roadmap_login_details_dialog_show_un_pw();
    	 }
         break;
      }


      default:

         if( is_network_error(gs_CI.LastError))
         {
            if(  !gs_bQuiteErrorMode                           &&
                 gs_bHadAtleastOneGoodSession                  &&
                (gs_CI.LastError != err_net_request_pending)   &&
                (gs_CI.LastError != err_net_no_path_to_destination))
               roadmap_net_mon_error(roadmap_result_string(gs_CI.LastError));
            else
               roadmap_log(ROADMAP_DEBUG,
                           "OnTransactionCompleted() - Not presenting network error (Virgin: %d; Last-Err: '%s'; Quite err mode: %d)",
                           !gs_bHadAtleastOneGoodSession, roadmap_result_string(gs_CI.LastError), gs_bQuiteErrorMode);
         }
         else
         {
			   if( succeeded != gs_CI.LastError)
			   {
            	roadmap_net_mon_error(roadmap_result_string(gs_CI.LastError));
			   }

			   // ignoring other non-network non-login errors
		      gs_ST.timeLastGoodSession = time(NULL);

		      if( !gs_bHadAtleastOneGoodSession)
		         gs_bHadAtleastOneGoodSession = TRUE;
         }

         break;
   }

   // Special case: If 'no internet' - stop service:
   if( err_net_no_path_to_destination  == gs_CI.LastError)
   {
      roadmap_log( ROADMAP_INFO, "OnTransactionCompleted() - !!! NO INTERNET !!! REALTIME SERVICE IS STOPPING AUTOMATICALLY !!!");
      Realtime_Stop( FALSE);
      gs_bWasStoppedAutoamatically = TRUE;
   }

   // Check for NO-NETWORK case:
   HandleNetworkErrors();

//   if( RTSystemMessageQueue_Size())
//      ShowSystemMessages();

   if( VUS_NA != gs_VU.eSeverity)
      UpgradeVersion();

   Realtime_ResetTransactionState( FALSE /* Redraw */);

   if( gs_pfnOnLoginTestResult )
      TestLoginDetails();
   else
   {
      if(!gs_CI.bLoggedIn                                                     ||
         !RTNet_TransactionQueue_ProcessSingleItem( &bNewTransactionStarted)  ||
         !bNewTransactionStarted)
         gs_CI.eTransactionStatus = TS_Idle;
   }

   if( (TS_Idle == gs_CI.eTransactionStatus))
   {
   	if( gs_pfnOnSystemIsIdle)
	   {
	      gs_pfnOnSystemIsIdle();
	      gs_pfnOnSystemIsIdle = NULL;
	   }
	   if( gs_CI.bLoggedIn )
	   {
	   	roadmap_main_set_periodic( 10, Realtime_LoginCallback );
	   }
   }
   roadmap_screen_refresh();
}

void OnAsyncOperationCompleted_AllTogether( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether(POST) - The 'AllTogether' packet-send had failed");

   editor_track_report_conclude_export( succeeded == rc);

   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_NodePath( void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
   {
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_NodePath() - 'NodePath' succeeded (if there where points to send - they were sent)");
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_NodePath() - TRANSACTION FULLY COMPLETED");
   }
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_NodePath(POST) - 'NodePath' had failed");

   OnTransactionCompleted( ctx, rc);
}


void OnAsyncOperationCompleted_CreateAccount( void* ctx, roadmap_result rc)
{
   // Finilize the transation
   OnTransactionCompleted(  ctx,  rc );
   // Handle the errors
   roadmap_login_update_details_on_response( rc );
}


BOOL HaveNodePathToSend()
{ return (1 <= gs_pPI->num_nodes);}

BOOL SendMessage_NodePath( char* packet_only)
{
   BOOL  bRes;

   bRes = RTNet_NodePath( &gs_CI,
                           gs_pPI->nodes[0].GPS_time,
                           gs_pPI->nodes,
                           gs_pPI->num_nodes,
                           gs_pPI->user_points,
                           gs_pPI->num_user_points,
                           OnAsyncOperationCompleted_NodePath,
                           packet_only);

   return bRes;
}

void OnAsyncOperationCompleted_GPSPath( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_GPSPath(POST) - 'GPSPath' had failed");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_GPSPath() - 'GPSPath' succeeded (if there where points to send - they were sent)");

   if( HaveNodePathToSend())
   {
      if( SendMessage_NodePath(NULL))
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_GPSPath() - Sending 'NodePath'...");
      else
      {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_GPSPath(PRE) - Failed to send 'NodePath'");
         OnTransactionCompleted( ctx, rc);
      }
   }
   else
      OnAsyncOperationCompleted_NodePath( ctx, succeeded);
}


BOOL HaveGPSPointsToSend()
{
   return (1 < gs_pPI->num_points);
}

BOOL GPSPointsMultipleCycles()
{
   return (RTTRK_GPSPATH_MAX_POINTS < gs_pPI->num_points);
}

BOOL SendMessage_GPSPath( char* packet_only)
{
   BOOL  bRes;

   bRes = RTNet_GPSPath(&gs_CI,
                        gs_pPI->points[0].GPS_time,
                        gs_pPI->points,
                        gs_pPI->num_points,
                        OnAsyncOperationCompleted_GPSPath,
                        packet_only);


   return bRes;
}


void OnAsyncOperationCompleted_CreateNewRoads( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_CreateNewRoads(POST) - 'CreateNewRoads' had failed");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_CreateNewRoads() - 'CreateNewRoads' was sent!");

   if( HaveGPSPointsToSend())
   {
      if( SendMessage_GPSPath(NULL))
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_CreateNewRoads() - Sending 'GPSPath'...");
      else
      {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_CreateNewRoads(PRE) - Failed to send 'GPSPath'");
         OnTransactionCompleted( ctx, err_failed);
      }
   }
   else
      OnAsyncOperationCompleted_GPSPath( ctx, succeeded);  // Move on to next handler...
}


void OnAsyncOperationCompleted_ReportAlert( void* ctx, roadmap_result rc)
{
   RTAlerts_CloseProgressDlg();
   if( succeeded == rc){
      RTAlerts_Resert_Minimized();
      roadmap_trip_restore_focus();
      roadmap_messagebox_timeout ("Thank you!!!", "Your report was successfully send to wazers in your area", 3);
   }else{
      roadmap_messagebox_timeout ("Error", "Sending report failed. Please resend later", 5);
   }

   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_SendSMS( void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_messagebox_timeout ("Thank you!!!", "Message sent", 3);
   else
      roadmap_messagebox ("Error", "Sending message failed");

   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_TwitterConnect( void* ctx, roadmap_result rc)
{
   if( succeeded != rc) {
      roadmap_messagebox ("Error", "There is no network connection.Updating your twitter account details failed.");
      roadmap_twitter_set_logged_in(FALSE);
   }

   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_Foursquare( void* ctx, roadmap_result rc)
{
   if( succeeded != rc) {
      roadmap_foursquare_request_failed(rc);
   }
   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_PostComment( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_messagebox_timeout("Error", "Sending comment failed", 5);

   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_ReportMapProblem( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_messagebox_timeout("Error", "Sending message failed", 5);


   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_TripServer( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_TripServer(POST)  failed (rc=%d)", rc);
      OnTransactionCompleted( ctx, rc);
      return;
   }
}
BOOL HaveCreateNewRoadsToSend()
{
   return (0 < gs_pPI->num_update_toggles);
}

BOOL SendMessage_CreateNewRoads( char* packet_only)
{
   BOOL bStatus = FALSE;
   RoadMapStateFn fnState;

   fnState = roadmap_state_find ("new_roads");

   if (fnState == NULL)
   {
      roadmap_log (ROADMAP_ERROR, "Failed to retrieve new_roads state");
      return TRUE;
   }

     bStatus = (fnState() != 0);

   return RTNet_CreateNewRoads(
                           &gs_CI,
                           gs_pPI->num_update_toggles,
                           gs_pPI->update_toggle_times,
                           gs_pPI->first_update_toggle_state,
                           OnAsyncOperationCompleted_CreateNewRoads,
                           packet_only);
}

void OnAsyncOperationCompleted_AlertReport( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AlertReport(POST) - 'AlertReport' had failed");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AlertReport() - 'AlertReport' was sent!");

   if (HaveCreateNewRoadsToSend ())
   {
      if (SendMessage_CreateNewRoads (NULL)) {
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AlertReport() - Sending 'CreateNewRoads'...");
      }
      else {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AlertReport(PRE) - Failed to send 'CreateNewRoads'");
         OnTransactionCompleted( ctx, err_failed);
      }
   }
   else
      OnAsyncOperationCompleted_CreateNewRoads( ctx, succeeded);  // Move on to next handler...
}

void OnAsyncOperationCompleted_MapDisplayed( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed(POST) - 'MapDisplayed' had failed");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed() - 'MapDisplayed' was sent!");

   if (HaveCreateNewRoadsToSend ())
   {
      if (SendMessage_CreateNewRoads (NULL)) {
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed() - Sending 'CreateNewRoads'...");
      }
      else {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed(PRE) - Failed to send 'CreateNewRoads'");
         OnTransactionCompleted( ctx, err_failed);
      }
   }
   else
      OnAsyncOperationCompleted_CreateNewRoads( ctx, succeeded);  // Move on to next handler...
}


BOOL SendMessage_MapDisplyed( char* packet_only)
{
   RoadMapArea MapPosition;

   roadmap_math_screen_edges( &MapPosition);
   if((gs_CI.LastMapPosSent.west  == MapPosition.west ) &&
      (gs_CI.LastMapPosSent.south == MapPosition.south) &&
      (gs_CI.LastMapPosSent.east  == MapPosition.east ) &&
      (gs_CI.LastMapPosSent.north == MapPosition.north))
   {
      roadmap_log( ROADMAP_DEBUG, "SendMessage_MapDisplyed() - Skipping operation; Current coordinates where already sent...");

      if( !packet_only)
         OnAsyncOperationCompleted_MapDisplayed( NULL, succeeded);

      return TRUE;
   }

   if( RTNet_MapDisplyed(  &gs_CI,
                           &MapPosition,
            roadmap_math_get_scale(0),
                           OnAsyncOperationCompleted_MapDisplayed,
                           packet_only))
   {
      gs_CI.LastMapPosSent = MapPosition;
      return TRUE;
   }

   return FALSE;
}

void OnAsyncOperationCompleted_At( void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_At() - My position is set!");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_At(POST) - Failed to set my position; Ignoring and continueing...");

   if( SendMessage_MapDisplyed( NULL))
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_At() - Sending 'MapDisplayed'...");
   else
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_At(PRE) - Failed to send 'MapDisplayed'");
      OnTransactionCompleted( ctx, err_failed);
   }
}

BOOL SendMessage_At( char* packet_only)
{
   RoadMapGpsPosition   MyLocation;
   int                  from;
   int                  to;
   int                  direction;
   static BOOL          sendGpsLocation = TRUE;

   if( !editor_track_report_get_current_position( &MyLocation, &from, &to, &direction))
   {
      BOOL has_reception = (roadmap_gps_reception_state() != GPS_RECEPTION_NONE) && (roadmap_gps_reception_state() != GPS_RECEPTION_NA);
      const RoadMapPosition *Location = roadmap_trip_get_position ("GPS");

      if (( Location == NULL) || IS_DEFAULT_LOCATION(Location) || !has_reception){
         // No GPS position, try cell one
    	  Location = roadmap_trip_get_position( "Location" );
      }

      if ((Location != NULL) && !IS_DEFAULT_LOCATION(Location) && sendGpsLocation ){
         sendGpsLocation = FALSE;
         MyLocation.latitude = Location->latitude;
         MyLocation.longitude = Location->longitude;
         MyLocation.altitude = 0;
         MyLocation.speed = 0;
         MyLocation.steering = 0;
         return RTNet_At(  &gs_CI,
                  &MyLocation,
                  -1,
                  -1,
                  OnAsyncOperationCompleted_At,
                  packet_only);
      }
      else{
         roadmap_log( ROADMAP_DEBUG, "SendMessage_At() - 'editor_track_report_get_current_position()' failed");
         return FALSE;
      }
   }


   sendGpsLocation = FALSE;
   return RTNet_At(  &gs_CI,
        &MyLocation,
        from,
        to,
        OnAsyncOperationCompleted_At,
        packet_only);
}

void OnAsyncOperationCompleted_SetVisability( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_SetVisability(POST) - Failed to set visability");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_SetVisability() - Visability set!");

   //   Send current location:
   if( SendMessage_At( NULL))
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_SetVisability() - Setting my position...");
   else
   {
      roadmap_log( ROADMAP_WARNING, "OnAsyncOperationCompleted_SetVisability(PRE) - Failed to set my position ('At')");
      OnAsyncOperationCompleted_At( ctx, err_failed);
   }
}

void OnAsyncOperationCompleted_SetMood( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_SetVMood(POST) - Failed to set visability");
   }
   
   OnTransactionCompleted( ctx, rc);
}

void OnAsyncOperationCompleted_UserPoints( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_UserPoints(POST) - Failed to send user points");
      gs_bShouldSendUserPoints = TRUE;
   }
   
   OnTransactionCompleted( ctx, rc);
}

BOOL SendMessage_SetMyVisability( char* packet_only)
{
   ERTVisabilityGroup eVisability;
   ERTVisabilityReport eVisabilityReport;
   BOOL downloadWazers;
   BOOL downloadReports;
   BOOL downloadTraffic;
   if( !gs_bShouldSendMyVisability)
   {
      if( packet_only)
         (*packet_only) = '\0';

      return TRUE;
   }
 
   downloadWazers = roadmap_download_settings_isDownloadWazers();
   downloadReports = roadmap_download_settings_isDownloadReports();
   downloadTraffic = roadmap_download_settings_isDownloadTraffic();
   
   eVisability = ERTVisabilityGroup_from_string( roadmap_config_get( &RT_CFG_PRM_VISGRP_Var));

   eVisabilityReport = ERTVisabilityReport_from_string(roadmap_config_get( &RT_CFG_PRM_VISREP_Var));

   if( RTNet_SetMyVisability( &gs_CI,
                              eVisability,
                              eVisabilityReport,
                              OnAsyncOperationCompleted_SetVisability,
                              downloadWazers,
                              downloadReports,
                              downloadTraffic,
                              packet_only))
   {
      gs_bShouldSendMyVisability = FALSE;
      return TRUE;
   }

   return FALSE;
}

BOOL SendMessage_SetMood( char* packet_only)
{
   if( !gs_bShouldSendSetMood)
   {
      if( packet_only)
         (*packet_only) = '\0';

      return TRUE;
   }

   if( RTNet_SetMood( &gs_CI,
   					  roadmap_mood_state(),
                      OnAsyncOperationCompleted_SetMood,
                      packet_only))
   {
      gs_bShouldSendSetMood = FALSE;
      return TRUE;
   }

   return FALSE;
}

BOOL SendMessage_UserPoints( char* packet_only)
{
   static int s_iUserPoints = 0;
   
   if( !gs_bShouldSendUserPoints && s_iUserPoints == editor_points_get_total_points())
   {
      if( packet_only)
         (*packet_only) = '\0';

      return TRUE;
   }

   if( RTNet_UserPoints( &gs_CI,
                         editor_points_get_total_points(),
                         OnAsyncOperationCompleted_UserPoints,
                         packet_only))
   {
      s_iUserPoints = editor_points_get_total_points();
      gs_bShouldSendUserPoints = FALSE;
      return TRUE;
   }

   return FALSE;
}

void OnAsyncOperationCompleted_MapDisplayed__only( void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed__only() - 'MapDisplayed' was sent successfully");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed__only(POST) - 'MapDisplayed' had failed");

   OnTransactionCompleted( ctx, rc);
}

BOOL Realtime_SendCurrentViewDimentions()
{
   RoadMapArea MapPosition;
   BOOL        bRes;

   if( !gs_bRunning)
   {
      roadmap_log( ROADMAP_ERROR, "Realtime_SendCurrentViewDimentions() - Realtime service is currently disabled; Exiting method");
      return FALSE;
   }

   roadmap_math_screen_edges( &MapPosition);
   if((gs_CI.LastMapPosSent.west  == MapPosition.west ) &&
      (gs_CI.LastMapPosSent.south == MapPosition.south) &&
      (gs_CI.LastMapPosSent.east  == MapPosition.east ) &&
      (gs_CI.LastMapPosSent.north == MapPosition.north))
   {
      roadmap_log( ROADMAP_DEBUG, "Realtime_SendCurrentViewDimentions() - Skipping operation; Current coordinates where already sent...");
      return TRUE;
   }

   //   Remove all users:
   RTUsers_ResetUpdateFlag( &gs_CI.Users);

   bRes = RTNet_MapDisplyed(  &gs_CI,
                              &MapPosition,
               roadmap_math_get_scale(0),
                              OnAsyncOperationCompleted_MapDisplayed__only,
                              NULL);
   if( bRes)
   {
      gs_CI.LastMapPosSent = MapPosition;
      roadmap_log( ROADMAP_DEBUG, "Realtime_SendCurrentViewDimentions() - Sending 'MapDisplayed'...");
   }
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_SendCurrentViewDimentions(PRE) - 'RTNet_MapDisplyed()' had failed");

   return bRes;
}

BOOL SendAllMessagesTogether_SendPart2( BOOL bFirstCycle);

void OnAsyncOperationCompleted_AllTogether_Part1( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether_Part1(POST) - 'Part1' had failed");
      editor_track_report_conclude_export( 0);
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AllTogether_Part1() - 'Part1' was sent!");

   if (SendAllMessagesTogether_SendPart2(TRUE)) {
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AllTogether_Part1() - Sending 'Part2'...");
   } else {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether_Part1(PRE) - Failed to send 'Part2'");
      OnTransactionCompleted( ctx, err_failed);
   }
}

void OnAsyncOperationCompleted_AllTogether_Part2( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether_Part2(POST) - 'Part2' had failed");
      editor_track_report_conclude_export( 0);
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AllTogether_Part2() - 'Part2' was sent!");

   if (SendAllMessagesTogether_SendPart2(FALSE)) {
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_AllTogether_Part2() - Sending new 'Part2'...");
   } else {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether_Part2(PRE) - Failed to send new 'Part2'");
      OnTransactionCompleted( ctx, err_failed);
   }
}

BOOL SendAllMessagesTogether_SendPart1( BOOL bSummaryOnly)
{
   char*    p;
   ebuffer  buf;
   char*    Packet;
   BOOL     bRes;

   ebuffer_init( &buf);
   
   Packet = ebuffer_alloc( &buf,
                           MESSAGE_MAX_SIZE__AllTogether__dynamic( 0,
                                                                   0,
                                                                   RTTRK_CREATENEWROADS_MAX_TOGGLES));
   p = Packet;
   
   if( !bSummaryOnly)
   {
      // See me
      if( !SendMessage_SetMyVisability( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_SetMyVisability()' had failed");
         ebuffer_free( &buf);
         return FALSE;
      }
      p = Packet + strlen( Packet);

      // Set mood
      if( !SendMessage_SetMood( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_SetMood()' had failed");
         ebuffer_free( &buf);
         return FALSE;
      }
      p = Packet + strlen( Packet);
   }
   
   // User points
   if( SendMessage_UserPoints( p))
      p = Packet + strlen( Packet);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_UserPoints()' did not send; Ignoring and continueing");

   // At (my location)
   if( SendMessage_At( p))
      p = Packet + strlen( Packet);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_At()' had failed; Ignoring and continueing");

   if( !bSummaryOnly)
   {
      if ( gs_bShouldSendMapDisplayed)
      {
         // Map displayed
         if( !SendMessage_MapDisplyed( p))
         {
            roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_MapDisplyed()' had failed");
            ebuffer_free( &buf);
            return FALSE;
         }
         p = Packet + strlen( Packet);
      }
   }

   // Allow new roads toggles
   if (HaveCreateNewRoadsToSend ())
   {
      if (!SendMessage_CreateNewRoads ( p)) {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_Part1(PRE) - 'SendMessage_CreateNewRoads()' had failed");
         ebuffer_free( &buf);
         return FALSE;
      }
      p = Packet + strlen( Packet);
   }


   if( 0 < strlen( Packet)) {
      bRes = RTNet_GeneralPacket( &gs_CI,
                                  Packet,
                                  OnAsyncOperationCompleted_AllTogether_Part1);
      ebuffer_free( &buf);
      return bRes;
   }

   ebuffer_free( &buf);
   gs_CI.LastError = err_rt_no_data_to_send;
   return FALSE;
}

BOOL SendAllMessagesTogether_SendPart2( BOOL bFirstCycle)
{
   static int           iPoint = 0;
   char*                p;
   ebuffer              buf;
   char*                Packet;
   RTPathInfo           pi;
   static RTPathInfo    *pOrigPI;
   CB_OnWSTCompleted    pfnOnCompleted;
   BOOL                 bRes;
   BOOL                 bLastPacket = FALSE;
   
   if (bFirstCycle) {
      iPoint = 0;
      pOrigPI = gs_pPI;
   } else {
      if (iPoint >= pOrigPI->num_points) {
         return FALSE;
      }
      iPoint += RTTRK_GPSPATH_MAX_POINTS;
   }
   
   if (iPoint + RTTRK_GPSPATH_MAX_POINTS >= pOrigPI->num_points) {
      pfnOnCompleted = OnAsyncOperationCompleted_AllTogether;
      bLastPacket = TRUE;
   } else {
      pfnOnCompleted = OnAsyncOperationCompleted_AllTogether_Part2;
   }
      

   ebuffer_init( &buf);

   Packet = ebuffer_alloc( &buf,
                           MESSAGE_MAX_SIZE__AllTogether__dynamic( RTTRK_GPSPATH_MAX_POINTS,
                                                                   RTTRK_NODEPATH_MAX_POINTS,
                                                                   0));

   p = Packet;
   pi = *pOrigPI;

   pi.num_points = pOrigPI->num_points - iPoint;
   if( pi.num_points > RTTRK_GPSPATH_MAX_POINTS)
      pi.num_points = RTTRK_GPSPATH_MAX_POINTS;
   pi.points = pOrigPI->points + iPoint;

   gs_pPI = &pi;
   
   // At (my location)
   if (bLastPacket) {
      if( SendMessage_At( p))
         p = Packet + strlen( Packet);
      else
         roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether_Part2(PRE) - 'SendMessage_At()' had failed; Ignoring and continueing");
   }
      
   // GPS points path
   if( !SendMessage_GPSPath( p))
   {
      roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_SendSplit2(PRE) - 'SendMessage_GPSPath()' had failed");
      ebuffer_free( &buf);
      return FALSE;
   }
   p = Packet + strlen(Packet);


   // Nodes path
   if( bLastPacket && HaveNodePathToSend())
   {
      if( !SendMessage_NodePath( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_SendPart2(PRE) - 'SendMessage_NodePath()' had failed");
         ebuffer_free( &buf);
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }
   
   
   if( 0 < strlen( Packet)) {
      bRes = RTNet_GeneralPacket( &gs_CI,
                                  Packet,
                                  pfnOnCompleted);
      ebuffer_free( &buf);
      return bRes;
   }

   
   roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether_Part2(PRE) - 'SendMessage_GPSPath()' had failed");
   ebuffer_free( &buf);
   gs_CI.LastError = err_rt_no_data_to_send;
   return FALSE;
}

BOOL SendAllMessagesTogether_BuildPacket( BOOL bSummaryOnly, char* Packet)
{
   char* p = Packet;

	if( !bSummaryOnly)
	{
	   // See me
	   if( !SendMessage_SetMyVisability( p))
	   {
	      roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_SetMyVisability()' had failed");
	      return FALSE;
	   }
	   p = Packet + strlen(Packet);

	   // Set mood
	   if( !SendMessage_SetMood (p))
	   {
	      roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_SetMood()' had failed");
	      return FALSE;
	   }
	   p = Packet + strlen(Packet);
	}
   
   // User points
   if( SendMessage_UserPoints( p))
      p = Packet + strlen( Packet);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether(PRE) - 'SendMessage_UserPoints()' did not send; Ignoring and continueing");

   // At (my location)
   if( SendMessage_At( p))
      p = Packet + strlen(Packet);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether(PRE) - 'SendMessage_At()' had failed; Ignoring and continueing");

	if( !bSummaryOnly)
	{
	   if (gs_bShouldSendMapDisplayed)
	   {
	      // Map displayed
	      if( !SendMessage_MapDisplyed( p))
	      {
	         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_MapDisplyed()' had failed");
	         return FALSE;
	      }
	      p = Packet + strlen(Packet);
	   }
	}

   // Allow new roads toggles
   if (HaveCreateNewRoadsToSend ())
   {
      if (!SendMessage_CreateNewRoads (p)) {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_CreateNewRoads()' had failed");
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }

   // GPS points path
   if( HaveGPSPointsToSend())
   {
      if( !SendMessage_GPSPath( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_GPSPath()' had failed");
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }

   // Nodes path
   if( HaveNodePathToSend())
   {
      if( !SendMessage_NodePath( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_NodePath()' had failed");
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }


   if( 0 < strlen( Packet))
      return TRUE;

   gs_CI.LastError = err_rt_no_data_to_send;
   return FALSE;
}

BOOL SendAllMessagesTogether( BOOL bSummaryOnly, BOOL bCalledAfterLogin)
{
   ebuffer  Packet;
   int      iGPSPointsCount;
   int      iNodePointsCount;
   int      iAllowNewRoadsCount;
   char*    Buffer = NULL;
   BOOL     bTransactionStarted = FALSE;

   ebuffer_init( &Packet);

   gs_pPI = editor_track_report_begin_export (0);
   iGPSPointsCount      = gs_pPI->num_points;
   iNodePointsCount     = gs_pPI->num_nodes;
   iAllowNewRoadsCount  = gs_pPI->num_update_toggles;

   if (GPSPointsMultipleCycles()) {
      roadmap_log( ROADMAP_WARNING, "SendAllMessagesTogether() - Long data, splitting packets...");
      bTransactionStarted = SendAllMessagesTogether_SendPart1( bSummaryOnly);

      if (!bTransactionStarted && gs_CI.LastError == err_rt_no_data_to_send) 
            bTransactionStarted = SendAllMessagesTogether_SendPart2(TRUE);
   } else {
      Buffer               = ebuffer_alloc( &Packet,
                             MESSAGE_MAX_SIZE__AllTogether__dynamic(iGPSPointsCount,
                                                                    iNodePointsCount,
                                                                    iAllowNewRoadsCount));
      if( SendAllMessagesTogether_BuildPacket( bSummaryOnly, Buffer))
         bTransactionStarted = RTNet_GeneralPacket(&gs_CI,
                                                   Buffer,
                                                   OnAsyncOperationCompleted_AllTogether);
   }
   
   if (!bTransactionStarted)
      roadmap_log( ROADMAP_WARNING, "SendAllMessagesTogether() - NOT SENDING (maybe no data to send?)");

	if( bSummaryOnly)
      roadmap_log( ROADMAP_WARNING, "SendAllMessagesTogether() - High Frequency Summary mode (jammed..)");

   if( !bTransactionStarted && bCalledAfterLogin)
      OnTransactionCompleted( NULL, succeeded /* Called on behalf of the 'login', which DID succeed... */);

   if (!bTransactionStarted)
      editor_track_report_conclude_export (0);

   ebuffer_free( &Packet);

   return bTransactionStarted;
}

void OnAsyncOperationCompleted_Login( void* ctx, roadmap_result rc)
{
   if( succeeded != rc || !gs_CI.bLoggedIn)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_Login(POST) - Failed to log in");
      OnTransactionCompleted( ctx, rc);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_Login() - User logged in!");
   Realtime_SessionDetailsSave();
   Realtime_DumpOffline(); //Dump offline data prior to login. Will be uploaded on next launch

   gs_bFirstLoginFailure = FALSE;

   SendAllMessagesTogether( FALSE, TRUE /* Called After Login */);

   roadmap_main_set_periodic( 10, Realtime_LoginCallback);
}

void OnAsyncOperationCompleted_Register( void* ctx, roadmap_result rc)
{
   CB_OnWSTCompleted pfnLoginAfterRegister = gs_pfnOnLoginAfterRegister;

   gs_pfnOnLoginAfterRegister = NULL;

   // assert( pfnLoginAfterRegister);

   if( succeeded == rc)
   {
	  Realtime_SetLoginUsername( gs_CI.UserNm );
	  Realtime_SetLoginPassword( gs_CI.UserPW );
	  Realtime_SetLoginNickname( gs_CI.UserNk );
	  gs_bSaveLoginInfoOnSuccess = TRUE;			// Force saving username and password to the configuration
	  Realtime_set_random_user( TRUE );
      roadmap_log(ROADMAP_DEBUG,
                  "OnAsyncOperationCompleted_Register(POST) - The 'Register' operation has succeeded! (Name: '%s'; PW: '%s')",
                  gs_CI.UserNm, gs_CI.UserPW);
   }
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_Register(POST) - The 'Register' operation had failed");

   Realtime_ResetTransactionState( FALSE /* Redraw */);

   if( succeeded == rc )
   {
	  Realtime_ResetLoginState( FALSE /* Redraw */ );

	  if ( pfnLoginAfterRegister )
	  {	/* There is special function for the login callback */
		  Login( pfnLoginAfterRegister, FALSE );
	  }
	  else
	  {
		  TestLoginMain();
	  }
   }
}


void OnAsyncOperationCompleted_GetGeoConfig( void* ctx, roadmap_result rc)
{

   if( succeeded == rc)
   {
   }
   else{
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_GetGeoConfig(POST) - The 'GetGeoConfig' operation had failed, rc=%s", roadmap_result_string(rc));

      roadmap_geo_config_transaction_failed();
   }
   
   Realtime_ResetTransactionState( FALSE /* Redraw */);


}

/***********************************************************
 *  Name        : Login
 *  Purpose     : Tries to login to the Realtime server with the supplied user name and password
 *
 *  Params      : [in]  callback - the callback to be executed upon transaction completion
 *  			  [in]  bShowRegistration - indicates if registration dialog should be opened in case of clean user data
 *              : [out] - none
 *  Returns     : Always TRUE  - FFU
 *  Notes       :
 */
static BOOL Login( CB_OnWSTCompleted callback, BOOL bShowRegistration )
{
   const char* szName = NULL;
   const char* szPassword = NULL;
   const char* szNickname = NULL;

   szName = gs_LoginDetails.username;
   szPassword = gs_LoginDetails.pwd;
   szNickname = gs_LoginDetails.nickname;

   gs_bShouldSendMyVisability = TRUE;
   gs_bShouldSendSetMood      = TRUE;

   /*
    *  If user and password exist - try to login and handle the results in callback
    */
   if( (*szName) && (*szPassword) )
   {
      roadmap_log( ROADMAP_DEBUG, "Login() - Trying to login user '%s'", szName);
      return RTNet_Login( &gs_CI, szName, szPassword, szNickname, callback);
   }

   /*
    * User and password do not exist.
    * Check if there is request to show the registration
    */
   if( !bShowRegistration )
   {
      roadmap_log( ROADMAP_DEBUG, "Login() - Do not have 'name&password' and  (bShowRegistration == FALSE) - returning FALSE...");
      return TRUE;
   }

   gs_pfnOnLoginAfterRegister = callback;

   roadmap_log( ROADMAP_DEBUG, "Login() - Do not have 'name&password' - show the new/existing dialog" );

   roadmap_login_new_existing_dlg();

   return TRUE;
}

BOOL Realtime_RandomUserRegister()
{
   return RTNet_RandomUserRegister( &gs_CI, OnAsyncOperationCompleted_Register );
}

BOOL TransactionStarted( BOOL bSummaryOnly)
{
   if( gs_bRunning && !gs_CI.bLoggedIn)
      return Login( OnAsyncOperationCompleted_Login, TRUE /* Auto register? */);
   
   return SendAllMessagesTogether( bSummaryOnly, FALSE /* Called After Login */);
}

BOOL NameAndPasswordAlreadyFailedAuthentication()
{
   const char* szName;
   const char* szPassword;

   szName = gs_LoginDetails.username;
   szPassword = gs_LoginDetails.pwd;

   if( err_rt_wrong_name_or_password != gs_CI.LastError)
      return FALSE;

   return (!strcmp(gs_CI.UserNm, szName) && !strcmp(gs_CI.UserPW, szPassword));
}

BOOL StartTransaction( BOOL bSummaryOnly)
{
   if( gs_bQuiteErrorMode)
   {
      static BOOL s_skip_this_time = FALSE;

      s_skip_this_time = !s_skip_this_time;

      if( s_skip_this_time)
      {
         roadmap_log( ROADMAP_WARNING, "StartTransaction() - QUIET ERROR MODE - Skipping this iteration (one on one off)");
         return TRUE;
      }
   }

   // Do we want to start session?
   if( succeeded != gs_CI.LastError)
   {
      if( NameAndPasswordAlreadyFailedAuthentication())
      {
         roadmap_log( ROADMAP_WARNING, "StartTransaction() - NOT ATTEMPTING A NEW SESSION - Last login attempt failed with 'wrong name/pw'; Login details were not modified");

         return FALSE;
      }

      //gs_CI.LastError = succeeded; //this causes bad network indication. AR
   }

   // START TRANSACTION:
   RTUsers_ResetUpdateFlag( &gs_CI.Users);   // User[i].updated = FALSE
   gs_CI.eTransactionStatus = TS_Active;     // We are in transaction now
   if( TransactionStarted( bSummaryOnly))                 // Start...
      return TRUE;

   // TRANSACTION FAILED TO START
   //    ...rollback...

   gs_CI.eTransactionStatus = TS_Idle;       // We are idle
   RTUsers_RedoUpdateFlag( &gs_CI.Users);    // User[i].updated = TRUE  (undo above)

   if( succeeded == gs_CI.LastError)
      gs_CI.LastError = err_failed;          // Last error - General error

   if( err_rt_no_data_to_send == gs_CI.LastError)
      gs_CI.LastError = succeeded;

   return FALSE;
}


static ECycleType Realtime_GetCycleType(void)
{
	static int 				s_lastFullCycle = 0;
	static int				s_lastCycle = 0;
	static int				s_lastCommCheck = 0;
	int 						timeNow = time( NULL);
	static PluginLine		s_lastLine;
	static int				s_lastDirection;
	static int				s_lastTime = 0;
	PluginLine				currentLine;
	int						currentDirection;

	// test for communication failure
	if( gs_bQuiteErrorMode ||
		 (!gs_bHadAtleastOneGoodSession) ||
		 (gs_CI.LastError >= network_errors &&
		  gs_CI.LastError <= network_errors_end) ||
        gs_CI.LastError == err_rt_unknown_login_id)
	{
		if( 0 == s_lastCommCheck ||
			 timeNow < s_lastCommCheck + gs_iMaxCommCheckSeconds)
		{
			if( 0 == s_lastCommCheck)
				s_lastCommCheck = timeNow;
			s_lastCycle = s_lastFullCycle = timeNow - gs_iCycleRoundoffSeconds;
			return CT_Full;
		}
	}
	else
	{
		s_lastCommCheck = 0;
	}

	// test if full cycle time is reached
	if( timeNow >= s_lastFullCycle + gs_iCycleTimeSeconds)
	{
		s_lastCycle = s_lastFullCycle = timeNow - gs_iCycleRoundoffSeconds;
		return CT_Full;
	}

	// test if an operation is needed
	if( timeNow < s_lastCycle + gs_iSummaryCycleSeconds)
	{
		return CT_None;
	}

	// test if we are jammed
	if( !roadmap_navigate_is_jammed())
	{
		s_lastTime = 0;
		return CT_None;
	}

	// test if current segment is already marked as jammed
	if( (-1 == roadmap_navigate_get_current( NULL, &currentLine, &currentDirection)) ||
		 (ROADMAP_PLUGIN_ID != currentLine.plugin_id) ||
		 (-1 != RTTrafficInfo_Get_Line( currentLine.line_id, currentLine.square, ROUTE_DIRECTION_AGAINST_LINE == currentDirection)) )
	{
		s_lastTime = 0;
		return CT_None;
	}

	// check if this is a new jammed segment
	if( (0 == s_lastTime) ||
		 (!roadmap_plugin_same_line( &s_lastLine, &currentLine)) ||
		 (s_lastDirection != currentDirection) )
	{
		s_lastTime = timeNow;
		s_lastLine = currentLine;
		s_lastDirection = currentDirection;
	}

	// check how much time we are jammed at this segment
	if( timeNow >= s_lastTime + gs_iCycleTimeSeconds)
	{
		return CT_None;
	}

	s_lastCycle = timeNow - gs_iCycleRoundoffSeconds;
	return CT_Summary;
}


void OnTimer_Realtime(void)
{
	ECycleType ct;

   if( !gs_bRunning)
   {
      assert(0);
      return;
   }

	ct = Realtime_GetCycleType();
	if( CT_None == ct)
	{
		//roadmap_log (ROADMAP_DEBUG, "Skipping comm cycle");
		return;
	}

   switch(gs_CI.eTransactionStatus)
   {
      case TS_Idle:
         StartTransaction( CT_Summary == ct);
         break;

      case TS_Active:
         RTNet_Watchdog( &gs_CI);
         break;

      default:
    	  break;
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnSettingsChanged_EnableDisable(void)
{
   BOOL bEnabled = GetEnableDisableState();

   if( bEnabled)
   {
      if( !gs_bRunning)
         Realtime_Start();
   }
   else
   {
      if( gs_bRunning)
         Realtime_Stop( TRUE /* Enable Logout? */);
   }
}

void OnSettingsChanged_VisabilityGroup(void)
{
   gs_bShouldSendMyVisability = TRUE;
   SendMessage_SetMyVisability( NULL );
}

void OnMoodChanged(void)
{
   gs_bShouldSendSetMood = TRUE;
   SendMessage_SetMood(NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RemoveWazerNearby (void) {
   if (gs_WazerNearbyID != -1)
      gs_WazerNearbyID = -1;
   
   gs_WazerNearbyLastShown = time(NULL);
   
   if (gs_iWazerNearbyState != 0){
      gs_iWazerNearbyState = 0;
      if ( !roadmap_screen_refresh() )
         roadmap_screen_redraw();
   }
}

void Realtime_ShowWazerNearby (void) {
   LPRTUserLocation user;

   if (gs_iWazerNearbyState == 0)
      return;
   
   user = RTUsers_UserByID (&gs_CI.Users, gs_WazerNearbyID);
   RTUsers_Popup(&gs_CI.Users, user->sGUIID, RT_USERS_CENTER_ON_ME);
}

int Realtime_WazerNearbyState (void) {
   return gs_iWazerNearbyState;
}

void wazer_nearby_timeout(void) {
   roadmap_main_remove_periodic(wazer_nearby_timeout);
   
   RemoveWazerNearby();
}

static void realtime_after_refresh (void) {
   RoadMapArea screen_area;
   LPRTUserLocation user;

   if (gs_iWazerNearbyState == 0 || gs_WazerNearbyID == -1){
      if (realtime_prev_after_refresh) {
         (*realtime_prev_after_refresh) ();
      }
      return;
   }
   
   user = RTUsers_UserByID(&gs_CI.Users, gs_WazerNearbyID);
   
   if (user ){
   
      if (roadmap_math_point_is_visible(&user->position))
         RemoveWazerNearby();
      else { //calc best position (1 = N, 2 = W ...)
         roadmap_math_screen_edges(&screen_area);
         if (user->position.longitude < screen_area.west)
            gs_iWazerNearbyState = 2;
         else if (user->position.longitude > screen_area.east)
            gs_iWazerNearbyState = 4;
         else if (user->position.latitude > screen_area.north)
            gs_iWazerNearbyState = 1;
         else 
            gs_iWazerNearbyState = 3;
      }
   }
   if (realtime_prev_after_refresh) {
      (*realtime_prev_after_refresh) ();
   }
}

static void SetWazerNearby (int id, RoadMapPosition *point) {
   
   RoadMapArea screen_area;
   
   if (gs_iWazerNearbyState != 0)
      return;
   
   if (time(NULL) -  gs_WazerNearbyLastShown < WAZER_NEARBY_SLEEP_TIME)
      return;
   
   
   gs_WazerNearbyID = id;
   
   
   //calc best position (1 = N, 2 = W ...)
   roadmap_math_screen_edges(&screen_area);
   if (point->longitude < screen_area.west)
      gs_iWazerNearbyState = 2;
   else if (point->longitude > screen_area.east)
      gs_iWazerNearbyState = 4;
   else if (point->latitude > screen_area.north)
      gs_iWazerNearbyState = 1;
   else 
      gs_iWazerNearbyState = 3;
   
   roadmap_main_set_periodic(WAZER_NEARBY_TIME*1000, wazer_nearby_timeout);
}

static void OnUserShortClick (const char *name,
                              const char *sprite,
                              const char *image,
                              const RoadMapGpsPosition *gps_position,
                              const char *id) {
   RTUsers_Popup(&gs_CI.Users, id, RT_USERS_CENTER_NONE);
}


void OnAddUser(LPRTUserLocation pUI)
{
   char guid_crown[RT_USERID_MAXSIZE + 20];
   RoadMapDynamicString GUI_ID_CROWN;
   RoadMapDynamicString Image_Crown;
   static int initialized = 0;
   const char* mood_str;
   
   RoadMapGpsPosition   Pos;
   RoadMapPosition      Point;
   RoadMapDynamicString Group = roadmap_string_new( "Friends");
   RoadMapDynamicString GUI_ID= roadmap_string_new( pUI->sGUIID);
   RoadMapDynamicString Name  = roadmap_string_new( pUI->sName);
   RoadMapDynamicString Sprite= roadmap_string_new( "Friend");
   RoadMapDynamicString Image;
   
   if (!initialized) {
      realtime_prev_after_refresh =
               roadmap_screen_subscribe_after_refresh (realtime_after_refresh);
      initialized = 1;
   }
   
   if (pUI->iMood != -1){
         mood_str = roadmap_mood_to_string(pUI->iMood);
   		Image = roadmap_string_new( mood_str);
   		free((void *)mood_str);
   }
   else
   		Image = roadmap_string_new( "Friend");
   
   snprintf(guid_crown, sizeof(guid_crown), "%s_crown",pUI->sGUIID);
   GUI_ID_CROWN = roadmap_string_new(guid_crown);
   Image_Crown = roadmap_string_new( "crown");

   Pos.longitude  = pUI->position.longitude;
   Pos.latitude   = pUI->position.latitude;
   Pos.altitude   = 0;   //   Hieght
   Pos.speed      = (int)pUI->fSpeed;
   Pos.steering   = pUI->iAzimuth;

   roadmap_object_add( Group, GUI_ID, Name, Sprite, Image);
   roadmap_object_move( GUI_ID, &Pos);
   roadmap_object_set_action(GUI_ID, OnUserShortClick);
   
   if (pUI->iAddon == 1) {
      roadmap_object_add( Group, GUI_ID_CROWN, Name, Sprite, Image_Crown);
      roadmap_object_move( GUI_ID_CROWN, &Pos);
   }
   
   Point.latitude = Pos.latitude;
   Point.longitude = Pos.longitude;
   if (!roadmap_math_point_is_visible(&Point))
      SetWazerNearby(pUI->iID, &Point);
   else
      RemoveWazerNearby(); //reset sleep time
   

   roadmap_string_release( Group);
   roadmap_string_release( GUI_ID);
   roadmap_string_release( Name);
   roadmap_string_release( Sprite);
   roadmap_string_release( Image);
   roadmap_string_release( GUI_ID_CROWN);
   roadmap_string_release(Image_Crown);
}

void OnMoveUser(LPRTUserLocation pUI)
{
   RoadMapPosition      Point;
   RoadMapGpsPosition   Pos;
   RoadMapDynamicString GUI_ID_CROWN;
   RoadMapDynamicString GUI_ID   = roadmap_string_new( pUI->sGUIID);
   
   char guid_crown[RT_USERID_MAXSIZE + 20];
   snprintf(guid_crown, sizeof(guid_crown), "%s_crown",pUI->sGUIID);
   GUI_ID_CROWN = roadmap_string_new(guid_crown);

   Pos.longitude  = pUI->position.longitude;
   Pos.latitude   = pUI->position.latitude;
   Pos.altitude   = 0;   //   Hieght
   Pos.speed      = (int)pUI->fSpeed;
   Pos.steering   = pUI->iAzimuth;

   roadmap_object_move( GUI_ID, &Pos);
   
   if (pUI->iAddon == 1) {
      roadmap_object_move( GUI_ID_CROWN, &Pos);
   }
   
   Point.latitude = Pos.latitude;
   Point.longitude = Pos.longitude;
   
   if (roadmap_math_point_is_visible(&Point))
      RemoveWazerNearby();

   roadmap_string_release( GUI_ID);
   roadmap_string_release( GUI_ID_CROWN);
}

void OnRemoveUser(LPRTUserLocation pUI)
{
   RoadMapDynamicString GUI_ID_CROWN;

   RoadMapDynamicString   GUI_ID   = roadmap_string_new( pUI->sGUIID);
   
   char guid_crown[RT_USERID_MAXSIZE + 20];
   snprintf(guid_crown, sizeof(guid_crown), "%s_crown",pUI->sGUIID);
   GUI_ID_CROWN = roadmap_string_new(guid_crown);
   
   roadmap_object_remove ( GUI_ID);
   
   if (pUI->iAddon == 1) {
      roadmap_object_remove( GUI_ID_CROWN);
   }
   
   if (pUI->iID == gs_WazerNearbyID)
      RemoveWazerNearby();
   
   roadmap_string_release( GUI_ID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Report_Alert(int iAlertType, const char * szDescription, int iDirection, const char* szImageId, BOOL bForwardToTwitter )
{
   BOOL success;


   success = RTNet_ReportAlert(&gs_CI, iAlertType, szDescription, iDirection, szImageId, bForwardToTwitter, OnAsyncOperationCompleted_ReportAlert);

   if( !success)
   {
      roadmap_messagebox_timeout("Error", "Sending report failed",5);
      return FALSE;
   }

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_SendSMS(const char * szPhoneNumber)
{
   return RTNet_SendSMS(&gs_CI, szPhoneNumber, OnAsyncOperationCompleted_SendSMS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_TwitterConnect(const char * userName, const char *passWord)
{
   return RTNet_TwitterConnect(&gs_CI, userName, passWord, OnAsyncOperationCompleted_TwitterConnect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_FoursquareConnect(const char * userName, const char *passWord)
{
   return RTNet_FoursquareConnect(&gs_CI, userName, passWord, OnAsyncOperationCompleted_Foursquare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_FoursquareSearch(RoadMapPosition* coordinates)
{
   return RTNet_FoursquareSearch(&gs_CI, coordinates, OnAsyncOperationCompleted_Foursquare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_FoursquareCheckin(const char* vid)
{
   return RTNet_FoursquareCheckin(&gs_CI, vid, OnAsyncOperationCompleted_Foursquare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_TripServer_CreatePOI  (const char* name, RoadMapPosition* coordinates, BOOL overide){

   return RTNet_TripServer_CreatePOI(&gs_CI, name, coordinates,overide,  OnAsyncOperationCompleted_TripServer);
}



//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_TripServer_DeletePOI(const char * name)
{
   return RTNet_TripServer_DeletePOI(&gs_CI, name, OnAsyncOperationCompleted_TripServer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_TripServer_FindTrip  (RoadMapPosition*     coordinates){
	return RTNet_TripServer_FindTrip  (&gs_CI, coordinates, OnAsyncOperationCompleted_TripServer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Post_Alert_Comment(int iAlertId, const char * szCommentText,BOOL bForwardToTwitter)
{
   BOOL success;
   success = RTNet_PostAlertComment(&gs_CI, iAlertId, szCommentText, bForwardToTwitter, OnAsyncOperationCompleted_PostComment);

   if( !success)
   {
      roadmap_messagebox_timeout("Error", "Sending comment failed",5);
      return FALSE;
   }

   return success;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Alert_ReportAtLocation(int iAlertType, const char * szDescription, int iDirection, RoadMapGpsPosition   MyLocation)
{
   BOOL success = RTNet_ReportAlertAtPosition(&gs_CI, iAlertType, szDescription, iDirection, "", FALSE, &MyLocation, -1, -1, OnAsyncOperationCompleted_AlertReport);

   if (!success)
   {
      roadmap_messagebox_timeout ("Error", "Sending report failed",5);
   }

   return success;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_ReportMapProblem(const char*  szType, const char* szDescription, const RoadMapGpsPosition *MyLocation){
   BOOL success = RTNet_ReportMapProblem(&gs_CI, szType, szDescription, MyLocation, OnAsyncOperationCompleted_ReportMapProblem);

   if (!success)
   {
      roadmap_messagebox_timeout("Error", "Sending report failed",5);
   }

   return success;

}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Remove_Alert(int iAlertId){
   BOOL success;
   success = RTNet_RemoveAlert(&gs_CI, iAlertId, OnTransactionCompleted);

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void OnTransactionCompleted_ReportMarkers( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_ReportMarkers() - failed");

   if(gs_pfnOnExportMarkersResult)
      gs_pfnOnExportMarkersResult( rc, gs_CI.LastError);

   OnTransactionCompleted( ctx, rc);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnTransactionCompleted_ReportSegments( void* ctx, roadmap_result rc)
{
   if( succeeded != rc)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_ReportSegments() - failed");

   if (gs_pfnOnExportSegmentsResult) gs_pfnOnExportSegmentsResult(rc, gs_CI.LastError);
   OnTransactionCompleted( ctx, rc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   ReportOneMarker(char* packet, int iMarker)
{
   const char*         szName;
   const char*         szType;
   const char*         szDescription;
   const char*         szKeys[ED_MARKER_MAX_ATTRS];
   char*               szValues[ED_MARKER_MAX_ATTRS];
   int               iKeyCount;
   RoadMapPosition   position;
   int               steering;

   editor_marker_position(iMarker, &position, &steering);
   editor_marker_export(iMarker, &szName, &szDescription,
                        szKeys, szValues, &iKeyCount);
   szType = editor_marker_type(iMarker);

   return RTNet_ReportMarker(&gs_CI, szType,
                             position.longitude,
                             position.latitude,
                             steering,
                             szDescription,
                             iKeyCount,
                             szKeys,
                             (const char**)szValues,
                             OnTransactionCompleted_ReportMarkers,
                             packet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
char*   Realtime_Report_Markers(void)
{
   BOOL   success = FALSE;
   int   nMarkers;
   int   iMarker;
   char*   packet;
   char*   p;

   nMarkers = editor_marker_count ();

   if( !nMarkers)
      return NULL;

   packet = malloc(nMarkers * 1024); //TBD

   p = packet;
   *p = '\0';
   for (iMarker = 0; iMarker < nMarkers; iMarker++)
   {
      if (editor_marker_committed (iMarker)) continue;
      success = ReportOneMarker(p, iMarker);
      if (!success)
      {
         roadmap_messagebox_timeout("Error", "Sending markers failed",5);
         free(packet);
         return NULL;
      }

      p += strlen(p);
   }

   return packet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
char* Realtime_Report_Segments(void)
{
   BOOL   success = FALSE;
   char*   packet;
   char*   p;
   int required_size = 0;
   int count;
   int i;

   count = editor_line_get_count ();
   for (i = 0; i < count; i++)
   {
      required_size += RTNet_ReportOneSegment_MaxLength (i);
   }

   if (!required_size) return NULL;

   packet = malloc (required_size);

   p = packet;
   *p = '\0';
   for (i = 0; i < count; i++)
   {
      if (editor_line_committed (i)) continue;
      success = RTNet_ReportOneSegment_Encode (p, i);
      if (!success)
      {
         roadmap_messagebox_timeout("Error", "Sending update failed",5);
         free(packet);
         return NULL;
      }

      p += strlen(p);
   }


   return packet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   Realtime_Editor_ExportMarkers(PFN_LOGINTESTRES editor_cb)
{
   BOOL   success = FALSE;
   char*   packet;

   packet = Realtime_Report_Markers ();

   if (packet) {
      if (*packet) {
         if (gs_bWritingOffline) {
            Realtime_OfflineWrite (packet);
            editor_cb (TRUE, 0);
            success = TRUE;
         } else {
            gs_pfnOnExportMarkersResult = editor_cb;
            success = RTNet_GeneralPacket(&gs_CI, packet, OnTransactionCompleted_ReportMarkers);
         }
      }
      free(packet);
   }

   return success;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   Realtime_Editor_ExportSegments(PFN_LOGINTESTRES editor_cb)
{
   BOOL   success = FALSE;
   char*   packet;

   packet = Realtime_Report_Segments ();

   if (packet) {
      if (*packet) {
         if (gs_bWritingOffline) {
            Realtime_OfflineWrite (packet);
            editor_cb (TRUE, 0);
            success = TRUE;
         } else {
            gs_pfnOnExportSegmentsResult = editor_cb;
            success = RTNet_GeneralPacket(&gs_CI, packet, OnTransactionCompleted_ReportSegments);
         }
      }
      free(packet);
   }

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_SendTrafficInfo(int mode){

   BOOL success;
	//Not implemented yet
   return TRUE;

    success = RTNet_SendTrafficInfo(&gs_CI, mode, OnTransactionCompleted);
    return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_TrafficAlertFeedback(int iTrafficInfoId,
                                   int iValidity){
   return RTNet_TrafficAlertFeedback(&gs_CI, iTrafficInfoId, iValidity, OnTransactionCompleted);
}


BOOL Realtime_CreateAccount(const char*          userName,
                            const char*          passWord,
                            const char*          email,
                            BOOL                 send_updates){
   gs_CI.LastError = succeeded;
   gs_CI.eTransactionStatus = TS_Active;     	// We are in transaction now
   return RTNet_CreateAccount(&gs_CI, userName, passWord, email, send_updates, OnAsyncOperationCompleted_CreateAccount);
}

BOOL Realtime_UpdateProfile(const char*          userName,
                            const char*          passWord,
                            const char*          email,
                            BOOL                 send_updates){
   gs_CI.LastError = succeeded;
   gs_CI.eTransactionStatus = TS_Active;     	// We are in transaction now
   return RTNet_UpdateProfile(&gs_CI, userName, passWord, email, send_updates, OnAsyncOperationCompleted_CreateAccount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef J2ME
#define  MAP_DRAG_EVENT__PEEK_INTERVAL             (1000) // Milliseconds
#define  MAP_DRAG_EVENT__TERMINATION_THRESHOLD     (10)   // Seconds
#else
#define  MAP_DRAG_EVENT__PEEK_INTERVAL             (1000) // Milliseconds
#define  MAP_DRAG_EVENT__TERMINATION_THRESHOLD     (2)   // Seconds
#endif

static   BOOL     gs_bMapMoveTimerWasSet     = FALSE;
static   time_t   gs_TimeOfLastMapMoveEvent  = 0;

void OnTimePassedFromLastMapDragEvent(void)
{
   time_t   now                        = time(NULL);
   int      iSecondsPassedFromLastEvent= (int)(now - gs_TimeOfLastMapMoveEvent);

   if( iSecondsPassedFromLastEvent < MAP_DRAG_EVENT__TERMINATION_THRESHOLD)
      return;

   roadmap_main_remove_periodic( OnTimePassedFromLastMapDragEvent);
   gs_bMapMoveTimerWasSet = FALSE;

   if( gs_bRunning && gs_CI.bLoggedIn)
      Realtime_SendCurrentViewDimentions();
}

void OnMapMoved(void)
{
   if( !gs_bRunning)
   {
      assert(0);
      return;
   }

   if( !gs_CI.bLoggedIn)
      return;

   if( !gs_bMapMoveTimerWasSet)
   {
      roadmap_main_set_periodic( MAP_DRAG_EVENT__PEEK_INTERVAL, OnTimePassedFromLastMapDragEvent);
      gs_bMapMoveTimerWasSet = TRUE;
   }

   gs_TimeOfLastMapMoveEvent = time(NULL);
}

void OnDeviceEvent( device_event event, void* context)
{
   EConnectionStatus eNewStatus = gs_eConnectionStatus;
   int               iSecondsPassedFromLastGoodSession;

   roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - Event: %d (%s)", event, get_device_event_name(event));

   switch(event)
   {
      case device_event_network_connected:
      case device_event_RS232_connection_established:
         eNewStatus = CS_Connected;
         break;

      case device_event_network_disconnected:
         eNewStatus = CS_Disconnected;
         break;

      default:
         break;
   }

   // Anything changed?
   if( eNewStatus == gs_eConnectionStatus)
   {
      if( (CS_Connected == gs_eConnectionStatus) && !gs_bHadAtleastOneGoodSession)
      {
         roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - Just got connected AND is a virgin: INITIATING A NEW SESSION!");
         OnTimer_Realtime();
      }

      return;
   }

   // Update current status
   gs_eConnectionStatus = eNewStatus;

   // Connected?
   if( CS_Connected != gs_eConnectionStatus)
   {
      roadmap_log( ROADMAP_DEBUG,"OnDeviceEvent() - New state: Disconnected");

      if( !gs_bWasStoppedAutoamatically)
      {
         roadmap_log( ROADMAP_INFO, "OnDeviceEvent() - !!! GOT DISCONNECTED !!! REALTIME SERVICE IS STOPPING AUTOMATICALLY !!!");
         Realtime_Stop( FALSE);
         gs_bWasStoppedAutoamatically = TRUE;
      }

      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - New state: Connected");

   if( gs_bWasStoppedAutoamatically)
   {
      roadmap_log( ROADMAP_INFO, "OnDeviceEvent() - !!! REALTIME SERVICE AUTO-RESTART !!!");
      Realtime_Start();
      gs_bWasStoppedAutoamatically = FALSE;
      return;
   }

   // Verify we are not currently in the transaction:
   if( TS_Idle != gs_CI.eTransactionStatus)
      return;

   // Check time threshold:
   iSecondsPassedFromLastGoodSession = (int)(time(NULL) - gs_ST.timeLastGoodSession);
   if( !gs_bHadAtleastOneGoodSession || (RT_MAX_SECONDS_BETWEEN_GOOD_SESSIONS < iSecondsPassedFromLastGoodSession))
   {
      roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - %d seconds passed from last-good-session; INITIATING A NEW SESSION!", iSecondsPassedFromLastGoodSession);
      OnTimer_Realtime();
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
int RealTimeLoginState(void){
   if (gs_CI.bLoggedIn && (!is_network_error(gs_CI.LastError)) && (!is_realtime_error(gs_CI.LastError)))
      return 1;
   else
      return 0;

}

BOOL Realtime_IsLoggedIn( void )
{
	return gs_CI.bLoggedIn;
}

const char *RealTime_GetUserName(){
   return roadmap_config_get( &RT_CFG_PRM_NAME_Var);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void   RealTime_Auth (void) {

   char   Command [MESSAGE_MAX_SIZE__Auth];

   //if (gs_CI.bLoggedIn) {

      RTNet_Auth_BuildCommand (Command,
                               gs_CI.iServerID,
                               gs_CI.ServerCookie,
                               gs_LoginDetails.username,
                               RT_DEVICE_ID,
                               roadmap_start_version ());
   /*} else {

      char  Name    [(RT_USERNM_MAXSIZE * 2) + 1];
      const char* szName      = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
      if( HAVE_STRING(szName))
         PackNetworkString( szName, Name, RT_USERNM_MAXSIZE);
      else
         Name[0] = '\0';

      RTNet_Auth_BuildCommand (Command,
                               -1,
                               "",
                               Name,
                               RT_DEVICE_ID,
                               roadmap_start_version ());
   }
*/
   Realtime_OfflineWrite (Command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL LoginDetailsChanged()
{
   char  Name    [(RT_USERNM_MAXSIZE * 2) + 1];
   char  Password[(RT_USERPW_MAXSIZE * 2) + 1];
   char  Nickname[(RT_USERNK_MAXSIZE * 2) + 1];


   const char* szName      = gs_LoginDetails.username;
   const char* szPassword  = gs_LoginDetails.pwd;
   const char* szNickname  = gs_LoginDetails.nickname;

   // If we are not logged-in - we do not have any details cached:
   if( !gs_CI.bLoggedIn)
      return FALSE;  // Question is not applicable

   // Convert all strings to 'network strings'     (',' == "\,")
   if( HAVE_STRING(szName))
      PackNetworkString( szName, Name, RT_USERNM_MAXSIZE);
   else
      Name[0] = '\0';
   if( HAVE_STRING(szPassword))
      PackNetworkString( szPassword, Password, RT_USERPW_MAXSIZE);
   else
      Password[0] = '\0';
   if( HAVE_STRING(szNickname))
      PackNetworkString( szNickname, Nickname, RT_USERNK_MAXSIZE);
   else
      Nickname[0] = '\0';

   // Compare
   if(strcmp( Name,     gs_CI.UserNm)  ||
      strcmp( Password, gs_CI.UserPW)  ||
      strcmp( Nickname, gs_CI.UserNk))
      return TRUE;

   return FALSE;
}

static void TestLoginDetailsCompleted( BOOL bRes )
{
   PFN_LOGINTESTRES  pfn   = gs_pfnOnLoginTestResult;
   gs_pfnOnLoginTestResult = NULL;
   gs_CI.eTransactionStatus= TS_Idle;

   if ( pfn )
   {
	   pfn( bRes, gs_CI.LastError);
   }

      if ( !roadmap_screen_refresh() )
         roadmap_screen_redraw();
}

void OnTransactionCompleted_TestLoginDetails_Login( void* ctx, roadmap_result rc )
{
   TestLoginDetailsCompleted( succeeded == rc );

   if( gs_CI.bLoggedIn )
   {
	   roadmap_main_set_periodic( 10, Realtime_LoginCallback );
   }

   if (rc == succeeded)
   {
      Realtime_SessionDetailsSave();
      SendAllMessagesTogether( FALSE, TRUE /* Called After Login */);
   }
   else
   {
	  roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_TestLoginDetails_Login() - 'Login' failed");
   }
}

/***********************************************************
 *  Name        : TestLoginMain()
 *  Purpose     : Executes the transaction for checking the login if not logged in now
 *  Params      : [in]  - none
 *              : [out] - Always TRUE  - FFU
 *  Returns     :
 *  Notes       :
 */
static BOOL TestLoginMain()
{
   // Catch the process flag:
   BOOL bRes = TRUE;
   gs_CI.eTransactionStatus= TS_Active;
   gs_CI.LastError = succeeded;
   if( Login( OnTransactionCompleted_TestLoginDetails_Login, FALSE /* Auto register? */))
   {

      gs_CI.eTransactionStatus = TS_Active;     	// We are in transaction now
      RTUsers_ResetUpdateFlag( &gs_CI.Users );   	// User[i].updated = FALSE
      bRes = TRUE;
   }
   else
   {
      gs_CI.LastError = err_failed;
      bRes = FALSE;
   }
   return bRes;
}

/***********************************************************
 *  Name        : TestLoginDetails
 *  Purpose     : Main login flow function - tests the login details according to the system state
 *  				The result of the testing is handled in the callback provided by Realtime_VerifyLoginDetails
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns     : Always TRUE  - FFU
 *  Notes       :
 */
static BOOL TestLoginDetails()
{
   BOOL bRes = TRUE;
   if( gs_CI.bLoggedIn && !LoginDetailsChanged() )
   {  /* Currently logged in: If the details did not change - do nothing*/
	  roadmap_log( ROADMAP_DEBUG, "TestLoginDetails() - Login details did not changed - nothing to do" );
   }
   else
   {  /* Currently not logged in: Try to login to server */
      if( NameAndPasswordAlreadyFailedAuthentication() )
      {
         bRes = FALSE;
         TestLoginDetailsCompleted( FALSE );
      }
      else
      {
		 Realtime_ResetLoginState( TRUE /* Redraw */ );
 		 bRes = TestLoginMain();
      }
   }

   /* If we now that the result is false - accomplish the result, otherwise
    * wait for the server answer
    */
   if ( bRes == FALSE )
   {
	   TestLoginDetailsCompleted( bRes );
   }

   return bRes;
}

/***********************************************************
 *  Name        : Realtime_SaveLoginInfo
 *  Purpose     : Saves login info to the configuration based on the login data in the transaction context
 *
 *  Params      : [in]  - none
 *              : [out] - none
 *  Returns     : void
 *  Notes       :
 */
void Realtime_SaveLoginInfo( void )
{
    roadmap_config_set( &RT_CFG_PRM_NAME_Var,    gs_CI.UserNm );
    roadmap_config_set( &RT_CFG_PRM_PASSWORD_Var,gs_CI.UserPW );
    roadmap_config_set( &RT_CFG_PRM_NKNM_Var,    gs_CI.UserNk );
}

void Realtime_Relogin(void){
   Realtime_ResetLoginState( TRUE /* Redraw */ );
   Realtime_LoginDetailsInit();
   Login( OnAsyncOperationCompleted_Login, TRUE /* Auto register? */);

}

/***********************************************************
 *  Name        : Realtime_VerifyLoginDetails
 *  Purpose     : API for the asynchronous login test.
 *
 *  Params      : [in]  pfn - the callback executed on the login result
 *              : [out] - none
 *  Returns     : Always TRUE  - FFU
 *  Notes       :
 */
 BOOL Realtime_VerifyLoginDetails( PFN_LOGINTESTRES pfn )
 {
   if( gs_bInitialized && pfn && !gs_pfnOnLoginTestResult)
   {
	   gs_pfnOnLoginTestResult = pfn;
	   gs_bSaveLoginInfoOnSuccess = TRUE;
	   TestLoginDetails();
   }
   else
   {
 	   roadmap_log( ROADMAP_ERROR, "Wrong Realtime engine state: ( gs_bInitialized: %d, pfn: %x, gs_pfnOnLoginTestResult: %x )",
			   gs_bInitialized, pfn, gs_pfnOnLoginTestResult );

   }
   return TRUE;
}

void OnAsyncOperationCompleted_ReportOnNavigation( void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_ReportOnNavigation() - 'MapDisplayed' was sent successfully");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_ReportOnNavigation(POST) - 'MapDisplayed' had failed");

   OnTransactionCompleted( ctx, rc);
}

BOOL Realtime_ReportOnNavigation( const RoadMapPosition* cordinates, address_info_ptr aiptr)
{
   address_info   ai = (*aiptr);
   BOOL           bRes;

   NULLSTRING_TO_EMPTYSTRING(ai.state)
   NULLSTRING_TO_EMPTYSTRING(ai.country)
   NULLSTRING_TO_EMPTYSTRING(ai.city)
   NULLSTRING_TO_EMPTYSTRING(ai.street)
   NULLSTRING_TO_EMPTYSTRING(ai.house)

   if( !gs_bInitialized)
   {
//      assert(0);
      return FALSE;
   }

   if( !gs_bRunning)
   {
      roadmap_log( ROADMAP_ERROR, "Realtime_ReportOnNavigation() - Realtime service is currently disabled; Exiting method");
      return FALSE;
   }

   bRes = RTNet_NavigateTo(   &gs_CI,
                              cordinates,
                              &ai,
                              OnAsyncOperationCompleted_ReportOnNavigation);
   if( bRes)
      roadmap_log( ROADMAP_DEBUG, "Realtime_ReportOnNavigation()");
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_ReportOnNavigation(PRE) - 'RTNet_NavigateTo()' had failed");

   return bRes;
}

void OnAsyncOperationCompleted_RequestRoute (void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_RequestRoute() - succeeded");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_RequestRoute() - failed");

   OnTransactionCompleted( ctx, rc);
}

BOOL Realtime_RequestRoute(int						iRoute,
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
									BOOL						bReRoute)
{

   BOOL bRes = RTNet_RequestRoute(&gs_CI,
   										 iRoute,
   										 iType,
   										 iTripId,
   										 iAltId,
   										 nMaxRoutes,
   										 nMaxSegments,
   										 nMaxPoints,
   										 posFrom,
   										 iFrSegmentId,
   										 iFrNodeId,
   										 szFrStreet,
   										 bFrAllowBidi,
   										 posTo,
   										 iToSegmentId,
   										 iToNodeId,
   										 szToStreet,
   										 szToStreetNumber,
   										 szToCity,
   										 szToState,
   										 bToAllowBidi,
   										 nOptions,
   										 iOptionNumeral,
   										 bOptionValue,
   										 iTwitterLevel,
   										 bReRoute,
                              	 OnAsyncOperationCompleted_RequestRoute);
   if( bRes)
      roadmap_log( ROADMAP_DEBUG, "Realtime_RequestRoute()");
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_RequestRoute() - failed");

   return bRes;
}


BOOL Realtime_GetGeoConfig(const RoadMapPosition   *pGPSPosition, wst_handle websvc){
   return RTNet_GetGeoConfig(&gs_CI,websvc,pGPSPosition, OnAsyncOperationCompleted_GetGeoConfig);
}

static void OnAsyncOperationCompleted_SelectRoute (void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_SelectRoute() - succeeded");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_SelectRoute() - failed");

   OnTransactionCompleted( ctx, rc);
}

static void OnAsyncOperationCompleted_CollectBonus (void* ctx, roadmap_result rc)
{
   if( succeeded == rc)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_CollectBonus() - succeeded");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_CollectBonus() - failed");

   OnTransactionCompleted( ctx, rc);
}
BOOL	Realtime_SelectRoute	(int							iRoute,
								 	 int							iAltId)
{
	BOOL bRes = RTNet_SelectRoute (&gs_CI,
   										 iRoute,
   										 iAltId,
                              	 OnAsyncOperationCompleted_SelectRoute);

   if( bRes)
      roadmap_log( ROADMAP_DEBUG, "Realtime_SelectRoute()");
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_SelectRoute() - failed");

   return bRes;
}

BOOL Realtime_CollectBonus(int                  iId,
                           int                  iToken,
                           BOOL                 bForwardToTwitter){
   
   BOOL bRes = RTNet_CollectBonus(&gs_CI,
                                  iId,
                                  iToken,
                                  bForwardToTwitter,
                                  OnAsyncOperationCompleted_CollectBonus);

   if( bRes)
      roadmap_log( ROADMAP_DEBUG, "Realtime_CollectBonus()");
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_CollectBonus() - failed bRes=%d", bRes);

   return bRes;
}

static BOOL keyboard_callback(int         exit_code,
                              const char* value,
                              void*       context)
{
    if( dec_ok != exit_code)
        return TRUE;

    if( !value || !(*value))
      return FALSE;

    return Realtime_SendSMS( value);
}


///////////////////////////////////////////////////////////////////////////////////////////
static void recommend_waze_dialog_callbak(int exit_code, void *context){

    if (exit_code != dec_yes)
         return;

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN))
    ShowEditbox(roadmap_lang_get("Phone number"), "",
            keyboard_callback, NULL, EEditBoxEmptyForbidden | EEditBoxNumeric );
#else
   ssd_show_keyboard_dialog(  roadmap_lang_get( "Phone number"),
                              NULL,
                              keyboard_callback,
                              NULL);
#endif

}

void RecommentToFriend(){

   ssd_confirm_dialog("Recommend Waze to a friend", "To recommend to a friend, please enter their phone number. Send?", FALSE, recommend_waze_dialog_callbak,  NULL);
}


void Realtime_DumpOffline (void)
{
   ebuffer      Packet;
   int         iPoint;
   int         iNode;
   int         iToggle;
   char*       Buffer;
   RTPathInfo   pi;
   RTPathInfo   *pOrigPI;

   Realtime_OfflineOpen (editor_sync_get_export_path (),
                         editor_sync_get_export_name ());

   ebuffer_init( &Packet);

   pOrigPI = editor_track_report_begin_export (1);

   if (pOrigPI && pOrigPI->num_nodes + pOrigPI->num_points + pOrigPI->num_update_toggles > 0)
   {
      Buffer               = ebuffer_alloc( &Packet,
                                MESSAGE_MAX_SIZE__AllTogether__dynamic(RTTRK_GPSPATH_MAX_POINTS,
                                                                       RTTRK_NODEPATH_MAX_POINTS,
                                                                       RTTRK_CREATENEWROADS_MAX_TOGGLES));

      pi = *pOrigPI;

      for (iToggle = 0;
           iToggle < pOrigPI->num_update_toggles;
           iToggle += RTTRK_CREATENEWROADS_MAX_TOGGLES)
      {
         pi.num_update_toggles = pOrigPI->num_update_toggles - iToggle;
         if (pi.num_update_toggles > RTTRK_CREATENEWROADS_MAX_TOGGLES)
            pi.num_update_toggles = RTTRK_CREATENEWROADS_MAX_TOGGLES;
         pi.update_toggle_times = pOrigPI->update_toggle_times + iToggle;

         gs_pPI = &pi;
         if (SendMessage_CreateNewRoads (Buffer))
         {
            Realtime_OfflineWrite (Buffer);
         }
      }

      for (iPoint = 0;
           iPoint < pOrigPI->num_points;
           iPoint += RTTRK_GPSPATH_MAX_POINTS)
      {
         pi.num_points = pOrigPI->num_points - iPoint;
         if (pi.num_points > RTTRK_GPSPATH_MAX_POINTS)
            pi.num_points = RTTRK_GPSPATH_MAX_POINTS;
         pi.points = pOrigPI->points + iPoint;

         gs_pPI = &pi;
         if (SendMessage_GPSPath (Buffer))
         {
            Realtime_OfflineWrite (Buffer);
         }
      }
      for (iNode = 0;
           iNode < pOrigPI->num_nodes;
           iNode += RTTRK_NODEPATH_MAX_POINTS)
      {
         pi.num_nodes = pOrigPI->num_nodes - iNode;
         if (pi.num_nodes > RTTRK_NODEPATH_MAX_POINTS)
            pi.num_nodes = RTTRK_NODEPATH_MAX_POINTS;
         pi.nodes = pOrigPI->nodes + iNode;

         gs_pPI = &pi;
         if (SendMessage_NodePath (Buffer))
         {
            Realtime_OfflineWrite (Buffer);
         }
      }
      ebuffer_free (&Packet);
   }
   editor_track_report_conclude_export (1);

   gs_bWritingOffline = TRUE;
   editor_report_markers ();
   editor_report_segments ();
   gs_bWritingOffline = FALSE;

   Realtime_OfflineClose ();
}

int RealTime_GetMyTotalPoints(){
	return gs_CI.iMyTotalPoints;
}

int RealTime_GetMyRanking(){
	return gs_CI.iMyRanking;
}

void RealTime_SetMapDisplayed(BOOL should_send) {
	gs_bShouldSendMapDisplayed = should_send;
}

void Realtime_SetLoginUsername( const char* username )
{
	roadmap_login_set_username( &gs_LoginDetails, username );
}

void Realtime_SetLoginPassword( const char* pwd )
{
	roadmap_login_set_pwd( &gs_LoginDetails, pwd );
}

void Realtime_SetLoginNickname( const char* nickname )
{
	roadmap_login_set_nickname( &gs_LoginDetails, nickname );
}

void Realtime_LoginDetailsReset( void )
{
	gs_LoginDetails.username[0] = 0;
	gs_LoginDetails.pwd[0] = 0;
	gs_LoginDetails.nickname[0] = 0;
	gs_LoginDetails.email[0] = 0;
}

void Realtime_LoginDetailsInit( void )
{
   const char* szName         = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
   const char* szPassword     = roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var);
   const char* szNickname     = roadmap_config_get( &RT_CFG_PRM_NKNM_Var);

   Realtime_SetLoginUsername( szName );
   Realtime_SetLoginPassword( szPassword );
   Realtime_SetLoginNickname( szNickname );
}

static void Realtime_SessionDetailsClear( void )
{
   roadmap_config_set(&RT_CFG_PRM_SERVER_ID_Var, RT_CFG_PRM_SERVER_ID_Default);
   roadmap_config_set(&RT_CFG_PRM_SERVER_COOKIE_Var, RT_CFG_PRM_SERVER_COOKIE_Default);
   roadmap_config_save(1);
}

static void Realtime_SessionDetailsInit( void )
{
   int iServerID              = roadmap_config_get_integer( &RT_CFG_PRM_SERVER_ID_Var);
   const char* ServerCookie   = roadmap_config_get( &RT_CFG_PRM_SERVER_COOKIE_Var);
   BOOL tryLastSessionLogin   = TRUE;
 
   if (iServerID != -1)
      gs_CI.iServerID = iServerID;
   else
      tryLastSessionLogin = FALSE;
 
   if (HAVE_STRING(ServerCookie))
      strncpy_safe(gs_CI.ServerCookie, ServerCookie, RTNET_SERVERCOOKIE_MAXSIZE+1)
   else
      tryLastSessionLogin = FALSE;
      
   /*  uncomment to try reconnect last session after launch
   if (tryLastSessionLogin)
      gs_CI.bLoggedIn = TRUE;
      */
}

static void Realtime_SessionDetailsSave( void )
{
   roadmap_config_set_integer(&RT_CFG_PRM_SERVER_ID_Var, gs_CI.iServerID);
   roadmap_config_set(&RT_CFG_PRM_SERVER_COOKIE_Var, gs_CI.ServerCookie);
   roadmap_config_save(1);
}

char* Realtime_GetServerCookie(void)
{
   return &gs_CI.ServerCookie[0];
}

int  Realtime_AddonState(void) {
   return gs_CI.iMyAddon;
}
