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

#ifndef	__REALTIME_ALERT_H__
#define	__REALTIME_ALERT_H__

#include "../roadmap_sound.h"
#include "../ssd/ssd_widget.h"
#include "../roadmap_alerter.h"

// Alerts types
#define RT_ALERT_TYPE_CHIT_CHAT			   0
#define RT_ALERT_TYPE_POLICE				   1
#define RT_ALERT_TYPE_ACCIDENT 				2
#define RT_ALERT_TYPE_TRAFFIC_JAM			3
#define RT_ALERT_TYPE_TRAFFIC_INFO			4
#define RT_ALERT_TYPE_HAZARD				   5
#define RT_ALERT_TYPE_OTHER				   6
#define RT_ALERT_TYPE_CONSTRUCTION        7
#define RT_ALERT_TYPE_PARKING             8
#define RT_ALERT_TYPE_DYNAMIC             9
#define RT_ALERTS_LAST_KNOWN_TYPE			9

#define POLICE_TYPE_VISIBLE 0
#define POLICE_TYPE_HIDING  1

#define ACCIDENT_TYPE_MINOR  0
#define ACCIDENT_TYPE_MAJOR  1

#define HAZARD_TYPE_ON_ROAD      0
#define HAZARD_TYPE_ON_SHOULDER  1
#define HAZARD_TYPE_WEATHER      2

#define HAZARD_TYPE_ON_ROAD                  0
#define HAZARD_TYPE_ON_SHOULDER              1
#define HAZARD_TYPE_WEATHER                  2
#define HAZARD_TYPE_ON_ROAD_OBJECT           3
#define HAZARD_TYPE_ON_ROAD_POT_HOLE         4
#define HAZARD_TYPE_ON_ROAD_ROAD_KILL        5
#define HAZARD_TYPE_ON_SHOULDER_CAR_STOPPED  6
#define HAZARD_TYPE_ON_SHOULDER_ANIMALS      7
#define HAZARD_TYPE_ON_SHOULDER_MISSING_SIGN 8
#define HAZARD_TYPE_WEATHER_FOG              9
#define HAZARD_TYPE_WEATHER_HAIL             10
#define HAZARD_TYPE_WEATHER_HEAVY_RAIN       11
#define HAZARD_TYPE_WEATHER_HEAVY_SNOW       12
#define HAZARD_TYPE_WEATHER_FLOOD            13
#define HAZARD_TYPE_WEATHER_MONSOON          14
#define HAZARD_TYPE_WEATHER_TORNADO          15
#define HAZARD_TYPE_WEATHER_HEAT_WAVE        16
#define HAZARD_TYPE_WEATHER_HURRICANE        17
#define HAZARD_TYPE_WEATHER_FREEZING_RAIN    18
#define HARARD_TYPE_ON_ROAD_LANE_CLOSED      19
#define HAZARD_TYPE_ON_ROAD_OIL              20
#define HAZARD_TYPE_ON_ROAD_ICE              21
#define HAZRAD_TYPE_ON_ROAD_CONSTRUCTION     22

#define JAM_TYPE_MODERATE_TRAFFIC            0
#define JAM_TYPE_HEAVY_TRAFFIC               1
#define JAM_TYPE_STAND_STILL_TRAFFIC         2
#define JAM_TYPE_LIGHT_TRAFFIC               3

//Alerts direction
#define RT_ALERT_BOTH_DIRECTIONS 			0
#define RT_ALERT_MY_DIRECTION	 	   		1
#define RT_ALERT_OPPSOITE_DIRECTION 		2

#define	RT_MAXIMUM_ALERT_COUNT           500
#define RT_ALERT_LOCATION_MAX_SIZE        150
#define RT_ALERT_DESCRIPTION_MAXSIZE      400
#define RT_ALERT_IMAGEID_MAXSIZE		      100
#define RT_ALERT_VOICEID_MAXSIZE		      100
#define RT_ALERT_USERNM_MAXSIZE 			   100
#define RT_ALERT_FACEBOOK_USERNM_MAXSIZE  100
#define RT_ALERT_GROUP_MAXSIZE            200
#define RT_ALERT_GROUP_ICON_MAXSIZE       100

