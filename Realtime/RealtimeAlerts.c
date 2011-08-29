/* RealtimeAlerts.c - Manage real time alerts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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

#include "../roadmap_main.h"
#include "../roadmap_path.h"
#include "../roadmap_factory.h"
#include "../roadmap_config.h"
#include "../roadmap_navigate.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_types.h"
#include "../roadmap_alerter.h"
#include "../roadmap_db_alert.h"
#include "../roadmap_display.h"
#include "../roadmap_time.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_lang.h"
#include "../roadmap_trip.h"
#include "../roadmap_res.h"
#include "../roadmap_layer.h"
#include "../roadmap_square.h"
#include "../roadmap_locator.h"
#include "../roadmap_line_route.h"
#include "../roadmap_line.h"
#include "../roadmap_gps.h"
#include "../roadmap_mood.h"
#include "../roadmap_softkeys.h"
#include "../roadmap_social.h"
#include "../editor/db/editor_street.h"
#include "../roadmap_camera_image.h"
#include "../roadmap_recorder.h"
#include "../roadmap_jpeg.h"
#include "../roadmap_res_download.h"
#include "../roadmap_social_image.h"
#include "../navigate/navigate_main.h"
#include "../roadmap_map_settings.h"
#include "../roadmap_general_settings.h"
#include "../roadmap_groups.h"
#include "../roadmap_message_ticker.h"
#include "../roadmap_analytics.h"

#include "RealtimeAlerts.h"
#include "RealtimeAlertsList.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeAlertCommentsList.h"
#include "RealtimeNet.h"
#include "RealtimePrivacy.h"
#include "Realtime.h"
#include "RealtimeExternalPoi.h"
#include "RealtimePopUp.h"

#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_checkbox.h"
#include "../ssd/ssd_contextmenu.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_choice.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_menu.h"
#include "../ssd/ssd_keyboard_dialog.h"
#include "../ssd/ssd_bitmap.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_entry.h"
#include "../ssd/ssd_popup.h"
#include "../ssd/ssd_separator.h"
#include "../ssd/ssd_segmented_control.h"
#include "../ssd/ssd_progress_msg_dialog.h"
#include "../ssd/ssd_confirm_dialog.h"
#include "../ssd/ssd_separator.h"
#include "roadmap_editbox.h"
#include "tts_apptext.h"

#ifdef IPHONE
#include "iphone/roadmap_list_menu.h"
#endif //IPHONE

static RTAlerts gAlertsTable;
static int gIterator;
static int gIdleScrolling;
static int gThumbsUpScrolling = FALSE;
static int gCurrentAlertId;
static int gCurrentCommentId;
static char gCurrentImageId[ROADMAP_IMAGE_ID_BUF_LEN] = "";
static char gCurrentVoiceId[ROADMAP_VOICE_ID_BUF_LEN] = "";
static char *gCurrentImagePath;
static BOOL gPopAllTimerActive;
static BOOL gCentered;

static int gState =STATE_NONE;

static int gGroupState =STATE_OLD;
static zoom_t gSavedZoom = -1;
static RoadMapPosition gSavedPosition;
static const char *gSavedFocus = NULL;
static int gMinimizeState = -1;
static int gPopUpType = -1;
static int gDisplayClearedTimeStamp = 0;

static int gMapProblems[MAX_MAP_PROBLEM_COUNT] ; // will hold the list of map problems
static int gMapProblemsCount = 0 ; // will hold the number of map problems
static BOOL gMapProblemsInit = FALSE;
static BOOL gIgnoreAlertMaxDist = TRUE;

#define COMMENT_POPUP_TIMER    15
#define PING_POPUP_TIMER       15
#define THUMBS_UP_POPUP_TIMER  8
#define GROUP_POPUP_TIMER      12

static int g_alert_popup_seconds;
static BOOL g_alert_popup_active;
static int g_thumbs_up_seconds;
static int g_comments_dlg_seconds;
static int g_alert_ahead_seconds;
static int g_ping_dlg_seconds;

static int g_traffic_info_time_stamp = -1;

#define MAX_CATEGORIES    5
static int gOnRoadHazardCategories[MAX_CATEGORIES] ;
static int gOnRoadHazardCategoriesCount = 0 ;
static int gOnShoulderHazardCategories[MAX_CATEGORIES] ;
static int gOnShoulderHazardCategoriesCount = 0 ;
static int gWeatherHazardCategories[MAX_CATEGORIES] ;
static int gWeatherHazardCategoriesCount = 0 ;


static RTThumbsUpTable gThumbsUpTable;

#ifdef IPHONE
#define VOICE_FILENAME "voice_capture.caf"
#elif defined(ANDROID)
#define VOICE_FILENAME "voice_capture.mp4"
#else
#define VOICE_FILENAME "voice_capture.caf"
#endif

#define POP_UP_COMMENT 1
#define POP_UP_ALERT   2

#ifdef ANDROID
#define ALERT_IMAGE_CAPTURE_ASYNC
#endif


static RoadMapConfigDescriptor LastCommentAlertIDCfg =
                        ROADMAP_CONFIG_ITEM("Alerts", "Last comment alert ID");

static RoadMapConfigDescriptor LastJamStreetName =
                        ROADMAP_CONFIG_ITEM("Alerts", "Last Jam Street name");

static RoadMapConfigDescriptor AllowPoliceSubtypeCfg =
                        ROADMAP_CONFIG_ITEM("Report", "Allow Police subtypes");

static RoadMapConfigDescriptor LastCommentIDCfg =
                        ROADMAP_CONFIG_ITEM("Alerts", "Last comment ID");

static RoadMapConfigDescriptor RoadMapConfigMapProblemOptions =
                        ROADMAP_CONFIG_ITEM("General", "Map Problem Options(new)");


static RoadMapConfigDescriptor RoadMapConfigMaxAlertPopDist =
                        ROADMAP_CONFIG_ITEM("Alerts", "Max Dist");

static RoadMapConfigDescriptor RoadMapConfigSecondsToPopup =
                        ROADMAP_CONFIG_ITEM("Alerts", "Seconds to popup");

static RoadMapConfigDescriptor RoadMapConfigSmallAlertScaleFactor =
                        ROADMAP_CONFIG_ITEM("Alerts", "Alert scale factor");

static RoadMapConfigDescriptor RoadMapConfigOnRoadHazardCategories =
                        ROADMAP_CONFIG_ITEM("Harard", "On road categories");

static RoadMapConfigDescriptor RoadMapConfigOnShoulderHazardCategories =
                        ROADMAP_CONFIG_ITEM("Harard", "On shoulder categories");

static RoadMapConfigDescriptor RoadMapConfigWeatherHazardCategories =
                        ROADMAP_CONFIG_ITEM("Harard", "Weather categories");

static  char*  MAP_PROBLEMS_OPTION[] = { MAP_PROBLEM_INCORRECT_TURN,   MAP_PROBLEM_INCORRECT_ADDRESS , MAP_PROBLEM_INCORRECT_ROUTE ,
											MAP_PROBLEM_INCORRECT_MISSING_ROUNDABOUT , MAP_PROBLEM_INCORRECT_GENERAL_ERROR,MAP_PROBLEM_TURN_NOT_ALLOWED,MAP_PROBLEM_INCORRECT_JUNCTION,MAP_PROBLEM_MISSING_BRIDGE_OVERPASS,
												MAP_PROBLEM_WRONG_DRIVING_DIRECTIONS , MAP_PROBLEM_MISSING_EXIT,MAP_PROBLEM_MISSING_ROAD };
static void RTAlerts_ReleaseImagePath(void);
static void OnAlertAdd(RTAlert *pAlert);
static void OnAlertRemove(void);
static void RTAlerts_Comment_PopUp(RTAlertComment *Comment, RTAlert *Alert);
static void RTAlerts_popup_PingWazer(RTAlert *pAlert);
static void RTAlerts_Comment_PopUp_Hide(void);
static void RTAlerts_popup_alert(int alertId, int iCenterAround);
static void RTAlerts_Show_Image( int alertId, RoadMapImage image );
static void RTinitMapProblems(void);
static void RTinitOnRoadHazardCategories(void);
static void RTinitOnShoulderHarzardCategories(void);
static void RTinitWeatherHarzardCategories(void);

static void report_abuse(void);
static void CreateAlertObject(RTAlert *pAlert);
static void DeleteAlertObject(RTAlert *pAlert);
static int on_groups_callbak (SsdWidget widget, const char *new_value);
static BOOL showThumbsUp(void);
static void close_ThumbsUp_Dlg(void);
BOOL RTAlerts_IsThumbsUpScrolling(void);
static void ThumbsUpDel(ThumbsUp *thumbsUp);
static ThumbsUp *getThumbsUp(void);
static void RTAlerts_Alert_PopUp_Timer(void);

#define CENTER_NONE				-1
#define CENTER_ON_ALERT       1
#define CENTER_ON_ME 		   2
#define MINIMUM_DISTANCE_TO_CHECK 80
#ifdef TOUCH_SCREEN
static void report_dialog(int iAlertType);
#endif

BOOL RTAlerts_is_square_dependent(void);
int RTAlerts_get_priority(void);
BOOL RTAlerts_distance_check(RoadMapPosition gps_pos);
roadmap_alerter_location_info *  RTAlerts_get_location_info(int record);

#ifndef TOUCH_SCREEN
// Context menu:
typedef enum real_time_context_menu_items
{
   rt_cm_add_comments,
   rt_cm_view_comments,
   rt_cm_view_image,
   rt_cm_report_abuse,
   rt_cm_cancel,
   rt_cm_send_thumbs_up,
   rt_cm__count,
   rt_cm__invalid
}  real_time_context_menu_items;

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Add comment",       rt_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "View comments",     rt_cm_view_comments),
   SSD_CM_INIT_ITEM  ( "View image",        rt_cm_view_image),
   SSD_CM_INIT_ITEM  ( "Report abuse",      rt_cm_report_abuse),
   SSD_CM_INIT_ITEM  ( "Thumbs up",    rt_cm_send_thumbs_up),
   SSD_CM_INIT_ITEM  ( "Cancel",            rt_cm_cancel),
};
static   BOOL g_context_menu_is_active= FALSE;

static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

#endif


BOOL RTAlerts_is_square_dependent(void);
int RTAlerts_get_priority(void);
BOOL RTAlerts_distance_check(RoadMapPosition gps_pos);
roadmap_alerter_location_info *  RTAlerts_get_location_info(int record);

typedef struct
{
	int alertId;
} AttachmentDownloadCbContext;


typedef struct tag_upload_context{
	int iType;
	int iSubType;
	char* desc;
	int iDirection;
	char *group;
}upload_Image_context;


static void continue_report_after_image_upload(void * context);

roadmap_alert_provider RoadmapRealTimeAlertProvider =
{ "RealTimeAlerts", RTAlerts_Count, RTAlerts_Get_Id, RTAlerts_Get_Position,
        RTAlerts_Get_Speed, RTAlerts_Get_Map_Icon, RTAlerts_Get_Alert_Icon,
        RTAlerts_Get_Warn_Icon, RTAlerts_Get_Distance, RTAlerts_Get_Sound,
        RTAlerts_Is_Alertable, RTAlerts_Get_String, RTAlerts_Is_Cancelable, Rtalerts_Delete,
        RTAlerts_Check_Same_Street, NULL,RTAlerts_is_square_dependent,
        RTAlerts_get_location_info,RTAlerts_distance_check, RTAlerts_get_priority, RTAlerts_Get_Additional_String,  RTAlerts_Can_Send_Thumbs_up, Rtalerts_Thumbs_Up, RTAlerts_ShowDisrance, RTAlerts_HandleAlert, NULL, RTAlertsOnAlerterStart, RTAlertsOnAlerterStop};




static const char * Get_LastJamStreetName(void){
   return roadmap_config_get(&LastJamStreetName);
}

static void Set_LastJamStreetName(const char *name){
   roadmap_config_set(&LastJamStreetName, name);
   roadmap_config_save(0);
}
/**
 * calculate the delta between two azymuths
 * @param az1, az2 - the azymuths
 * @return the delta
 */
static int azymuth_delta(int az1, int az2)
{

    int delta;

    delta = az1 - az2;

    while (delta > 180)
        delta -= 360;
    while (delta < -180)
        delta += 360;

    return delta;
}

/**
 * Initialize the an alert structure
 * @param pAlert - pointer to the alert
 * @return None
 */
void RTAlerts_Alert_Init(RTAlert *pAlert)
{
    pAlert->iID = -1;
    pAlert->iLongitude = 0;
    pAlert->iLatitude = 0;
    pAlert->iAzymuth = 0;
    pAlert->iType = 0;
    pAlert->iSubType = -1;
    pAlert->iRank = 0;
    pAlert->iMood = 0;
    pAlert->iReportTime = 0L;
    pAlert->iNode1 = 0;
    pAlert->iNode2 = 0;
    pAlert->iDistance = 0;
    pAlert->bAlertByMe = FALSE;
    pAlert->bAlertIsOnRoute = FALSE;
    pAlert->bPingWazer = FALSE;
    pAlert->iReportedElapsedTime = 0;
    pAlert->iDisplayTimeStamp = 0;
    pAlert->iGroupRelevance = GROUP_RELEVANCE_NONE;
    memset(pAlert->sReportedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(pAlert->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
    memset(pAlert->sLocationStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sNearStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sImageIdStr, 0, RT_ALERT_IMAGEID_MAXSIZE);
    memset(pAlert->sVoiceIdStr, 0, RT_ALERT_VOICEID_MAXSIZE);
    memset(pAlert->sStreetStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sCityStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sAddOnName,0, sizeof(pAlert->sAddOnName));
    memset(pAlert->sAlertType,0, sizeof(pAlert->sAlertType));

    pAlert->pMenuAddOnName = NULL;
    pAlert->pMapAddOnName = NULL;
    pAlert->pIconName = NULL;
    pAlert->pMapIconName = NULL;
    pAlert->pAlertTitle = NULL;

    pAlert->iNumComments = 0;
    pAlert->Comment = NULL;
    pAlert->location_info.line_id = -1;
    pAlert->location_info.square_id = ALERTER_SQUARE_ID_UNINITIALIZED;
    pAlert->location_info.time_stamp = -1;
    memset(pAlert->sFacebookName, 0, RT_ALERT_FACEBOOK_USERNM_MAXSIZE);
    memset(pAlert->sGroup, 0, RT_ALERT_GROUP_MAXSIZE);
    memset(pAlert->sGroupIcon, 0, RT_ALERT_GROUP_ICON_MAXSIZE);
    pAlert->bShowFacebookPicture = FALSE;
    pAlert->iGroupRelevance = GROUP_RELEVANCE_NONE;
    pAlert->bArchive = FALSE;
    pAlert->iPopUpPriority = 0;
    pAlert->bPopedUp = FALSE;
    pAlert->iNumThumbsUp = 0;
    pAlert->bThumbsUpByMe = FALSE;
    pAlert->bIsAlertable = TRUE;
    pAlert->bAlertHandled = FALSE;
    pAlert->iNumViewed = 0;

}

BOOL RTAlerts_IsIdleAlertScrolling(void){


   return gIdleScrolling;
}

static void InitVoiceRecording(void){
   const char *path = roadmap_path_first ("config");
   const char *file_name = VOICE_FILENAME;
   if (roadmap_file_exists(path, file_name))
      roadmap_file_remove(path, file_name);
}

/**
 * Initialize the Realtime alerts
 * @param None
 * @return None
 */
void RTAlerts_Init()
{
   int i;

   static BOOL registered = FALSE;

   static RealtimeIdleScroller scroller = {RTAlerts_Scroll_All, RTAlerts_Stop_Scrolling, RTAlerts_IsIdleAlertScrolling};
   static RealtimeIdleScroller thumbs_up_scroller = {showThumbsUp, close_ThumbsUp_Dlg, RTAlerts_IsThumbsUpScrolling};

   static BOOL registered_provider = FALSE;
	if(!registered_provider){
		roadmap_alerter_register(&RoadmapRealTimeAlertProvider);
		registered_provider = TRUE;
	}

    for (i=0; i<RT_MAXIMUM_ALERT_COUNT; i++)
        gAlertsTable.alert[i] = NULL;

    gAlertsTable.iCount = 0;
    gAlertsTable.iGroupCount = 0;
    gAlertsTable.iArchiveCount = 0;


    for (i=0; i<RT_THUMBS_UP_QUEUE_MAXSIZE; i++)
       gThumbsUpTable.thumbsUp[i] = NULL;
    gThumbsUpTable.iCount = 0;

    gIdleScrolling = FALSE;
    gIterator = 0;
    gCurrentAlertId = -1;
    gCurrentCommentId = -1;


    if (!registered){
       RealtimePopUp_Register(&scroller, SCROLLER_NORMAL);
       RealtimePopUp_Register(&thumbs_up_scroller, SCROLLER_HIGH);
       registered = TRUE;
    }


    gPopAllTimerActive = FALSE;

    gSavedZoom = -1;
    gSavedFocus = NULL;
    gSavedPosition.latitude = -1;
    gSavedPosition.longitude = -1;

    roadmap_config_declare ("session", &LastCommentAlertIDCfg, "-1", NULL);

    roadmap_config_declare ("session", &LastJamStreetName, "", NULL);
    Set_LastJamStreetName("");

    roadmap_config_declare ("session", &LastCommentIDCfg, "-1", NULL);

    roadmap_config_declare ("preferences", &RoadMapConfigMaxAlertPopDist, "30000", NULL);

    roadmap_config_declare ("preferences", &RoadMapConfigSecondsToPopup, "15", NULL);

    roadmap_config_declare ("preferences", &RoadMapConfigSmallAlertScaleFactor, "60", NULL);

    roadmap_config_declare ("preferences", &AllowPoliceSubtypeCfg, "yes", NULL);


    InitVoiceRecording();
 	 RTinitMapProblems();
 	 RTinitOnRoadHazardCategories();
 	 RTinitOnShoulderHarzardCategories();
 	 RTinitWeatherHarzardCategories();
}

/**
 * Terminate the Realtime alerts
 * @param None
 * @return None
 */
void RTAlerts_Term()
{
    RTAlerts_Clear_All();
}

/**
 * The number of alerts currently in the table
 * @param None
 * @return the number of alerts
 */
int RTAlerts_Count(void)
{
    return gAlertsTable.iCount;
}

/**
 * The number of group relevant alerts currently in the table
 * @param None
 * @return the number of alerts
 */
int RTAlerts_GroupCount(void)
{
    return gAlertsTable.iGroupCount;
}


/**
 * The STRING of alerts currently in the table
 * @param None
 * @return string with number of alerts
 */
const char * RTAlerts_Count_Str(void)
{
	static char text[20];
	snprintf(text, sizeof(text), "%d",gAlertsTable.iCount - gAlertsTable.iArchiveCount);
   return &text[0];
}

/**
 * The STRING of group relevant alerts currently in the table
 * @param None
 * @return string with number of alerts
 */
const char * RTAlerts_GroupCount_Str(void)
{
   static char text[20];
   snprintf(text, sizeof(text), "%d",gAlertsTable.iGroupCount - gAlertsTable.iArchiveCount);

   return &text[0];
}
/**
 * Checks whether the alerts table is empty
 * @param None
 * @return TRUE if there are no alerts, FALSE otherwise
 */
BOOL RTAlerts_Is_Empty()
{
    return (0 == gAlertsTable.iCount);
}

/**
 * Checks whether the alerts table (excluding archived) is empty
 * @param None
 * @return TRUE if there are no alerts, FALSE otherwise
 */
BOOL RTAlerts_Live_Is_Empty()
{
   return (0 == gAlertsTable.iCount - gAlertsTable.iArchiveCount);
}

/**
 * Retrieve an alert from table by record number
 * @param record - the record number in the table
 * @return alert - pointer to the alert
 */
RTAlert * RTAlerts_Get(int record)
{
    return gAlertsTable.alert[record];
}

/**
 * Retrieve an alert from table by alert ID
 * @param iID - The id of the alert to retrieve
 * @return alert - pointer to the alert, NULL if not found
 */
RTAlert *RTAlerts_Get_By_ID(int iID)
{
    int i;

    //   Find alert:
    for (i=0; i< gAlertsTable.iCount; i++)
        if (gAlertsTable.alert[i]->iID == iID)
        {
            return (gAlertsTable.alert[i]);
        }

    return NULL;
}

/**
 * Clear all alerts from alerts table
 * @param None
 * @return None
 */
void RTAlerts_Clear_All()
{
    int i;
    RTAlert * pAlert;

    for (i=0; i<gAlertsTable.iCount; i++)
    {
        pAlert = RTAlerts_Get(i);
      //DeleteAlertObject(pAlert);
        RTAlerts_Delete_All_Comments(pAlert);
        if (pAlert->pAlertTitle)
           free(pAlert->pAlertTitle);

        if (pAlert->pIconName)
            free(pAlert->pIconName);

        if (pAlert->pMapIconName)
            free(pAlert->pMapIconName);

        if (pAlert->pMapAddOnName)
            free(pAlert->pMapAddOnName);

        if (pAlert->pMenuAddOnName)
            free(pAlert->pMenuAddOnName);

        free(pAlert);
        gAlertsTable.alert[i] = NULL;
    }

    OnAlertRemove();

    gAlertsTable.iCount = 0;
    gAlertsTable.iGroupCount = 0;
    gAlertsTable.iArchiveCount = 0;

    gThumbsUpTable.iCount = 0;
}

/**
 * Refresh all alerts on map based on settings
 * @param None
 * @return None
 */
void RTAlerts_RefreshOnMap(void)
{
   int i;
   RTAlert * pAlert;
   RoadMapDynamicString GUI_ID;
   char                 text[128];

   for (i=0; i<gAlertsTable.iCount; i++)
   {
      pAlert = RTAlerts_Get(i);
      //DeleteAlertObject(pAlert);
      snprintf(text, sizeof(text), "RTAlert_%d", pAlert->iID);
      GUI_ID = roadmap_string_new(text);

      if (roadmap_map_settings_show_report(pAlert->iType) &&
          !roadmap_object_exists(GUI_ID) &&
          !pAlert->bArchive)
         CreateAlertObject(pAlert);
      else if (!roadmap_map_settings_show_report(pAlert->iType) &&
               roadmap_object_exists(GUI_ID))
         DeleteAlertObject(pAlert);

      roadmap_string_release(GUI_ID);
   }
}

/**
 * Checks whether an alerts ID exists
 * @param iID - the id to check
 * @return TRUE the alert exist, FALSE otherwise
 */
BOOL RTAlerts_Exists(int iID)
{
    if ( NULL == RTAlerts_Get_By_ID(iID))
        return FALSE;

    return TRUE;
}


/**
 * Returns the state of the Realtime Alerts
 * @param None
 * @return STATE_OLD, STATE_NEW, STATE_NEW_COMMENT
 */
int RTAlerts_State()
{
    if (RTAlerts_Is_Empty())
        return STATE_NONE;
    else
        return gState;
}


/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height) {
   SsdWidget space;

	if ( roadmap_screen_is_hd_screen() )
	{
	   height *= 10;
	}

   space = ssd_container_new("spacer", NULL, SSD_MAX_SIZE, height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   return space;
}

/**
 * Add an alert to the list of the alerts
 * @param pAlert - pointer to the alert
 * @return TRUE operation was successful
 */
BOOL RTAlerts_Add(RTAlert *pAlert)
{
    int iDirection;
    RoadMapPosition position;
    RoadMapPosition lineFrom, lineTo;
    int iLineId = -1;
    int line_from_point;
    int line_to_point;
    int line_azymuth;
    int delta;
    int iSquare = -1;

    // Full?
    if ( RT_MAXIMUM_ALERT_COUNT == gAlertsTable.iCount)
        return FALSE;

#ifdef RT_ALERT_POLICE_FORBIDDEN
    if ( pAlert->iType == RT_ALERT_TYPE_POLICE )
            return TRUE;
#endif // RT_ALERT_POLICE_FORBIDDEN

    // Already exists?
    if (RTAlerts_Exists(pAlert->iID))
    {
        roadmap_log( ROADMAP_INFO, "RTAlerts_Add - cannot add Alert  (%d) alert already exist", pAlert->iID);
        return TRUE;
    }

    if (pAlert->iType > RT_ALERTS_LAST_KNOWN_TYPE)
    {
        roadmap_log( ROADMAP_WARNING, "RTAlerts_Add - add Alert(%d) unknown type (type=%d)", pAlert->iID, pAlert->iType);
        return TRUE;
    }

    if ((gState != STATE_NEW_COMMENT) && (!pAlert->bAlertByMe))
        gState = STATE_NEW;

	 // in case the only alert is mine. set the state to old.
	 if ((RTAlerts_Is_Empty())&& (pAlert->bAlertByMe))
	 	gState = STATE_OLD;

    gAlertsTable.alert[gAlertsTable.iCount] = calloc(1, sizeof(RTAlert));
    if (gAlertsTable.alert[gAlertsTable.iCount] == NULL)
    {
        roadmap_log( ROADMAP_ERROR, "RTAlerts_Add - cannot add Alert  (%d) calloc failed", pAlert->iID);
        return FALSE;
    }

    RTAlerts_Alert_Init(gAlertsTable.alert[gAlertsTable.iCount]);

    *gAlertsTable.alert[gAlertsTable.iCount]   = (*pAlert);



    if (pAlert->sAlertType[0] != 0){
       char temp_title[RT_ALERTS_MAX_TITLE_NAME];
       char temp_icon[RT_ALERTS_MAX_ICON_NAME];

       snprintf(temp_title, RT_ALERTS_MAX_TITLE_NAME, "%s%s", ALERT_TITLE_PREFIX, pAlert->sAlertType);
       gAlertsTable.alert[gAlertsTable.iCount]->pAlertTitle = strdup(temp_title);

       snprintf(temp_icon, RT_ALERTS_MAX_ICON_NAME, "%s%s", ALERT_ICON_PREFIX, pAlert->sAlertType);
       gAlertsTable.alert[gAlertsTable.iCount]->pIconName = strdup(temp_icon);

       snprintf(temp_icon, RT_ALERTS_MAX_ICON_NAME, "%s%s", ALERT_PIN_PREFIX, pAlert->sAlertType);
       gAlertsTable.alert[gAlertsTable.iCount]->pMapIconName = strdup(temp_icon);
    }


    if (pAlert->sAddOnName[0] != 0){
       char temp_addon[RT_ALERTS_MAX_ADD_ON_NAME];
       snprintf(temp_addon, RT_ALERTS_MAX_ADD_ON_NAME, "%s%s", ALERT_PIN_ADDON_PREFIX, pAlert->sAddOnName);
       gAlertsTable.alert[gAlertsTable.iCount]->pMapAddOnName = strdup(temp_addon);

       snprintf(temp_addon, RT_ALERTS_MAX_ADD_ON_NAME, "%s%s", ALERT_ICON_ADDON_PREFIX, pAlert->sAddOnName);
       gAlertsTable.alert[gAlertsTable.iCount]->pMenuAddOnName = strdup(temp_addon);
    }

    if ((gAlertsTable.alert[gAlertsTable.iCount]->pIconName) && (roadmap_res_get(RES_BITMAP,RES_SKIN, gAlertsTable.alert[gAlertsTable.iCount]->pIconName) == NULL)){
       roadmap_res_download(RES_DOWNLOAD_IMAGE, gAlertsTable.alert[gAlertsTable.iCount]->pIconName,NULL, "",FALSE, 0, NULL, NULL );
    }

    if ((gAlertsTable.alert[gAlertsTable.iCount]->pMapIconName) && (roadmap_res_get(RES_BITMAP,RES_SKIN, gAlertsTable.alert[gAlertsTable.iCount]->pMapIconName) == NULL)){
       roadmap_res_download(RES_DOWNLOAD_IMAGE, gAlertsTable.alert[gAlertsTable.iCount]->pMapIconName,NULL, "",FALSE, 0, NULL, NULL );
    }

    if ((gAlertsTable.alert[gAlertsTable.iCount]->pMenuAddOnName) && (roadmap_res_get(RES_BITMAP,RES_SKIN, gAlertsTable.alert[gAlertsTable.iCount]->pMenuAddOnName) == NULL)){
        roadmap_res_download(RES_DOWNLOAD_IMAGE, gAlertsTable.alert[gAlertsTable.iCount]->pMenuAddOnName,NULL, "",FALSE, 0, NULL, NULL );
    }

    if ((gAlertsTable.alert[gAlertsTable.iCount]->pMapAddOnName) && (roadmap_res_get(RES_BITMAP,RES_SKIN, gAlertsTable.alert[gAlertsTable.iCount]->pMapAddOnName) == NULL)){
        roadmap_res_download(RES_DOWNLOAD_IMAGE, gAlertsTable.alert[gAlertsTable.iCount]->pMapAddOnName,NULL, "",FALSE, 0, NULL, NULL );
    }

    if ((gAlertsTable.alert[gAlertsTable.iCount]->sGroupIcon[0] != 0) && (roadmap_res_get(RES_BITMAP,RES_SKIN, gAlertsTable.alert[gAlertsTable.iCount]->sGroupIcon) == NULL)){
        roadmap_res_download(RES_DOWNLOAD_IMAGE, gAlertsTable.alert[gAlertsTable.iCount]->sGroupIcon,NULL, "",FALSE, 0, NULL, NULL );
    }

    position.longitude = pAlert->iLongitude;
    position.latitude = pAlert->iLatitude;

    if (pAlert->sLocationStr[0] == 0){

       const char *street;
       const char *city;

       // Save Location Description string
       if (!RTAlerts_Get_City_Street(position, &city, &street, &iSquare, &iLineId, pAlert->iDirection))
       {
         city = pAlert->sCityStr;
         street = pAlert->sStreetStr;
       }

       if (pAlert->sStreetStr[0] != 0)
          street = pAlert->sStreetStr;

       if (pAlert->sCityStr[0] != 0)
          city = pAlert->sCityStr;

       if (!((city == NULL) && (street == NULL)))
    	 {
      	  if ((city != NULL) && (strlen(city) == 0))
         	   snprintf(
            	        gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
               	             + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                  	  sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
                     	       - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                    		"%s", street);
        		else if ((street != NULL) && (strlen(street) == 0))
            	snprintf(
               	     gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
                  	          + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
	                    sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
   	                         - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
      	              "%s", city);
        		else
            	snprintf(
               	     gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
                  	          + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
	                    sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
   	                         - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
      	              "%s, %s", street, city);
      }
    }

    if (pAlert->sNearStr[0] != 0){
    	sprintf (gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr), " %s %s", 			roadmap_lang_get("Near"), pAlert->sNearStr);
    }


    gAlertsTable.alert[gAlertsTable.iCount]->iSquare = iSquare;
    gAlertsTable.alert[gAlertsTable.iCount]->iLineId = iLineId;

    if (pAlert->iDirection == RT_ALERT_OPPSOITE_DIRECTION)
    {
        iDirection = pAlert->iAzymuth + 180;
        while (iDirection > 360)
            iDirection -= 360;
        gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth = iDirection;
    }

    if (iLineId != -1){
    	roadmap_square_set_current(iSquare);
    	roadmap_line_points(iLineId, &line_from_point, &line_to_point);
    	roadmap_line_from(iLineId, &lineFrom);
    	roadmap_line_to(iLineId, &lineTo);
    	line_azymuth = roadmap_math_azymuth(&lineFrom, &lineTo);
    	delta = azymuth_delta(gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth,
    			line_azymuth);
    	if ((delta > 90) || (delta < -90))
    	{
        	gAlertsTable.alert[gAlertsTable.iCount]->iNode1 = line_to_point;
        	gAlertsTable.alert[gAlertsTable.iCount]->iNode2 = line_from_point;
    	}
    	else
    	{
        	gAlertsTable.alert[gAlertsTable.iCount]->iNode1 = line_from_point;
        	gAlertsTable.alert[gAlertsTable.iCount]->iNode2 = line_to_point;
    	}
    }

    if (gAlertsTable.alert[gAlertsTable.iCount]->iGroupRelevance != GROUP_RELEVANCE_NONE ){
       gAlertsTable.iGroupCount++;
       if ((!gAlertsTable.alert[gAlertsTable.iCount]->bAlertByMe) && !(gAlertsTable.alert[gAlertsTable.iCount]->bArchive))
          gGroupState = STATE_NEW;

       gAlertsTable.alert[gAlertsTable.iCount]->iDisplayTimeStamp = time(NULL);
    }

   if (pAlert->bArchive)
      gAlertsTable.iArchiveCount++;

    gAlertsTable.iCount++;

    OnAlertAdd(gAlertsTable.alert[gAlertsTable.iCount-1]);


    return TRUE;
}

/**
 * Initialize a comment
 * @param comment - pointer to the comment
 * @return None
 */
void RTAlerts_Comment_Init(RTAlertComment *comment)
{
    comment->iID = -1;
    comment->iAlertId = -1;
    comment->i64ReportTime = 0L;
    comment->iRank = 0;
    comment->iMood = 0;
    comment->bCommentByMe = FALSE;
    comment->bShowFacebookPicture = FALSE;
    memset(comment->sPostedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(comment->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
    memset(comment->sFacebookName, 0, RT_ALERT_FACEBOOK_USERNM_MAXSIZE);
}

/**
 * Checks whether a comment ID exists for that alert
 * @param alert - pointer to the alert, iCommentId the id to check
 * @return TRUE the comment exist, FALSE otherwise
 */
BOOL RTAlerts_Comment_Exist(RTAlert *pAlert, int iCommentId)
{

    RTAlertCommentsEntry * current = pAlert->Comment;

    while (current != NULL)
    {
        if (current->comment.iID == iCommentId)
            return TRUE;
        current = current->next;
    }

    return FALSE;
}

/**
 * Add a comment to the comments list of the alert
 * @param comment - pointer to the comment
 * @return TRUE operation was successful
 */

BOOL RTAlerts_Comment_Add(RTAlertComment *Comment)
{
    BOOL bCommentedByMe= FALSE;
    RTAlert *pAlert;
    RTAlertCommentsEntry * current;
    RTAlertCommentsEntry *CommentEntry;

    pAlert = RTAlerts_Get_By_ID(Comment->iAlertId);
    if (pAlert == NULL)
        return FALSE;

    if (RTAlerts_Comment_Exist(pAlert, Comment->iID))
        return TRUE;

    if (Comment->sDescription[0] == 0)
    	return TRUE;


   if (pAlert->iNumComments == 0)
      DeleteAlertObject(pAlert);

    CommentEntry = calloc(1, sizeof(RTAlertCommentsEntry));
    CommentEntry->comment = (*Comment);

    CommentEntry->previous = NULL;
    CommentEntry->next = NULL;

    current= pAlert->Comment;
    if (current != NULL)
    {
        while (current != NULL)
        {
            if (current->comment.bCommentByMe)
                bCommentedByMe = TRUE;
            if (current->next == NULL)
            {
                current->next = CommentEntry;
                CommentEntry->previous = current;
                current = NULL;
            }
            else
            {
                current = current->next;
            }
        }
    }
    else
    { /* the list is empty. = */
        pAlert->Comment = CommentEntry;
    }


    pAlert->iNumComments++;


   if (pAlert->iNumComments == 1 &&
       roadmap_map_settings_show_report(pAlert->iType) &&
       !pAlert->bArchive)
      CreateAlertObject(pAlert);
    if (Comment->bDisplay){
       if (!Comment->bCommentByMe){
          if (pAlert->bAlertByMe || bCommentedByMe || pAlert->bThumbsUpByMe){
             gState = STATE_NEW_COMMENT;
             RTAlerts_Comment_PopUp(Comment,pAlert);
          }
          else
             gState = STATE_NEW;
       }
    }


    return TRUE;
}

/**
 * Remove all comments of an alert
 * @param pAlert - pointer to the alert
 * @return None
 */
void RTAlerts_Delete_All_Comments(RTAlert *pAlert)
{
    if (pAlert->Comment != NULL)
    {
        RTAlertCommentsEntry * current =
                pAlert->Comment;
        while (current != NULL)
        {
            RTAlertCommentsEntry * new_current = current->next;
            free(current);
            current = new_current;
        }
    }
}

/**
 * Remove an alert from table
 * @param iID - Id of the alert
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */
BOOL RTAlerts_Remove(int iID)
{
    BOOL bFound= FALSE;

    //   Are we empty?
    if ( 0 == gAlertsTable.iCount){
       roadmap_log( ROADMAP_ERROR, "RemoveAlert() - Failed (List is empty). ID %d not found", iID);
       return TRUE;
    }

    if (gAlertsTable.alert[gAlertsTable.iCount-1]->iID == iID)
    {
      DeleteAlertObject(gAlertsTable.alert[gAlertsTable.iCount-1]);
        RTAlerts_Delete_All_Comments(gAlertsTable.alert[gAlertsTable.iCount-1]);
        if (gAlertsTable.alert[gAlertsTable.iCount-1]->pAlertTitle)
           free(gAlertsTable.alert[gAlertsTable.iCount-1]->pAlertTitle);

        if (gAlertsTable.alert[gAlertsTable.iCount-1]->pIconName)
            free(gAlertsTable.alert[gAlertsTable.iCount-1]->pIconName);

        if (gAlertsTable.alert[gAlertsTable.iCount-1]->pMapIconName)
            free(gAlertsTable.alert[gAlertsTable.iCount-1]->pMapIconName);

        if (gAlertsTable.alert[gAlertsTable.iCount-1]->pMapAddOnName)
            free(gAlertsTable.alert[gAlertsTable.iCount-1]->pMapAddOnName);

        if (gAlertsTable.alert[gAlertsTable.iCount-1]->pMenuAddOnName)
            free(gAlertsTable.alert[gAlertsTable.iCount-1]->pMenuAddOnName);

        if (gAlertsTable.alert[gAlertsTable.iCount-1]->iGroupRelevance != GROUP_RELEVANCE_NONE ){
              gAlertsTable.iGroupCount--;
        }

       if (gAlertsTable.alert[gAlertsTable.iCount-1]->bArchive ){
          gAlertsTable.iArchiveCount--;
       }

        free(gAlertsTable.alert[gAlertsTable.iCount-1]);
        bFound = TRUE;
    }
    else
    {
        int i;

        for (i=0; i<(gAlertsTable.iCount-1); i++)
        {
            if (bFound)
                gAlertsTable.alert[i] = gAlertsTable.alert[i+1];
            else
            {
                if (gAlertsTable.alert[i]->iID == iID)
                {
               DeleteAlertObject(gAlertsTable.alert[i]);
                    RTAlerts_Delete_All_Comments(gAlertsTable.alert[i]);

                    if ((gAlertsTable.alert[i]->iGroupRelevance != GROUP_RELEVANCE_NONE ))
                       gAlertsTable.iGroupCount--;

                   if (gAlertsTable.alert[i]->bArchive ){
                      gAlertsTable.iArchiveCount--;
                   }

                    if (gAlertsTable.iGroupCount == 0)
                       gGroupState = STATE_OLD;

                    free(gAlertsTable.alert[i]);
                    gAlertsTable.alert[i] = gAlertsTable.alert[i+1];
                    bFound = TRUE;
                }
            }
        }
    }

    if (bFound)
    {
        gAlertsTable.iCount--;

        gAlertsTable.alert[gAlertsTable.iCount] = NULL;

        OnAlertRemove();
    }

    if (!bFound){
       roadmap_log( ROADMAP_DEBUG, "RemoveAlert() - Failed. ID %d not found", iID);
    }

    return TRUE;
}

/**
 * Get the number of comments for a specific alert
 * @param iAlertId - Id of the alert
 * @return The number of alerts
 */
int RTAlerts_Get_Number_of_Comments(int iAlertId)
{
    RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertId);
    if (pAlert == NULL)
        return 0;
    else
        return pAlert->iNumComments;
}

/**
 * Get the image id of the alert
 * @param iAlertId - Id of the alert
 * @return The pointer to the image Id buffer
 */
const char* RTAlerts_Get_Image_Id( int iAlertId )
{
    RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertId);
    if ( pAlert == NULL )
        return NULL;
    else
        return pAlert->sImageIdStr;
}

/**
 * Returns the indication whether the image is attached to the alert
 * @param iAlertId - Id of the alert
 * @return TRUE if image exists
 */
BOOL RTAlerts_Has_Image( int iAlertId )
{
	const char* image_id = RTAlerts_Get_Image_Id( iAlertId );

	return ( image_id && image_id[0] );
}

/**
 * Get the voice id of the alert
 * @param iAlertId - Id of the alert
 * @return The pointer to the voice Id buffer
 */
const char* RTAlerts_Get_Voice_Id( int iAlertId )
{
   RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertId);
   if ( pAlert == NULL )
      return NULL;
   else
      return pAlert->sVoiceIdStr;
}
/**
 * Returns the indication whether the voice is attached to the alert
 * @param iAlertId - Id of the alert
 * @return TRUE if voice exists
 */
BOOL RTAlerts_Has_Voice( int iAlertId )
{
	const char* voice_id = RTAlerts_Get_Voice_Id( iAlertId );

	return ( voice_id && voice_id[0] );
}

/**
 * Get the number of position for a specific alert
 * @param record - The alert record
 * @param position - pointer to the RoadMapPosition of the alert
 * @param steering - The steering of the alert
 * @None
 */
void RTAlerts_Get_Position(int record, RoadMapPosition *position, int *steering)
{

    RTAlert *pAlert = RTAlerts_Get(record);
    if (pAlert == NULL)
    	return;

    position->longitude = pAlert->iLongitude;
    position->latitude = pAlert->iLatitude;

    *steering = pAlert->iAzymuth;
}

/**
 * Get the type of an Alert
 * @param record - The Record number in the table
 * @return The type of Alert
 */
int RTAlerts_Get_Type(int record)
{

    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);
    if (pAlert != NULL)
        return pAlert->iType;
    else
        return 0;
}

/**
 * Get the type of an Alert
 * @param record - The Record number in the table
 * @return The type of Alert
 */
int RTAlerts_Get_Type_By_Id(int iId)
{

    RTAlert *pAlert = RTAlerts_Get_By_ID(iId);
    if (pAlert != NULL)
        return pAlert->iType;
    else
        return 0;
}

/**
 * Get the type of an Alert
 * @param record - The Record number in the table
 * @return The type of Alert
 */
const char* RTAlerts_Get_GroupName_By_Id(int iId)
{

    RTAlert *pAlert = RTAlerts_Get_By_ID(iId);
    if (pAlert != NULL)
        return pAlert->sGroup;
    else
        return NULL;
}

/**
 * Get the ID of an Alert
 * @param record - The Record number in the table
 * @return The id of Alert, -1 the record does not exist
 */
int RTAlerts_Get_Id(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->iID;
    else
        return -1;

}

/**
 * Get the Location string of an Alert
 * @param record - The Record number in the table
 * @return The location string of the Alert, NULL if the record does not exist
 */
char * RTAlerts_Get_LocationStr(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->sLocationStr;
    else
        return NULL;

}

/**
 * Get the Location string of an Alert
 * @param ID - The Alert ID
 * @return The location string of the Alert, NULL if the record does not exist
 */
char * RTAlerts_Get_LocationStrByID(int Id)
{
    RTAlert *pAlert = RTAlerts_Get_By_ID(Id);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->sLocationStr;
    else
        return "";

}
/**
 * Get the speed of an Alert
 * @param record - The Record number in the table
 * @return The speed of the Alert, -1 if the record does not exist
 */
unsigned int RTAlerts_Get_Speed(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        if (pAlert->iType == RT_ALERT_TYPE_POLICE)
           return pAlert->iSpeed;
        else
           return 0;
    else
        return -1;
}

/**
 * Get the an alert Alert
 * @param record - The Record number in the table
 * @return a new pointer to the alert
 */
RoadMapAlert * RTAlerts_Get_Alert(int record)
{
    RoadMapAlert *pRoadMapAlert;
    RTAlert *pAlert;

    pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    pRoadMapAlert = (RoadMapAlert *) malloc(sizeof(pRoadMapAlert));

    pRoadMapAlert->pos.latitude = pAlert->iLatitude;
    pRoadMapAlert->pos.longitude = pAlert->iLongitude;
    pRoadMapAlert->steering = pAlert->iAzymuth;
    pRoadMapAlert->speed = RTAlerts_Get_Speed(record);
    pRoadMapAlert->id = pAlert->iID;
    pRoadMapAlert->category = pAlert->iType;

    return pRoadMapAlert;

}

/**
 * Get the icon to display of an alert type
 * @param alertId - The ID of the alert
 * @param has_comment - The alert with comment or not
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_IconByType(RTAlert *pAlert, int iAlertType, BOOL has_comments)
{

    switch (iAlertType)
    {

   case RT_ALERT_TYPE_POLICE:
        return "alert_icon_police";
        break;

    case RT_ALERT_TYPE_ACCIDENT:
        return "alert_icon_accident";
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
        return "alert_icon_chit_chat";
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
        return "alert_icon_traffic_jam";
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
        return "alert_icon_traffic_info";
        break;

    case RT_ALERT_TYPE_HAZARD:
        return "alert_icon_hazard";
        break;

    case RT_ALERT_TYPE_OTHER:
        return "alert_icon_other";
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
        return "alert_icon_road_construction";
        break;

    case RT_ALERT_TYPE_PARKING:
        return "alert_icon_parking";
        break;

    case RT_ALERT_TYPE_DYNAMIC:
       if (pAlert && pAlert->pIconName)
          return pAlert->pIconName;
       else
          return "alert_icon_chit_chat";

    default:
        return "alert_icon_chit_chat";
    }
}

/**
 * Get the type corresponding to an icon
 * @param icon_name
 * @param has_comment - The alert with comment or not
 * @return the icon name, NULL if the alert is not found or type unknown
 */
int RTAlerts_Get_TypeByIconName(const char *icon_name)
{

		if (!strcmp(icon_name, "alert_icon_police"))
            return RT_ALERT_TYPE_POLICE;

		if (!strcmp(icon_name, "alert_icon_accident"))
            return RT_ALERT_TYPE_ACCIDENT;

		if (!strcmp(icon_name, "alert_icon_chit_chat"))
            return RT_ALERT_TYPE_CHIT_CHAT;

		if (!strcmp(icon_name, "alert_icon_traffic_jam"))
            return RT_ALERT_TYPE_TRAFFIC_JAM;

		if (!strcmp(icon_name, "alert_icon_traffic_info") )
            return RT_ALERT_TYPE_TRAFFIC_INFO;

		if (!strcmp(icon_name, "alert_icon_hazard"))
            return RT_ALERT_TYPE_HAZARD;

		if (!strcmp(icon_name, "alert_icon_other"))
            return RT_ALERT_TYPE_OTHER;

		if (!strcmp(icon_name, "alert_icon_road_construction"))
            return RT_ALERT_TYPE_CONSTRUCTION;

      if (!strcmp(icon_name, "alert_icon_parking"))
            return RT_ALERT_TYPE_PARKING;


      else
    	    return RT_ALERT_TYPE_CHIT_CHAT;
}

/**
 * Get the icon to display of an alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Icon(int alertId)
{

    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

	return RTAlerts_Get_IconByType(pAlert, pAlert->iType, (pAlert->iNumComments != 0));
}

/**
 * Get the icon to display of an alert to display on map
 * @param type
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Map_Icon_By_Type(int iType)
{

   static char icon_name[128];
    switch (iType)
    {
    case RT_ALERT_TYPE_POLICE:
       strcpy( icon_name,  "alert_pin_police" );
      break;

    case RT_ALERT_TYPE_ACCIDENT:
       strcpy( icon_name,  "alert_pin_accident" );
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
       strcpy( icon_name,  "alert_pin_chit_chat" );
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
       strcpy( icon_name,  "alert_pin_loads" );
        break;

    case RT_ALERT_TYPE_HAZARD:
       strcpy( icon_name,  "alert_pin_hazard" );
      break;

    case RT_ALERT_TYPE_PARKING:
       strcpy( icon_name,  "alert_pin_parking" );
        break;

    case RT_ALERT_TYPE_OTHER:
       strcpy( icon_name,  "alert_pin_other" );
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
       strcpy( icon_name,  "alert_pin_road_construction" );
        break;

    case RT_ALERT_TYPE_DYNAMIC:
            return NULL;
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
         return NULL;
        break;

    default:
        return NULL;
    }
   return icon_name;
}

/**
 * Get the icon to display of an alert to display on map
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Map_Icon(int alertId)
{

    RTAlert *pAlert;
    static char icon_name[128];
    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
       strcpy( icon_name,  "alert_pin_police" );
    	break;

    case RT_ALERT_TYPE_ACCIDENT:
       strcpy( icon_name,  "alert_pin_accident" );
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
       strcpy( icon_name,  "alert_pin_chit_chat" );
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
       strcpy( icon_name,  "alert_pin_loads" );
        break;

    case RT_ALERT_TYPE_HAZARD:
       strcpy( icon_name,  "alert_pin_hazard" );
    	break;

    case RT_ALERT_TYPE_PARKING:
       strcpy( icon_name,  "alert_pin_parking" );
        break;

    case RT_ALERT_TYPE_OTHER:
       strcpy( icon_name,  "alert_pin_other" );
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
       strcpy( icon_name,  "alert_pin_road_construction" );
        break;

    case RT_ALERT_TYPE_DYNAMIC:
        if (pAlert->pMapIconName)
           strcpy( icon_name, pAlert->pMapIconName );
        else
           return NULL;
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
       	return NULL;
        break;

    default:
        return NULL;
    }
	return icon_name;
}

/**
 * Get the map icon addon to display on map
 * @param alertId - The ID of the alert
 * @param AddOnIndex - The Index of the addon
 * * @return the addon icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Map_AddOn(int alertId, int AddOnIndex){

   RTAlert *pAlert;
   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
        return NULL;

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      return NULL;

   if (AddOnIndex == 0){
      if (pAlert->iNumComments != 0)
         return "alert_pin_addon_comment";
      else if (pAlert->sImageIdStr[0] || pAlert->sVoiceIdStr[0])
         return  "alert_pin_addon_attachment";
      else if (pAlert->pMapAddOnName)
         return  pAlert->pMapAddOnName;
      else
         return NULL;
   }
   else if (AddOnIndex == 1){
      if (pAlert->sImageIdStr[0] || pAlert->sVoiceIdStr[0])
         return  "alert_pin_addon_attachment";
      else if (pAlert->pMapAddOnName)
         return  pAlert->pMapAddOnName;
      else
         return NULL;
   }
   else if (AddOnIndex == 2){
      if (pAlert->pMapAddOnName)
          return  pAlert->pMapAddOnName;
      else
         return NULL;
   }

   return NULL;
}

/**
 * Get the the number of icon addons
 * @param alertId - The ID of the alert
 * * @return the The number of map addons
 */
int RTAlerts_Get_Number_Of_Map_AddOns(int alertId){

   RTAlert *pAlert;
   int count = 0;
   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
        return 0;

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      return 0;

   if (pAlert->iNumComments != 0)
      count++;

   if (pAlert->sImageIdStr[0] || pAlert->sVoiceIdStr[0])
      count++;

   if (pAlert->pMapAddOnName)
      count++;

   return count;
}

/**
 * Get the the number of icon addons
 * @param alertId - The ID of the alert
 * * @return the The number of map addons
 */
int RTAlerts_Get_Number_Of_AddOns(int alertId){

   RTAlert *pAlert;
   int count = 0;
   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
        return 0;

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      return 1;

   if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM) && (pAlert->sAddOnName[0] == 0))
      count++;

   if (pAlert->iNumComments != 0)
      count++;

   if (pAlert->sAddOnName[0])
      count++;

   return count;
}

/**
 * Get the icon addon to display
 * @param alertId - The ID of the alert
 * @param AddOnIndex - The Index of the addon
 * * @return the addon icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_AddOn(int alertId, int AddOnIndex){

   RTAlert *pAlert;
   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
        return NULL;

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      return "alert_icon_addon_waze";

   if (AddOnIndex == 0){
      if (pAlert->iNumComments != 0)
         return "alert_icon_addon_comment";
      else if (pAlert->pMenuAddOnName)
         return  pAlert->pMenuAddOnName;
      else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
         return "alert_icon_addon_user";
      else
         return NULL;
   }
   else if (AddOnIndex == 1){
      if (pAlert->pMenuAddOnName)
         return  pAlert->pMenuAddOnName;
      else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
               return "alert_icon_addon_user";
      else
         return NULL;
   }

   return NULL;
}
/**
 * Get the icon to display of an in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Alert_Icon(int alertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "alert_icon_police";
    case RT_ALERT_TYPE_ACCIDENT:
        return "alert_icon_accident";
    case RT_ALERT_TYPE_HAZARD:
        return "alert_icon_hazard";
    case RT_ALERT_TYPE_TRAFFIC_INFO:
    case RT_ALERT_TYPE_TRAFFIC_JAM:
         return "alert_icon_traffic_jam";
    default:
        return NULL;
    }
}

/**
 * Get the icon to warn of an in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Warn_Icon(int alertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "alert_icon_police";
    case RT_ALERT_TYPE_ACCIDENT:
        return "alert_icon_accident";
    case RT_ALERT_TYPE_HAZARD:
         return "alert_icon_hazard";
    case RT_ALERT_TYPE_TRAFFIC_INFO:
    case RT_ALERT_TYPE_TRAFFIC_JAM:
         return "alert_icon_traffic_jam";
    default:
         return NULL;
    }
}

/**
 * Get the name of the alert
 * @param alertId - The ID of the alert
 * @return the name, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_String(int alertId)
{
    RTAlert *pAlert;
    static char str[256];

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

   switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
    case RT_ALERT_TYPE_ACCIDENT:
    case RT_ALERT_TYPE_HAZARD:
    case RT_ALERT_TYPE_TRAFFIC_INFO:
    case RT_ALERT_TYPE_TRAFFIC_JAM:
       sprintf(str, "%s %s", RTAlerts_get_title(pAlert, pAlert->iType, pAlert->iSubType), roadmap_lang_get("ahead"));
       return &str[0];

    default:
        return NULL;
    }
}

/**
 * Get the alert additional text
 * @param alertId - The ID of the alert
 * @return the additional text, NULL if the alert is not found or type unknown
 */
const char * RTAlerts_Get_Additional_String(int alertId)
{
    RTAlert *pAlert;
    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) || (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM))
      if (navigate_main_state() == 0)
         return roadmap_lang_get("This route is still the fastest");
//         return pAlert->sLocationStr;

    return pAlert->sDescription;

}

/**
 * Get the distance to alert in case of approaching the alert
 * @param record - record number in the table
 * @return the distance in meters to alerts before the alert, default 0
 */
int RTAlerts_Get_Distance(int record)
{

    RTAlert *pAlert;
    pAlert = RTAlerts_Get(record);
    if (pAlert == NULL)
        return 0;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return 600;
    case RT_ALERT_TYPE_HAZARD:
        return 600;
    case RT_ALERT_TYPE_ACCIDENT:
        return 2000;
    case RT_ALERT_TYPE_TRAFFIC_INFO:
    case RT_ALERT_TYPE_TRAFFIC_JAM:
       return 3000;
        break;
    default:
        return 0;
    }
}

/**
 * Get the sound file name to play in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the sound name, empty sound list if the alert is not found
 */
RoadMapSoundList RTAlerts_Get_Sound(int alertId)
{

    RoadMapSoundList sound_list = NULL;
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return sound_list;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
       if ( tts_apptext_available( TTS_APPTEXT_POLICE ) ) {
          sound_list = tts_apptext_get_sound( TTS_APPTEXT_POLICE );
       }
       else {
          sound_list = roadmap_sound_list_create(0);
          roadmap_sound_list_add(sound_list, "Police");
       }
       break;
    case RT_ALERT_TYPE_ACCIDENT:
       if ( tts_apptext_available( TTS_APPTEXT_APPROACH_ACCIDENT ) ) {
          sound_list = tts_apptext_get_sound( TTS_APPTEXT_APPROACH_ACCIDENT );
       }
       else {
          sound_list = roadmap_sound_list_create(0);
          roadmap_sound_list_add(sound_list, "ApproachAccident");
       }
       break;
    case RT_ALERT_TYPE_HAZARD:
       if ( tts_apptext_available( TTS_APPTEXT_APPROACH_HAZARD ) ) {
          sound_list = tts_apptext_get_sound( TTS_APPTEXT_APPROACH_HAZARD );
       }
       else {
          sound_list = roadmap_sound_list_create(0);
          roadmap_sound_list_add(sound_list, "ApproachHazard");
       }
       break;
    case RT_ALERT_TYPE_TRAFFIC_INFO:
    case RT_ALERT_TYPE_TRAFFIC_JAM:
       if ( tts_apptext_available( TTS_APPTEXT_APPROACH_TRAFFIC ) ) {
          sound_list = tts_apptext_get_sound( TTS_APPTEXT_APPROACH_TRAFFIC );
       }
       else {
          sound_list = roadmap_sound_list_create(0);
          roadmap_sound_list_add(sound_list, "ApproachTraffic");
       }
       break;
     default:
        break;
    }

    return sound_list;
}

/**
 * Check whether an alert is alertable
 * @param record - The record number of the alert
 * @return TRUE if the alert is alertable, FALSE otherwise
 */
int RTAlerts_Is_Alertable(int record)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get(record);

    if (pAlert == NULL)
        return FALSE;

    if (pAlert->bArchive)
        return FALSE;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return TRUE;
    case RT_ALERT_TYPE_ACCIDENT:
       if (pAlert->bIsAlertable )
          return TRUE;
       else
          return FALSE;
    case RT_ALERT_TYPE_CHIT_CHAT:
          return FALSE;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
       return FALSE;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
       if (pAlert->bIsAlertable){
             if ( (g_traffic_info_time_stamp == -1) || ((time(NULL) - g_traffic_info_time_stamp) > 3*60) ){
                return TRUE;
             }
             else{
                return FALSE;
             }
       }else{
             return FALSE;
       }
    case RT_ALERT_TYPE_HAZARD:
        return TRUE;
    case RT_ALERT_TYPE_OTHER:
        return FALSE;
    case RT_ALERT_TYPE_CONSTRUCTION:
        return FALSE;
    case RT_ALERT_TYPE_PARKING:
        return FALSE;
    case RT_ALERT_TYPE_DYNAMIC:
       return FALSE;
    default:
        return FALSE;
    }
}

BOOL RTAlerts_ShowDisrance(int AlertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(AlertId);

    if (pAlert == NULL)
        return FALSE;

    if (pAlert->bArchive)
        return FALSE;

    switch (pAlert->iType)
    {
       case RT_ALERT_TYPE_TRAFFIC_JAM:
       case RT_ALERT_TYPE_TRAFFIC_INFO:
          return TRUE;
       default:
          return TRUE;
    }
}

BOOL RTAlerts_Is_On_Route(int AlertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(AlertId);

    if (pAlert == NULL)
        return FALSE;

    return (pAlert->bAlertIsOnRoute);
}

/**
 * Check whether an alert is reroutable (ask the user to calculate for a new route)
 * @param pAlert - pointer to the alert
 * @return TRUE if the alert is reroutable, FALSE otherwise
 */
int RTAlerts_Is_Reroutable(RTAlert *pAlert)
{

    if (pAlert == NULL)
        return FALSE;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return FALSE;
    case RT_ALERT_TYPE_ACCIDENT:
        return FALSE;
    case RT_ALERT_TYPE_CHIT_CHAT:
        return FALSE;
    case RT_ALERT_TYPE_TRAFFIC_JAM:
        return FALSE;
    case RT_ALERT_TYPE_TRAFFIC_INFO:
        return FALSE;
    case RT_ALERT_TYPE_HAZARD:
        return FALSE;
    case RT_ALERT_TYPE_OTHER:
        return FALSE;
    case RT_ALERT_TYPE_CONSTRUCTION:
        return FALSE;
    case RT_ALERT_TYPE_PARKING:
        return FALSE;
    case RT_ALERT_TYPE_DYNAMIC:
        return FALSE;

    default:
        return FALSE;
    }
}

static void OnAlertShortClick (const char *name,
                               const char *sprite,
                               RoadMapDynamicString *images,
                               int  image_count,
                               const RoadMapGpsPosition *gps_position,
                               const RoadMapGuiPoint    *offset,
                               BOOL is_visible,
                               int scale,
                               int opacity,
                               int scale_y,
                               const char *id,
                               ObjectText *texts,
                               int        text_count,
                               int rotation) {
   int AlertId = atoi(name);
   RTAlerts_Popup_By_Id_No_Center(AlertId);
}