#define RT_ALERTS_MAX_ALERT_TYPE           64
#define RT_ALERTS_MAX_ADD_ON_NAME         128
#define RT_ALERTS_MAX_ICON_NAME           128
#define RT_ALERTS_MAX_TITLE_NAME          128

#define RT_ALERT_RES_TITLE_MAX_SIZE       64
#define RT_ALERT_RES_TEXT_MAX_SIZE        512

#define RT_THUMBS_UP_QUEUE_MAXSIZE        50

#define RT_ALERTS_PROGRESS_DLG_NAME		"Alert progress dialog"

#define ALERT_ICON_PREFIX  "alert_icon_"
#define ALERT_PIN_PREFIX   "alert_pin_"

#define ALERT_TITLE_PREFIX "alert_title_"

#define ALERT_ICON_ADDON_PREFIX "alert_icon_addon_"
#define ALERT_PIN_ADDON_PREFIX  "alert_pin_addon_"

#define REROUTE_MIN_DISTANCE              30000

#define STATE_NONE		   -1
#define STATE_OLD          1
#define STATE_NEW          2
#define STATE_NEW_COMMENT  3
#define NEW_LINE "\n"


//////////////////////////////////////////////////////////////////////////////////////////////////
// Sort Method
typedef enum alert_sort_method
{
   sort_proximity,
   sort_recency,
   sort_priority
}  alert_sort_method;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Filter alerts
typedef enum alert_filter
{
   filter_none,
   filter_on_route,
   filter_group
}  alert_filter;

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Comment
typedef struct
{
    int iID; //	Comment ID (within the server)
    int iAlertId; //  Alert Type
    long long i64ReportTime; // The time the comment was posted
    char sPostedBy [RT_ALERT_USERNM_MAXSIZE+1]; // The User who posted the comment
    char sDescription [RT_ALERT_DESCRIPTION_MAXSIZE+1]; // The comment
    BOOL bCommentByMe;
    BOOL bDisplay;
    int  iRank;
    int  iMood;
    char sFacebookName[RT_ALERT_FACEBOOK_USERNM_MAXSIZE]; //Facebook name
    BOOL bShowFacebookPicture; // Show facebook Picture
} RTAlertComment;


//////////////////////////////////////////////////////////////////////////////////////////////////
// Thumbs Up
typedef struct
{
    int iID; // ID
    char sUserID [RT_ALERT_USERNM_MAXSIZE+1];// User ID
    char sNickName[RT_ALERT_USERNM_MAXSIZE+1]; // User NickName
    char sFacebookName[RT_ALERT_FACEBOOK_USERNM_MAXSIZE]; //Facebook name
    BOOL bShowFacebookPicture; // Show facebook Picture
    int  iIndex;
    BOOL bDisplayed;
} ThumbsUp;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ThubsUp Table
typedef struct
{
   ThumbsUp *thumbsUp[RT_THUMBS_UP_QUEUE_MAXSIZE];
   int iCount;
} RTThumbsUpTable;



#define MAP_PROBLEM_INCORRECT_TURN               "Incorrect turn"
#define MAP_PROBLEM_INCORRECT_ADDRESS            "Incorrect address"
#define MAP_PROBLEM_INCORRECT_ROUTE              "Incorrect route"
#define MAP_PROBLEM_INCORRECT_MISSING_ROUNDABOUT "Missing roundabout"
#define MAP_PROBLEM_INCORRECT_GENERAL_ERROR      "General map error"
#define MAP_PROBLEM_TURN_NOT_ALLOWED 			 "Turn not allowed"
#define MAP_PROBLEM_INCORRECT_JUNCTION			 "Incorrect junction"
#define MAP_PROBLEM_MISSING_BRIDGE_OVERPASS 	 "Missing bridge or overpass"
#define MAP_PROBLEM_WRONG_DRIVING_DIRECTIONS     "Wrong driving directions"
#define MAP_PROBLEM_MISSING_EXIT				 "Missing exit"
#define MAP_PROBLEM_MISSING_ROAD				 "Missing road"