static void DeleteAlertObject(RTAlert *pAlert)
{
   RoadMapDynamicString GUI_ID;
   const char*          icon;
   char                 text[128];

   icon = RTAlerts_Get_Map_Icon(pAlert->iID);
   if (icon != NULL) {
      snprintf(text, sizeof(text), "RTAlert_%d", pAlert->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_remove( GUI_ID);

      roadmap_string_release(GUI_ID);
#ifndef OPENGL
      snprintf(text, sizeof(text), "RTAlert_small_%d", pAlert->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_remove( GUI_ID);

      roadmap_string_release(GUI_ID);
#endif
   }
}

/**
 * Create a screen object for alert
 * @param pAlert - pointer to the alert
 * @return None
 */
static void CreateAlertObject(RTAlert *pAlert)
{
   RoadMapDynamicString Image;
   RoadMapDynamicString GUI_ID;
   RoadMapDynamicString Group = roadmap_string_new( "RealtimeAlerts");
   RoadMapDynamicString Name;
   RoadMapDynamicString Sprite= roadmap_string_new( "RealtimeAlerts");
   const char*          icon;
   char                 text[128];
   int                  iAddon;
   int                  numAddons;
   RoadMapGpsPosition   Pos;
   RoadMapGuiPoint      Offset;
   RoadMapImage         image;

   icon = RTAlerts_Get_Map_Icon(pAlert->iID);
   if (icon == NULL)
      return;

   snprintf(text, sizeof(text), "%d", pAlert->iID);
   Name  = roadmap_string_new( text);

   Pos.longitude = pAlert->iLongitude;
   Pos.latitude = pAlert->iLatitude;
   Offset.x = ADJ_SCALE(4);

   
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, icon);
      if (image)
         Offset.y = -roadmap_canvas_image_height(image) /2 + ADJ_SCALE(4);
      Image = roadmap_string_new(icon);
      snprintf(text, sizeof(text), "RTAlert_%d", pAlert->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_add_with_priority( Group, GUI_ID, Name, Sprite, Image, &Pos, &Offset,
                         OBJECT_ANIMATION_POP_IN | OBJECT_ANIMATION_POP_OUT | OBJECT_ANIMATION_WHEN_VISIBLE, NULL, OBJECT_PRIORITY_HIGH);
      roadmap_object_set_action(GUI_ID, OnAlertShortClick);

#ifdef OPENGL
      roadmap_object_set_zoom (GUI_ID, -1, roadmap_layer_get_declutter(ROADMAP_ROAD_PRIMARY));
      roadmap_object_set_scale_factor(GUI_ID, roadmap_layer_get_declutter(ROADMAP_ROAD_MAIN) +1, roadmap_config_get_integer(&RoadMapConfigSmallAlertScaleFactor));
#else
      roadmap_object_set_zoom (GUI_ID, -1, roadmap_layer_get_declutter(ROADMAP_ROAD_MAIN));
#endif
      roadmap_string_release(Image);
      roadmap_string_release(GUI_ID);

#ifndef OPENGL
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "alert_marker_small");
      if (image)
         Offset.y = -roadmap_canvas_image_height(image) /2 + ADJ_SCALE(4);
      Offset.x =  ADJ_SCALE(6);
      Image = roadmap_string_new("alert_marker_small");
      snprintf(text, sizeof(text), "RTAlert_small_%d", pAlert->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_add_with_priority( Group, GUI_ID, Name, Sprite, Image, &Pos, &Offset,
                         OBJECT_ANIMATION_POP_IN | OBJECT_ANIMATION_POP_OUT,
                         NULL, OBJECT_PRIORITY_HIGH);
      roadmap_object_set_action(GUI_ID, OnAlertShortClick);
      roadmap_object_set_zoom (GUI_ID, roadmap_layer_get_declutter(ROADMAP_ROAD_MAIN) +1, roadmap_layer_get_declutter(ROADMAP_ROAD_PRIMARY));

      roadmap_string_release(Image);
      roadmap_string_release(GUI_ID);
#endif


   //addons
   numAddons = RTAlerts_Get_Number_Of_Map_AddOns(pAlert->iID);
   for (iAddon = 0; iAddon < numAddons; iAddon++ ){
      icon = RTAlerts_Get_Map_AddOn(pAlert->iID, iAddon);
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, icon);
      if (image)
         Offset.y = -roadmap_canvas_image_height(image) /2 + ADJ_SCALE(4);
      Image = roadmap_string_new(icon);
      roadmap_object_add_image(GUI_ID, Image);
      roadmap_string_release(Image);
   }

   roadmap_string_release( Group);
   roadmap_string_release( Name);
   roadmap_string_release( Sprite);
}

/**
 * Action to be taken after an alert was added
 * @param pAlert - pointer to the alert
 * @return None
 */
static void OnAlertAdd(RTAlert *pAlert)
{

    if (pAlert->bPingWazer && Realtime_AllowPing() && (!pAlert->bArchive)){
       RTAlerts_popup_PingWazer(pAlert);
    }
    else{
       if ((!pAlert->bAlertByMe) && (!pAlert->bArchive) && (pAlert->iReportedElapsedTime < 120) && pAlert->sGroup[0] && (roadmap_groups_get_popup_config()) && pAlert->iGroupRelevance){
          if ((roadmap_groups_get_popup_config() == POPUP_REPORT_FOLLOWING_GROUPS) ||
               roadmap_groups_get_popup_config() == pAlert->iGroupRelevance ){
                   RTAlerts_Popup_By_Id_Timed(pAlert->iID, TRUE, GROUP_POPUP_TIMER);
          }
       }
    }

   if (roadmap_map_settings_show_report(pAlert->iType) && !pAlert->bArchive)
       CreateAlertObject(pAlert);
}

/**
 * Action to be taken after an alert was removed
 * @param None
 * @return None
 */
static void OnAlertRemove(void)
{
}


BOOL RTAlerts_IsThumbsUpScrolling(void){
   return gThumbsUpScrolling;
}



static void stop_scrolling(void){
   if (RTAlerts_IsIdleAlertScrolling()){
      RTAlerts_Stop_Scrolling();
      Realtime_SendCurrentViewDimentions();
   }

   if (RTAlerts_IsThumbsUpScrolling())
      close_ThumbsUp_Dlg();

   if (RealtimeExternalPoi_IsPromotionsScrolling())
      RealtimeExternalPoi_StopPromotionScrolling();
}




/**
 * Zoomin map to the alert.
 * @param AlertPosition - The position of the alert
 * @param iCenterAround - Whether to center around the alert or the GPS position
 * @return None
 */
static void RTAlerts_zoom(RoadMapPosition AlertPosition, int iCenterAround)
{
    int distance;
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition pos;
    PluginLine line;
    int Direction;
    long scale;

    if (iCenterAround == CENTER_ON_ALERT)
    {
    	  roadmap_trip_set_point("Hold", &AlertPosition);
        roadmap_screen_update_center_animated(&AlertPosition, 600, 0);
		  scale = 1000;
        gCentered = TRUE;

    } else {

	    gCentered = FALSE;
	    if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) != -1)
	    {

		    pos.latitude = CurrentPosition.latitude;
		    pos.longitude = CurrentPosition.longitude;
		    distance = roadmap_math_distance(&pos, &AlertPosition);

		    if (distance < 1000)
			    scale = 1000;
		    else if (distance < 2000)
			    scale = 1500;
		    else if (distance < 5000)
			    scale = 2500;
		    else
			    scale = -1;

			 if (scale == -1) {
				 scale = 2500;
		    }
		    roadmap_screen_update_center_animated(&pos, 2500, 0);
	    } else {
   		 scale = 5000;
          roadmap_screen_update_center_animated(&AlertPosition, 600, 0);
		 }
	 }

    roadmap_screen_set_scale(scale, roadmap_screen_height() / 3);
    //roadmap_screen_set_orientation_fixed();
}

/**
 * Find the name of the street and city near an alert
 * @param AlertPosition - the position of the alert
 * @param city_name [OUT] - the name of the city, NULL if not found
 * @param street_name [OUT] - the name of the street, NULL if not found
 * @param line_id [OUT] - the line_id near the alert, -1 if not found
 * @param direction - the direction of the alert
 * @return None
 */
BOOL RTAlerts_Get_City_Street(RoadMapPosition AlertPosition,
        const char **city_name, const char **street_name, int *square, int *line_id,
        int direction)
{

    RoadMapNeighbour neighbours[16];
    int layers[128];
    int layers_count;
    RoadMapStreetProperties properties;
    RoadMapPosition context_save_pos;
    zoom_t context_save_zoom;
    int line_directions;
    const char *street_name2;
    int count = 0;
    *square = roadmap_tile_get_id_from_position(0, &AlertPosition);

    if(!roadmap_square_set_current(*square)){
       return FALSE;
    }
    roadmap_math_get_context(&context_save_pos, &context_save_zoom);
    roadmap_math_set_context((RoadMapPosition *)&AlertPosition, 20);
    layers_count = roadmap_layer_all_roads(layers, 128);
    count = roadmap_street_get_closest(&AlertPosition, 0, layers, layers_count,
            1, &neighbours[0], 1);

    if (count != 0)
    {
		  roadmap_square_set_current (neighbours[0].line.square);

        if (roadmap_locator_activate(neighbours[0].line.fips) < 0)
        {
            *city_name = NULL;
            *street_name = NULL;
            *line_id = -1;
            *square = -1;
            roadmap_math_set_context(&context_save_pos, context_save_zoom);
            return FALSE;
        }
        roadmap_street_get_properties(neighbours[0].line.line_id, &properties);

        line_directions =roadmap_line_route_get_direction(
                neighbours[0].line.line_id, ROUTE_CAR_ALLOWED);
        *street_name = roadmap_street_get_street_name(&properties);

        if ((line_directions == ROUTE_DIRECTION_ANY) || (direction
                == RT_ALERT_MY_DIRECTION))
        {

            *city_name = roadmap_street_get_street_city(&properties,
            ROADMAP_STREET_LEFT_SIDE);
            *line_id = neighbours[0].line.line_id;
            *square = neighbours[0].line.square;
            roadmap_math_set_context(&context_save_pos, context_save_zoom);
        }
        else
        {
            int i;

            // The alert is on the opposite side but the current line is one-way
            roadmap_square_set_current (neighbours[0].line.square);
            street_name2 = strdup(*street_name);
            *city_name = roadmap_street_get_street_city(&properties,
            ROADMAP_STREET_LEFT_SIDE);
            *line_id = neighbours[0].line.line_id;
			   *square = neighbours[0].line.square;

            layers[0] = neighbours[0].line.cfcc;
            count = roadmap_street_get_closest(&AlertPosition, 0, layers, 1,
                    1, &neighbours[0], 10);
            for (i=0; i<count; i++)
            {
            	 roadmap_square_set_current (neighbours[i].line.square);
                roadmap_street_get_properties(neighbours[i].line.line_id,
                        &properties);
                *street_name = roadmap_street_get_street_name(&properties);
                if (strcmp(street_name2, *street_name) && !strncmp(
                        street_name2, *street_name, 2))
                {
                    *city_name = roadmap_street_get_street_city(&properties,
                    ROADMAP_STREET_LEFT_SIDE);
                    *square = neighbours[i].line.square;
                    *line_id = neighbours[i].line.line_id;
                    roadmap_math_set_context(&context_save_pos, context_save_zoom);
                    free((void *)street_name2);
                    return TRUE;
                }
            }
            free((void *)street_name2);
            roadmap_street_get_properties(neighbours[0].line.line_id, &properties);
            *street_name = roadmap_street_get_street_name(&properties);
        }

    }
    else
    {
        *city_name = NULL;
        *street_name = NULL;
        *line_id = -1;
        *square = -1;
        roadmap_math_set_context(&context_save_pos, context_save_zoom);
        return FALSE;
    }
    roadmap_math_set_context(&context_save_pos, context_save_zoom);
    return TRUE;
}

/**
 * Compare method for qsort. sorts according to distance to alert
 * @param r1 - alert element
 * @param r2 - alert element
 * @return 0 if they are equal, -1 if distance to r1 is less than r2, 1 if distance to r2 is more than r1
 */
static int compare_proximity(const void *r1, const void *r2)
{

    RTAlert * const *elem1 = (RTAlert * const *)r1;
    RTAlert * const *elem2 = (RTAlert * const *)r2;

	    if ((*elem1)->iDistance < (*elem2)->iDistance)
	        return -1;

	    else if ((*elem1)->iDistance > (*elem2)->iDistance)
	        return 1;

	    else
   	     return 0;
}


/**
 * Compare method for qsort. sorts according to distance to alert
 * @param r1 - alert element
 * @param r2 - alert element
 * @return 0 if they are equal, -1 if distance to r1 is less than r2, 1 if distance to r2 is more than r1
 */
static int compare_priority(const void *r1, const void *r2)
{

    RTAlert * const *elem1 = (RTAlert * const *)r1;
    RTAlert * const *elem2 = (RTAlert * const *)r2;

       if ((*elem1)->iPopUpPriority < (*elem2)->iPopUpPriority)
           return -1;

       else if ((*elem1)->iPopUpPriority > (*elem2)->iPopUpPriority)
           return 1;

       else{
          return compare_proximity(r1,r2);
       }
}

/**
 * Compare method for qsort. sorts according to distance to time
 * @param r1 - alert element
 * @param r2 - alert element
 * @return 0 if they are equal, -1 if distance to r1 is less than r2, 1 if distance to r2 is more than r1
 */
static int compare_recency(const void *r1, const void *r2)
{

    RTAlert * const *elem1 = (RTAlert * const *)r1;
    RTAlert * const *elem2 = (RTAlert * const *)r2;

    if ((*elem1)->iReportTime > (*elem2)->iReportTime)
	        return -1;
	 else if ((*elem1)->iReportTime < (*elem2)->iReportTime)
	        return 1;
	 else
   	     return 0;
}

static void set_popup_priority(RTAlert *pAlert){

   if (!pAlert)
      return;

   // Do not display alerts which were already poped up, archived, by me or outside the range
   if ((pAlert->bPopedUp) || (pAlert->iDistance > roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist)))
      pAlert->iPopUpPriority = 1000;
   else if (!roadmap_map_settings_show_report(pAlert->iType))
      pAlert->iPopUpPriority = 1000;
   else if ((pAlert->bArchive) || (pAlert->bAlertByMe))
      pAlert->iPopUpPriority = 1000;

   //Group reports (if not set to pop) � priority 1
   else if ((pAlert->iGroupRelevance == GROUP_RELEVANCE_ACTIVE)  && !(pAlert->bPopedUp))
      pAlert->iPopUpPriority = 1;
   else if ((pAlert->iGroupRelevance == GROUP_RELEVANCE_FOLLOWING)  && !(pAlert->bPopedUp))
      pAlert->iPopUpPriority = 2;
   //User generated reports with text  in 2 km radius � priority 3
   else if ((pAlert->iType != RT_ALERT_TYPE_TRAFFIC_INFO) && (pAlert->sDescription[0] != 0) && (pAlert->iDistance <= 2000))
      pAlert->iPopUpPriority = 3;
   //User generated reports without text  in 2 km radius � priority 4
   else if ((pAlert->iType != RT_ALERT_TYPE_TRAFFIC_INFO) && (pAlert->sDescription[0] == 0) && (pAlert->iDistance <= 2000))
      pAlert->iPopUpPriority = 4;
   //Automated traffic reports Standstill traffic 0.5 km  radius � Priority 5
   else if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) && (pAlert->iSubType == STAND_STILL_TRAFFIC) && (pAlert->iDistance <= 500))
      pAlert->iPopUpPriority = 5;
   //On my route � Priority 6
   else if (pAlert->bAlertIsOnRoute)
      pAlert->iPopUpPriority = 6;
   //User generated reports with text  in 50 km radius � priority 7
   else if ((pAlert->iType != RT_ALERT_TYPE_TRAFFIC_INFO) && (pAlert->sDescription[0] != 0) && (pAlert->iDistance <= roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist)))
      pAlert->iPopUpPriority = 7;
   //User Generated reports with no text  50 km radius -   priority 8
   else if ((pAlert->iType != RT_ALERT_TYPE_TRAFFIC_INFO) && (pAlert->sDescription[0] == 0) && (pAlert->iDistance <= roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist)))
      pAlert->iPopUpPriority = 8;
   //Other automated traffic (not standstill)  up to our max radius -  priority 9
   else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
      pAlert->iPopUpPriority = 9;
   else
      pAlert->iPopUpPriority = 10;

}

/**
 * Sort list of alerts according to distance from my current location
 * @param None
  * @return None
 */

void RTAlerts_Sort_List(alert_sort_method sort_method)
{
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition position, current_pos;
    const RoadMapPosition *GpsPosition;
    PluginLine line;
    int Direction, distance;
    int i;
    gState = STATE_OLD;
    for (i=0; i < gAlertsTable.iCount; i++)
    {
        position.longitude = gAlertsTable.alert[i]->iLongitude;
        position.latitude = gAlertsTable.alert[i]->iLatitude;

        if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction)
                != -1)
        {

            current_pos.latitude = CurrentPosition.latitude;
            current_pos.longitude = CurrentPosition.longitude;

            distance = roadmap_math_distance(&current_pos, &position);
            gAlertsTable.alert[i]->iDistance = distance;
        }
        else
        {
           GpsPosition = roadmap_trip_get_position("Location");
           if ((GpsPosition != NULL) && !IS_DEFAULT_LOCATION( GpsPosition ) ){
              current_pos.latitude = GpsPosition->latitude;
              current_pos.longitude = GpsPosition->longitude;
              distance = roadmap_math_distance(&current_pos, &position);
              gAlertsTable.alert[i]->iDistance = distance;
           }
           else
           {
              GpsPosition = roadmap_trip_get_position("GPS");
              if (GpsPosition != NULL)
              {
                 current_pos.latitude = GpsPosition->latitude;
                 current_pos.longitude = GpsPosition->longitude;
                 distance = roadmap_math_distance(&current_pos, &position);
                 gAlertsTable.alert[i]->iDistance = distance;
              }
              else
              {
                 distance = 0;
              }
           }

        }
        if (sort_method == sort_priority){
           set_popup_priority(gAlertsTable.alert[i]);
        }
    }

#ifndef J2ME
    if (sort_method == sort_proximity)
    	qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_proximity);
    else if (sort_method == sort_recency)
    	qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_recency);
    else if (sort_method == sort_priority)
      qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_priority);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse_confirm_dlg_callback(int exit_code, void *context){
   if (exit_code == dec_yes){
      ssd_dialog_hide_current(dec_close);
      Realtime_ReportAbuse(gCurrentAlertId, gCurrentCommentId);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse(void){
   ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), TRUE,report_abuse_confirm_dlg_callback, NULL, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
}


#ifndef TOUCH_SCREEN
static int real_time_post_alert_comment_sk_cb(SsdWidget widget, const char *new_value, void *context){
	real_time_post_alert_comment();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_view_image(){


        RTAlerts_Download_Image( gCurrentAlertId );
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show_comments(){

        RealtimeAlertCommentsList(gCurrentAlertId);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_report_abuse(){

   report_abuse();
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_otions_send_thumbs_up(){

   Rtalerts_Thumbs_Up(gCurrentAlertId);
}



///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   real_time_list_context_menu_items   selection;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
      return;

   selection = (real_time_list_context_menu_items)item->id;

   switch( selection)
   {

      case rt_cm_add_comments:
         real_time_post_alert_comment();
         break;

      case rt_cm_view_comments:
         on_option_show_comments();
         break;

      case rt_cm_view_image:
         on_option_view_image();
         break;

      case rt_cm_report_abuse:
         on_option_report_abuse();
         break;

      case rt_cm_send_thumbs_up:
         on_otions_send_thumbs_up();
         break;

     case rt_cm_cancel:
         g_context_menu_is_active = FALSE;
         roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   BOOL has_comments = FALSE;
   BOOL has_image = FALSE;
   BOOL is_auto_jam = FALSE;
   BOOL report_abuse = FALSE;
   BOOL send_thumbs_up = FALSE;

   RTAlert *pAlert = RTAlerts_Get_By_ID(gCurrentAlertId);;

   if (RTAlerts_Get_Type_By_Id(gCurrentAlertId) == RT_ALERT_TYPE_TRAFFIC_INFO)
         is_auto_jam = TRUE;

   if (RTAlerts_Get_Number_of_Comments(gCurrentAlertId) != 0)
      has_comments = TRUE;

   has_image = RTAlerts_Has_Image( gCurrentAlertId );


   report_abuse = !is_auto_jam && pAlert->bPingWazer;

   send_thumbs_up = !is_auto_jam && !pAlert->bAlertByMe && !pAlert->bThumbsUpByMe;

   ssd_contextmenu_show_item( &context_menu,
                              rt_cm_view_image,
                              has_image,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rt_cm_view_comments,
                              has_comments,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rt_cm_add_comments,
                              !is_auto_jam,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rt_cm_report_abuse,
                              report_abuse,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rt_cm_send_thumbs_up,
                              send_thumbs_up,
                              FALSE);

   if  (ssd_widget_rtl (NULL))
      menu_x = SSD_X_SCREEN_RIGHT;
   else
      menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,              // X
							   SSD_Y_SCREEN_BOTTOM, // Y
							   &context_menu,
							   on_option_selected,
							   NULL,
							   dir_default,
							   0,
							   TRUE);

   g_context_menu_is_active = TRUE;
   return 0;
}

static void set_left_softkey(SsdWidget dlg, RTAlert *pAlert){
   BOOL has_comments = FALSE;
   BOOL has_image = FALSE;
   BOOL is_auto_jam = FALSE;
   if (RTAlerts_Get_Number_of_Comments(pAlert->iID) != 0)
      has_comments = TRUE;

   has_image = RTAlerts_Has_Image( pAlert->iID );

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
         is_auto_jam = TRUE;

   if (is_auto_jam){
      ssd_widget_set_left_softkey_callback(dlg, NULL);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get(""));
   }
   else{
      ssd_widget_set_left_softkey_callback(dlg, on_options);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Options"));
#ifdef RIMAPI
      ssd_contextmenu_menu_button_register(on_options,dlg->name);
#endif
   }
}

static int real_time_hide_sk_cb(SsdWidget widget, const char *new_value, void *context){
	RTAlerts_Cancel_Scrolling();
	return 0;
}

static void set_right_softkey(SsdWidget dlg){
	//ssd_widget_set_right_softkey_callback(dlg, real_time_hide_sk_cb);+++
	ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Hide"));
}

#endif



/**
 * Display the pop up of an alert according to the iterator
 * @param None
 * @return None
 */
static void RTAlerts_Popup(void)
{
    RTAlert *pAlert;
    if (!gPopAllTimerActive)
       return;

    if ((ssd_dialog_is_currently_active() && (strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg")))){
        //Other dialog is active
        return;
     }


    if (gIterator >= gAlertsTable.iCount)
    {
        gIterator = -1;
    }
    gIterator++;
    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;

     // reached an alert that was added while scrolling, sort again
    if (pAlert->iDistance<=0){
    	RTAlerts_Sort_List(sort_priority);
    	gIterator = 0; // return to begining
    }

    // if idle scrolling, and alert is bigger than maximal distance,
    // then return to the beginning ( closest alert ).
    if (pAlert->iPopUpPriority == 1000)
    {
    	gIterator = 0;
    }

    if ((roadmap_general_settings_events_radius() != -1) && (pAlert->iDistance > roadmap_general_settings_events_radius()*1000))
    {
       gIterator = 0;
    }

    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL){
          if (RTAlerts_IsIdleAlertScrolling())
              RTAlerts_Stop_Scrolling();
          return;
    }

    if (pAlert->iPopUpPriority == 1000){
       if (RTAlerts_IsIdleAlertScrolling())
           RTAlerts_Stop_Scrolling();
       return;
    }

   RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ALERT);

}

char *getHazardStr(int subtype){
   static char temp[20];
   char *str;
   RoadMapConfigDescriptor param;
   param.category = "Hazard";
   sprintf(temp, "%d", subtype);
   param.name = &temp[0];
   roadmap_config_declare("preferences",&param, "", NULL);
   str = roadmap_config_get(&param);
   if (*str == 0)
      return roadmap_lang_get("Hazard");
   else
      return roadmap_lang_get(str);
}
const char *RTAlerts_get_title(RTAlert *pAlert, int iAlertType, int iAlertSubType){

   switch (iAlertType){

      //Accident
      case RT_ALERT_TYPE_ACCIDENT:
         switch (iAlertSubType){
            case ACCIDENT_TYPE_MAJOR:
               return roadmap_lang_get("Major accident");
            case ACCIDENT_TYPE_MINOR:
               return roadmap_lang_get("Minor accident");
            default:
               return roadmap_lang_get("Accident");
         }

      //Police
      case RT_ALERT_TYPE_POLICE:
         switch (iAlertSubType){
            case POLICE_TYPE_VISIBLE:
               return roadmap_lang_get("Police (visible)");
            case POLICE_TYPE_HIDING:
               return roadmap_lang_get("Police (hidden)");
            default:
               return roadmap_lang_get("Police trap");
         }

      //Jam
      case RT_ALERT_TYPE_TRAFFIC_INFO:
      case RT_ALERT_TYPE_TRAFFIC_JAM:
         switch (iAlertSubType){
            case JAM_TYPE_LIGHT_TRAFFIC:
               return roadmap_lang_get("Light traffic");
            case JAM_TYPE_MODERATE_TRAFFIC:
               return roadmap_lang_get("Moderate traffic");
            case JAM_TYPE_HEAVY_TRAFFIC:
               return roadmap_lang_get("Heavy traffic");
            case JAM_TYPE_STAND_STILL_TRAFFIC:
               return roadmap_lang_get("Complete standstill");
            default:
               return roadmap_lang_get("Traffic jam");
         }

       // Hazard
       case RT_ALERT_TYPE_HAZARD:
       switch (iAlertSubType){
          case HAZARD_TYPE_ON_ROAD:
             return roadmap_lang_get("Hazard on road");
          case HAZARD_TYPE_ON_SHOULDER:
             return roadmap_lang_get("Hazard on shoulder");
          case HAZARD_TYPE_WEATHER:
             return roadmap_lang_get("Weather hazard");
          case HAZARD_TYPE_ON_ROAD_OBJECT:
             return roadmap_lang_get("Object on road");
          case HAZARD_TYPE_ON_ROAD_POT_HOLE:
             return roadmap_lang_get("Pot hole on road");
          case HAZARD_TYPE_ON_ROAD_ROAD_KILL:
             return roadmap_lang_get("Road kill on road");
          case HAZARD_TYPE_ON_SHOULDER_CAR_STOPPED:
             return roadmap_lang_get("Car stopped on shoulder");
          case HAZARD_TYPE_ON_SHOULDER_ANIMALS:
             return roadmap_lang_get("Animals near road");
          case HAZARD_TYPE_ON_SHOULDER_MISSING_SIGN:
             return roadmap_lang_get("Missing sign");
          case HAZARD_TYPE_WEATHER_FOG:
             return roadmap_lang_get("Foggy weather");
          case HAZARD_TYPE_WEATHER_HAIL:
             return roadmap_lang_get("Hail");
          case HAZARD_TYPE_WEATHER_HEAVY_RAIN:
             return roadmap_lang_get("Heavy rain");
          case HAZARD_TYPE_WEATHER_HEAVY_SNOW:
             return roadmap_lang_get("Heavy snow");
          case HAZARD_TYPE_WEATHER_FLOOD:
             return roadmap_lang_get("Flood");
          case HAZARD_TYPE_WEATHER_MONSOON:
             return roadmap_lang_get("Monsoon");
          case HAZARD_TYPE_WEATHER_TORNADO:
             return roadmap_lang_get("Tornado");
          case HAZARD_TYPE_WEATHER_HEAT_WAVE:
             return roadmap_lang_get("Tornado");
          case HAZARD_TYPE_WEATHER_HURRICANE:
             return roadmap_lang_get("hurricane");
          case HAZARD_TYPE_WEATHER_FREEZING_RAIN:
             return roadmap_lang_get("Freezing rain");
          case HARARD_TYPE_ON_ROAD_LANE_CLOSED:
             return roadmap_lang_get("Lane closed");
          case HAZARD_TYPE_ON_ROAD_OIL:
             return roadmap_lang_get("Oil spill");
          case HAZARD_TYPE_ON_ROAD_ICE:
             return roadmap_lang_get("Ice on road");
          case HAZRAD_TYPE_ON_ROAD_CONSTRUCTION:
             return roadmap_lang_get("Construction zone");
          case -1:
             return roadmap_lang_get("Hazard");
          default:
             return getHazardStr(iAlertSubType);
      }

      // Other
      case RT_ALERT_TYPE_OTHER:
         return roadmap_lang_get("Other");

      // Construction
      case RT_ALERT_TYPE_CONSTRUCTION:
         return roadmap_lang_get("Road construction");

      // Parking
      case RT_ALERT_TYPE_PARKING:
         return roadmap_lang_get("Parking");

      // Dynamic
      case RT_ALERT_TYPE_DYNAMIC:
         if ((pAlert != NULL))
            return roadmap_lang_get(pAlert->pAlertTitle);
         else
            return NULL;

      // Rest is default
      default:
         return roadmap_lang_get("Chit chat");
   }
}

static void on_popup_close(int exit_code, void* context){

   if (gSavedZoom == -2){
      // do nothing
   }
   else if (gSavedZoom == -1){
       roadmap_trip_set_focus("GPS");
       roadmap_math_zoom_reset();
   }else{
   	  roadmap_math_set_context(&gSavedPosition, gSavedZoom);
   	  gSavedPosition.latitude = -1;
   	  gSavedPosition.longitude = -1;
   	  gSavedZoom = -1;
   }

   if (g_alert_popup_active)
      roadmap_main_remove_periodic(RTAlerts_Alert_PopUp_Timer);

   if (gSavedFocus)
      roadmap_trip_set_focus(gSavedFocus);
   gSavedFocus = NULL;

   if (exit_code == dec_close)
   {
	  if (gIdleScrolling){
	     RealtimePopUp_set_Stoped_Popups();
	     Realtime_SendCurrentViewDimentions();
       gIdleScrolling = FALSE;
	  }

	  if (gPopAllTimerActive){
			roadmap_main_remove_periodic(RTAlerts_Popup) ;
			gPopAllTimerActive = FALSE;
		}
   }

   if (gPopAllTimerActive){
       roadmap_main_remove_periodic(RTAlerts_Popup) ;
       gPopAllTimerActive = FALSE;
    }

   roadmap_screen_set_orientation_dynamic();

   roadmap_softkeys_remove_right_soft_key("Me on map");
   roadmap_layer_adjust();
   if (!roadmap_screen_refresh())
   	roadmap_screen_redraw();

}

#ifdef TOUCH_SCREEN
static int on_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide("AlertPopUPDlg", dec_close);
   return 1;
}

static int on_button_close_AlertAhead (SsdWidget widget, const char *new_value){
   ssd_dialog_hide("AlertAheadDlg", dec_close);
   return 1;
}

static int on_ping_wazer_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide("PingWazerPopUPDlg", dec_close);
   return 1;
}

static int on_add_comment (SsdWidget widget, const char *new_value){
	gPopUpType = POP_UP_ALERT;
    real_time_post_alert_comment_by_id(gCurrentAlertId);
	return 0;
}

static int on_report_abuse (SsdWidget widget, const char *new_value){
   report_abuse();
   return 0;
}


static int on_view_comments (SsdWidget widget, const char *new_value){
	RTAlerts_CurrentComments();
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
static int on_view_attachment( SsdWidget widget, const char *new_value )
{
   RTAlerts_Download_Image( gCurrentAlertId );
   RTAlerts_Download_Voice( gCurrentAlertId );


   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int on_thumbs_up( SsdWidget widget, const char *new_value )
{
   SsdWidget text;
   char temp[20];

   text = ssd_widget_get(widget->parent, "ThumbsUpText");
   if (text)
      ssd_widget_show(text);

   temp[0] = 0;
   text = ssd_widget_get(widget->parent, "ThumbsUp_buttonTxt");
   if (!strcmp(ssd_text_get_text(text),"+"))
         snprintf(temp, sizeof(temp),"1");
   else{
         int iNumThumbsUp = atoi(ssd_text_get_text(text));
         snprintf(temp, sizeof(temp),"%d", iNumThumbsUp+1);
   }
   ssd_text_set_text(text, temp);
   ssd_button_disable(widget);

   Rtalerts_Thumbs_Up(gCurrentAlertId );
   return 1;
}

#else
static BOOL Alert_OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags){
	if( !this)
      return FALSE;

    if((KEYBOARD_VIRTUAL_KEY & flags))
    {
      return FALSE;
    }
    if( flags & KEYBOARD_ASCII)
      switch( *utf8char)
      {
            case 'v':
            case 'V':
            case '4':
               RTAlerts_Scroll_Prev();
               return TRUE;
               break;

      	    case 't':
            case 'T':
            case '6':
               RTAlerts_Scroll_Next();
               return TRUE;
               break;

            case 'y':
            case 'Y':
            case '8':
               RTAlerts_Cancel_Scrolling();
               return TRUE;
               break;

            case 'i':
            case 'I':
            case '+':
            case '*':
                roadmap_screen_zoom_in();
                break;

            case 'o':
            case 'O':
            case '-':
            case '#':
                roadmap_screen_zoom_out();
                break;

      }

    return FALSE;
}

#endif //TOUCH_SCREEN



/**
 * Builds the string containing the report info ( who and when reported )
 * @param pAlert - the pointer to the alert
 * @param buf - the buffer to write to
 * @param buf_len - the buffer max len
 * @return None
 */
void RTAlerts_get_report_info_str( RTAlert *pAlert, char* buf, int buf_len )
{
    int timeDiff;
    time_t now;
	const char *tmpStr;
    // Display when the alert was generated
    now = time(NULL);
    timeDiff = (int) difftime( now, (time_t) pAlert->iReportTime );
    if ( timeDiff < 0 )
        timeDiff = 0;

     buf[0] = 0;
  	 if ( pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO )
  		tmpStr = roadmap_lang_get("Updated ");
    else
    	tmpStr = roadmap_lang_get("");

  	snprintf( buf, buf_len, "%s", tmpStr );


    if (timeDiff < 60)
        snprintf( buf  + strlen( buf ), buf_len - strlen( buf ),
                roadmap_lang_get("%d seconds ago"), timeDiff );
    else if ((timeDiff >= 60) && (timeDiff < 3600))
        snprintf( buf  + strlen( buf ), buf_len - strlen( buf ),
                roadmap_lang_get("%d minutes ago"), timeDiff/60 );
    else
        snprintf( buf  + strlen( buf ), buf_len - strlen( buf ), roadmap_lang_get("%2.1f hours ago"), (float)timeDiff/3600 );
}

/**
 * Builds the string containing the report distance string
 * @param pAlert - the pointer to the alert
 * @param buf - the buffer to write to
 * @param buf_len - the buffer max len
 * @return None
 */
static void RTAlerts_get_report_distance_str( RTAlert *pAlert, char* buf, int buf_len )
{
    RoadMapPosition position, current_pos;
    const RoadMapPosition *GpsPosition;
    int distance;
    RoadMapGpsPosition CurrentPosition;
    PluginLine line;
    int Direction;
    int distance_far;
    char str[100];
    char unit_str[20];
    int tenths;
    buf[0] = 0;

    position.longitude = pAlert->iLongitude;
    position.latitude = pAlert->iLatitude;

    // check the distance to the alert
    if ( roadmap_navigate_get_current( &CurrentPosition, &line, &Direction ) != -1 )
    {

        current_pos.latitude = CurrentPosition.latitude;
        current_pos.longitude = CurrentPosition.longitude;
        distance = roadmap_math_distance( &current_pos, &position );
        distance_far = roadmap_math_to_trip_distance( distance );
    }
    else
    {
        GpsPosition = roadmap_trip_get_position("Location");
        if (GpsPosition != NULL)
        {
            current_pos.latitude = GpsPosition->latitude;
            current_pos.longitude = GpsPosition->longitude;
            distance = roadmap_math_distance( &current_pos, &position );
            distance_far = roadmap_math_to_trip_distance( distance );
        }
        else
        	return; // Can not compute distance return
    }

    tenths = roadmap_math_to_trip_distance_tenths( distance );
    snprintf( str, sizeof(str), "%d.%d", distance_far, tenths % 10 );
    snprintf( unit_str, sizeof( unit_str ), "%s", roadmap_math_trip_unit());

    snprintf( buf, buf_len, roadmap_lang_get("%s %s"), str, roadmap_lang_get(unit_str));
}

void RTAlerts_start_glow(RTAlert *pAlert, int seconds) {
#ifdef OPENGL
   RoadMapPosition position;
   RoadMapGuiPoint offset = {0,0};
   position.longitude = pAlert->iLongitude;
   position.latitude = pAlert->iLatitude;
   if (pAlert->iType != RT_ALERT_TYPE_TRAFFIC_INFO) {
      offset.x = ADJ_SCALE(-4);
      offset.y = ADJ_SCALE(25);
   }
   roadmap_screen_start_glow(&position, seconds, &offset);


#endif //OPENGL
}

void RTAlerts_stop_glow() {
#ifdef OPENGL
   roadmap_screen_stop_glow();

#endif //OPENGL
}

static void update_popup(SsdWidget popup, RTAlert *pAlert, int iCenterAround){
   char AlertStr[700];
   char ReportedByStr[100];
   SsdWidget button;
   SsdWidget text;
   RoadMapPosition position;
   int i, num_addOns;
   SsdWidget facebook_image;
   SsdWidget groups_container;

   ssd_widget_reset_cache(popup);
   // Set the title
   ssd_widget_set_value( popup, "popuup_text", RTAlerts_get_title(pAlert, pAlert->iType, pAlert->iSubType) );

   ssd_bitmap_update(ssd_widget_get(popup, "alert_icon"), RTAlerts_Get_Icon(pAlert->iID));
   ssd_bitmap_remove_overlays(ssd_widget_get(popup, "alert_icon"));
   num_addOns = RTAlerts_Get_Number_Of_AddOns(pAlert->iID);
   for (i = 0 ; i < num_addOns; i++){
       const char *AddOn = RTAlerts_Get_AddOn(pAlert->iID, i);
       if (AddOn)
          ssd_bitmap_add_overlay(ssd_widget_get(popup, "alert_icon"),AddOn);
   }
   RTAlerts_update_location_str(pAlert);
   ssd_widget_set_value(popup, "alert_location", pAlert->sLocationStr);

   RTAlerts_get_report_info_str( pAlert, AlertStr, sizeof( AlertStr ) );
   ssd_widget_set_value(popup, "alert_info", AlertStr);
   ReportedByStr[0] = 0;
   RTAlerts_get_reported_by_string(pAlert,ReportedByStr,sizeof(ReportedByStr));
   if (ReportedByStr[0] == 0)
      ssd_widget_hide(ssd_widget_get(popup, "user_info_container"));
   else
      ssd_widget_show(ssd_widget_get(popup, "user_info_container"));

   facebook_image = ssd_widget_get(popup, "facebook_image");
   if (facebook_image){
       ssd_bitmap_update(facebook_image, "facebook_default_image_small");
       if (pAlert->bShowFacebookPicture){
          int facebook_image_size = ADJ_SCALE(32);
          ssd_widget_show(ssd_widget_get(popup, "FB_IMAGE_container"));
//          if (roadmap_screen_is_hd_screen())
//             facebook_image_size = 50;
          roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, pAlert->iID, -1, facebook_image_size, facebook_image);
       }
       else{
          ssd_widget_hide(ssd_widget_get(popup, "FB_IMAGE_container"));
       }
   }

   if (pAlert->sFacebookName[0] != 0){
          ssd_text_set_text(ssd_widget_get(popup,"facebook_user_name"), pAlert->sFacebookName);
          ssd_widget_show(ssd_widget_get(popup,"facebook_user_name"));
          ssd_widget_show(ssd_widget_get(popup,"hspacer_fb"));
   }
   else{
      ssd_text_set_text(ssd_widget_get(popup,"facebook_user_name"), "");
      ssd_widget_hide(ssd_widget_get(popup,"facebook_user_name"));
      ssd_widget_hide(ssd_widget_get(popup,"hspacer_fb"));
   }

   groups_container = ssd_widget_get(popup, "Groups.container");
   if (pAlert->sGroup[0] != 0){
      char      s_groups[RT_ALERT_GROUP_MAXSIZE+20];
	  SsdWidget group_image;
      SsdWidget group_text;
      groups_container->callback = on_groups_callbak;
      groups_container->context = pAlert;
      group_image = ssd_widget_get(groups_container, "Groups.image");
      group_text = ssd_widget_get(groups_container, "Groups.text");
      if (pAlert->sGroupIcon[0] != 0)
	     ssd_bitmap_update(group_image, pAlert->sGroupIcon);
       else
        ssd_bitmap_update(group_image, "groups_default_icons");
      snprintf( s_groups, sizeof(s_groups), "\n %s: %s", roadmap_lang_get("group"), pAlert->sGroup);
      ssd_text_set_text(group_text, s_groups);
      ssd_widget_show(groups_container);
   }
   else{
      groups_container->callback = NULL;
      groups_container->context = NULL;
      ssd_widget_hide(groups_container);
   }

   ssd_widget_set_value(popup, "reported_by_info", ReportedByStr);
   RTAlerts_update_stars(ssd_widget_get(popup,"position_container"),pAlert);
   RTAlerts_show_space_before_desc(popup,pAlert);
   AlertStr[0] = 0;
   RTAlerts_get_report_distance_str( pAlert, AlertStr, sizeof( AlertStr ) );
   ssd_widget_set_value(popup, "alert_distance", AlertStr);
   if (pAlert->sDescription[0] != 0){
      ssd_widget_set_value(popup, "descripion_text", pAlert->sDescription);
      ssd_widget_show(ssd_widget_get(popup,"descripion_text"));
   }
   else{
      ssd_widget_set_value(popup, "descripion_text", "");
      ssd_widget_hide(ssd_widget_get(popup,"descripion_text"));
   }

   if (RtAlerts_get_addional_text(pAlert)){
      ssd_widget_show(ssd_widget_get(popup, "addition_text_container"));
      ssd_text_set_text(ssd_widget_get(popup, "additional_text"), RtAlerts_get_addional_text(pAlert));
      ssd_widget_show(ssd_widget_get(popup, "additional_text"));

      if (RtAlerts_get_addional_text_icon(pAlert)){
          ssd_bitmap_update(ssd_widget_get(popup, "addition_bitmap"), RtAlerts_get_addional_text_icon(pAlert));
          ssd_widget_show(ssd_widget_get(popup, "addition_bitmap"));
      }
      else{
         ssd_widget_hide(ssd_widget_get(popup, "addition_bitmap"));
      }
   }
   else{
      ssd_widget_hide(ssd_widget_get(popup, "addition_text_container"));
   }

#ifdef TOUCH_SCREEN
   button = ssd_widget_get(popup,"Comment_button");
   if (button){
      if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO))
         ssd_widget_hide(button);
      else
         ssd_widget_show(button);

      if ( pAlert->iNumComments != 0){
           char temp[20];
           temp[0] = 0;
           snprintf(temp, sizeof(temp),"%d", pAlert->iNumComments);
           ssd_text_set_text(ssd_widget_get(button, "AddCommentTxt"), temp);
           button->callback = on_view_comments;
      }
      else{
         ssd_text_set_text(ssd_widget_get(button, "AddCommentTxt"), "+");
         button->callback = on_add_comment;
      }
   }

   button = ssd_widget_get( popup, "ViewImage_button");
   // Attachment image
   if (button){
      if ( !pAlert->sImageIdStr[0] && !pAlert->sVoiceIdStr[0])
      {
         ssd_widget_hide(button);
      } else {
         char *icon[3];

         if (pAlert->sImageIdStr[0] && pAlert->sVoiceIdStr[0]) {
            icon[0] = "button_image_voice_attachment_up";
            icon[1] = "button_image_voice_attachment_down";
            icon[2] = NULL;
         } else if (pAlert->sVoiceIdStr[0]) {
            icon[0] = "button_voice_attachment_up";
            icon[1] = "button_voice_attachment_down";
            icon[2] = NULL;
         } else {
            icon[0] = "button_image_attachment_up";
            icon[1] = "button_image_attachment_down";
            icon[2] = NULL;
         }

         ssd_widget_show(button);
         ssd_button_change_icon(button, (const char**) &icon[0], 2);
   }
   }

   button = ssd_widget_get( popup, "ThumbsUp_button");
   // Thumbs up button
   if (button){
       if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
       {
           ssd_widget_hide(button);
       }
       else{
          if ((pAlert->bAlertByMe) || (pAlert->bThumbsUpByMe))
          {
                ssd_widget_show(button);
                ssd_button_disable(button);
          }
          else{
                ssd_widget_show(button);
                ssd_button_enable(button);
          }

          if ( pAlert->iNumThumbsUp != 0){
                 char temp[20];
                 temp[0] = 0;
                 snprintf(temp, sizeof(temp),"%d", pAlert->iNumThumbsUp);
                 ssd_text_set_text(ssd_widget_get(button, "ThumbsUp_buttonTxt"), temp);
          }
          else{
               ssd_text_set_text(ssd_widget_get(button, "ThumbsUp_buttonTxt"), "+");
          }
       }
   }

   text = ssd_widget_get(popup, "ThumbsUpText");
   if (text)
      ssd_widget_hide(text);