#define MAX_MAP_PROBLEM_COUNT 15 // the maximum number of different map problems we might let the user report
//////////////////////////////////////////////////////////////////////////////////////////////////
//	Comments List
typedef struct RTAlertCommentsEntry_s
{
    struct RTAlertCommentsEntry_s *next;
    struct RTAlertCommentsEntry_s *previous;
    RTAlertComment comment;
} RTAlertCommentsEntry;

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Alert
typedef struct
{
    int iID; //	Alert ID (within the server)
    int iType; //  Alert Type
    int iSubType; //Alert Sub Type
    int iDirection; //  0 – node's direction, 1 – segment direction, 2 – opposite direction
    int iLongitude; //	Alert location:	Longtitue
    int iLatitude; //	Alert location:	Latitude
    int iAzymuth; //	Alert Azimuth
    int iSpeed; // Alowed speed to alert
    char sReportedBy [RT_ALERT_USERNM_MAXSIZE+1]; // The User who reported the alert
    int  iRank;
    int  iMood;
    int iReportTime; // The time the alert was reported
    int iNode1; // Alert's segment
    int iNode2; // Alert's segment
    char sDescription [RT_ALERT_DESCRIPTION_MAXSIZE+1]; // Alert's description
    char sLocationStr[RT_ALERT_LOCATION_MAX_SIZE+1]; //alert location
    char sImageIdStr[RT_ALERT_IMAGEID_MAXSIZE+1];
    char sVoiceIdStr[RT_ALERT_VOICEID_MAXSIZE+1];
    char sNearStr[RT_ALERT_LOCATION_MAX_SIZE+1];
    char sStreetStr[RT_ALERT_LOCATION_MAX_SIZE+1];
    char sCityStr[RT_ALERT_LOCATION_MAX_SIZE+1];
    int  iDistance;
    int  iLineId;
    int  iSquare;
    int  iNumComments;
    BOOL bAlertByMe;
    BOOL bPingWazer;
    roadmap_alerter_location_info location_info;
    RTAlertCommentsEntry *Comment;
    char sAddOnName[RT_ALERTS_MAX_ADD_ON_NAME];
    char sAlertType[RT_ALERTS_MAX_ALERT_TYPE];
    char *pMenuAddOnName;
    char *pMapAddOnName;
    char *pIconName;
    char *pMapIconName;
    char *pAlertTitle;
    BOOL bAlertIsOnRoute; // Is alert on my route
    char sFacebookName[RT_ALERT_FACEBOOK_USERNM_MAXSIZE]; // Facebook name
    BOOL bShowFacebookPicture;         // Show facebook Picture
    char sGroup[RT_ALERT_GROUP_MAXSIZE];
    char sGroupIcon[RT_ALERT_GROUP_ICON_MAXSIZE];
    int  iGroupRelevance;
    int  iReportedElapsedTime;
    int  iDisplayTimeStamp;
    BOOL bArchive;
    int  iPopUpPriority;
    BOOL bPopedUp;
    int  iNumThumbsUp;
    BOOL bThumbsUpByMe;
    BOOL bIsAlertable;
    BOOL bAlertHandled;
} RTAlert;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alert Res
typedef struct
{
   int  iPoints;
   char title[RT_ALERT_RES_TITLE_MAX_SIZE];
   char msg[RT_ALERT_RES_TEXT_MAX_SIZE];
} RTAlertRes;

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Runtime Alert Table
typedef struct
{
    RTAlert *alert[RT_MAXIMUM_ALERT_COUNT];
    int iCount;
    int iGroupCount;
    int iArchiveCount;
} RTAlerts;

void RTAlerts_Alert_Init(RTAlert *alert);
void RTAlerts_Clear_All(void);
void RTAlerts_Init(void);
void RTAlerts_Term(void);
BOOL RTAlerts_Add(RTAlert *alert);
BOOL RTAlerts_Remove(int iID);
void RTAlerts_RefreshOnMap(void);
int RTAlerts_Count(void);
int RTAlerts_GroupCount(void);
const char * RTAlerts_Count_Str(void);
const char * RTAlerts_GroupCount_Str(void);
RTAlert *RTAlerts_Get(int record);
RTAlert *RTAlerts_Get_By_ID(int iID);
const char* RTAlerts_Get_GroupName_By_Id(int iId);
BOOL RTAlerts_Is_Empty();
BOOL RTAlerts_Exists(int iID);
void RTAlerts_Get_Position(int alert, RoadMapPosition *position, int *steering);
int RTAlerts_Get_Type(int record);
int RTAlerts_Get_Type_By_Id(int iId);
int RTAlerts_Get_Id(int record);
char *RTAlerts_Get_LocationStr(int record);
char * RTAlerts_Get_LocationStrByID(int Id);
unsigned int RTAlerts_Get_Speed(int record);
int RTAlerts_Get_Distance(int record);
const char * RTAlerts_Get_Map_Icon(int alert);
const char * RTAlerts_Get_Icon(int alertId);
const char * RTAlerts_Get_Alert_Icon(int alertId);
const char * RTAlerts_Get_Warn_Icon(int alertId);
const char * RTAlerts_Get_String(int alertId);
const char * RTAlerts_Get_Additional_String(int alertId);
RoadMapSoundList RTAlerts_Get_Sound(int alertId);
int RTAlerts_Is_Alertable(int record);
BOOL RTAlerts_ShowDisrance(int AlertId);
BOOL RTAlerts_Is_On_Route(int AlertId);
void RTAlerts_Sort_List(alert_sort_method sort_method);
BOOL RTAlerts_Get_City_Street(RoadMapPosition AlertPosition,
        const char **city_name, const char **street_name,  int *square, int*line_id,
        int direction);
void RTAlerts_Popup_By_Id(int alertId, BOOL saveContext);
void RTAlerts_Popup_By_Id_Timed(int iID, BOOL saveContext, int timeOut);
void RTAlerts_Popup_By_Id_No_Center(int iID);
BOOL RTAlerts_Scroll_All(void);
void RTAlerts_Stop_Scrolling(void);
void RTAlerts_Scroll_Next(void);
void RTAlerts_Scroll_Prev(void);
void RTAlerts_Cancel_Scrolling(void);
BOOL RTAlerts_Zoomin_To_Aler(void);
int RTAlerts_State(void);
int RTAlerts_Get_Current_Alert_Id(void);
int RTAlerts_Penalty(int line_id, int against_dir);
void RTAlerts_Delete_All_Comments(RTAlert *alert);
const char* RTAlerts_Get_Image_Id( int iAlertId );
BOOL RTAlerts_Has_Image( int iAlertId );
BOOL RTAlerts_Has_Voice( int iAlertId );

void RTAlerts_Comment_Init(RTAlertComment *comment);
BOOL RTAlerts_Comment_Add(RTAlertComment *comment);
int RTAlerts_Get_Number_of_Comments(int iAlertId);

void RealtimeAlertsMenu(void);
int RTAlerts_Alert_near_position( RoadMapPosition position, int distance);