#else
    set_left_softkey(popup->parent, pAlert);
    set_right_softkey(popup->parent);
#endif

   position.longitude = pAlert->iLongitude;
   position.latitude = pAlert->iLatitude;
   ssd_popup_update_location(popup, &position);
   ssd_dialog_activate("AlertPopUPDlg", NULL);
   ssd_dialog_refresh_current_softkeys();
   ssd_widget_reset_cache(popup);
   ssd_widget_reset_position(popup);
   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }
}

static void update_ping_wazer_popup(SsdWidget popup, RTAlert *pAlert){
   char AlertStr[700];
   char name[RT_ALERT_FACEBOOK_USERNM_MAXSIZE+10];
   SsdWidget facebook_image;
   SsdWidget fb_image_cont;
   SsdWidget moodW;
   const char *mood_str;
   SsdWidget button;
   ssd_widget_reset_cache(popup);

   mood_str = roadmap_mood_to_string(pAlert->iMood);
   moodW = ssd_widget_get(popup, "ping_mood_bitmap");
   ssd_bitmap_update(moodW, mood_str);
   free((void *)mood_str);

   ssd_widget_set_value(popup, "PingUserFrom", pAlert->sReportedBy);

   if (pAlert->sFacebookName[0] != 0){
       snprintf(name, sizeof(name), "(%s)",pAlert->sFacebookName);
       ssd_widget_set_value(popup, "fb_user_name", name);
       ssd_widget_show(ssd_widget_get(popup, "fb_user_name"));
   }
   else{
      ssd_widget_hide(ssd_widget_get(popup, "fb_user_name"));
   }


   if (pAlert->sDescription[0] != 0){
      ssd_widget_set_value(popup, "descripion_text", pAlert->sDescription);
      AlertStr[0] = 0;
      if (RealtimePrivacyState() == VisRep_Anonymous){
         snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
               "%s:", roadmap_lang_get("Hi"));
      }
      else{
         snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
               "%s %s:", roadmap_lang_get("Hi"), Realtime_GetNickName());
      }
      ssd_widget_set_value(popup, "hello_descripion_text", AlertStr);
   }
   else{
      ssd_widget_set_value(popup, "descripion_text", "");
   }

#ifndef TOUCH_SCREEN
    set_left_softkey(popup->parent, pAlert);
    set_right_softkey(popup->parent);
#endif

   facebook_image = ssd_widget_get(popup, "facebook_image");
   fb_image_cont = ssd_widget_get(popup, "FB_IMAGE_container");

   if (pAlert->bShowFacebookPicture){
      if (facebook_image){

         ssd_bitmap_update(facebook_image, "facebook_default_image");
         roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, pAlert->iID, -1, SOCIAL_IMAGE_SIZE_SQUARE, facebook_image);
         ssd_widget_set_color(fb_image_cont, "#ffffff", "#ffffff");
         ssd_widget_set_offset(moodW, -20, 12);
         moodW->flags |=  SSD_ALIGN_BOTTOM;
         moodW->flags &=  ~ SSD_ALIGN_CENTER;
         ssd_widget_show(facebook_image);
      }
   }
   else{
      ssd_widget_set_color(fb_image_cont, NULL, NULL);
      moodW->flags &= ~ SSD_ALIGN_BOTTOM;
      moodW->flags |=  SSD_ALIGN_CENTER;
      ssd_widget_set_offset(moodW, 0, 0);
      ssd_widget_hide(facebook_image);
   }

   button = ssd_widget_get(popup, "Close_button" );
   if (button)
      ssd_button_change_text(button, roadmap_lang_get ("Close") );


   ssd_dialog_activate("PingWazerPopUPDlg", NULL);
   ssd_dialog_refresh_current_softkeys();
   ssd_widget_reset_cache(popup);
   ssd_widget_reset_position(popup);

   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }
}

char *RtAlerts_get_addional_text(RTAlert *pAlert){
   if (!strcmp(pAlert->sAddOnName,"twitter"))
      return (char *)roadmap_lang_get("Traffic tweets via twitter");
   else
      return NULL;
}

char *RtAlerts_get_addional_text_icon(RTAlert *pAlert){
   if (!strcmp(pAlert->sAddOnName,"twitter"))
      return "twitter_small_icon";
   else
      return NULL;
}

char *RtAlerts_get_addional_keyboard_text(RTAlert *pAlert){
   if (!strcmp(pAlert->sAddOnName,"twitter"))
      return (char *)roadmap_lang_get("your comment is also tweeted to the twitter user");
   else
      return NULL;
}

static int on_groups_callbak (SsdWidget widget, const char *new_value){
   RTAlert *pAlert = (RTAlert *)widget->context;
   if (!pAlert)
      return 0;
   roadmap_groups_show_group(pAlert->sGroup);
   return 1;
}


void RTAlerts_update_location_str(RTAlert *pAlert){

   if (pAlert->sLocationStr[0] == 0){

      const char *street;
      const char *city;
      int   iSquare;
      int   iLineId;
      RoadMapPosition position;

      if (!pAlert)
         return;

      position.longitude = pAlert->iLongitude;
      position.latitude = pAlert->iLatitude;

      // Save Location Description string
      if (!RTAlerts_Get_City_Street(position, &city, &street, &iSquare, &iLineId, pAlert->iDirection))
      {
        city = pAlert->sCityStr;
        street = pAlert->sStreetStr;
      }

      if (!((city == NULL) && (street == NULL)))
      {
          if ((city != NULL) && (strlen(city) == 0))
              snprintf(pAlert->sLocationStr + strlen(pAlert->sLocationStr), sizeof(pAlert->sLocationStr)- strlen(pAlert->sLocationStr), "%s", street);
           else if ((street != NULL) && (strlen(street) == 0))
              snprintf(pAlert->sLocationStr + strlen(pAlert->sLocationStr), sizeof(pAlert->sLocationStr)- strlen(pAlert->sLocationStr),"%s", city);
           else
              snprintf(pAlert->sLocationStr + strlen(pAlert->sLocationStr), sizeof(pAlert->sLocationStr)- strlen(pAlert->sLocationStr),"%s, %s", street, city);
     }

     if (pAlert->sNearStr[0] != 0){
        sprintf (pAlert->sLocationStr + strlen(pAlert->sLocationStr), " %s %s",  roadmap_lang_get("Near"), pAlert->sNearStr);
     }
   }

}
/**
 * Display the pop up of an alert
 * @param alertId - the ID of the alert to display
 * @param iCenterAround - whether to center around the alert or the GPS position
 * @return None
 */
static void RTAlerts_popup_alert(int alertId, int iCenterAround)
{
    RTAlert *pAlert;
    char AlertStr[700];
    const char *focus;
    RoadMapPosition position;
    SsdWidget text, position_con;
    SsdWidget user_info_container;
    SsdWidget user_info_text_container;
    SsdWidget fb_image_cont;
    SsdWidget groups_container;
    SsdWidget group_image;
    SsdWidget group_text;
    SsdWidget groups_text_container;
    SsdWidget groups_image_container;
    SsdWidget facebook_image;
    SsdWidget wgt_hspace;
    SsdWidget button_txt;
    SsdWidget addition_text_container;
    SsdWidget additional_bitmap;
    static SsdWidget popup;
    SsdWidget text_con;
    SsdWidget spacer;
    SsdWidget image_con;
    char *icon[3];
    char temp[10];
    SsdSize size;
    SsdSize image_size;
    SsdWidget bitmap;
    int width = roadmap_canvas_width() ;
    char ReportedByStr[100];
    int image_container_width = ADJ_SCALE(52);
    int num_addOns, i;
    int fb_width = ADJ_SCALE(34);
    int fb_height = ADJ_SCALE(34);
#ifdef TOUCH_SCREEN
    SsdWidget button;
#endif
    AlertStr[0] = 0;

#ifdef IPHONE
   width = ADJ_SCALE(320);
#else
   if (width > roadmap_canvas_height())
      width = roadmap_canvas_height();
#endif // IPHONE

    focus = roadmap_trip_get_focus_name();

    if (gAlertsTable.iCount == 0)
        return;

   //if ( roadmap_screen_is_hd_screen() ){
//      image_container_width = 77;
//      fb_width = 52;
//      fb_height = 52;
//   }

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return;

   pAlert->bPopedUp = TRUE;
   position.longitude = pAlert->iLongitude;
   position.latitude = pAlert->iLatitude;
   AlertStr[0] =0;

   if (iCenterAround != CENTER_NONE)
     RTAlerts_zoom(position, iCenterAround);

   if ( ssd_dialog_exists( "AlertPopUPDlg" ) )
   {
      SsdWidget button = ssd_widget_get(popup, "Close_button" );
      if (button)
         ssd_button_change_text(button,roadmap_lang_get ("Close") );

      gCurrentAlertId = alertId;
      update_popup(popup, pAlert, iCenterAround);

      return;
   }
   gCurrentCommentId = -1;
	bitmap = ssd_bitmap_new("alert_icon", RTAlerts_Get_Icon(pAlert->iID),SSD_ALIGN_CENTER|SSD_WS_TABSTOP);
   num_addOns = RTAlerts_Get_Number_Of_AddOns(pAlert->iID);
   for (i = 0 ; i < num_addOns; i++){
       const char *AddOn = RTAlerts_Get_AddOn(pAlert->iID, i);
       if (AddOn)
          ssd_bitmap_add_overlay(bitmap,AddOn);
   }

	ssd_widget_get_size(bitmap, &size, NULL);
    position_con =
      ssd_container_new ("position_container", "", width-size.width - image_container_width, SSD_MIN_SIZE,0);
   ssd_widget_set_color(position_con, NULL, NULL);

    text_con =
      ssd_container_new ("text_conatiner", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
    ssd_widget_set_color(text_con, NULL, NULL);

    text = ssd_text_new("popuup_text", RTAlerts_get_title(pAlert, pAlert->iType, pAlert->iSubType), 18, SSD_END_ROW);
    ssd_widget_set_color(text,"#f6a201", NULL);
    ssd_widget_add(position_con, text);

   // Display when the alert street name and city
   RTAlerts_update_location_str(pAlert);
   text = ssd_text_new("alert_location", pAlert->sLocationStr, 16, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
	ssd_widget_set_color(text,"#f6a201", NULL);
	ssd_widget_add(position_con, text);
	AlertStr[0] = 0;
   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( position_con, spacer );

   if (pAlert->sDescription[0] != 0){
     //Display the alert description
     snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "%s", pAlert->sDescription);
     text = ssd_text_new("descripion_text", AlertStr, 16,0);
     ssd_widget_set_color(text,"#ffffff", NULL);
     ssd_widget_add(text_con, text);
     ssd_widget_add(position_con, text_con);
     AlertStr[0] = 0;
   }
   else{
      text = ssd_text_new("descripion_text", "", 14,0);
      ssd_widget_set_color(text,"#ffffff", NULL);
      ssd_widget_add(text_con, text);
      ssd_widget_add(position_con, text_con);
      AlertStr[0] = 0;
   }

   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( position_con, spacer );

    RTAlerts_get_report_info_str( pAlert, AlertStr, sizeof( AlertStr ) );

    text = ssd_text_new("alert_info", AlertStr, 14,SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(position_con, text);

    //ssd_dialog_add_vspace(position_con, 5, 0);
    user_info_container =
      ssd_container_new ("user_info_container", "", width-size.width -2, SSD_MIN_SIZE,0);
    ssd_widget_set_color(user_info_container, NULL, NULL);

    user_info_text_container =
      ssd_container_new ("user_info_text_container", "", width-size.width -2-fb_width-4, SSD_MIN_SIZE,0);
    ssd_widget_set_color(user_info_text_container,NULL, NULL);

    fb_image_cont = ssd_container_new ("FB_IMAGE_container", "", fb_width, fb_height, SSD_ALIGN_VCENTER);
    ssd_widget_set_color(fb_image_cont, "#ffffff", "#ffffff");
    facebook_image = ssd_bitmap_new("facebook_image", "facebook_default_image_small", SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
    ssd_widget_add(fb_image_cont, facebook_image);
    ssd_widget_add(user_info_container, fb_image_cont);



    ReportedByStr[0] = 0;
    RTAlerts_get_reported_by_string(pAlert,ReportedByStr,sizeof(ReportedByStr));
    text = ssd_text_new("reported_by_info", ReportedByStr, 14,SSD_TEXT_NORMAL_FONT);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_dialog_add_hspace(user_info_text_container, 5, 0);
    ssd_widget_add(user_info_text_container, text);
    RTAlerts_update_stars(user_info_text_container, pAlert);

    text = ssd_text_new("facebook_user_name", "", 14,SSD_TEXT_NORMAL_FONT);
    wgt_hspace = ssd_container_new( "hspacer_fb", "", 5, 20, SSD_START_NEW_ROW );
    ssd_widget_set_color( wgt_hspace, NULL, NULL );
    ssd_widget_add( user_info_text_container, wgt_hspace );

    ssd_widget_set_color(text,"#ffffff", NULL);
    if (pAlert->sFacebookName[0] != 0){
       char name[RT_ALERT_FACEBOOK_USERNM_MAXSIZE+2];
       snprintf(name, sizeof(name), "(%s)",pAlert->sFacebookName);
       ssd_text_set_text(text, name);
    }
    else{
       ssd_widget_hide(text);
       ssd_widget_hide(ssd_widget_get(user_info_text_container,"hspacer_fb"));
    }

    ssd_widget_add(user_info_text_container, text);
    ssd_widget_add(user_info_container, user_info_text_container);

    ssd_widget_add(position_con, user_info_container);

    // Groups container
    groups_container =
      ssd_container_new ("Groups.container", "", width-size.width -2, SSD_MIN_SIZE,0);
    ssd_widget_set_color(groups_container, NULL, NULL);
    groups_container->callback = on_groups_callbak;
    groups_container->context = pAlert;
    ssd_dialog_add_vspace(groups_container, 3, 0);
    groups_image_container =
      ssd_container_new ("Groups.image.container", "",SSD_MIN_SIZE, SSD_MIN_SIZE,0);
    ssd_widget_set_color(groups_image_container, NULL, NULL);
    group_image = ssd_bitmap_new("Groups.image", "groups_default_icons", SSD_ALIGN_VCENTER);
    ssd_widget_add(groups_image_container, group_image);
    ssd_dialog_add_hspace(groups_image_container, 5, 0);
    ssd_widget_add(groups_container, groups_image_container);
    ssd_widget_get_size(groups_image_container, &image_size, NULL);
    groups_text_container =
      ssd_container_new ("Groups.text.container", "",width-size.width -2-image_size.width-5, SSD_MIN_SIZE,SSD_ALIGN_VCENTER);
    ssd_widget_set_color(groups_text_container, NULL, NULL);
    group_text = ssd_text_new("Groups.text", "", -1, SSD_ALIGN_VCENTER);
    ssd_widget_set_color(group_text,"#ffffff", NULL);
    ssd_widget_add(groups_text_container, group_text);
    ssd_widget_add(groups_container, groups_text_container);
    ssd_widget_add(position_con, groups_container);
    if (pAlert->sGroup[0]){
       char      s_groups[RT_ALERT_GROUP_MAXSIZE+20];
       snprintf( s_groups, sizeof(s_groups), "\n %s: %s", roadmap_lang_get("group"), pAlert->sGroup);
       if (pAlert->sGroupIcon[0] != 0)
          ssd_bitmap_update(group_image, pAlert->sGroupIcon);
       else
          ssd_bitmap_update(group_image, "groups_default_icons");
       ssd_text_set_text(group_text, s_groups);
    }
    else{
       ssd_widget_hide(groups_container);
    }
    ssd_widget_add (position_con,
    ssd_container_new ("space_before_descrip", NULL, 0, 10, SSD_END_ROW));
    RTAlerts_show_space_before_desc(position_con,pAlert);
    AlertStr[0] = 0;

    if (ReportedByStr[0] == 0)
       ssd_widget_hide(user_info_container);

    else if (pAlert->bShowFacebookPicture){
       int facebook_image_size = ADJ_SCALE(32);
       //if (roadmap_screen_is_hd_screen())
//          facebook_image_size = 50;
       roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, pAlert->iID, -1, facebook_image_size,  facebook_image );
    }
    else{
       ssd_widget_hide(fb_image_cont);
    }


    gCurrentAlertId = alertId;
    gCurrentCommentId = -1;

    popup = ssd_popup_new("AlertPopUPDlg", "", on_popup_close, SSD_MAX_SIZE, SSD_MIN_SIZE,&position, SSD_ROUNDED_BLACK|SSD_POINTER_LOCATION,DIALOG_ANIMATION_FROM_TOP);


    /* Makes it possible to click in the bottom vicinity of the buttons  */
    ssd_widget_set_click_offsets_ext( popup, 0, 0, 0, 15 );

    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( popup, spacer );

    image_con =
      ssd_container_new ("IMAGE_container", "", image_container_width, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
    ssd_widget_set_color(image_con, NULL, NULL);

#ifndef TOUCH_SCREEN
      bitmap->key_pressed = Alert_OnKeyPressed;
#endif

	ssd_widget_add(image_con, bitmap);
   AlertStr[0] = 0;
   RTAlerts_get_report_distance_str( pAlert, AlertStr, sizeof( AlertStr ) );

   text = ssd_text_new("alert_distance", AlertStr, 14, SSD_ALIGN_CENTER);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(image_con, text);

   ssd_widget_add(popup, image_con);

    ssd_widget_add(popup, position_con);

    addition_text_container =
          ssd_container_new ("addition_text_container", "", SSD_MIN_SIZE, SSD_MIN_SIZE,0);
    ssd_widget_set_color(addition_text_container, NULL, NULL);

    additional_bitmap = ssd_bitmap_new("addition_bitmap", "", SSD_ALIGN_VCENTER);
    ssd_widget_add(addition_text_container, additional_bitmap);
    ssd_dialog_add_hspace(addition_text_container, 2, 0);

    text = ssd_text_new("additional_text", "", 14, SSD_ALIGN_VCENTER|SSD_END_ROW);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(addition_text_container, text);

    if (RtAlerts_get_addional_text(pAlert)){
       ssd_text_set_text(text, RtAlerts_get_addional_text(pAlert));
       if (RtAlerts_get_addional_text_icon(pAlert))
          ssd_bitmap_update(additional_bitmap, RtAlerts_get_addional_text_icon(pAlert));
       else
          ssd_widget_hide(additional_bitmap);
    }
    else{
       ssd_widget_hide(addition_text_container);
    }


    ssd_widget_add(popup,addition_text_container);

#ifdef TOUCH_SCREEN

    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( popup, spacer );

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    ssd_widget_add(popup, button);

    icon[0] = "button_comment_up";
    icon[1] = "button_comment_down";
    icon[2] = NULL;
    button = ssd_button_new( "Comment_button", "Comment", (const char**) &icon[0], 2, 0, on_add_comment );
    button_txt = ssd_text_new("AddCommentTxt","+", 16, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
    if (ssd_widget_rtl(NULL))
       ssd_widget_set_offset(button_txt, ADJ_SCALE(-10), 0);
    else
       ssd_widget_set_offset(button_txt, ADJ_SCALE(10), 0);
    ssd_text_set_color(button_txt, "#ffffff");
    ssd_widget_add(button, button_txt);
//    button = ssd_button_label("Comment_button", roadmap_lang_get("Comment"), SSD_ALIGN_CENTER, on_add_comment);
    ssd_widget_add(popup, button);


    if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)){
       ssd_widget_hide(button);
    }
    if ( pAlert->iNumComments != 0){
       temp[0] = 0;
       snprintf(temp, sizeof(temp),"%d", pAlert->iNumComments);
       ssd_text_set_text(button_txt, temp);
       button->callback = on_view_comments;
    }



    // Thumbs Up
    icon[0] = "thumbs_up_button";
    icon[1] = "thumbs_up_button_down";
    icon[2] = "thumbs_up_button_disabled";
    button = ssd_button_new( "ThumbsUp_button", "ThumbsUp", (const char**) &icon[0], 3, SSD_ALIGN_CENTER, on_thumbs_up );
    button_txt = ssd_text_new("ThumbsUp_buttonTxt","+", 16, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
    if (ssd_widget_rtl(NULL))
        ssd_widget_set_offset(button_txt, ADJ_SCALE(-10), 0);
    else
        ssd_widget_set_offset(button_txt, ADJ_SCALE(10), 0);
    ssd_text_set_color(button_txt, "#ffffff");
    ssd_widget_add(button, button_txt);
    if ( pAlert->iNumThumbsUp != 0){
        temp[0] = 0;
        snprintf(temp, sizeof(temp),"%d", pAlert->iNumThumbsUp);
        ssd_text_set_text(button_txt, temp);
    }


    ssd_widget_add(popup, button);

    // Thumbs up button
    if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
    {
       ssd_widget_hide(button);
    }

    if ((pAlert->bAlertByMe) || (pAlert->bThumbsUpByMe))
    {
        ssd_button_disable(button);
    }

    icon[0] = "button_image_attachment_up";
    icon[1] = "button_image_attachment_down";
    icon[2] = NULL;
    button = ssd_button_new( "ViewImage_button", "View image", (const char**) &icon[0], 2, SSD_ALIGN_CENTER, on_view_attachment );
    ssd_widget_add(popup, button);

    // Attachment image
    if ( !pAlert->sImageIdStr[0] && !pAlert->sVoiceIdStr[0])
    {
       ssd_widget_hide(button);
    } else {
       if (pAlert->sImageIdStr[0] && pAlert->sVoiceIdStr[0]) {
          icon[0] = "button_image_voice_attachment_up";
          icon[1] = "button_image_voice_attachment_down";
          icon[2] = NULL;
          ssd_button_change_icon(button, (const char**) &icon[0], 2);
       } else if (pAlert->sVoiceIdStr[0]) {
          icon[0] = "button_voice_attachment_up";
          icon[1] = "button_voice_attachment_down";
          icon[2] = NULL;
          ssd_button_change_icon(button, (const char**) &icon[0], 2);
    }
    }



    text = ssd_text_new("ThumbsUpText",roadmap_lang_get("Thanks sent to user"), -1, SSD_START_NEW_ROW|SSD_ALIGN_CENTER);
    ssd_text_set_color(text, "#f6a201");
    ssd_widget_add(popup, text);
    ssd_widget_hide(text);

#else
    set_left_softkey(popup->parent, pAlert);
    set_right_softkey(popup->parent);
#endif

   ssd_dialog_activate("AlertPopUPDlg", NULL);
   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }

}

static void RTAlerts_PingWazer_Timer(void){
   g_ping_dlg_seconds --;
   if (g_ping_dlg_seconds > 0){
#ifdef TOUCH_SCREEN
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "PingWazerPopUPDlg"))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close_button" );
         if (g_ping_dlg_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_ping_dlg_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
#endif
      return;
   }

   ssd_dialog_hide("PingWazerPopUPDlg", dec_close);

   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

static void on_ping_dlg_close  (int exit_code, void* context){
   roadmap_main_remove_periodic(RTAlerts_PingWazer_Timer);
}

static void RTAlerts_popup_PingWazer(RTAlert *pAlert)
{
    char name[RT_ALERT_FACEBOOK_USERNM_MAXSIZE+10];
    char AlertStr[700];
    const char *focus;
    SsdWidget text, position_con;
    static SsdWidget pingWazerPopUP;
    SsdWidget text_con;
    SsdWidget spacer;
    SsdWidget image_con;
    const char *mood_str;
    SsdSize size;
    SsdWidget bitmap;
    SsdWidget facebook_image;
    SsdWidget fb_image_cont;
    static RoadMapSoundList list;
    int width = roadmap_canvas_width() ;
    int image_container_width = ADJ_SCALE(70);
    int f_width = ADJ_SCALE(52);
    int f_height = ADJ_SCALE(52);
    char *icon[3];
#ifdef TOUCH_SCREEN
    SsdWidget button;
#endif
    AlertStr[0] = 0;

    if (width > roadmap_canvas_height())
      width = roadmap_canvas_height();
    focus = roadmap_trip_get_focus_name();

    if (gAlertsTable.iCount == 0)
        return;

//   if ( roadmap_screen_is_hd_screen() )
//      image_container_width = 100;

   if (pAlert == NULL)
        return;


   AlertStr[0] =0;

   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "ping");
      roadmap_res_get (RES_SOUND, 0, "ping");
   }
   roadmap_sound_play_list (list);

   if (pingWazerPopUP){
      gCurrentAlertId = pAlert->iID;
      gCurrentCommentId = -1;
      update_ping_wazer_popup(pingWazerPopUP, pAlert);
      g_ping_dlg_seconds = PING_POPUP_TIMER;
      roadmap_main_set_periodic(1000, RTAlerts_PingWazer_Timer);

      return;
   }

   mood_str = roadmap_mood_to_string(pAlert->iMood);
   bitmap = ssd_bitmap_new("ping_mood_bitmap", mood_str, SSD_ALIGN_BOTTOM);
   free((void *)mood_str);

   ssd_widget_get_size(bitmap, &size, NULL);
   position_con =
      ssd_container_new ("position_container", "", width -size.width - image_container_width- ADJ_SCALE(10), SSD_MIN_SIZE,0);
   ssd_widget_set_color(position_con, NULL, NULL);

   text_con =
      ssd_container_new ("text_conatiner", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
   ssd_widget_set_color(text_con, NULL, NULL);

   text = ssd_text_new("popuup_text", roadmap_lang_get("Ping from: "), 18, 0);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(position_con, text);
   ssd_dialog_add_hspace(position_con, 5, 0);
   text = ssd_text_new("PingUserFrom", pAlert->sReportedBy, 18, SSD_END_ROW);
   ssd_widget_set_color(text,"#f6a201", NULL);
   ssd_widget_add(position_con, text);

   if (pAlert->sFacebookName[0] != 0){
       snprintf(name, sizeof(name), "(%s)",pAlert->sFacebookName);
       text = ssd_text_new("fb_user_name", name, 18,SSD_END_ROW);
       ssd_widget_set_color(text,"#f6a201", NULL);
       ssd_widget_add(position_con, text);
   }
   else{
      text = ssd_text_new("fb_user_name", "", 18,SSD_END_ROW);
      ssd_widget_set_color(text,"#f6a201", NULL);
      ssd_widget_add(position_con, text);
      ssd_widget_hide(text);
   }
   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( position_con, spacer );

   if (pAlert->sDescription[0] != 0){
      AlertStr[0] = 0;
      if (RealtimePrivacyState() == VisRep_Anonymous){
         snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
               "%s:", roadmap_lang_get("Hi"));
      }
      else{
         snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
               "%s %s:", roadmap_lang_get("Hi"), Realtime_GetNickName());
      }
      text = ssd_text_new("hello_descripion_text", AlertStr, 18,0);
      ssd_widget_set_color(text,"#ffffff", NULL);
      ssd_widget_add(text_con, text);

      spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
      ssd_widget_set_color( spacer, NULL, NULL );
      ssd_widget_add( text_con, spacer );

      text = ssd_text_new("descripion_text", pAlert->sDescription, 18,SSD_END_ROW);
      ssd_widget_set_color(text,"#ffffff", NULL);
      ssd_widget_add(text_con, text);

      ssd_widget_add(position_con, text_con);
      AlertStr[0] = 0;
    }
    else{
       text = ssd_text_new("hello_descripion_text", "", 18,0);
       ssd_widget_set_color(text,"#ffffff", NULL);
       ssd_widget_add(text_con, text);
       text = ssd_text_new("descripion_text", "", 20,0);
       ssd_widget_set_color(text,"#f6a201", NULL);
       ssd_widget_add(text_con, text);
       ssd_widget_add(position_con, text_con);
       AlertStr[0] = 0;
    }


   gCurrentAlertId = pAlert->iID;
   gCurrentCommentId = -1;

    pingWazerPopUP = ssd_popup_new("PingWazerPopUPDlg", "", on_ping_dlg_close, SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_ROUNDED_BLACK|SSD_POINTER_COMMENT,DIALOG_ANIMATION_FROM_TOP);

    /* Makes it possible to click in the bottom vicinity of the buttons  */
    ssd_widget_set_click_offsets_ext( pingWazerPopUP, 0, 0, 0, 15 );

    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( pingWazerPopUP, spacer );

    image_con =
      ssd_container_new ("IMAGE_container", "", image_container_width, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
    ssd_widget_set_color(image_con, NULL, NULL);

#ifndef TOUCH_SCREEN
      bitmap->key_pressed = Alert_OnKeyPressed;
#endif


    //if (roadmap_screen_is_hd_screen()){
//       f_height = 77;
//       f_width = 77;
//    }

    fb_image_cont = ssd_container_new ("FB_IMAGE_container", "", f_width, f_height, SSD_ALIGN_CENTER);
    ssd_widget_set_color(fb_image_cont, "#ffffff", "#ffffff");

    facebook_image = ssd_bitmap_new("facebook_image", "facebook_default_image", SSD_ALIGN_CENTER);
    ssd_widget_set_offset(facebook_image, 0, 1);
    ssd_widget_add(fb_image_cont, facebook_image);
    ssd_widget_set_offset(bitmap, ADJ_SCALE(-20), ADJ_SCALE(12));
    ssd_widget_add(image_con, fb_image_cont);
    spacer = ssd_container_new( "space", "", image_container_width, ADJ_SCALE(7), 0 );
    ssd_widget_set_color(spacer, NULL, NULL);
    ssd_widget_add(image_con, spacer);
    //mood
    ssd_widget_add(fb_image_cont, bitmap);

   if (pAlert->bShowFacebookPicture){
       roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, pAlert->iID, -1, SOCIAL_IMAGE_SIZE_SQUARE, facebook_image);
   }
   else{
      ssd_widget_set_color(fb_image_cont, NULL, NULL);
      bitmap->flags &= ~ SSD_ALIGN_BOTTOM;
      bitmap->flags |=  SSD_ALIGN_CENTER;
      ssd_widget_set_offset(bitmap, 0, 0);
      ssd_widget_hide(facebook_image);
   }

   AlertStr[0] = 0;
   RTAlerts_get_report_distance_str( pAlert, AlertStr, sizeof( AlertStr ) );

   ssd_widget_add(pingWazerPopUP, image_con);

    ssd_widget_add(pingWazerPopUP, position_con);
#ifdef TOUCH_SCREEN

    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, ADJ_SCALE(5), SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( pingWazerPopUP, spacer );

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_ping_wazer_button_close);
    ssd_widget_add(pingWazerPopUP, button);

    button = ssd_button_label("Comment_button", roadmap_lang_get("Reply"), SSD_ALIGN_CENTER, on_add_comment);
    ssd_widget_add(pingWazerPopUP, button);

    icon[0] = "button_report_abuse";
    icon[1] = "button_report_abuse_down";
    icon[2] = NULL;
    button = ssd_button_new( "Reprt_Abuse_Button", "ReportAbuse", (const char**) &icon[0], 2, SSD_ALIGN_CENTER, on_report_abuse );
    ssd_widget_add(pingWazerPopUP, button);

#else
    set_left_softkey(pingWazerPopUP->parent, pAlert);
    set_right_softkey(pingWazerPopUP->parent);
#endif
    ssd_dialog_add_vspace(pingWazerPopUP, 3, 0);
    text = ssd_text_new("disclaimer", roadmap_lang_get("FYI, Pings are publicly viewable."), 12,0);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(pingWazerPopUP, text);
    ssd_dialog_add_vspace(pingWazerPopUP, 3, 0);

    g_ping_dlg_seconds = PING_POPUP_TIMER;
    roadmap_main_set_periodic(1000, RTAlerts_PingWazer_Timer);

    ssd_dialog_activate("PingWazerPopUPDlg", NULL);
    if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
    }

}
/**
 * Display the pop up of a specific alert
 * @param IId - the ID of the alert to popup
 * @return None
 */
void RTAlerts_Popup_By_Id(int iID, BOOL saveContext)
{
    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {
       if (saveContext){
          const char *focus = roadmap_trip_get_focus_name();
          if (focus && (strcmp(focus, "GPS")))
             roadmap_math_get_context(&gSavedPosition, &gSavedZoom);

          gSavedFocus = focus;
       }
       else{
          gSavedZoom = -2;
       }
       roadmap_screen_hold();
    }
    g_alert_popup_active = FALSE;
    RTAlerts_popup_alert(iID, CENTER_ON_ALERT);
}


static void RTAlerts_Alert_PopUp_Timer(void){
   g_alert_popup_seconds --;
   if (g_alert_popup_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close_button" );
         if (g_alert_popup_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_alert_popup_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }

   roadmap_main_remove_periodic(RTAlerts_Alert_PopUp_Timer);
   g_alert_popup_active = FALSE;
   ssd_dialog_hide("AlertPopUPDlg", dec_close);

   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

void RTAlerts_Popup_By_Id_Timed(int iID, BOOL saveContext, int timeOut){
   RTAlerts_Popup_By_Id(iID, saveContext);
   if (g_alert_popup_active)
      roadmap_main_remove_periodic(RTAlerts_Alert_PopUp_Timer);

   g_alert_popup_active = TRUE;
   g_alert_popup_seconds = timeOut;
   roadmap_main_set_periodic(1000, RTAlerts_Alert_PopUp_Timer);
}

#define SSD_RT_ALERT_IMAGE_DLG_NAME				"RT Alert View Image Dialog"
#define SSD_RT_ALERT_IMAGE_DLG_IMGCON			"RT Alert View Image Dialog.Image container"
#define SSD_RT_ALERT_IMAGE_DLG_BITMAP			"RT Alert View Image Dialog.Bitmap"
/**
 * Close the image display dialog
 * @param -
 * @return None
 */
int RTAlerts_View_Image_DlgClose_Callback( SsdWidget widget, const char *new_value )
{
	RoadMapImage image = ( RoadMapImage ) ssd_dialog_get_current_data();
	// Deallocate the image resources
	roadmap_canvas_free_image( image );

	ssd_dialog_hide( SSD_RT_ALERT_IMAGE_DLG_NAME, 0 );
	return 1;
}

/**
 * Download callback for the voice
 * @param - the ID of the alert
 * @return None
 */
static void RTAlerts_Download_Voice_Callback( void* context, int status, const char *voice_path  )
{
	int alertId = ((AttachmentDownloadCbContext*)context)->alertId;

	RTAlerts_CloseProgressDlg();

	if ( status == 0 )
	{
      RoadMapSoundList     list  = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, voice_path);
      roadmap_sound_play_list (list);
	}
	else
	{
		roadmap_log( ROADMAP_ERROR, "Error downloading voice. Alert ID: %d. Status : %d. Path: %s",
                  alertId, status, voice_path );
		roadmap_messagebox("Oops", "Voice download failed");
	}

   free( context );
}

/**
 * Trying to download the voice attached to the alert
 * @param - the ID of the alert
 * @return None
 */
void RTAlerts_Download_Voice( int alertId )
{
	RTAlert *pAlert;
	AttachmentDownloadCbContext *context = ( AttachmentDownloadCbContext* ) malloc( sizeof( AttachmentDownloadCbContext ) );
	context->alertId = alertId;

	pAlert = RTAlerts_Get_By_ID( alertId );

	if (pAlert && pAlert->sVoiceIdStr[0]){
	   roadmap_analytics_log_event(ANALYTICS_EVENT_DOWNLOAD_ATTACHMENT, ANALYTICS_EVENT_INFO_TYPE, "VOICE");
      roadmap_recorder_voice_download(pAlert->sVoiceIdStr, context, RTAlerts_Download_Voice_Callback );
	}
}

/**
 * Download callback for the image
 * @param - the ID of the alert
 * @return None
 */
static void RTAlerts_Download_Image_Callback( void* context, int status, const char *image_path  )
{
	RoadMapImage image;
	int alertId = ((AttachmentDownloadCbContext*)context)->alertId;

	RTAlerts_CloseProgressDlg();

	if ( status == 0 )
	{

		image = roadmap_jpeg_read_file( NULL, image_path );

		if ( image ) {
			RTAlerts_Show_Image( alertId, image );
		} else {
			roadmap_log( ROADMAP_ERROR, "Error reading image from file: %d", alertId );
			roadmap_messagebox("Oops", "Image download failed");
		}
	}
	else
	{
		roadmap_log( ROADMAP_ERROR, "Error downloading image. Alert ID: %d. Status : %d. Path: %s",
				alertId, status, image_path );
		roadmap_messagebox("Oops", "Image download failed");
	}

    free( context );

}

/**
 * Trying to download the image attached to the alert
 * @param - the ID of the alert
 * @return None
 */
void RTAlerts_Download_Image( int alertId )
{
	RTAlert *pAlert;
	AttachmentDownloadCbContext *context = ( AttachmentDownloadCbContext* ) malloc( sizeof( AttachmentDownloadCbContext ) );
	context->alertId = alertId;

	pAlert = RTAlerts_Get_By_ID( alertId );

	if (pAlert && pAlert->sImageIdStr[0]){
	   roadmap_analytics_log_event(ANALYTICS_EVENT_DOWNLOAD_ATTACHMENT, ANALYTICS_EVENT_INFO_TYPE, "IMG");
      roadmap_camera_image_download( pAlert->sImageIdStr, context, RTAlerts_Download_Image_Callback );
   }

}
/**
 * Display the image attached to the alert
 * @param - the ID of the alert
 * @param image - the image to display
 * @return None
 */
static void RTAlerts_Show_Image( int alertId, RoadMapImage image )
{
	SsdWidget dialog = NULL;
	static SsdWidget image_bitmap = NULL;

    dialog = ssd_dialog_activate( SSD_RT_ALERT_IMAGE_DLG_NAME, NULL );

    if ( !dialog )
    {
#ifdef TOUCH_SCREEN
        SsdWidget close_btn;
     	SsdWidget top_space;
    	char* icon[2];
    	SsdClickOffsets btn_close_offsets = {-20, -20, 20, 20};
#endif
        SsdWidget image_container;
		dialog = ssd_dialog_new( SSD_RT_ALERT_IMAGE_DLG_NAME, "", NULL, SSD_CONTAINER_BORDER|
									SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK );
		if ( !dialog )
		{
			roadmap_log( ROADMAP_ERROR, "Error creating view image dialog" );
			return;
		}
#ifdef TOUCH_SCREEN
		// Top space container
		top_space = ssd_container_new( "RT Alert View Image Dialog.Top Space", NULL,
				50, SSD_MIN_SIZE, SSD_ALIGN_RIGHT|SSD_END_ROW );
		ssd_widget_set_color ( top_space, NULL, NULL );
		// Close button
		icon[0] = "rm_quit";
		icon[1] = "rm_quit_pressed";
		close_btn = ssd_button_new( "RT Alert View Image Dialog.Close", "", (const char**) &icon[0], 2,
				SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER, RTAlerts_View_Image_DlgClose_Callback );
		ssd_widget_add( top_space, close_btn );
		ssd_widget_set_click_offsets( close_btn, &btn_close_offsets );
		ssd_widget_set_click_offsets( top_space, &btn_close_offsets );
		ssd_widget_add( dialog, top_space );
#endif
		image_container = ssd_container_new( SSD_RT_ALERT_IMAGE_DLG_IMGCON, NULL,
				roadmap_canvas_image_width( image ) + 4, /* All downloaded images supposed to be of the same size per compilation/configuration */
				roadmap_canvas_image_height( image ) + 3, /* additional pixels for the borders 															*/
				SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_END_ROW );
		image_bitmap = ssd_bitmap_image_new( SSD_RT_ALERT_IMAGE_DLG_BITMAP, image, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER );
		ssd_widget_set_offset( image_bitmap, 0, -1 );
		ssd_widget_add( image_container, image_bitmap );
		ssd_widget_add( dialog, image_container );

		ssd_dialog_activate( SSD_RT_ALERT_IMAGE_DLG_NAME, NULL );

    }

    ssd_bitmap_image_update( image_bitmap, image );

    dialog->data = (void*) image;

    ssd_dialog_draw();
}

/**
 * Display the pop up of a specific alert without centering
 * @param IId - the ID of the alert to popup
 * @return None
 */
void RTAlerts_Popup_By_Id_No_Center(int iID)
{
    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {
        gSavedFocus = roadmap_trip_get_focus_name();
        roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
        roadmap_screen_hold();
    }
    else{
       ssd_dialog_hide_current(dec_cancel);
       roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
       roadmap_screen_hold();
    }
    roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
    RTAlerts_popup_alert(iID, CENTER_NONE);
}
/**
 * Scrolls through all the alerts by popups
 * @param None
 * @return None
 */
BOOL RTAlerts_Scroll_All(void)
{
    RTAlert *pAlert;
    const char *focus = roadmap_trip_get_focus_name();

    RTAlerts_Set_Ignore_Max_Distance(FALSE);
    RealTime_SetMapDisplayed(FALSE);
    gIdleScrolling = TRUE;

    if (!ssd_dialog_is_currently_active())
    {
       roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
       gSavedFocus = focus;

       if (RTAlerts_Live_Is_Empty())
			  return FALSE;
        RTAlerts_Sort_List(sort_priority);
        pAlert = RTAlerts_Get(0);
        if ((pAlert->iDistance > roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist))&&
    	(!gIgnoreAlertMaxDist))
   		{
   			return FALSE; // closest alert is too far away! don't scroll at all.
   		}
        roadmap_screen_hold();
        roadmap_screen_set_Xicon_state(FALSE);
        gIterator = -1;
        gCurrentAlertId = -1;
        gCurrentCommentId = -1;
        gPopAllTimerActive = TRUE;
        RTAlerts_Popup();
        roadmap_main_set_periodic(6000, RTAlerts_Popup);
        return TRUE;
    }
    return FALSE;
}

/**
 * Remove the pop up of the alert and stop scrolling
 * @param None
 * @return None
 */
void RTAlerts_Stop_Scrolling()
{

    if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg")))
    {
    	if (gSavedZoom == -1){
           roadmap_trip_set_focus("GPS");
           roadmap_math_zoom_reset();
    	}else{
    	   roadmap_math_set_context(&gSavedPosition, gSavedZoom);
    	   roadmap_trip_set_focus("GPS");
    	   gSavedPosition.latitude = -1;
    	   gSavedPosition.longitude = -1;
    	   gSavedZoom = -2;
    	}

   	ssd_dialog_hide_current(dec_cancel);

      roadmap_layer_adjust();
      roadmap_screen_redraw();
    }
    else{
       if (gSavedZoom == -1){
            roadmap_trip_set_focus("GPS");
            roadmap_math_zoom_reset();
       }else{
          roadmap_math_set_context(&gSavedPosition, gSavedZoom);
          roadmap_trip_set_focus("GPS");
          gSavedPosition.latitude = -1;
          gSavedPosition.longitude = -1;
          gSavedZoom = -2;
       }

       roadmap_layer_adjust();
       roadmap_screen_redraw();

    }

    if (gPopAllTimerActive){
       roadmap_main_remove_periodic(RTAlerts_Popup) ;
       gPopAllTimerActive = FALSE;
    }
    gIterator = 0;
    gIdleScrolling = FALSE;
}

/**
 * Remove the alert and stop the Alerts timer.
 * @param None
 * @return None
 */
void RTAlerts_Cancel_Scrolling()
{
    RTAlerts_Stop_Scrolling();
}

/**
 * Zoomin to the alert
 * @param None
 * @return False, if we are already zoomed on the alert, TRUE otherwise
 */
BOOL RTAlerts_Zoomin_To_Aler()
{
    RTAlert *pAlert;

    if (gCentered)
        return FALSE;

    if (gIterator >= gAlertsTable.iCount)
        gIterator = 0;
    else if (gIterator < 0)
        gIterator = 0;

    pAlert = RTAlerts_Get(gIterator);
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ALERT);
    return TRUE;
}

/**
 * Popup the next alert
 * @param None
 * @return None
 */
void RTAlerts_Scroll_Next()
{
    RTAlert *pAlert;
    gState = STATE_OLD;

    if (RTAlerts_Is_Empty())
        return;

    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {
        RTAlerts_Sort_List(sort_proximity);
        gIterator = -1;
    }

    if (gIterator < gAlertsTable.iCount-1)
        gIterator ++;
    else
        gIterator = 0;

    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;
    roadmap_screen_hold();

    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);

}

/**
 * Popup the previous alert
 * @param None
 * @return None
 */
void RTAlerts_Scroll_Prev()
{
    RTAlert *pAlert;
    gState = STATE_OLD;
    if (RTAlerts_Is_Empty())
        return;

    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {
        RTAlerts_Sort_List(sort_proximity);
        gIterator = 0;
    }

    if (gIterator >0)
        gIterator --;
    else
        gIterator=gAlertsTable.iCount-1;

    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;

    roadmap_screen_hold();
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);
}

/**
 * Returns the currently displayed alert ID
 * @param None
 * @return the id of the alert currently being displayed
 */
int RTAlerts_Get_Current_Alert_Id()
{
    return gCurrentAlertId;
}

typedef struct
{
    int iType;
    int iSubType;
    int iDirection;
    const char * szDescription;
    const char * szGroup;
} RTAlertContext;

/**
 * Report Alert keyboard callback
 * @param type - the button that was pressed, new_value - text, context - hold a pointer to the alert context
 * @return int 1
 */
static BOOL on_keyboard_closed(  int         exit_code,
                                 const char* value,
                                 void*       context)
{
    const char *desc;
    upload_Image_context * uploadContext;
    RTAlertContext *AlertContext = (RTAlertContext *)context;
#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE
    if( dec_ok != exit_code)
        return TRUE;

	if ((AlertContext->iType == RT_ALERT_TYPE_CHIT_CHAT) &&  (value[0] == 0)){
		return FALSE;
	}

    if (value[0] == 0)
        desc = AlertContext->szDescription;
    else
        desc = value;

    if (!desc)
    	desc = "";

    RTAlerts_ShowProgressDlg();
    uploadContext = (upload_Image_context *)malloc(sizeof(upload_Image_context));
    uploadContext->desc = strdup(desc);
    uploadContext->iDirection = AlertContext->iDirection;
    uploadContext->iType = AlertContext->iType;
    uploadContext->iSubType = AlertContext->iSubType;

    if (AlertContext->szGroup)
       uploadContext->group = strdup(AlertContext->szGroup);
    else
       uploadContext->group = NULL;

    if ( gCurrentImagePath ){
       roadmap_analytics_log_event(ANALYTICS_EVENT_IMAGE_ALERT, NULL, NULL);
   	 if (!roadmap_camera_image_upload( NULL, gCurrentImagePath,
   			 gCurrentImageId, continue_report_after_image_upload, (void *)uploadContext )){
   		 roadmap_log(ROADMAP_ERROR,"Error in uploading image alert");
   		 continue_report_after_image_upload( ( (void *)uploadContext)); // error in image upload, continue
   	 }else{
				ssd_dialog_hide_all(dec_yes);
		 }
    }
    else{
   	 continue_report_after_image_upload( ( (void *)uploadContext));
    }

    return 1; // return 0, as async process started, and we don't want SSD to be closed in the meantime
}

/**
 * Show add additi -alert Type, szDescription - alert description, iDirection - direction of the alert
 * @return void
 */
void report_alert(int iAlertType, int iSubType, const char * szDescription, int iDirection, const char *szGroup)
{

    RTAlertContext *AlertConext = calloc(1, sizeof(RTAlertContext));
    gCurrentImageId[0] = 0;
    AlertConext->iType = iAlertType;
    AlertConext->iSubType = iSubType;
    AlertConext->szDescription = szDescription;
    AlertConext->iDirection = iDirection;
    AlertConext->szGroup = szGroup;

    ssd_dialog_hide_current(dec_close);
#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE) || defined(ANDROID)
    ShowEditbox(roadmap_lang_get("Additional Details"), "",
            on_keyboard_closed, (void *)AlertConext, EEditBoxStandard | EEditBoxAlphaNumeric );
#else
    ssd_show_keyboard_dialog_ext( roadmap_lang_get("Additional Details"), NULL, NULL, NULL, on_keyboard_closed, AlertConext, SSD_KB_DLG_TYPING_LOCK_ENABLE );

#endif
}

static char const *DirectionQuickMenu[] = {

   "mydirection",
   RoadMapFactorySeparator,
   "oppositedirection",
   NULL,
};


/**
 * Starts the Direction menu
 * @param actions - list of actions
 * @return void
 */
void alerts_direction_menu(const char *name, const RoadMapAction  *actions){

	const RoadMapGpsPosition   *TripLocation;

   TripLocation = RTAlerts_alerts_location(TRUE);
   if (TripLocation == NULL) {
         return;
    }


#ifdef IPHONE
	roadmap_list_menu_simple (name, NULL, DirectionQuickMenu, NULL, NULL, NULL, NULL, actions,
							SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL|SSD_ALIGN_VCENTER);
#else
	ssd_list_menu_activate (name, NULL, DirectionQuickMenu, NULL, actions,
    			SSD_DIALOG_FLOAT|SSD_DIALOG_VERTICAL|SSD_ALIGN_VCENTER);

#endif //IPHONE
}

/**
 * Report an alert for police
 * @param None
 * @return void
 */
#ifndef IPHONE
void add_real_time_alert_for_police()
{
#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_POLICE);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_police_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_police_opposite_direction}
	};

   alerts_direction_menu("Police Direction Menu", RoadMapAlertActions);
#endif //TOUCH_SCREEN
}

/**
 * Report an alert for accident
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident()
{

#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_ACCIDENT);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_accident_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_accident_opposite_direction}
	};

 alerts_direction_menu("Accident Direction Menu", RoadMapAlertActions);
#endif //TOUCH_SCREEN
}

/**
 * Report an alert for traffic jam
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam()
{

#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_TRAFFIC_JAM);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_traffic_jam_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_traffic_jam_opposite_direction}
	};

 	alerts_direction_menu("Jam Direction Menu" ,RoadMapAlertActions);
#endif //TOUCH_SCREEN
}


/**
 * Report an alert for hazard
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard()
{

#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_HAZARD);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_hazard_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_hazard_opposite_direction}
	};

 	alerts_direction_menu("Hazard Direction Menu" ,RoadMapAlertActions);
#endif //TOUCH_SCREEN
}

/**
 * Report an alert for parking
 * @param None
 * @return void
 */
void add_real_time_alert_for_parking()
{

#ifdef TOUCH_SCREEN
   report_dialog(RT_ALERT_TYPE_PARKING);
#else
   RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_hazard_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_hazard_opposite_direction}
   };

   alerts_direction_menu("Parking Direction Menu" ,RoadMapAlertActions);
#endif //TOUCH_SCREEN
}

/**
 * Report an alert for other
 * @param None
 * @return void
 */
void add_real_time_alert_for_other()
{

#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_OTHER);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_other_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_other_opposite_direction}
	};

 	alerts_direction_menu("Other Direction Menu" ,RoadMapAlertActions);
#endif //TOUCH_SCREEN
}