void add_real_time_alert_for_police(void);
void add_real_time_alert_for_accident(void);
void add_real_time_alert_for_traffic_jam(void);
void add_real_time_alert_for_hazard(void);
void add_real_time_alert_for_other(void);
void add_real_time_alert_for_parking(void);
void add_real_time_alert_for_construction(void);
void add_real_time_alert_for_police_my_direction(void);
void add_real_time_alert_for_accident_my_direction(void);
void add_real_time_alert_for_traffic_jam_my_direction(void);
void add_real_time_alert_for_hazard_my_direction(void);
void add_real_time_alert_for_parking_my_direction(void);
void add_real_time_alert_for_other_my_direction(void);
void add_real_time_alert_for_construction_my_direction(void);
void add_real_time_alert_for_police_opposite_direction(void);
void add_real_time_alert_for_accident_opposite_direction(void);
void add_real_time_alert_for_traffic_jam_opposite_direction(void);
void add_real_time_alert_for_hazard_opposite_direction(void);
void add_real_time_alert_for_parking_opposite_direction(void);
void add_real_time_alert_for_other_opposite_direction(void);
void add_real_time_alert_for_construction_opposite_direction(void);
void add_real_time_chit_chat(void);
void real_time_post_alert_comment(void);
void real_time_post_alert_comment_by_id(int iAlertId);
BOOL real_time_remove_alert(int iAlertId);
int RTAlerts_ScrollState(void);
int RTAlerts_Is_Cancelable(int alertId);
int Rtalerts_Delete(int alertId);
int RTAlerts_Check_Same_Street(int record);
int RTAlerts_HandleAlert(int alertId);
int RTAlertsOnAlerterStop(int alertId);
int RTAlertsOnAlerterStart(int alertId);
int RTAlerts_is_reply_popup_on(void);
int RTAlerts_CurrentAlert_Has_Comments(void);
void RTAlerts_CurrentComments(void);
const char * RTAlerts_Get_IconByType(RTAlert *pAlert, int iAlertType, BOOL has_comments);
int RTAlerts_Get_TypeByIconName(const char *icon_name);
const char *RTAlerts_get_title(RTAlert *pAlert,int iAlertType, int iAlertSubType);
int  RTAlerts_Get_Minimize_State(void);
void RTAlerts_Minimized_Alert_Dialog(void);
void RTAlerts_Reset_Minimized(void);
void RTAlerts_report_map_problem(void);
void RTAlerts_ShowProgressDlg(void);
void RTAlerts_CloseProgressDlg(void);
void RTAlerts_Download_Image( int alertId );
void RTAlerts_Download_Voice( int alertId );
void RTAlerts_add_comment_stars(SsdWidget container,  RTAlertComment *pAlertComment);
const char * RTAlerts_Get_Stars_Icon(int starNum);
void RTAlerts_update_stars(SsdWidget container,  RTAlert *Alert);
void RTAlerts_Set_Ignore_Max_Distance(BOOL ignore);
void RTAlerts_show_space_before_desc( SsdWidget containter, RTAlert *pAlert );
int RTAlertsGetMapProblems (int **outMapProblems, char **outMapProblemsOption[]);
BOOL RTAlerts_Can_Send_Thumbs_up(int alertId);
int  Rtalerts_Thumbs_Up(int alertId);
void RTAlerts_Update(int iID, int iNumThumbsUp, BOOL bIsOnRoute, BOOL bIsArchive);
void RTAlerts_ThumbsUpRecordInit(ThumbsUp *thumbsUp);
BOOL RTAlerts_ThumbsUpReceived(ThumbsUp *thumbsUp);

BOOL RTAlerts_isByMe(int iId);
BOOL RTAlerts_isAlertOnRoute(int iId);
const char * RTAlerts_Get_Map_AddOn(int alertId, int AddOnIndex);
int RTAlerts_Get_Number_Of_Map_AddOns(int alertId);
const char * RTAlerts_Get_AddOn(int alertId, int AddOnIndex);
int RTAlerts_Get_Number_Of_AddOns(int alertId);

char *RtAlerts_get_addional_text(RTAlert *pAlert);
char *RtAlerts_get_addional_text_icon(RTAlert *pAlert);
char *RtAlerts_get_addional_keyboard_text(RTAlert *pAlert);

void RTAlerts_get_reported_by_string( RTAlert *pAlert, char* buf, int buf_len );
void RTAlerts_get_report_info_str( RTAlert *pAlert, char* buf, int buf_len );
void RTAlerts_clear_group_counter(void);
int  RTAlerts_get_group_state(void);

void RTAlerts_start_glow(RTAlert *pAlert, int seconds);
void RTAlerts_stop_glow();

BOOL RTAlerts_ThumbsUpByMe(int iId);

int         RTAlerts_get_number_of_sub_types(int iAlertType);
const char* RTAlerts_get_subtype_label(int iAlertType, int iAlertSubType);
int         RTAlerts_get_default_subtype(int iAlertType);
char*       RTAlerts_get_subtype_icon(int iAlertType, int iAlertSubType);
int         RTAlerts_get_num_categories(int iAlertType, int iAlertSubType);
int         RTAlerts_get_categories_subtype(int iAlertType, int iAlertSubType, int index);

void RTAlerts_update_location_str(RTAlert *pAlert);
#endif	//	__REALTIME_ALERT_H__