/**
 * Report an alert for construction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction()
{

#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_CONSTRUCTION);
#else
	RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_construction_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_construction_opposite_direction}
	};

 	alerts_direction_menu("Construction Direction Menu" ,RoadMapAlertActions);
#endif //TOUCH_SCREEN
}
#endif //IPHONE

/**
 * Report an alert for police my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_police_my_direction()
{

    report_alert(RT_ALERT_TYPE_POLICE, -1, "" , RT_ALERT_MY_DIRECTION,"");
}


/**
 * Report an alert for Accident my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_my_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, -1, "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for Traffic Jam my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_my_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, -1,  "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for Hazard my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_my_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD, -1,  "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for Parking my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_parking_my_direction()
{

    report_alert(RT_ALERT_TYPE_PARKING, -1,  "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for Other my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_my_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, -1, "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for constrcution my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_my_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, -1, "",RT_ALERT_MY_DIRECTION,"");
}

/**
 * Report an alert for police on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_police_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_POLICE, -1,  "" , RT_ALERT_OPPSOITE_DIRECTION,"");
}

/**
 * Report an alert for accident on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, -1,  "" , RT_ALERT_OPPSOITE_DIRECTION,"");
}

/**
 * Report an alert for Traffic Jam on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, -1,  "", RT_ALERT_OPPSOITE_DIRECTION,"");
}


/**
 * Report an alert for Hazard on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD,  -1, "", RT_ALERT_OPPSOITE_DIRECTION,"");
}

/**
 * Report an alert for Parking on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_parking_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_PARKING, -1,  "", RT_ALERT_OPPSOITE_DIRECTION,"");
}
/**
 * Report an alert for Other on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, -1,  "", RT_ALERT_OPPSOITE_DIRECTION,"");
}

/**
 * Report an alert for Construction on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, -1,  "", RT_ALERT_OPPSOITE_DIRECTION,"");
}

#ifndef IPHONE
///////////////////////////////////////////////////////////////////////////////////////////
void add_real_time_chit_chat()
{
    const RoadMapGpsPosition   *TripLocation;
#ifndef TOUCH_SCREEN
    RTAlertContext *AlertConext;
#endif

    TripLocation = RTAlerts_alerts_location(TRUE);
    if (TripLocation == NULL){
          return;
     }

   if (Realtime_RandomUserMsg()) {
      return ;
   }

   if (Realtime_AnonymousMsg()) {
      return;
   }
#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_CHIT_CHAT);
#else
    AlertConext = calloc(1, sizeof(RTAlertContext));

    AlertConext->iType = RT_ALERT_TYPE_CHIT_CHAT;
    AlertConext->iSubType = -1;
    AlertConext->szDescription = "";
    AlertConext->iDirection = RT_ALERT_MY_DIRECTION;

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(ANDROID)
    ShowEditbox(roadmap_lang_get("Chat"), "", on_keyboard_closed,
            AlertConext, EEditBoxEmptyForbidden | EEditBoxAlphaNumeric );
#else
   ssd_show_keyboard_dialog_ext(  roadmap_lang_get( "Chat"),
                              NULL, NULL, NULL,
                              on_keyboard_closed,
                              AlertConext, SSD_KB_DLG_TYPING_LOCK_ENABLE );

#endif
#endif //TOUCH_SCREEN
}
#endif //IPHONE

/**
 * Keyboard callback for posting a comment
 * @param type - the button that was pressed, new_value - text, context - hold a pointer to the alert
 * @return int
 */
static BOOL post_comment_keyboard_callback(int         exit_code,
                                          const char* value,
                                          void*       context)
{
    BOOL success;
    RTAlert *pAlert = (RTAlert *)context;

    if( dec_ok != exit_code)
        return TRUE;

	if (value[0] == 0)
		return FALSE;

    success = Realtime_Post_Alert_Comment(pAlert->iID, value,
          roadmap_twitter_is_sending_enabled() && roadmap_twitter_logged_in(),
          roadmap_facebook_is_sending_enabled() && roadmap_facebook_logged_in());
    if (success){
        ssd_dialog_hide_all(dec_close);
#ifdef IPHONE
		roadmap_main_show_root(0);
#endif //IPHONE
        if (gPopUpType == POP_UP_ALERT){
			RTAlerts_Stop_Scrolling();
			gPopUpType = -1;
        }else{
        	RTAlerts_Comment_PopUp_Hide();
        	gPopUpType = -1;
        }
    }

    return success;
}

/**
 * Post a comment to the a specific alert ID
 * @param iAlertId - the alert ID to post the comment to
 * @return void
 */
void real_time_post_alert_comment_by_id(int iAlertid)
{
    RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertid);
    if (pAlert == NULL)
        return;

   if (Realtime_RandomUserMsg()) {
      return ;
   }

   if (Realtime_AnonymousMsg()) {
      return;
   }

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE) || defined(ANDROID)
    ShowEditbox(roadmap_lang_get("Comment"), "", post_comment_keyboard_callback,
            pAlert, EEditBoxEmptyForbidden | EEditBoxAlphaNumeric );
#else
    ssd_show_keyboard_dialog_ext(  roadmap_lang_get("Comment"),
                              NULL,
                              NULL,
                              RtAlerts_get_addional_keyboard_text(pAlert),
                              post_comment_keyboard_callback,
                              pAlert, 0);
#endif

}

/**
 * Post a comment to the currently active alert
 * @param None
 * @return void
 */
void real_time_post_alert_comment()
{

	gPopUpType = POP_UP_ALERT;
    real_time_post_alert_comment_by_id(gCurrentAlertId);
}

/**
 * Remove an alert
 * @param iAlertId - the id of the alert to delete
 * @return void
 */
BOOL real_time_remove_alert(int iAlertId)
{
    return Realtime_Remove_Alert(iAlertId);
}

/**
 * Checks the penalty of an alert if it is on the line.
 * @param line_id- the line ID to check, against_dir -the direction of the travel
 * @param the penalty of the alert, 0 if no alert is on that line.
 * @return void
 */
int RTAlerts_Penalty(int line_id, int against_dir)
{
    int i;
    int line_from_point;
    int line_to_point;
    int square = roadmap_square_active ();

    if (gAlertsTable.iCount == 0)
        return FALSE;
    for (i=0; i<gAlertsTable.iCount; i++)
    {
        if (RTAlerts_Is_Reroutable(gAlertsTable.alert[i]))
        {
            if (gAlertsTable.alert[i]->iLineId == line_id &&
					 gAlertsTable.alert[i]->iSquare == square)
            {
                roadmap_line_points(line_id, &line_from_point, &line_to_point);
                if (((line_from_point == gAlertsTable.alert[i]->iNode1)
                        && (!against_dir)) || ((line_to_point
                        == gAlertsTable.alert[i]->iNode1) && (against_dir)))
                {
                    if (gAlertsTable.alert[i]->iType == RT_ALERT_TYPE_ACCIDENT)
                        return 3600;
                    else
                        return 0;
                }
            }
        }
    }
    return 0;
}

int RTAlerts_Alert_near_position( RoadMapPosition position, int distance)
{
	 RoadMapPosition context_save_pos;
    zoom_t context_save_zoom;
    RoadMapPosition alert_position;
    int new_distance, saved_distance = 5000;
    int alert_id = -1;
    int i;

    if (RTAlerts_Is_Empty())
    	return -1;

    roadmap_math_get_context(&context_save_pos, &context_save_zoom);

    for (i=0; i < gAlertsTable.iCount; i++)
    {
        alert_position.longitude = gAlertsTable.alert[i]->iLongitude;
        alert_position.latitude = gAlertsTable.alert[i]->iLatitude;

        new_distance = roadmap_math_distance(&alert_position, &position);
        if (new_distance > distance)
        		continue;

        if (new_distance < saved_distance){
        		saved_distance = new_distance;
		  		alert_id = gAlertsTable.alert[i]->iID;
        }
    }


    roadmap_math_set_context(&context_save_pos, context_save_zoom);
    return alert_id;
}



/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_PopUp_Hide(void)
{
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "Comment Pop Up")))
    	ssd_dialog_hide_current(dec_close);

   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "PingWazerPopUPDlg")))
        ssd_dialog_hide_current(dec_close);
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_reply(void){
	gPopUpType = POP_UP_COMMENT;
	real_time_post_alert_comment_by_id(gCurrentAlertId);
}

#ifndef TOUCH_SCREEN
static int Comment_Reply_sk_cb(SsdWidget widget, const char *new_value, void *context){
	RTAlerts_Comment_reply();
	return 0;
}

static int Comment_Ignore_sk_cb(SsdWidget widget, const char *new_value, void *context){
	RTAlerts_Comment_PopUp_Hide();
	return 0;
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_set_softkeys(SsdWidget dlg){

	ssd_widget_set_left_softkey_callback(dlg, on_options);
	ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Options"));

	ssd_widget_set_right_softkey_callback(dlg, Comment_Ignore_sk_cb);
	ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Ignore"));

}

static int ThumbsUp_Ignore_sk_cb(SsdWidget widget, const char *new_value, void *context){
   ssd_dialog_hide("ThumbsUp Pop Up", dec_close);
   return 0;
}

static int ThumbsUp_Reply_sk_cb(SsdWidget widget, const char *new_value, void *context){
   real_time_post_alert_comment_by_id(gCurrentAlertId);
   return 0;
}


static void RTAlerts_ThumbsUp_set_softkeys(SsdWidget dlg){

   ssd_widget_set_left_softkey_callback(dlg, ThumbsUp_Reply_sk_cb);
   ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Reply"));

   ssd_widget_set_right_softkey_callback(dlg, ThumbsUp_Ignore_sk_cb);
   ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Ignore"));

}

#else
/////////////////////////////////////////////////////////////////////
static int Comment_Reply_button_callback (SsdWidget widget, const char *new_value) {
	RTAlerts_Comment_reply();
	return 0;
}
/////////////////////////////////////////////////////////////////////
static int Comment_Close_button_callback (SsdWidget widget, const char *new_value) {
   ssd_dialog_hide("Comment Pop Up", dec_close);
   return 0;
}
/////////////////////////////////////////////////////////////////////
static int ThumbsUp_Reply_button_callback (SsdWidget widget, const char *new_value) {
   real_time_post_alert_comment_by_id(gCurrentAlertId);
   return 0;
}
/////////////////////////////////////////////////////////////////////
static int ThumbsUp_Close_button_callback (SsdWidget widget, const char *new_value) {
   ThumbsUp *thumbsUp;
   ssd_dialog_hide("ThumbsUp Pop Up", dec_close);
   thumbsUp = getThumbsUp();
   ThumbsUpDel(thumbsUp);
   RealtimePopUp_set_Stoped_Popups();
   return 0;
}

#endif


static void RTAlerts_Comment_PopUp_Timer(void){
   g_comments_dlg_seconds --;
   if (g_comments_dlg_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "Comment Pop Up"))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close" );
         if (g_comments_dlg_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_comments_dlg_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }

   ssd_dialog_hide("Comment Pop Up", dec_close);

   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

static void  on_Comment_PopUp_close   (int exit_code, void* context){
   roadmap_main_remove_periodic(RTAlerts_Comment_PopUp_Timer);
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_PopUp(RTAlertComment *Comment, RTAlert *Alert)
{
	char CommentStr[700];
	SsdWidget dialog;
	SsdWidget text;
	SsdWidget bitmap;
	SsdWidget image_con;
	SsdWidget text_con;
   char *icon[3];
   int i, num_addOns;
#ifdef TOUCH_SCREEN
   SsdWidget button;
#endif
   int image_container_width = ADJ_SCALE(52);
   int width = roadmap_canvas_width();

#ifdef IPHONE_NATIVE
   width = ADJ_SCALE(300);
#else
   if (width > roadmap_canvas_height())
     width = roadmap_canvas_height();
#endif
	CommentStr[0] = 0;

//   if ( roadmap_screen_is_hd_screen() )
//     image_container_width = 78;


	 dialog =  ssd_popup_new("Comment Pop Up", roadmap_lang_get("Response"),on_Comment_PopUp_close,SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_ROUNDED_BLACK|SSD_POINTER_COMMENT,DIALOG_ANIMATION_FROM_TOP);

	 image_con =  ssd_container_new ("IMAGE_container", "", image_container_width, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
    ssd_widget_set_color(image_con, NULL, NULL);

    text_con =  ssd_container_new ("Text_Container", "", width - image_container_width - 40, SSD_MIN_SIZE, 0);
    ssd_widget_set_color(text_con, NULL, NULL);

    if (Alert->iType == RT_ALERT_TYPE_ACCIDENT)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Accident"));
    else if (Alert->iType == RT_ALERT_TYPE_POLICE)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Police trap"));
    else if (Alert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Traffic jam"));
    else if (Alert->iType == RT_ALERT_TYPE_HAZARD)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Hazard"));
    else if (Alert->iType == RT_ALERT_TYPE_PARKING)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Parking"));
    else if (Alert->iType == RT_ALERT_TYPE_OTHER)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Other"));
    gCurrentAlertId = Alert->iID;
    gCurrentCommentId = Comment->iID;
    snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s", Alert->sLocationStr);

	bitmap = ssd_bitmap_new("alert_icon",RTAlerts_Get_IconByType(Alert, Alert->iType, FALSE),0);
	num_addOns = RTAlerts_Get_Number_Of_AddOns(Alert->iID);
	for (i = 0 ; i < num_addOns; i++){
	       const char *AddOn = RTAlerts_Get_AddOn(Alert->iID, i);
	       if (AddOn)
	          ssd_bitmap_add_overlay(bitmap,AddOn);
	}
	ssd_widget_add(image_con, bitmap);
	ssd_widget_add(dialog, image_con);

	text = ssd_text_new ("Comment Str", CommentStr, 14,SSD_END_ROW);
	ssd_widget_set_color(text,"#f6a201", NULL);
	ssd_widget_add(text_con, text);
	CommentStr[0] = 0;
   snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s %s", roadmap_lang_get("by"), roadmap_lang_get(Comment->sPostedBy));

   text = ssd_text_new ("Comment Str", CommentStr, 14,SSD_TEXT_NORMAL_FONT|SSD_END_ROW );
   ssd_widget_add(text_con, text);
   ssd_widget_set_color(text,"#ffffff", NULL);
   RTAlerts_add_comment_stars(text_con, Comment);

   CommentStr[0] = 0;
   snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s%s", Comment->sDescription, NEW_LINE);
   ssd_widget_add(text_con, space(2));
   text = ssd_text_new ("Comment Str", CommentStr, 16,SSD_START_NEW_ROW);
   ssd_widget_add(text_con, text);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(dialog, text_con);

#ifdef TOUCH_SCREEN
	ssd_widget_add(dialog, space(2));
   ssd_widget_add (dialog,
                  ssd_button_label ("Close", roadmap_lang_get ("Close"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, Comment_Close_button_callback));
	ssd_widget_add (dialog,
   					ssd_button_label ("Reply", roadmap_lang_get ("Reply"),
                   	SSD_WS_TABSTOP|SSD_ALIGN_CENTER, Comment_Reply_button_callback));
   icon[0] = "button_report_abuse";
   icon[1] = "button_report_abuse_down";
   icon[2] = NULL;
   button = ssd_button_new( "Reprt_Abuse_Button", "ReportAbuse", (const char**) &icon[0], 2, 0, on_report_abuse );
   ssd_widget_add(dialog, button);
   ssd_widget_add(dialog, space(2));

   g_comments_dlg_seconds = COMMENT_POPUP_TIMER;
   roadmap_main_set_periodic (1000, RTAlerts_Comment_PopUp_Timer);

#else
   RTAlerts_Comment_set_softkeys(dialog->parent);
#endif

	ssd_dialog_activate ("Comment Pop Up", NULL);
	roadmap_screen_redraw();


}


int RTAlerts_CurrentAlert_Has_Comments(void)
{
    if (RTAlerts_Get_Number_of_Comments(gCurrentAlertId) )
		if (ssd_dialog_is_currently_active())
			return 2;
		else
    		return 1;
    else
    	return 0;
}

void RTAlerts_CurrentComments(){
	RealtimeAlertCommentsList(gCurrentAlertId);
}

int RTAlerts_is_reply_popup_on(void){
    int sign_active = (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "Comment Pop Up")));

   if (sign_active)
        return 1;
   else
	   return 0;
}

int RTAlerts_Is_Cancelable(int alertId){
   RTAlert *pAlert;

   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
       return FALSE;

   if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) || (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM) || (pAlert->iType == RT_ALERT_TYPE_ACCIDENT))
    return FALSE;

   return TRUE;
}

BOOL RTAlerts_Can_Send_Thumbs_up(int alertId){
   RTAlert *pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
       return FALSE;
   else
       return (!pAlert->bAlertByMe && !pAlert->bThumbsUpByMe);

}

int Rtalerts_Thumbs_Up(int alertId){

   RTAlert *pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
      return 0;

   pAlert->bThumbsUpByMe = TRUE;

   Realtime_ThumbsUp(alertId);
   return 1;
}

void RTAlerts_Update(int iID, int iNumThumbsUp, BOOL bIsOnRoute, BOOL bIsArchive, int iNumViewed){

   RTAlert *pAlert = RTAlerts_Get_By_ID(iID);
   if (pAlert == NULL){
      roadmap_log( ROADMAP_ERROR, "RTAlerts_Update -Alert ID %d not found", iID);
      return;
   }
   pAlert->iNumThumbsUp = iNumThumbsUp;
   pAlert->bAlertIsOnRoute = bIsOnRoute;
   pAlert->bArchive = bIsArchive;
   pAlert->iNumViewed = iNumViewed;
}

static void close_ThumbsUp_Dlg(void){
   ssd_dialog_hide("ThumbsUp Pop Up", dec_close);
}

static void ThumbsUpTimer(void){
   g_thumbs_up_seconds --;
   if (g_thumbs_up_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "ThumbsUp Pop Up"))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close" );
         if (g_thumbs_up_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_thumbs_up_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }

   close_ThumbsUp_Dlg();
   ThumbsUpDel(getThumbsUp());

   gThumbsUpScrolling = FALSE;
   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

static void OnThumbsUpPopUpClose(int exit_code, void* context){
   roadmap_main_remove_periodic(ThumbsUpTimer);
   gThumbsUpScrolling = FALSE;
}

static void RTAlerts_ThumbsUp_PopUp(ThumbsUp *thumbsUp)
{
   RTAlert *pAlert;
   char ThumbsUpStr[700];
   SsdWidget dialog;
   SsdWidget text;
   SsdWidget bitmap;
   SsdWidget title_con;
   SsdWidget text_con;
   SsdWidget fb_image_cont;
   SsdWidget facebook_image;
   RoadMapSoundList list = NULL;


   int width = roadmap_canvas_width();
   int fb_width = ADJ_SCALE(52);
   int fb_height = ADJ_SCALE(52);

  if (!thumbsUp->bShowFacebookPicture){
     fb_width = 0;
  }
  else{
     fb_image_cont = ssd_container_new ("FB_IMAGE_container", "", fb_width, fb_height, SSD_ALIGN_RIGHT);
     ssd_widget_set_color(fb_image_cont, "#ffffff", "#ffffff");
     facebook_image = ssd_bitmap_new("facebook_image", "facebook_default_image", SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
     ssd_widget_add(fb_image_cont, facebook_image);
     roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_USER, atoi(thumbsUp->sUserID), -1, SOCIAL_IMAGE_SIZE_SQUARE,  facebook_image );
  }

#ifdef IPHONE_NATIVE
   width = ADJ_SCALE(300);
#else
   if (width > roadmap_canvas_height())
     width = roadmap_canvas_height();
#endif
   ThumbsUpStr[0] = 0;

   pAlert = RTAlerts_Get_By_ID(thumbsUp->iID);
   if (pAlert == NULL){
       roadmap_log( ROADMAP_ERROR, "RTAlerts_ThumbsUp_PopUp -Alert ID %d not found", thumbsUp->iID);
       return;
   }

   if (!list) {
       list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
       roadmap_sound_list_add (list, "ping");
       roadmap_res_get (RES_SOUND, 0, "ping");
    }
    roadmap_sound_play_list (list);

    dialog =  ssd_popup_new("ThumbsUp Pop Up", NULL,OnThumbsUpPopUpClose,SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_ROUNDED_BLACK|SSD_POINTER_COMMENT,0);

    title_con =  ssd_container_new ("ThumbsUp.Title.Container", "", width - ADJ_SCALE(42) - fb_width, SSD_MIN_SIZE, 0);
    ssd_widget_set_color(title_con, NULL, NULL);
    ssd_dialog_add_vspace(title_con, ADJ_SCALE(5), 0);
    bitmap = ssd_bitmap_new("ThumbsUp","thumbs_up",SSD_ALIGN_VCENTER);
    ssd_widget_add(title_con, bitmap);
    ssd_dialog_add_hspace(title_con, ADJ_SCALE(5), 0);
    text = ssd_text_new ("ThumbsUp.Title.Container.Str", roadmap_lang_get("Thanks from:"), 18,SSD_END_ROW );
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(title_con, text);

    ssd_widget_add(dialog, title_con);

    ThumbsUpStr[0] = 0;
    text_con =  ssd_container_new ("Text_Container", "", width - ADJ_SCALE(42) - fb_width, SSD_MIN_SIZE, 0);
    ssd_widget_set_color(text_con, NULL, NULL);
    text = ssd_text_new ("Comment Str", thumbsUp->sNickName, 18,SSD_END_ROW );
    ssd_widget_set_color(text,"#f6a201", NULL);
    ssd_widget_add(text_con, text);

    if (thumbsUp->sFacebookName && thumbsUp->sFacebookName[0] != 0){
       snprintf(ThumbsUpStr, sizeof(ThumbsUpStr) , "(%s)", thumbsUp->sFacebookName);
       text = ssd_text_new ("FacebookUsername Str", ThumbsUpStr, 18,SSD_END_ROW );
       ssd_widget_set_color(text,"#f6a201", NULL);
       ssd_widget_add(text_con, text);
    }

    ssd_dialog_add_vspace(text_con, ADJ_SCALE(5),0);
    ThumbsUpStr[0] = 0;
    strcat(ThumbsUpStr, roadmap_lang_get("Re:"));
    if (pAlert->iType == RT_ALERT_TYPE_ACCIDENT)
        snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Accident"));
    else if (pAlert->iType == RT_ALERT_TYPE_POLICE)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Police trap"));
    else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Traffic jam"));
    else if (pAlert->iType == RT_ALERT_TYPE_HAZARD)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Hazard"));
    else if (pAlert->iType == RT_ALERT_TYPE_PARKING)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Parking"));
    else if (pAlert->iType == RT_ALERT_TYPE_OTHER)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Other"));
    else if (pAlert->iType == RT_ALERT_TYPE_CHIT_CHAT)
       snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s,", roadmap_lang_get("Chit chat"));
    gCurrentAlertId = pAlert->iID;
    gCurrentCommentId = -1;
    snprintf(ThumbsUpStr + strlen(ThumbsUpStr), sizeof(ThumbsUpStr)
                - strlen(ThumbsUpStr), "%s", pAlert->sLocationStr);


   text = ssd_text_new ("Comment Str", ThumbsUpStr, 14,SSD_END_ROW);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_dialog_add_hspace(text_con, ADJ_SCALE(5), 0);
   ssd_widget_add(text_con, text);


   ssd_widget_add(dialog, text_con);
   if (thumbsUp->bShowFacebookPicture)
      ssd_widget_add(dialog, fb_image_cont);

#ifdef TOUCH_SCREEN
   ssd_widget_add(dialog, space(2));
   ssd_widget_add (dialog,
                  ssd_button_label ("Close", roadmap_lang_get ("Close"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, ThumbsUp_Close_button_callback));
   ssd_widget_add (dialog,
                  ssd_button_label ("Reply", roadmap_lang_get ("Reply"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, ThumbsUp_Reply_button_callback));

#else
   RTAlerts_ThumbsUp_set_softkeys(dialog->parent);
#endif

   ssd_dialog_activate ("ThumbsUp Pop Up", NULL);
   g_thumbs_up_seconds = THUMBS_UP_POPUP_TIMER;
   roadmap_main_set_periodic (1000, ThumbsUpTimer);
   roadmap_screen_redraw();


}



void RTAlerts_ThumbsUpRecordInit(ThumbsUp *thumbsUp){
   thumbsUp->iID = -1;
   memset(thumbsUp->sUserID, 0, RT_ALERT_USERNM_MAXSIZE);
   memset(thumbsUp->sNickName, 0, RT_ALERT_USERNM_MAXSIZE);
   memset(thumbsUp->sFacebookName, 0, RT_ALERT_FACEBOOK_USERNM_MAXSIZE);
   thumbsUp->bShowFacebookPicture = FALSE;
   thumbsUp->bDisplayed = FALSE;
   thumbsUp->iIndex = -1;
}

static void addThumbsUp(ThumbsUp *thumbsUp){
   int i;

   for (i = 0; i < RT_THUMBS_UP_QUEUE_MAXSIZE; i++){
      if (gThumbsUpTable.thumbsUp[i] == NULL)
         break;
   }

   if (i != RT_THUMBS_UP_QUEUE_MAXSIZE-1){
      gThumbsUpTable.thumbsUp[i] = calloc(1, sizeof(ThumbsUp));
      *gThumbsUpTable.thumbsUp[i] = *thumbsUp;
      gThumbsUpTable.thumbsUp[i]->iIndex = i;
      gThumbsUpTable.iCount++;
   }
}

static void ThumbsUpDel(ThumbsUp *thumbsUp){

   int index;

   if (thumbsUp == NULL || thumbsUp->iIndex == -1)
      return;

   index = thumbsUp->iIndex;
   free(gThumbsUpTable.thumbsUp[index]);
   gThumbsUpTable.thumbsUp[index] = NULL;
}

static ThumbsUp *getThumbsUp(void){
   int i;
   for (i = 0; i < RT_THUMBS_UP_QUEUE_MAXSIZE; i++){
      if (gThumbsUpTable.thumbsUp[i] != NULL)
         return gThumbsUpTable.thumbsUp[i];
   }

   return NULL;

}

static BOOL showThumbsUp(void){

   ThumbsUp *thumbsUp= getThumbsUp();;
   if (thumbsUp){

      RTAlerts_ThumbsUp_PopUp(thumbsUp);
      gThumbsUpScrolling = TRUE;
      return TRUE;
   }
   gThumbsUpScrolling = FALSE;
   return FALSE;
}

BOOL RTAlerts_ThumbsUpReceived(ThumbsUp *thumbsUp){

   addThumbsUp(thumbsUp);
   return TRUE;
}

int Rtalerts_Delete(int alertId){
   RTAlerts_Remove(alertId);
	return real_time_remove_alert(alertId);
}

int RTAlerts_Check_Same_Street(int record){

   RTAlert *pAlert = RTAlerts_Get(record);
    if (pAlert != NULL)
        if ((pAlert->iType == RT_ALERT_TYPE_HAZARD) && (pAlert->iSubType >= HAZARD_TYPE_WEATHER_FOG) && (pAlert->iSubType <= HAZARD_TYPE_WEATHER_FREEZING_RAIN))
              return FALSE;
         else
              return TRUE;
    else
       return TRUE;
}

static void on_AlertAheadGLowEnd(void){

   roadmap_main_remove_periodic(on_AlertAheadGLowEnd);
   RTAlerts_stop_glow();
   focus_on_me();

}

static int on_AlertAhead_Show(RTAlert *pAlert){
   RoadMapPosition position, gps_pos;
   RoadMapGpsPosition CurrentPosition;
   int distance;
   
   //ssd_dialog_hide_current(dec_close);

   if (!pAlert)
      return 0;
   
   position.longitude = pAlert->iLongitude;
   position.latitude = pAlert->iLatitude;
   //roadmap_screen_hold();
   roadmap_trip_set_point("Hold", &position);
   roadmap_screen_hold();
   //roadmap_screen_update_center_animated(&position, 1200, 0);
   {
      if (roadmap_navigate_get_current(&CurrentPosition, NULL, NULL) != -1)
      {
         
         gps_pos.latitude = CurrentPosition.latitude;
         gps_pos.longitude = CurrentPosition.longitude;
         distance = roadmap_math_distance(&gps_pos, &position);
         
         roadmap_screen_set_scale(distance, roadmap_screen_height());
      }
   }
   RTAlerts_start_glow(pAlert,120);
   //roadmap_main_set_periodic(8000, on_AlertAheadGLowEnd);
   roadmap_main_set_periodic(5000, on_AlertAheadGLowEnd);
   
   return 1;
}

static int on_AlertAhead_Dlg_Show(SsdWidget widget, const char *new_value){
   RTAlert *pAlert;

   pAlert = (RTAlert *)widget->context;
   if (!pAlert)
      return 0;

   on_AlertAhead_Show(pAlert);

   return 1;
}


static void AlertAfeadTimner(void){
   g_alert_ahead_seconds --;
   if (g_alert_ahead_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertAheadDlg"))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close_button" );
         if (g_alert_ahead_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_alert_ahead_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }

   ssd_dialog_hide("AlertAheadDlg", dec_cancel);
   roadmap_main_remove_periodic(AlertAfeadTimner);
   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

void AlertAheadDlgDisplay(RTAlert *pAlert){
   SsdWidget dialog;
   SsdWidget image_con;
   SsdWidget text_con;
   SsdWidget text;
   SsdWidget button;
   SsdWidget spacer;
   SsdWidget bitmap;
   SsdWidget button_txt;
   SsdWidget title;

   int image_container_width = ADJ_SCALE(52);
   char *icon[3];
   char DistanceStr[100];
   char TextStr[100];
   int width = roadmap_canvas_width();

   gCurrentAlertId =pAlert->iID;
#ifdef IPHONE
   width = 320 * roadmap_screen_get_screen_scale() / 100;
#else
   if (width > roadmap_canvas_height())
      width = roadmap_canvas_height();
#endif // IPHONE

   dialog =  ssd_popup_new("AlertAheadDlg", RTAlerts_Get_String(pAlert->iID), NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_PERSISTENT|SSD_ROUNDED_BLACK, DIALOG_ANIMATION_FROM_TOP);
   title = ssd_widget_get(dialog, "popuup_text");
   ssd_widget_set_color(title,"#f6a201", "#f6a201");


   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( dialog, spacer );

   image_con =
      ssd_container_new ("AlertAheadDlg.IMAGE_container", "", image_container_width, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
    ssd_widget_set_color(image_con, NULL, NULL);
    bitmap = ssd_bitmap_new("AlertAheadDlg.alert_icon", RTAlerts_Get_Icon(pAlert->iID),SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP);
    ssd_widget_add(image_con, bitmap);
    ssd_widget_add(dialog, image_con);

    text_con =
      ssd_container_new ("AlertAheadDlg.IMAGE_container", "",width-image_container_width-50, SSD_MIN_SIZE, SSD_END_ROW);
    ssd_widget_set_color( text_con, NULL, NULL);

    DistanceStr[0] = 0;
    RTAlerts_get_report_distance_str( pAlert, DistanceStr, sizeof( DistanceStr ) );

    sprintf(TextStr,"%s %s", roadmap_lang_get("In"), DistanceStr);

    text = ssd_text_new("DistanceTxt",TextStr, 16, SSD_END_ROW);
    ssd_widget_set_color(text, "#ffffff", "#ffffff");
    ssd_widget_add(text_con, text);
    ssd_dialog_add_vspace(text_con, 5, 0);
    text = ssd_text_new("DistanceTxt",RTAlerts_Get_Additional_String(pAlert->iID), 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
    ssd_text_set_color(text, "#ffffff");
    ssd_widget_add(text_con, text);

    ssd_widget_add(dialog, text_con);


#ifdef TOUCH_SCREEN
    ssd_dialog_add_vspace(dialog, 5, 0);
    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close_AlertAhead);
    ssd_widget_add(dialog, button);

    // Thumbs Up
    icon[0] = "thumbs_up_button";
    icon[1] = "thumbs_up_button_down";
    icon[2] = NULL;
    button = ssd_button_new( "ThumbsUp_button", "ThumbsUp", (const char**) &icon[0], 2, SSD_ALIGN_CENTER, on_thumbs_up );
    ssd_widget_add(dialog, button);

    // Thumbs up button
    if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) || (pAlert->bAlertByMe) || (pAlert->bThumbsUpByMe))
    {
       ssd_widget_hide(button);
    }

    button = ssd_button_label("Show_button", roadmap_lang_get("Show"), SSD_ALIGN_CENTER, on_AlertAhead_Dlg_Show);
    button->context = (void *)pAlert;
    ssd_widget_add(dialog, button);


#else
    set_left_softkey(dialog->parent, pAlert);
    set_right_softkey(dialog->parent);
#endif // TOUCH_SCREEN

   ssd_dialog_activate("AlertAheadDlg", pAlert);
}

BOOL  AlertAheadHandler(int alertId){
   RTAlert *pAlert;
   RoadMapSoundList sound_list;

   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
         return FALSE;

   if (ssd_dialog_is_currently_active())
      return TRUE;

   pAlert->bAlertHandled = TRUE;
   g_traffic_info_time_stamp = time(NULL);

   if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO){

      if (!strcmp(pAlert->sLocationStr, Get_LastJamStreetName()))
         return TRUE;

      Set_LastJamStreetName(pAlert->sLocationStr);
   }

   sound_list = RTAlerts_Get_Sound(alertId);
   if (sound_list)
      roadmap_sound_play_list (sound_list);

   //AlertAheadDlgDisplay(pAlert);
//   roadmap_main_set_periodic(1000, AlertAfeadTimner);
   if (!strcmp(roadmap_trip_get_focus_name(), "GPS"))
      on_AlertAhead_Show(pAlert);
   g_alert_ahead_seconds = 12;
   return TRUE;
}


int RTAlerts_HandleAlert(int alertId){
   RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return FALSE;

    if (pAlert->bAlertHandled)
       return TRUE;


    if ((pAlert->iType == RT_ALERT_TYPE_ACCIDENT) || (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)){
      if (!pAlert->bAlertHandled)
         return AlertAheadHandler(alertId);
      else
         return TRUE;
   }

   return FALSE;
}


int RTAlertsOnAlerterStart(int alertId){
   RTAlert *pAlert;

   pAlert = RTAlerts_Get_By_ID(alertId);
   if (pAlert == NULL)
        return FALSE;
   RTAlerts_start_glow(pAlert, 120);

   return TRUE;
}

int RTAlertsOnAlerterStop(int alertId){

   RTAlerts_stop_glow();

   return TRUE;
}


BOOL RTAlerts_is_square_dependent(void){
	return FALSE;
}

int RTAlerts_get_priority(void){
	return ALERTER_PRIORITY_MEDIUM;
}

BOOL RTAlerts_distance_check(RoadMapPosition gps_pos){
	static RoadMapPosition last_checked_position = {0,0};
	int distance;
	if(!last_checked_position.latitude){ // first time
		last_checked_position = gps_pos;
		return TRUE;
	}

	distance = roadmap_math_distance(&gps_pos, &last_checked_position);
	if (distance < MINIMUM_DISTANCE_TO_CHECK)
		return FALSE;
	else{
		last_checked_position = gps_pos;
		return TRUE;
	}
}

roadmap_alerter_location_info *  RTAlerts_get_location_info(int record){
	return &gAlertsTable.alert[record]->location_info;
}

/////////////////////////////////////////////////////////////////////
static void get_location_str(char *sLocationStr, int iDirection, const RoadMapGpsPosition *Location){
   const char *street;
   const char *city;
   int iLineId;
   int   iSquare;
   RoadMapPosition AlertPosition;

   if (Location == NULL)
      Location = roadmap_trip_get_gps_position("AlertSelection");

   if (Location == NULL){
      snprintf(sLocationStr, RT_ALERT_LOCATION_MAX_SIZE," ");
      return;
   }

   AlertPosition.latitude = Location->latitude;
   AlertPosition.longitude = Location->longitude;

   RTAlerts_Get_City_Street(AlertPosition, &city, &street, &iSquare, &iLineId, iDirection);
   if (!((city == NULL) && (street == NULL)))
    {
           if ((city != NULL) && (strlen(city) == 0))
            snprintf( sLocationStr,RT_ALERT_LOCATION_MAX_SIZE,"%s", street);
          else if ((street != NULL) && (strlen(street) == 0))
            snprintf(sLocationStr,RT_ALERT_LOCATION_MAX_SIZE,"%s", city);
         else
            snprintf(sLocationStr,RT_ALERT_LOCATION_MAX_SIZE,"%s, %s", street, city);
    }
}
const RoadMapGpsPosition *RTAlerts_alerts_location(BOOL showMsgBox){
   const RoadMapGpsPosition   *TripLocation;
   TripLocation = roadmap_trip_get_gps_position("AlertSelection");
   if ((TripLocation == NULL) /*|| (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)*/) {
      PluginLine line;
      int direction;
      RoadMapGpsPosition          *CurrentGpsPoint;
      const RoadMapPosition     *GpsPosition;

      int menu_file;
      BOOL has_position = FALSE;
      BOOL has_reception = roadmap_gps_have_reception();

      CurrentGpsPoint = malloc(sizeof(*CurrentGpsPoint));
      roadmap_check_allocated(CurrentGpsPoint);
      if (roadmap_navigate_get_current
          (CurrentGpsPoint, &line, &direction) == -1) {
         GpsPosition = roadmap_trip_get_position ("GPS");
         if ( (GpsPosition != NULL) && (has_reception)){
                       CurrentGpsPoint->latitude = GpsPosition->latitude;
                       CurrentGpsPoint->longitude = GpsPosition->longitude;
                       CurrentGpsPoint->speed = 0;
                       CurrentGpsPoint->steering = 0;
                       has_position = TRUE;
          } else{
             if (roadmap_verbosity () <= ROADMAP_MESSAGE_DEBUG) { // In debug mode allow to report from location
                GpsPosition = roadmap_trip_get_position( "Location" );
                if ((GpsPosition != NULL) && !IS_DEFAULT_LOCATION( GpsPosition )){
                   CurrentGpsPoint->latitude = GpsPosition->latitude;
                   CurrentGpsPoint->longitude = GpsPosition->longitude;
                   CurrentGpsPoint->speed = 0;
                   CurrentGpsPoint->steering = 0;
                   has_position = TRUE;
                }
             }
             else{
                       free(CurrentGpsPoint);
                       if (showMsgBox)
                       roadmap_messagebox_timeout("No GPS reception","Sorry, there's no GPS reception in this location. Make sure you are outdoors",5);
                       return NULL;
             }
           }
      }
      else
        has_position = TRUE;

      if (has_position)
      {
        int from_node, to_node;

        if (line.plugin_id != -1){
           roadmap_square_set_current (line.square);
           if (ROUTE_DIRECTION_AGAINST_LINE != direction)
              roadmap_line_point_ids (line.line_id, &from_node, &to_node);
           else
              roadmap_line_point_ids (line.line_id, &to_node, &from_node);
        }
        else{
           from_node = -1;
           to_node = -1;
        }

        roadmap_trip_set_gps_and_nodes_position( "AlertSelection", "Selection", "new_alert_marker",
                                         CurrentGpsPoint, from_node, to_node );
        roadmap_trip_set_animation("AlertSelection",OBJECT_ANIMATION_POP_IN|OBJECT_ANIMATION_POP_OUT|OBJECT_ANIMATION_WHEN_VISIBLE );
        roadmap_trip_set_focus("AlertSelection");
        TripLocation = CurrentGpsPoint;
        roadmap_screen_redraw();
        return TripLocation;
      }
      else{
         return NULL;
      }
    }
   else{
      return TripLocation;
   }
}


#ifdef TOUCH_SCREEN

////////////////////////////////////////////////////////////////////
static void on_recorder_closed( int exit_code, void *context ){
   SsdWidget button;
   SsdWidget dialog;
   char *icon[3];
   const char *path = roadmap_path_first ("config");
   const char *file_name = VOICE_FILENAME;

   dialog = (SsdWidget)context;
   if (dialog)
      button = ssd_widget_get(dialog, "Record_button");
      if (button){
        RoadMapImage image;
        if (roadmap_file_exists(path, file_name)) {
           icon[0] = "button_record_taken";
           icon[1] = "button_record_taken";
           icon[2] = NULL;
        } else {
           icon[0] = "button_record";
           icon[1] = "button_record";
           icon[2] = NULL;
        }

        ssd_button_change_icon(button, icon, 2 );

      }
}

////////////////////////////////////////////////////////////////////
static int on_record_button (SsdWidget widget, const char *new_value) {
   const char *path = roadmap_path_first ("config");
   const char *file_name = VOICE_FILENAME;
   roadmap_file_remove(path, file_name);

   if (path != NULL)
      roadmap_recorder("", 10, on_recorder_closed, 1, path, file_name, ssd_dialog_get_currently_active());

   return 1;
}

////////////////////////////////////////////////////////////////////
static BOOL is_voice_recorded( void )
{
   BOOL res = FALSE;
   const char *path = roadmap_path_first ("config");
   const char *file_name = VOICE_FILENAME;
   res = roadmap_file_exists( path, file_name );
   return res;
}

////////////////////////////////////////////////////////////////////
static int report_menu_buttons_callback (SsdWidget widget, const char *new_value) {
   upload_Image_context * uploadContext;
   RTAlertContext *AlertContext = (RTAlertContext *)widget->context;
   SsdWidget container = widget->parent->parent;

   if (!strcmp(widget->name, "right_title_button")){
      gMinimizeState = AlertContext->iType;
      ssd_dialog_hide_all(dec_yes);
   }

   else if (!strcmp(widget->name, "Send")){
      const char *description = ssd_widget_get_value(ssd_widget_get(container, roadmap_lang_get("Additional Details")),roadmap_lang_get("Additional Details"));
      const char *group = roadmap_groups_get_selected_group_name();

      if ((AlertContext->iType == RT_ALERT_TYPE_CHIT_CHAT) && (*description == 0) && (gCurrentImagePath == NULL) &&  !is_voice_recorded() )
      {
         return 1;
      }

      RTAlerts_ShowProgressDlg();
      gMinimizeState =AlertContext->iType;

      if (group == NULL){
         group = "";
      }

      uploadContext = (upload_Image_context *)malloc(sizeof(upload_Image_context));
      uploadContext->desc = strdup(description);
      uploadContext->iDirection = AlertContext->iDirection;
      uploadContext->iType =   AlertContext->iType;
      uploadContext->iSubType = AlertContext->iSubType;
      uploadContext->group = strdup(group);

      if ( gCurrentImagePath ){
         if(roadmap_camera_image_upload( NULL, gCurrentImagePath, gCurrentImageId,
               continue_report_after_image_upload, (void *) uploadContext ) == FALSE){
            roadmap_log(ROADMAP_ERROR,"Error in uploading image alert");
            continue_report_after_image_upload( ( (void *)uploadContext)); // error in image upload, continue
         }else{
            ssd_dialog_hide_all(dec_yes);
         }
      }
      else{
         continue_report_after_image_upload( ( (void *)uploadContext));
      }
   }

   else{
         gMinimizeState = -1;
   		roadmap_trip_restore_focus();
   		ssd_dialog_hide_all(dec_close);
   }

   return 1;
}


/////////////////////////////////////////////////////////////////////
static void on_groups_changed(void){

	char *icon[2];
	const char *name;

	icon[0] = (char *)roadmap_groups_get_selected_group_icon();
	icon[1] = NULL;
	ssd_dialog_change_bitmap(roadmap_lang_get ("GroupIcon"), roadmap_groups_get_selected_group_icon());
	name = roadmap_groups_get_selected_group_name();
	if (name && *name)
	   ssd_dialog_change_text("Group Text", roadmap_groups_get_selected_group_name());
	else
      ssd_dialog_change_text("Group Text", roadmap_lang_get("No group"));

}

/////////////////////////////////////////////////////////////////////
static int on_group_select( SsdWidget this, const char *new_value){
   roadmap_groups_dialog(on_groups_changed);
	return 0;
}

static void camera_image_alert_callback( CameraImageAlertContext* context, int res )
{
    RoadMapImage image_thumbnail = *context->image_thumbnail;
    const char *bitmap_names[2] = { NULL, NULL };
    SsdWidget widget = ( SsdWidget ) context->callback_data;
    SsdWidget bitmap = ssd_widget_get( widget, "AddImage" );
    // Replace the thumbnail in the button if the image has been taken successfully
    if ( res )
    {
       image_thumbnail = *context->image_thumbnail;
       roadmap_log( ROADMAP_INFO, "Camera image was successfully taken and stored at %s", gCurrentImagePath );
       /*
        * Image thumbnail is not supported meanwhile
        */
       if ( 0 && image_thumbnail )
       {

          RoadMapImage bitmap_images[1];
          bitmap_names[0] = "";
          bitmap_images[0] = image_thumbnail;
          ssd_button_change_images( bitmap, bitmap_images, bitmap_names, 1 );
       }
       else
       {
          SsdWidget text = ssd_widget_get( widget, "Add image Text");
          ssd_text_set_text(text, roadmap_lang_get("Image added"));
          *bitmap_names = "picture_added";
          ssd_button_change_icon( bitmap, bitmap_names, 1 );
       }
    }
    else
    {
       *bitmap_names = "add_image_box";
       ssd_button_change_icon( bitmap, bitmap_names, 1 );
       roadmap_log( ROADMAP_WARNING, "The camera image was not captured" );
    }

    free( context );

    roadmap_log( ROADMAP_DEBUG, "camera_image_alert_callback callback was called. Status: %d", res );

    roadmap_screen_refresh();
}

/////////////////////////////////////////////////////////////////////
static int on_add_image( SsdWidget this, const char *new_value )
{
#ifndef IPHONE
	static RoadMapImage image_thumbnail = NULL;
	const char *bitmap_names[2] = { NULL, NULL };
	BOOL image_taken;
	SsdWidget text;
	SsdWidget bitmap;

	bitmap = ssd_widget_get(this, "AddImage" );
	// Release the previous thumbnail
	if ( image_thumbnail )
	{
		roadmap_canvas_free_image( image_thumbnail );
		image_thumbnail = NULL;
	}

#ifdef ALERT_IMAGE_CAPTURE_ASYNC
	{
	   CameraImageAlertContext* ctx = malloc( sizeof( CameraImageAlertContext ) );
	   ctx->image_path = &gCurrentImagePath;
	   ctx->callback_data = (void*) this;
	   ctx->image_thumbnail = &image_thumbnail;
	   roadmap_camera_image_alert_async( camera_image_alert_callback, ctx );
	}
#else
	image_taken = roadmap_camera_image_alert( &gCurrentImagePath, &image_thumbnail );
	// Capture and upload the image
	// Replace the thumbnail in the button if the image has been taken successfully
	if ( image_taken )
	{
		roadmap_log( ROADMAP_INFO, "Camera image was successfully taken and stored at %s", gCurrentImagePath );
		if ( image_thumbnail )
		{
			RoadMapImage bitmap_images[1];
			bitmap_names[0] = "";
			bitmap_images[0] = image_thumbnail;
			ssd_button_change_images( bitmap, bitmap_images, bitmap_names, 1 );
		}
		else
		{
		   text = ssd_widget_get(this, "Add image Text");
			ssd_text_set_text(text, roadmap_lang_get("Picture added"));
	      *bitmap_names = "picture_added";
	      ssd_button_change_icon( bitmap, bitmap_names, 1 );
		}
	}
	else
	{
		*bitmap_names = "add_image_box";
		ssd_button_change_icon( bitmap, bitmap_names, 1 );
		roadmap_log( ROADMAP_WARNING, "The camera image was not captured" );
	}
	roadmap_screen_refresh();
#endif // ALERT_IMAGE_CAPTURE_ASYNC
#endif //IPHONE

	return 0;
}



static BOOL allow_poice_subtypes (void) {
   if (0 == strcmp (roadmap_config_get (&AllowPoliceSubtypeCfg), "yes")){
      return TRUE;
   }

   return FALSE;
}



int RTAlerts_get_number_of_sub_types(int iAlertType){
   switch (iAlertType){
       case RT_ALERT_TYPE_POLICE:
             if (allow_poice_subtypes())
                return 2;
             else
                return 0;
             break;

         case RT_ALERT_TYPE_ACCIDENT:
             return 2;
             break;

         case RT_ALERT_TYPE_CHIT_CHAT:
             return 0;
             break;

         case RT_ALERT_TYPE_TRAFFIC_JAM:
             return 3;
             break;

         case RT_ALERT_TYPE_HAZARD:
             return 3;
             break;

         default:
             return 0;
   }
}

const char* RTAlerts_get_subtype_label(int iAlertType, int iAlertSubType){
   switch (iAlertType){
      case RT_ALERT_TYPE_POLICE:
         if (iAlertSubType == POLICE_TYPE_VISIBLE)
              return roadmap_lang_get("Visible");
         else if (iAlertSubType == POLICE_TYPE_HIDING)
              return roadmap_lang_get("Hidden");

            break;

        case RT_ALERT_TYPE_ACCIDENT:
           if (iAlertSubType == ACCIDENT_TYPE_MINOR)
                return roadmap_lang_get("Minor");
           else if (iAlertSubType == ACCIDENT_TYPE_MAJOR)
                return roadmap_lang_get("Major");
            break;

        case RT_ALERT_TYPE_CHIT_CHAT:
            break;

        case RT_ALERT_TYPE_TRAFFIC_JAM:
           if (iAlertSubType == JAM_TYPE_LIGHT_TRAFFIC)
                return roadmap_lang_get("Light");
           else if (iAlertSubType == JAM_TYPE_MODERATE_TRAFFIC)
                return roadmap_lang_get("Moderate");
           else if (iAlertSubType == JAM_TYPE_HEAVY_TRAFFIC)
                return roadmap_lang_get("Heavy");
           else if (iAlertSubType == JAM_TYPE_STAND_STILL_TRAFFIC)
              return roadmap_lang_get("Standstill");
            break;

        case RT_ALERT_TYPE_HAZARD:
           switch (iAlertSubType){
           case HAZARD_TYPE_ON_ROAD:
              return roadmap_lang_get("Road");
           case HAZARD_TYPE_ON_SHOULDER:
              return roadmap_lang_get("Shoulder");
           case HAZARD_TYPE_WEATHER:
              return roadmap_lang_get("Weather");
           case HAZARD_TYPE_ON_ROAD_OBJECT:
              return roadmap_lang_get("Object");
           case HAZARD_TYPE_ON_ROAD_POT_HOLE:
              return roadmap_lang_get("Pot hole");
           case HAZARD_TYPE_ON_ROAD_ROAD_KILL:
              return roadmap_lang_get("Road kill");
           case HAZARD_TYPE_ON_SHOULDER_CAR_STOPPED:
              return roadmap_lang_get("Car stopped");
           case HAZARD_TYPE_ON_SHOULDER_ANIMALS:
              return roadmap_lang_get("Animals");
           case HAZARD_TYPE_ON_SHOULDER_MISSING_SIGN:
              return roadmap_lang_get("Missing sign");
           case HAZARD_TYPE_WEATHER_FOG:
              return roadmap_lang_get("Fog");
           case HAZARD_TYPE_WEATHER_HAIL:
              return roadmap_lang_get("Hail");
           case HAZARD_TYPE_WEATHER_HEAVY_RAIN:
              return roadmap_lang_get("Heavy rain");
           case HAZARD_TYPE_WEATHER_HEAVY_SNOW:
              return roadmap_lang_get("Heavy snow");
           case HAZARD_TYPE_WEATHER_FLOOD:
              return roadmap_lang_get("Flood");
           case HAZARD_TYPE_WEATHER_MONSOON:
              return roadmap_lang_get("Monsoon");
           case HAZARD_TYPE_WEATHER_TORNADO:
              return roadmap_lang_get("Tornado");
           case HAZARD_TYPE_WEATHER_HEAT_WAVE:
              return roadmap_lang_get("Tornado");
           case HAZARD_TYPE_WEATHER_HURRICANE:
              return roadmap_lang_get("hurricane");
           case HAZARD_TYPE_WEATHER_FREEZING_RAIN:
              return roadmap_lang_get("Freezing rain");
           case HARARD_TYPE_ON_ROAD_LANE_CLOSED:
              return roadmap_lang_get("Lane closed");
           case HAZARD_TYPE_ON_ROAD_OIL:
              return roadmap_lang_get("Oil spill");
           case HAZARD_TYPE_ON_ROAD_ICE:
              return roadmap_lang_get("Ice");
           case HAZRAD_TYPE_ON_ROAD_CONSTRUCTION:
              return roadmap_lang_get("Construction zone");
           case -1:
               return roadmap_lang_get("Hazard");
           default:
              return getHazardStr(iAlertSubType);
           }
           break;

        default:
            return "";
     }
   return "";
}

int RTAlerts_get_default_subtype(int iAlertType){
   switch (iAlertType){
       case RT_ALERT_TYPE_POLICE:
             return -1;
             break;

         case RT_ALERT_TYPE_ACCIDENT:
             return -1;
             break;

         case RT_ALERT_TYPE_TRAFFIC_JAM:
             return -1;
             break;

         case RT_ALERT_TYPE_HAZARD:
             return -1;
             break;

         default:
             return -1;
   }
}

char* RTAlerts_get_subtype_icon(int iAlertType, int iAlertSubType){
   switch (iAlertType){
      case RT_ALERT_TYPE_POLICE:
         if (iAlertSubType == POLICE_TYPE_VISIBLE)
            return "alert_icon_police_visible";
         else
            return "alert_icon_police_hiding";

      case RT_ALERT_TYPE_ACCIDENT:
        if (iAlertSubType == ACCIDENT_TYPE_MINOR)
         return "alert_icon_accident_minor";
        else
           return "alert_icon_accident_major";

      case RT_ALERT_TYPE_CHIT_CHAT:
            return "alert_icon_chit_chat";
            break;

      case RT_ALERT_TYPE_TRAFFIC_JAM:
        switch (iAlertSubType){
           case JAM_TYPE_LIGHT_TRAFFIC:
           case JAM_TYPE_MODERATE_TRAFFIC:
              return "alert_icon_traffic_jam_light";
           case JAM_TYPE_HEAVY_TRAFFIC:
              return "alert_icon_traffic_jam_heavy";
           case JAM_TYPE_STAND_STILL_TRAFFIC:
              return "alert_icon_traffic_jam_standstill";
           default:
              return "alert_icon_traffic_jam";
      }
      break;

      case RT_ALERT_TYPE_HAZARD:
           switch (iAlertSubType){
                case HAZARD_TYPE_ON_ROAD:
                   return "alert_icon_hazard_on_road";
                case HAZARD_TYPE_ON_SHOULDER:
                   return "alert_icon_hazard_on_shoulder";
                case HAZARD_TYPE_WEATHER:
                    return "alert_icon_hazard_weather";
                default:
                    return "alert_icon_hazard";
            }
            break;

      case RT_ALERT_TYPE_CONSTRUCTION:
            return "alert_icon_road_construction";
            break;

      case RT_ALERT_TYPE_PARKING:
            return "alert_icon_parking";
            break;

      default:
            return "";
     }
}


static int on_segmented_control_selected (SsdWidget widget, const char *new_value, void *context){
   RTAlertContext *AlertContext = (RTAlertContext *)context;
   AlertContext->iSubType = atoi(new_value);
   ssd_dialog_change_text("title_text",RTAlerts_get_title(NULL, AlertContext->iType, AlertContext->iSubType) );
   return 1;
}


int RTAlerts_get_num_categories(int iAlertType, int iAlertSubType){
   switch (iAlertType){
          case RT_ALERT_TYPE_HAZARD:
             switch (iAlertSubType){
                 case HAZARD_TYPE_ON_ROAD:
                    return gOnRoadHazardCategoriesCount;
                 case HAZARD_TYPE_ON_SHOULDER:
                    return gOnShoulderHazardCategoriesCount;
                 case HAZARD_TYPE_WEATHER:
                    return gWeatherHazardCategoriesCount;
                 default:
                    return 0;
             }
          default:
             return 0;
      }
}

int RTAlerts_get_categories_subtype(int iAlertType, int iAlertSubType, int index){
   switch (iAlertType){
          case RT_ALERT_TYPE_HAZARD:
             switch (iAlertSubType){
                 case HAZARD_TYPE_ON_ROAD:
                       return gOnRoadHazardCategories[index];
                 case HAZARD_TYPE_ON_SHOULDER:
                       return gOnShoulderHazardCategories[index];
                 case HAZARD_TYPE_WEATHER:
                       return gWeatherHazardCategories[index];
                 default:
                    return 0;
             }
   }
   return 0;
}

static const char *get_categories_values(int iAlertType, int iAlertSubType, int index){
   char str[5];

   sprintf(str, "%d", RTAlerts_get_categories_subtype(iAlertType,iAlertSubType, index));
   return strdup(&str[0]);
}

static int category_menu_cancel (SsdWidget widget, const char *new_value){
   RTAlertContext *AlertContext = (RTAlertContext *)widget->context;
   AlertContext->iSubType = -1;
   ssd_dialog_hide_current(dec_close);
   ssd_dialog_change_text("categoryText","");
   return 1;
}

static int category_menu_selected (SsdWidget widget, const char *new_value){
   RTAlertContext *AlertContext = (RTAlertContext *)widget->context;
   AlertContext->iSubType = atoi(widget->value);
   ssd_dialog_hide_current(dec_close);
   ssd_dialog_change_text("categoryText",RTAlerts_get_title(NULL, AlertContext->iType, AlertContext->iSubType) );
   return 1;
}


static void category_menu(int iAlertType, int iAlertSubType, RTAlertContext *AlertContext){
   int num_categories;
   SsdWidget dialog;
   SsdWidget container;
   SsdWidget box;
   SsdWidget box2;
   SsdWidget text;
   SsdWidget bitmap;
   int sc_height = roadmap_canvas_height();
   int sc_width = roadmap_canvas_width();

   int i;


   dialog = ssd_dialog_new ("contextmenu_dialog", "", NULL,
       SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT| SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_SHADOW_BG);
   ssd_widget_set_size(dialog, roadmap_canvas_width(), roadmap_canvas_height());

#ifndef EMBEDDED_CE
   if (sc_height < sc_width)
      sc_width = sc_height;
#endif

   sc_width -= 40;
   if (!roadmap_screen_is_hd_screen()){
     if (sc_width > 240)
        sc_width = 240;
   }
   else{
     if (sc_width > 480)
        sc_width = 480;
   }
   container = ssd_container_new("Alert.Catgories.Container", NULL, sc_width, SSD_MIN_SIZE,SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_CONTAINER_BORDER);
   ssd_widget_set_color(container, NULL, NULL);

   num_categories = RTAlerts_get_num_categories(iAlertType, iAlertSubType);
   if (num_categories == 0)
      return;

   for (i=0; i< num_categories; i++){

      box = ssd_container_new ("Label.Container", NULL, SSD_MAX_SIZE, ssd_container_get_row_height(), SSD_END_ROW|SSD_WS_TABSTOP);
      ssd_widget_set_color(box, NULL, NULL);
      text = ssd_text_new ("CancelTxt", RTAlerts_get_subtype_label(iAlertType, RTAlerts_get_categories_subtype(iAlertType,iAlertSubType,i)), 20, SSD_TEXT_NORMAL_FONT|SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
      ssd_widget_set_color(text,"#373737","#ffffff");
      ssd_widget_add(box, text);
      box->callback = category_menu_selected ;
      box->context = AlertContext;
      box->value = (char *)get_categories_values(iAlertType, iAlertSubType,i);
      ssd_widget_set_pointer_force_click( box );
      ssd_widget_add(container, box);
      ssd_widget_add(container, ssd_separator_new("Sep",0));
   }

   box = ssd_container_new ("Cancel", NULL, SSD_MAX_SIZE, ssd_container_get_row_height(), SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);

   box2 = ssd_container_new ("Cancel", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(box2, NULL, NULL);

   bitmap = ssd_bitmap_new("cancel", "cancel", SSD_ALIGN_VCENTER);
   ssd_widget_set_offset(bitmap, 0 ,5);
   ssd_widget_add(box2, bitmap);
   ssd_dialog_add_hspace(box2, 10, 0);

   text = ssd_text_new ("CancelTxt", roadmap_lang_get("Cancel"), 20, SSD_TEXT_NORMAL_FONT | SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#373737");
   ssd_widget_add(box2, text);
   box->context = AlertContext;
   box->callback = category_menu_cancel ;
   ssd_widget_set_pointer_force_click( box );
   ssd_widget_add(box, box2);
   ssd_widget_add(container, box);

   ssd_widget_add(dialog, container);

   ssd_dialog_activate("contextmenu_dialog", NULL);
}

static int on_category_selected (SsdWidget widget, const char *new_value, void *context){
   RTAlertContext *AlertContext = (RTAlertContext *)context;
   AlertContext->iSubType = atoi(new_value);
   ssd_dialog_change_text("title_text",RTAlerts_get_title(NULL, AlertContext->iType, AlertContext->iSubType) );
   category_menu(AlertContext->iType, AlertContext->iSubType, AlertContext);
   return 1;
}

static SsdSegmentedControlCallback get_type_callback(int iAlertType, int iAlertSubType){
   switch (iAlertType){
          case RT_ALERT_TYPE_HAZARD:
             switch (iAlertSubType){
                 case HAZARD_TYPE_ON_ROAD:
                    return on_category_selected;
                 case HAZARD_TYPE_ON_SHOULDER:
                    return on_category_selected;
                 case HAZARD_TYPE_WEATHER:
                    return on_category_selected;
                 default:
                    return on_segmented_control_selected;
             }
          default:
             return on_segmented_control_selected;
      }
}

static SsdWidget get_sub_type_container(int iAlertType,RTAlertContext *AlertConext ){

   SsdWidget container;
   int i;
   char name[3][5];
   SsdWidget buttons;
   const char *tab_labels[3];
   const char *tab_values[3];
   const char *tab_icons[3];
   SsdSegmentedControlCallback callbacks[3];

   if (RTAlerts_get_number_of_sub_types(iAlertType) == 0)
      return NULL;

   container = ssd_container_new ("Alert.SubType.Container", NULL,
                               SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_CENTER);

   for (i = 0; i < RTAlerts_get_number_of_sub_types(iAlertType); i++){
       name[i][0] = 0;
      sprintf(name[i], "%d",i);
      tab_labels[i] = RTAlerts_get_subtype_label(iAlertType,  i);
      tab_values[i] = name[i];
      tab_icons[i] = RTAlerts_get_subtype_icon(iAlertType, i);
      callbacks[i] = get_type_callback(iAlertType,i);

      if (i == RTAlerts_get_default_subtype(iAlertType))
         AlertConext->iSubType = i;
   }
   buttons =  ssd_segmented_icon_control_new ("Filter", RTAlerts_get_number_of_sub_types(iAlertType), tab_labels, (const void **)tab_values, tab_icons, SSD_END_ROW|SSD_ALIGN_CENTER, callbacks, AlertConext, RTAlerts_get_default_subtype(iAlertType));
   ssd_widget_add(container, buttons);

   return container;
}

int on_lane_button_selected (SsdWidget widget, const char *new_value, void *context){
   RTAlertContext *AlertContext = (RTAlertContext *)context;
   AlertContext->iDirection = atoi(new_value);
   return 1;
}

static void on_report_dialog_close(int exit_code, void* context){
   if ((exit_code == dec_ok) && (gMinimizeState != -1)){
      const char *path = roadmap_path_first ("config");
      const char *file_name = VOICE_FILENAME;

      gCurrentVoiceId[0] = 0;
      if (roadmap_file_exists(path, file_name)) {
         roadmap_file_remove(path, file_name);
      }
      gMinimizeState = -1;
      roadmap_trip_restore_focus();
   }
}



/////////////////////////////////////////////////////////////////////
static void report_dialog(int iAlertType){
	SsdWidget dialog;
   SsdWidget group;
   SsdWidget container;
   SsdWidget container1;
   SsdWidget container2;
	SsdWidget text,  box, button;
	SsdWidget text_box;
	SsdWidget tab;
	SsdWidget sub_type_container;
	SsdWidget wgt_hspace;
	RTAlertContext *AlertConext;
	char *icon[3];
   const char *tab_labels[3];
   const char *tab_values[3];
   SsdSegmentedControlCallback callbacks[3];
	BOOL isLongScreen;
	const char *group_name;
	const RoadMapGpsPosition   *TripLocation;
	int container_height = ssd_container_get_row_height();
	int container1_width = roadmap_canvas_width();
	int container2_width;
	int container2_height;
	int txtbox_height = ADJ_SCALE(43);
	SsdSize container1_size;

	const char *button_icon[]   = {"right_button_up", "right_button_down"};
	const char *edit_button_r[] = {"edit_right", "edit_right"};
	const char *edit_button_l[] = {"edit_left", "edit_left"};

   AlertConext = calloc(1, sizeof(RTAlertContext));
   gCurrentImageId[0] = 0;
   AlertConext->iType = iAlertType;
   AlertConext->iSubType = -1;

   gCurrentImageId[0] = 0;

   TripLocation = RTAlerts_alerts_location(TRUE);
   if (TripLocation == NULL) {
         return;
    }
   InitVoiceRecording();

    isLongScreen = (roadmap_canvas_height()>320);
#ifdef EMBEDDED_CE
	isLongScreen = FALSE;
#endif


	if (roadmap_canvas_width() > roadmap_canvas_height()){
	      container1_width = roadmap_canvas_height()+ADJ_SCALE(25);
	}


	gMinimizeState = -1;


	dialog = ssd_dialog_new ("Report",
                            RTAlerts_get_title(NULL, iAlertType, -1),
                            on_report_dialog_close,
                            SSD_CONTAINER_TITLE);


   group = ssd_container_new ("Report", NULL,
                        		SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   container1 = ssd_container_new ("Container1", NULL,
                                  container1_width-20,SSD_MIN_SIZE,
                                  SSD_WIDGET_SPACE);
   ssd_widget_set_color(container1,NULL, NULL);

	sub_type_container = get_sub_type_container(iAlertType, AlertConext);
   if (sub_type_container){
      ssd_widget_add(container1, sub_type_container);
      ssd_dialog_add_vspace(container1, 1, 0);
      text = ssd_text_new ("categoryText", "", 12,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
      ssd_text_set_color(text,"#4789ac");
      ssd_widget_add(container1, text);

   }
   else{
      ssd_dialog_add_vspace(container1,ADJ_SCALE(10),0);
   }

   box = ssd_container_new ("Additional details group", NULL,
                 SSD_MAX_SIZE,sub_type_container ? container_height : container_height +ADJ_SCALE(63),
                 SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color(box, NULL, NULL);
   if (!sub_type_container)
      txtbox_height = ADJ_SCALE(83);

#if (!defined(__SYMBIAN32__) && !defined(_WIN32))
//#ifndef __SYMBIAN32__
   icon[0] = "button_record";
   icon[1] = "button_record";
   icon[2] = NULL;
   button = ssd_button_new( "Record_button", "Record", (const char**) &icon[0], 2, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, on_record_button );
   ssd_widget_add(box, button);
   ssd_dialog_add_hspace(box, 5, 0);
#endif
   text_box = ssd_entry_new( roadmap_lang_get("Additional Details"),"",SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP|SSD_ALIGN_VCENTER,0, SSD_MAX_SIZE, txtbox_height,roadmap_lang_get("<Add description>"));
	ssd_entry_set_kb_params( text_box, roadmap_lang_get("Additional Details"), NULL, NULL, NULL, SSD_KB_DLG_TYPING_LOCK_ENABLE );
	ssd_widget_add(box, text_box);
   ssd_widget_add(container1, box);
   ssd_widget_add(container1, ssd_separator_new("sep", SSD_END_ROW));

   box = ssd_container_new ("Lanes group", NULL,
                 SSD_MAX_SIZE,container_height,
                 SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color(box, NULL, NULL);


   tab_labels[0] = roadmap_lang_get("My lane");
   tab_labels[1] = roadmap_lang_get("Other lane");
   tab_values[0] = roadmap_lang_get("1");
   tab_values[1] = roadmap_lang_get("2");
   callbacks[0] = on_lane_button_selected;
   callbacks[1] = on_lane_button_selected;
   AlertConext->iDirection = RT_ALERT_MY_DIRECTION;
   tab =  ssd_segmented_control_new ("Lanes", 2, tab_labels, (const void **)tab_values,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, callbacks, AlertConext, 0);

   ssd_widget_add(box, tab);
   ssd_widget_add(container1, box);
   ssd_widget_add(container1, ssd_separator_new("sep", SSD_ALIGN_BOTTOM));
   ssd_widget_add(group, container1);

   ssd_widget_get_size(container1, &container1_size, NULL);
   if (is_screen_wide()){
      wgt_hspace = ssd_container_new( "hspacer", "", 1, container1_size.height, 0 );
      ssd_widget_set_color( wgt_hspace, "#d8dae0", "#d8dae0" );
      ssd_widget_add( group, wgt_hspace );
   }

#ifndef EMBEDDED_CE
   if (is_screen_wide()){
      container2_width = roadmap_canvas_width() - container1_size.width-20;
      container2_height = container1_size.height;
   }
   else{
      container2_width = container1_width-20;
      container2_height = SSD_MIN_SIZE;
   }

   container2 = ssd_container_new ("Container2", NULL,
                                   container2_width, container2_height,
                                   SSD_WIDGET_SPACE);
   ssd_widget_set_color(container2, NULL, NULL);
   if (is_screen_wide()){
      container_height = container1_size.height/2;
   }

   box = ssd_container_new ("Groups group", NULL,
                SSD_MAX_SIZE,container_height,
                SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box, NULL, NULL);

    icon[0] = "add_image_box";
    icon[1] = NULL;
    ssd_widget_add (box,
               ssd_button_new ("AddImage", roadmap_lang_get("Add image"), (const char **)&icon[0], 1,
                              SSD_ALIGN_VCENTER, NULL));
    box->callback = on_add_image;
    ssd_widget_set_pointer_force_click( box );
    ssd_dialog_add_hspace(box, 5,0);
    text = ssd_text_new ("Add image Text", roadmap_lang_get("Take a picture"), 14,SSD_ALIGN_VCENTER);
    ssd_text_set_color(text,"#373737");
    ssd_widget_add(box, text);
    if (!is_screen_wide()){
        if (!ssd_widget_rtl(NULL))
           button = ssd_button_new ("edit_button", "", &edit_button_r[0], 2,
                    SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_group_select);
        else
           button = ssd_button_new ("edit_button", "", &edit_button_l[0], 2,
                    SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_group_select);
        if (!ssd_widget_rtl(NULL))
           ssd_widget_set_offset(button, -10, 0);
        else
           ssd_widget_set_offset(button, 11, 0);
        ssd_widget_add(box, button);
    }
    ssd_widget_add(container2, box);


      ssd_widget_add(container2, ssd_separator_new("sep", SSD_END_ROW));

      roadmap_groups_set_selected_group_name(roadmap_groups_get_active_group_name());
      roadmap_groups_set_selected_group_icon (roadmap_groups_get_active_group_icon());

      box = ssd_container_new ("Groups group", NULL,
                 SSD_MAX_SIZE,container_height,
                 SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);

      ssd_widget_set_color (box, NULL, NULL);

      if (roadmap_groups_get_num_following() > 0){

         box->callback = on_group_select;
         ssd_widget_set_pointer_force_click( box );

         group_name = roadmap_groups_get_active_group_name();
         container =  ssd_container_new ("space", NULL, roadmap_canvas_width()-100, SSD_MIN_SIZE,SSD_ALIGN_VCENTER);
         ssd_widget_set_color(container, NULL, NULL);
         text = ssd_text_new ("GroupTextTitle", roadmap_lang_get("Also send this report to:"), 12, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
         ssd_text_set_color(text, "#828282");
         ssd_dialog_add_hspace(container, 7,0);
         ssd_widget_add (container,text);

         ssd_dialog_add_hspace(container, 5,0);
         ssd_widget_add (container,  ssd_bitmap_new("GroupIcon",  roadmap_groups_get_active_group_icon(),  0));
         ssd_dialog_add_hspace(container, 5,0);

         if (group_name[0] == 0){
            text = ssd_text_new ("Group Text", roadmap_lang_get("No group"), 14, SSD_ALIGN_VCENTER);
            ssd_text_set_color(text,"#373737");
            ssd_dialog_add_hspace(container, 5,0);
            ssd_widget_add (container,text);

         }
         else{
            text = ssd_text_new ("Group Text", group_name, 14, SSD_ALIGN_VCENTER);
            ssd_text_set_color(text,"#373737");
            ssd_dialog_add_hspace(container, 5,0);
            ssd_widget_add (container,text);

         }
        // ssd_widget_add(box, ssd_separator_new("sep", SSD_ALIGN_BOTTOM));
         ssd_widget_add(box, container);

         if (!is_screen_wide()){
            if (!ssd_widget_rtl(NULL))
               button = ssd_button_new ("edit_button", "", &edit_button_r[0], 2,
                        SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_group_select);
            else
               button = ssd_button_new ("edit_button", "", &edit_button_l[0], 2,
                        SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_group_select);
            if (!ssd_widget_rtl(NULL))
               ssd_widget_set_offset(button, -10, 0);
            else
               ssd_widget_set_offset(button, 11, 0);
            ssd_widget_add(box, button);
         }


      }
      ssd_widget_add (container2, box);
      if (roadmap_groups_get_num_following() > 0)
         ssd_widget_add(container2, ssd_separator_new("sep", SSD_ALIGN_BOTTOM));

   ssd_widget_add (group,container2);
#endif
   if (is_screen_wide())
      container_height = SSD_MIN_SIZE;

   box = ssd_container_new ("Send.Container", NULL,
                SSD_MAX_SIZE,container_height,
                SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_BOTTOM);
   ssd_widget_set_color (box, NULL, NULL);
   if (!is_screen_wide())
      ssd_dialog_add_hspace(box, 10,0);
   icon[0] = "send_button";
   icon[1] = "send_button_s";
   icon[2] = NULL;
   button = ssd_button_label_custom( "Send", roadmap_lang_get ("Send"),SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, report_menu_buttons_callback,
         (const char **)icon, 2, "#FFFFFF", "#FFFFFF",18 );
   button->context = AlertConext;
   ssd_widget_add (box, button);
   ssd_widget_add (group, box);


	ssd_widget_add(dialog, group);



	ssd_dialog_activate("Report", NULL);

   ssd_dialog_change_button("right_title_button", button_icon, 2);
   button = ssd_dialog_right_title_button();
   button->context = AlertConext;
	button->callback = report_menu_buttons_callback;
	if (ssd_widget_rtl(NULL))
	   ssd_widget_set_offset(button, 10, 0);
	else
	   ssd_widget_set_offset(button, -10, 0);
	ssd_widget_show(button);
	#if defined (_WIN32) && !defined (OPENGL)
	    text = ssd_text_new ("label", roadmap_lang_get("Hide"), 12, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
	#else
	    text = ssd_text_new ("label", roadmap_lang_get("Hide"), 14, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
	#endif
	ssd_widget_set_color(text, "#ffffff", "#ffffff");
	ssd_widget_add (button,text);

}
#endif

void RTAlerts_Reset_Minimized(void){
  gMinimizeState = -1;
}

int RTAlerts_Get_Minimize_State(void){
	return gMinimizeState;
}

void RTAlerts_Minimized_Alert_Dialog(void){
	ssd_dialog_activate("Report", NULL);
	ssd_dialog_draw ();
}

void RTAlerts_ShowProgressDlg(void)
{
   //Also called from iPhone native dialog
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Sending report..." ) );
}

void RTAlerts_CloseProgressDlg(void)
{
	ssd_progress_msg_dialog_hide();
}

static void RTAlerts_ReleaseImagePath(void)
{
	if ( gCurrentImagePath )
		free( gCurrentImagePath );
	gCurrentImagePath = NULL;
}

#ifndef IPHONE
#endif //IPHONE
static   SsdWidget CheckboxTypeOfError[MAX_MAP_PROBLEM_COUNT];
#ifdef TOUCH_SCREEN
////////////////////////////////////////////////////////////////////
static int map_error_buttons_callback (SsdWidget widget, const char *new_value) {

   SsdWidget container = widget->parent;
   char type[3];

   BOOL success;
   if (!strcmp(widget->name, "Send")){
      const char *selected;
      const RoadMapGpsPosition   *TripLocation;
      int i;

      const char *description = ssd_widget_get_value(ssd_widget_get(container, roadmap_lang_get("Additional Details")),roadmap_lang_get("Additional Details"));

      TripLocation = roadmap_trip_get_gps_position("AlertSelection");
      for (i = 0; i < gMapProblemsCount; i++){
         selected = ssd_dialog_get_data (CheckboxTypeOfError[i]->name);
         if (!strcmp(selected, "yes")){
            snprintf(type,sizeof(type), "%d", gMapProblems[i]+6);
            break;
         }
      }

      if ((i == 5) && (description[0] == 0)){
         return 0;
      }

      success = Realtime_ReportMapProblem(type, description,TripLocation);
      roadmap_trip_restore_focus();
      ssd_dialog_hide_all(dec_close);
      if(success)
      	 roadmap_messagebox_timeout("Thanks for reporting", "Map problems are fixed by wazers like you. you can edit the map too on www.waze.com",10);
      if ( (!success) && (!RealTimeLoginState()))
      	 roadmap_messagebox_timeout("Thanks for reporting","We could not send your report due to problems in the network connection - Your reports will be sent automatically when your network connection is restored. Map problems are fixed by wazers like you. you can edit the map too on www.waze.com",10);
   }

   else{
         roadmap_trip_restore_focus();
         ssd_dialog_hide_all(dec_close);
   }

   return 1;
}

#endif

static int checkbox_callback (SsdWidget widget, const char *new_value){
   int i;
   for (i=0 ; i< gMapProblemsCount; i++){
      if (CheckboxTypeOfError[i]) {
         if (strcmp(widget->parent->name, CheckboxTypeOfError[i]->name))
            CheckboxTypeOfError[i]->set_data(CheckboxTypeOfError[i], "no");
         else
            CheckboxTypeOfError[i]->set_data(CheckboxTypeOfError[i], "yes");
      }
   }
   return 1;
}

#ifndef TOUCH_SCREEN
static int send_map_problem_sk_cb(SsdWidget widget, const char *new_value, void *context){
   SsdWidget container = widget;
   char type[3];
   BOOL success;

   const char *selected;
   const RoadMapGpsPosition   *TripLocation;
   int i;

   const char *description = ssd_widget_get_value(ssd_widget_get(container, roadmap_lang_get("Additional Details")),roadmap_lang_get("Additional Details"));

   TripLocation = roadmap_trip_get_gps_position("AlertSelection");

   for (i = 0; i < gMapProblemsCount; i++){
      selected = ssd_dialog_get_data (CheckboxTypeOfError[i]->name);
      if (!strcmp(selected, "yes")){
           snprintf(type, sizeof(type), "%d", gMapProblems[i]+6);
           break;
       }
    }
    success = Realtime_ReportMapProblem(type, description,TripLocation);
    ssd_dialog_hide_all(dec_ok);
    return 1;
}
#endif

#ifndef IPHONE

void on_map_problem_close (int exit_code, void* context){
   roadmap_trip_restore_focus();
}

void RTAlerts_report_map_problem(){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget space_w;
   SsdWidget text, bitmap, box;
   SsdWidget text_box;
   const RoadMapGpsPosition *TripLocation;
   char sLocationStr[200];
   int login_state;
   int i;
   int iDirection = RT_ALERT_MY_DIRECTION;
   int row_height = ssd_container_get_row_height();

   login_state = RealTimeLoginState();

   TripLocation = RTAlerts_alerts_location(TRUE);
   if (TripLocation == NULL)  {
         return;
   }


    gMinimizeState = -1;

   dialog = ssd_dialog_new ("MapErrorDlg", roadmap_lang_get ("Map error"),
            on_map_problem_close, SSD_CONTAINER_TITLE);

   group = ssd_container_new ("MapErrorGrp", NULL, ssd_container_get_width(), SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
                     | SSD_ROUNDED_WHITE | SSD_POINTER_NONE
                     | SSD_CONTAINER_BORDER | SSD_ALIGN_CENTER);
   //ssd_widget_add (group, space (1));

   box = ssd_container_new ("box", NULL, SSD_MIN_SIZE, row_height,
            SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color (box, NULL, NULL);
   bitmap = ssd_bitmap_new ("icon", "map_error_small", 0);
   ssd_widget_add (box, bitmap);
   get_location_str (&sLocationStr[0], iDirection,TripLocation );
   text = ssd_text_new ("Alert position", sLocationStr, -1, SSD_END_ROW
            | SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER);
   ssd_widget_set_color (text, "#206892", NULL);
   ssd_widget_add (box, text);
   ssd_widget_add (group, box);

   ssd_widget_add (group, ssd_separator_new ("separator", SSD_END_ROW));

   for (i = 0; i < gMapProblemsCount; i++) {
      box = ssd_container_new ("CheckBoxbox", NULL, SSD_MAX_SIZE, row_height,
               SSD_WIDGET_SPACE | SSD_END_ROW | SSD_WS_TABSTOP);

      CheckboxTypeOfError[i] = ssd_checkbox_new (MAP_PROBLEMS_OPTION[gMapProblems[i]], FALSE,
               SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT, checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_V);
      ssd_widget_add (box, CheckboxTypeOfError[i]);

      space_w = ssd_container_new ("spacer1", NULL, 10, 14, 0);
      ssd_widget_set_color (space_w, NULL, NULL);
      ssd_widget_add (box, space_w);

      ssd_widget_add (box, ssd_text_new ("TextCheckbox", roadmap_lang_get(MAP_PROBLEMS_OPTION[gMapProblems[i]]), SSD_MAIN_TEXT_SIZE,
               SSD_TEXT_NORMAL_FONT | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

      ssd_widget_add (group, box);
      ssd_widget_add (group, ssd_separator_new ("separator", SSD_END_ROW));
   }
   CheckboxTypeOfError[gMapProblemsCount - 1]->set_data (CheckboxTypeOfError[gMapProblemsCount
            - 1], "yes");

   ssd_widget_add (group, space (2));

   text_box = ssd_entry_new (roadmap_lang_get ("Additional Details"), "",
            SSD_WS_TABSTOP, 0, SSD_MAX_SIZE, SSD_MIN_SIZE, roadmap_lang_get (
                     "<Add description>"));
   ssd_widget_add (group, text_box);
   ssd_widget_add (group, space (2));

#ifdef TOUCH_SCREEN
   ssd_widget_add (group, ssd_button_label ("Send", roadmap_lang_get ("Send"),
            SSD_WS_TABSTOP | SSD_ALIGN_CENTER | SSD_ALIGN_BOTTOM,
            map_error_buttons_callback));

   ssd_widget_add (group, ssd_button_label ("Cancel", roadmap_lang_get (
            "Cancel"), SSD_WS_TABSTOP | SSD_ALIGN_CENTER | SSD_ALIGN_BOTTOM,
            map_error_buttons_callback));
#else
   ssd_widget_set_left_softkey_callback(dialog, send_map_problem_sk_cb);
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get ("Send"));
#endif
   ssd_widget_add (dialog, group);
   ssd_dialog_activate ("MapErrorDlg", NULL);
}
#endif //IPHONE


// initializes OnRoad Hazard categories
static void RTinitOnRoadHazardCategories(void){
    char * Categories;
    char * tempC;
    int i = 0;
    int tNum;
    static BOOL initialized = FALSE;

    if (initialized)
       return;

   roadmap_config_declare ("preferences", &RoadMapConfigOnRoadHazardCategories, "22-20-3-4", NULL);

   Categories = strdup(roadmap_config_get(&RoadMapConfigOnRoadHazardCategories));
   tempC  = strtok(Categories,"-"); // preferences "1-3-5-6" will lead to showing map problems 1,3,5,6 in the menu
   while (tempC!=NULL && i<MAX_CATEGORIES){
      tNum = atoi(tempC);
       tempC = strtok(NULL,"-");//parse
       gOnRoadHazardCategories[i] = tNum;
       gOnRoadHazardCategoriesCount++;
       i++;
   }
   free(Categories);
   initialized = TRUE;
}

// initializes OnRoad Hazard categories
static void RTinitOnShoulderHarzardCategories(void){
    char * Categories;
    char * tempC;
    int i = 0;
    int tNum;
    static BOOL initialized = FALSE;

    if (initialized)
       return;

   roadmap_config_declare ("preferences", &RoadMapConfigOnShoulderHazardCategories, "6-7-8", NULL);

   Categories = strdup(roadmap_config_get(&RoadMapConfigOnShoulderHazardCategories));
   tempC  = strtok(Categories,"-"); // preferences "1-3-5-6" will lead to showing map problems 1,3,5,6 in the menu
   while (tempC!=NULL && i<MAX_CATEGORIES){
      tNum = atoi(tempC);
       tempC = strtok(NULL,"-");//parse
       gOnShoulderHazardCategories[i] = tNum;
       gOnShoulderHazardCategoriesCount++;
       i++;
   }
   free(Categories);
   initialized = TRUE;
}

// initializes OnRoad Hazard categories
static void RTinitWeatherHarzardCategories(void){
    char * Categories;
    char * tempC;
    int i = 0;
    int tNum;
    static BOOL initialized = FALSE;

    if (initialized)
       return;

   roadmap_config_declare ("preferences", &RoadMapConfigWeatherHazardCategories, "9-10-11-12", NULL);

   Categories = strdup(roadmap_config_get(&RoadMapConfigWeatherHazardCategories));
   tempC  = strtok(Categories,"-"); // preferences "1-3-5-6" will lead to showing map problems 1,3,5,6 in the menu
   while (tempC!=NULL && i<MAX_CATEGORIES){
      tNum = atoi(tempC);
       tempC = strtok(NULL,"-");//parse
       gWeatherHazardCategories[i] = tNum;
       gWeatherHazardCategoriesCount++;
       i++;
   }
   free(Categories);
   initialized = TRUE;
}

// initializes the map problems array and counter
static void RTinitMapProblems(void){
    char * mapOptions;
	char * tempC;
	int i = 0;
	int tNum;
    int mapProblemsOptionsNum;
	if(gMapProblemsInit==TRUE)
		return;// already initialized

	roadmap_config_declare ("preferences", &RoadMapConfigMapProblemOptions, "", NULL);

	mapOptions = strdup(roadmap_config_get(&RoadMapConfigMapProblemOptions));
	tempC  = strtok(mapOptions,"-"); // preferences "1-3-5-6" will lead to showing map problems 1,3,5,6 in the menu
	while (tempC!=NULL){
		tNum = atoi(tempC);
	    tempC = strtok(NULL,"-");//parse
	    mapProblemsOptionsNum = sizeof(MAP_PROBLEMS_OPTION)/sizeof(char*);
	    if (tNum > mapProblemsOptionsNum -1 ) // for older version support - don't show map problems you don't recognize
			continue;
		gMapProblems[i] = tNum;
	    gMapProblemsCount++;
	    i++;
	}
	free(mapOptions);
	gMapProblemsInit= TRUE;
}



//gets map problems info
int RTAlertsGetMapProblems (int **outMapProblems, char **outMapProblemsOption[]) {
   *outMapProblems = &gMapProblems[0];
   *outMapProblemsOption= &MAP_PROBLEMS_OPTION[0];
   return gMapProblemsCount;
}

//gets the icon name with the given number of stars
const char * RTAlerts_Get_Stars_Icon(int starNum){
	switch(starNum){
		case 0:
			return "RT0Stars";
		case 1:
			return "RT1Star";
		case 2:
			return "RT2Stars";
		case 3:
			return "RT3Stars";
		case 4:
			return "RT4Stars";
		case 5:
			return "RT5Stars";
		default:
			return "RT0Stars";
	}
}



//adds the stars icon to the user who reported an alert
void RTAlerts_update_stars(SsdWidget container,  RTAlert *pAlert){
   SsdWidget stars_icon =  ssd_widget_get ( container, "stars_icon");
   if (stars_icon ==NULL){ // init the bitmap, call with 0 in the meanwhile.
   	  stars_icon = ssd_bitmap_new("stars_icon",RTAlerts_Get_Stars_Icon(0), 0);
        ssd_dialog_add_hspace(container, 5 , 0);
        ssd_widget_set_offset(stars_icon,0, 5);
	     ssd_widget_add(container, stars_icon);
   }
  // if alert was reported by unanonymous user add stars, otherwise hide
  if ( (pAlert->sReportedBy[0] != 0) &&  ( strcmp(pAlert->sReportedBy,roadmap_lang_get("anonymous"))))
  {
   		ssd_bitmap_update(stars_icon,RTAlerts_Get_Stars_Icon(pAlert->iRank));
   		ssd_widget_show(stars_icon);
  }
  else
   		ssd_widget_hide(stars_icon); // user is anonymous - need to remove stars icon{
}

//adds the stars icon to the user who commented
void RTAlerts_add_comment_stars(SsdWidget container,  RTAlertComment *pAlertComment){
	// add stars only if user isn't anonymous
     if ( strcmp(pAlertComment->sPostedBy,roadmap_lang_get("anonymous"))&&strcmp(pAlertComment->sPostedBy,"anonymous")){
      		 int rank = pAlertComment->iRank;
      		 if (rank != -1){
      		    SsdWidget stars_bitmap = ssd_bitmap_new("stars icon", RTAlerts_Get_Stars_Icon(rank),SSD_END_ROW);
      		    ssd_dialog_add_hspace(container, 5 , 0);
      		    ssd_widget_set_offset(stars_bitmap,0, 5);
      		    ssd_widget_add(container, stars_bitmap);
      		 }
    }
}

void RTAlerts_Set_Ignore_Max_Distance(BOOL ignore){
	gIgnoreAlertMaxDist = ignore;
}

/**
 * Builds the string containing the reported by info
 */
void RTAlerts_get_reported_by_string( RTAlert *pAlert, char* buf, int buf_len )
{

   // Display who reported the alert
   if (pAlert->sReportedBy[0] != 0)
   {
      if (pAlert->sFacebookName[0] != 0){
         snprintf(buf, buf_len,
                  " %s %s  ", roadmap_lang_get("by"), pAlert->sReportedBy );
      }
      else{
         snprintf(buf, buf_len,
                  " %s %s  ", roadmap_lang_get("by"), pAlert->sReportedBy );
      }
   }
}

/*
 * removes the space before the description if needed
 */
void RTAlerts_show_space_before_desc( SsdWidget containter, RTAlert *pAlert ){
   if(pAlert->sReportedBy[0] == 0)
    	ssd_widget_hide(ssd_widget_get(containter,"space_before_descrip"));
   else
   		ssd_widget_show(ssd_widget_get(containter,"space_before_descrip"));
}

static void continue_report_after_audio_upload(void * context){
	int success;
	upload_Image_context * uploadContext = (upload_Image_context *)context;
	if ( gCurrentImagePath )
	{
		RTAlerts_ReleaseImagePath();
	}


	success = Realtime_Report_Alert(uploadContext->iType, uploadContext->iSubType, uploadContext->desc,
			uploadContext->iDirection, gCurrentImageId, gCurrentVoiceId,
	      roadmap_twitter_is_sending_enabled() && roadmap_twitter_logged_in(),
	      roadmap_facebook_is_sending_enabled() && roadmap_facebook_logged_in(), uploadContext->group);

	if (success){
	  ssd_dialog_hide_all(dec_close);
	}

	if ( !success )
		RTAlerts_CloseProgressDlg();

	if (uploadContext->desc)
		free(uploadContext->desc);

	if (uploadContext->group)
		free(uploadContext->group);
	free(uploadContext);
}

static void continue_report_after_image_upload(void * context){
   const char *path = roadmap_path_first ("config");
   const char *file_name = VOICE_FILENAME;

   //Upload voice file
   gCurrentVoiceId[0] = 0;
   if (roadmap_file_exists(path, file_name)) {
      roadmap_analytics_log_event(ANALYTICS_EVENT_VOICE_ALERT, NULL, NULL);
      if (!roadmap_recorder_voice_upload( path, file_name,
                                       gCurrentVoiceId, continue_report_after_audio_upload, context )){
         roadmap_log(ROADMAP_ERROR,"Error in uploading voice alert");
         continue_report_after_audio_upload( ( context)); // error in image upload, continue
      }
   } else {
      continue_report_after_audio_upload( ( context));
   }
}
/**
 */
BOOL RTAlerts_isByMe(int iId)
{

    RTAlert *pAlert = RTAlerts_Get_By_ID(iId);
    if (!pAlert)
        return FALSE;
    else
        return pAlert->bAlertByMe;
}

/**
 */
BOOL RTAlerts_ThumbsUpByMe(int iId)
{

    RTAlert *pAlert = RTAlerts_Get_By_ID(iId);
    if (!pAlert)
        return FALSE;
    else
        return pAlert->bThumbsUpByMe;
}

/**
 */
BOOL RTAlerts_isAlertOnRoute(int iId)
{
   RTAlert *pAlert;

   if (navigate_main_state() == -1)
       return FALSE;

   pAlert = RTAlerts_Get_By_ID(iId);
   if (!pAlert)
       return FALSE;
   else{
       if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
          return pAlert->bAlertIsOnRoute;
       else
          return pAlert->bAlertIsOnRoute;
   }
}

void RTAlerts_clear_group_counter(void){
 gDisplayClearedTimeStamp = time(NULL);
 gGroupState = STATE_OLD;
}


int RTAlerts_get_group_state(void){
   if (gAlertsTable.iGroupCount == 0)
        return STATE_OLD;
    else
       return gGroupState;
}

BOOL RTAlerts_isAlertArchive(int iId)
{
   RTAlert *pAlert;

   pAlert = RTAlerts_Get_By_ID(iId);
   if (!pAlert)
      return FALSE;
   else
      return pAlert->bArchive;
}


int RTALerts_OnRouteCount(void){
   int count = 0;
   int i;


   for (i=0; i< gAlertsTable.iCount; i++){
      if (gAlertsTable.alert[i]->bAlertIsOnRoute)
         count++;
   }

   return count;
}
