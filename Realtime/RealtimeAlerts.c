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
#include "../roadmap_twitter.h"
#include "../editor/db/editor_street.h"
#include "roadmap_camera_image.h"
#include "roadmap_jpeg.h"

#include "../navigate/navigate_main.h"

#include "RealtimeAlerts.h"
#include "RealtimeAlertsList.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeAlertCommentsList.h"
#include "RealtimeNet.h"
#include "Realtime.h"

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_progress_msg_dialog.h"

#ifdef IPHONE
#include "iphone/roadmap_list_menu.h"
#include "iphone/roadmap_editbox.h"
#endif //IPHONE

static RTAlerts gAlertsTable;
static int gIterator;
static int gIdleCount;
static int gIdleScrolling;
static int gCurrentAlertId;
static char gCurrentImageId[ROADMAP_IMAGE_ID_BUF_LEN] = "";
static char *gCurrentImagePath;
static BOOL gTimerActive;
static BOOL gPopAllTimerActive;
static RTAlertCommentsEntry *gCurrentComment;
static BOOL gCentered;
static int gState =STATE_NONE;
static int gSavedZoom = -1;
static RoadMapPosition gSavedPosition;
static const char *gSavedFocus = NULL;
static int gMinimizeState = -1;
static int gPopUpType = -1;

static int gMapProblems[MAX_MAP_PROBLEM_COUNT] ; // will hold the list of map problems
static int gMapProblemsCount = 0 ; // will hold the number of map problems
static BOOL gMapProblemsInit = FALSE;
static BOOL gIgnoreAlertMaxDist = TRUE;

#define POP_UP_COMMENT 1
#define POP_UP_ALERT   2

static RoadMapConfigDescriptor LastCommentAlertIDCfg =
                        ROADMAP_CONFIG_ITEM("Alerts", "Last comment alert ID");

static RoadMapConfigDescriptor LastCommentIDCfg =
                        ROADMAP_CONFIG_ITEM("Alerts", "Last comment ID");

static RoadMapConfigDescriptor RoadMapConfigMapProblemOptions =
                        ROADMAP_CONFIG_ITEM("General", "Map Problem Options(new)");


static RoadMapConfigDescriptor RoadMapConfigMaxAlertPopDist =
                        ROADMAP_CONFIG_ITEM("Alerts", "Max Dist");


static  char*  MAP_PROBLEMS_OPTION[] = { MAP_PROBLEM_INCORRECT_TURN,   MAP_PROBLEM_INCORRECT_ADDRESS , MAP_PROBLEM_INCORRECT_ROUTE ,
											MAP_PROBLEM_INCORRECT_MISSING_ROUNDABOUT , MAP_PROBLEM_INCORRECT_GENERAL_ERROR,MAP_PROBLEM_TURN_NOT_ALLOWED,MAP_PROBLEM_INCORRECT_JUNCTION,MAP_PROBLEM_MISSING_BRIDGE_OVERPASS,
												MAP_PROBLEM_WRONG_DRIVING_DIRECTIONS , MAP_PROBLEM_MISSING_EXIT,MAP_PROBLEM_MISSING_ROAD };
static void RTAlerts_Timer(void);
static void RTAlerts_ReleaseImagePath(void);
static void RTAlerts_ImageUpload(void);
static void OnAlertAdd(RTAlert *pAlert);
static void OnAlertRemove(void);
static void create_ssd_dialog(RTAlert *pAlert);
static void OnAlertReRoute(RTAlert *pAlert);
static void RTAlerts_Comment_PopUp(RTAlertComment *Comment, RTAlert *Alert);
static void RTAlerts_Comment_PopUp_Hide(void);
static void RTAlerts_popup_alert(int alertId, int iCenterAround);
static void RTAlerts_ShowProgressDlg(void);
static void RTAlerts_Show_Image( int alertId, RoadMapImage image );
static void RTinitMapProblems(void);
static void RTAlerts_get_reported_by_string( RTAlert *pAlert, char* buf, int buf_len );
#define CENTER_NONE				-1
#define CENTER_ON_ALERT       1
#define CENTER_ON_ME 		   2

#ifdef TOUCH_SCREEN
static void report_dialog(int iAlertType);
#endif

#ifndef TOUCH_SCREEN
// Context menu:
typedef enum real_time_context_menu_items
{
   rt_cm_add_comments,
   rt_cm_view_comments,
   rt_cm_view_image,
   rt_cm_cancel,
   rt_cm__count,
   rt_cm__invalid
}  real_time_context_menu_items;

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Add comment",       rt_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "View comments",     rt_cm_view_comments),
   SSD_CM_INIT_ITEM  ( "View image",        rt_cm_view_image),
   SSD_CM_INIT_ITEM  ( "Cancel",            rt_cm_cancel),
};
static   BOOL g_context_menu_is_active= FALSE;
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

#endif

typedef struct
{
	int alertId;
} ImageDownloadCbContext;

// Builds the icon name according to the presence of the image attachmet
#define RTALERT_MAPICON_ATTACH( dest_buf, pAlert, name ) \
{	\
	strcpy( dest_buf, name );	\
	if ( pAlert->sImageIdStr[0] ) \
		strcat( dest_buf, "_attach" );	\
}


roadmap_alert_providor RoadmapRealTimeAlertProvidor =
{ "RealTimeAlerts", RTAlerts_Count, RTAlerts_Get_Id, RTAlerts_Get_Position,
        RTAlerts_Get_Speed, RTAlerts_Get_Map_Icon, RTAlerts_Get_Alert_Icon,
        RTAlerts_Get_Warn_Icon, RTAlerts_Get_Distance, RTAlerts_Get_Sound,
        RTAlerts_Is_Alertable, RTAlerts_Get_String, RTAlerts_Is_Cancelable, Rtalerts_Delete, RTAlerts_Check_Same_Street, RTAlerts_HandleAlert };

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
    pAlert->iSubType = 0;
    pAlert->iRank = 0;
    pAlert->iMood = 0;
    pAlert->iReportTime = 0L;
    pAlert->iNode1 = 0;
    pAlert->iNode2 = 0;
    pAlert->iDistance = 0;
    pAlert->bAlertByMe = FALSE;
    memset(pAlert->sReportedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(pAlert->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
    memset(pAlert->sLocationStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sNearStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sImageIdStr, 0, RT_ALERT_IMAGEID_MAXSIZE);
    memset(pAlert->sStreetStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    memset(pAlert->sCityStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    pAlert->iNumComments = 0;
    pAlert->Comment = NULL;

}

/**
 * Initialize the Realtime alerts
 * @param None
 * @return None
 */
void RTAlerts_Init()
{
    int i;

    for (i=0; i<RT_MAXIMUM_ALERT_COUNT; i++)
        gAlertsTable.alert[i] = NULL;

    gAlertsTable.iCount = 0;
    roadmap_alerter_register(&RoadmapRealTimeAlertProvidor);
    gIdleScrolling = FALSE;
    gIterator = 0;
    gCurrentAlertId = -1;
    gTimerActive = FALSE;
    gPopAllTimerActive = FALSE;

    gSavedZoom = -1;
    gSavedFocus = NULL;
    gSavedPosition.latitude = -1;
    gSavedPosition.longitude = -1;

    roadmap_config_declare ("session", &LastCommentAlertIDCfg, "-1", NULL);

    roadmap_config_declare ("session", &LastCommentIDCfg, "-1", NULL);

    roadmap_config_declare ("preferences", &RoadMapConfigMaxAlertPopDist, "30000", NULL);



 	 RTinitMapProblems();
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
 * The STRING of alerts currently in the table
 * @param None
 * @return string with number of alerts
 */
const char * RTAlerts_Count_Str(void)
{
	static char text[20];
	snprintf(text, sizeof(text), "%d",gAlertsTable.iCount);
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
        RTAlerts_Delete_All_Comments(pAlert);
        free(pAlert);
        gAlertsTable.alert[i] = NULL;
    }

    OnAlertRemove();

    gAlertsTable.iCount = 0;

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
 * @return STATE_OLD, STATE_NEW, STATE_SCROLLING, STATE_NEW_COMMENT
 */
int RTAlerts_State()
{
    if (RTAlerts_Is_Empty())
        return STATE_NONE;
    else
        return gState;
}

/**
 * Returns the state of the Realtime Alerts
 * @param None
 * @return STATE_OLD, STATE_NEW, STATE_SCROLLING, STATE_NEW_COMMENT
 */
int RTAlerts_ScrollState()
{
    int sign_active = (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg")));

   if (sign_active)
        return 0;
   else
	   return -1;
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height) {
   SsdWidget space;
#ifdef HI_RES_SCREEN
   height *= 10;
#endif
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
    const char *street;
    const char *city;
    int iLineId = -1;
    int line_from_point;
    int line_to_point;
    int line_azymuth;
    int delta;
    int			iSquare;


    // Full?
    if ( RT_MAXIMUM_ALERT_COUNT == gAlertsTable.iCount)
        return FALSE;

    // Already exists?
    if (RTAlerts_Exists(pAlert->iID))
    {
        roadmap_log( ROADMAP_INFO, "RTAlerts_Add - cannot add Alert  (%d) alert already exist", pAlert->iID);
        return TRUE;
    }

    if (pAlert->iType > RT_ALERTS_LAST_KNOWN_STATE)
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
    gAlertsTable.alert[gAlertsTable.iCount]->iLatitude = pAlert->iLatitude;
    gAlertsTable.alert[gAlertsTable.iCount]->iLongitude = pAlert->iLongitude;
    gAlertsTable.alert[gAlertsTable.iCount]->iID = pAlert->iID;
    gAlertsTable.alert[gAlertsTable.iCount]->iType = pAlert->iType;
    gAlertsTable.alert[gAlertsTable.iCount]->iSubType = pAlert->iSubType;
    gAlertsTable.alert[gAlertsTable.iCount]->iSpeed = pAlert->iSpeed;
    gAlertsTable.alert[gAlertsTable.iCount]->iReportTime
            = pAlert->iReportTime;
    gAlertsTable.alert[gAlertsTable.iCount]->bAlertByMe = pAlert->bAlertByMe;
    gAlertsTable.alert[gAlertsTable.iCount]->iRank = pAlert->iRank;
    gAlertsTable.alert[gAlertsTable.iCount]->iMood = pAlert->iMood;

    strncpy(gAlertsTable.alert[gAlertsTable.iCount]->sDescription,
            pAlert->sDescription, RT_ALERT_DESCRIPTION_MAXSIZE);
    strncpy(gAlertsTable.alert[gAlertsTable.iCount]->sReportedBy,
            roadmap_lang_get(pAlert->sReportedBy), RT_ALERT_USERNM_MAXSIZE);

    position.longitude = pAlert->iLongitude;
    position.latitude = pAlert->iLatitude;


    if (pAlert->sLocationStr[0] == 0){


       // Save Location Description string
       if (!RTAlerts_Get_City_Street(position, &city, &street, &iSquare, &iLineId, pAlert->iDirection))
       {
         city = pAlert->sCityStr;
         street = pAlert->sStreetStr;
       }

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
    else{
    	strcpy(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr, pAlert->sLocationStr);
    }

    if (pAlert->sNearStr[0] != 0){
    	sprintf (gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr), " %s %s", 			roadmap_lang_get("Near"), pAlert->sNearStr);
    }

    if ( pAlert->sImageIdStr[0] != 0 )
    {
        strncpy( gAlertsTable.alert[gAlertsTable.iCount]->sImageIdStr, pAlert->sImageIdStr, RT_ALERT_IMAGEID_MAXSIZE+1 );
    }

    gAlertsTable.alert[gAlertsTable.iCount]->iSquare = iSquare;
    gAlertsTable.alert[gAlertsTable.iCount]->iLineId = iLineId;
    gAlertsTable.alert[gAlertsTable.iCount]->iDirection = pAlert->iDirection;

    if (pAlert->iDirection == RT_ALERT_OPPSOITE_DIRECTION)
    {
        iDirection = pAlert->iAzymuth + 180;
        while (iDirection > 360)
            iDirection -= 360;
        gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth = iDirection;
    }
    else
    {
        gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth = pAlert->iAzymuth;
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
    memset(comment->sPostedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(comment->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
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

    CommentEntry = calloc(1, sizeof(RTAlertCommentsEntry));
    CommentEntry->comment.iID = Comment->iID;
    CommentEntry->comment.iAlertId = Comment->iAlertId;
    CommentEntry->comment.i64ReportTime = Comment->i64ReportTime;
    CommentEntry->comment.bCommentByMe = Comment->bCommentByMe;
    CommentEntry->comment.iMood = Comment->iMood;
    CommentEntry->comment.iRank = Comment->iRank;
    strncpy(CommentEntry->comment.sDescription, Comment->sDescription,
    			RT_ALERT_DESCRIPTION_MAXSIZE);
    strncpy(CommentEntry->comment.sPostedBy, roadmap_lang_get(Comment->sPostedBy),
    			RT_ALERT_USERNM_MAXSIZE);

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

    if (!Comment->bCommentByMe){
        if (pAlert->bAlertByMe || bCommentedByMe){
            gState = STATE_NEW_COMMENT;
            RTAlerts_Comment_PopUp(Comment,pAlert);
        }
        else
            gState = STATE_NEW;
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
        RTAlerts_Delete_All_Comments(gAlertsTable.alert[gAlertsTable.iCount-1]);
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
                    RTAlerts_Delete_All_Comments(gAlertsTable.alert[i]);
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
 * Get the speed of an Alert
 * @param record - The Record number in the table
 * @return The speed of the Alert, -1 if the record does not exist
 */
unsigned int RTAlerts_Get_Speed(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->iSpeed;
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
const char * RTAlerts_Get_IconByType(int iAlertType, BOOL has_comments)
{

    switch (iAlertType)
    {

   case RT_ALERT_TYPE_POLICE:
        if (!has_comments)
            return "real_time_police_icon";
        else
            return "real_time_police_with_comments";
        break;

    case RT_ALERT_TYPE_ACCIDENT:
        if (!has_comments)
            return "real_time_accident_icon";
        else
            return "real_time_accident_with_comments";
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
        if (!has_comments)
            return "real_time_chit_chat_icon";
        else
            return "real_time_chit_chat_with_comments";
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
        if (!has_comments)
            return "real_time_traffic_jam_icon";
        else
            return "real_time_trafficjam_with_comments";
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
        if (!has_comments)
            return "real_time_traffic_info_icon";
        else
            return "real_time_traffic_info_icon";
        break;

    case RT_ALERT_TYPE_HAZARD:
        if (!has_comments)
            return "real_time_hazard_icon";
        else
            return "real_time_hazard_icon_with_comments";
        break;

    case RT_ALERT_TYPE_OTHER:
        if (!has_comments)
            return "real_time_other_icon";
        else
            return "real_time_other_icon_with_comments";
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
        if (!has_comments)
            return "road_construction";
        else
            return "road_construction_with_comments";
        break;

    case RT_ALERT_TYPE_PARKING:
        if (!has_comments)
            return "parking_user";
        else
            return "parking_with_comments";
        break;

    default:
        return "real_time_chit_chat_icon";
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

		if (!strcmp(icon_name, "real_time_police_icon") || !strcmp(icon_name, "real_time_police_with_comments"))
            return RT_ALERT_TYPE_POLICE;

		if (!strcmp(icon_name, "real_time_accident_icon") || !strcmp(icon_name, "real_time_accident_with_comments"))
            return RT_ALERT_TYPE_ACCIDENT;

		if (!strcmp(icon_name, "real_time_chit_chat_icon") || !strcmp(icon_name, "real_time_chit_chat_with_comments"))
            return RT_ALERT_TYPE_CHIT_CHAT;

		if (!strcmp(icon_name, "real_time_traffic_jam_icon") || !strcmp(icon_name, "real_time_trafficjam_with_comments"))
            return RT_ALERT_TYPE_TRAFFIC_JAM;

		if (!strcmp(icon_name, "real_time_traffic_info_icon") )
            return RT_ALERT_TYPE_TRAFFIC_INFO;

		if (!strcmp(icon_name, "real_time_hazard_icon") || !strcmp(icon_name, "real_time_hazard_icon_with_comments"))
            return RT_ALERT_TYPE_HAZARD;

		if (!strcmp(icon_name, "real_time_other_icon") || !strcmp(icon_name, "real_time_other_icon_with_comments"))
            return RT_ALERT_TYPE_OTHER;

		if (!strcmp(icon_name, "road_construction") || !strcmp(icon_name, "road_construction_with_comments"))
            return RT_ALERT_TYPE_CONSTRUCTION;

      if (!strcmp(icon_name, "parking_user") || !strcmp(icon_name, "parking_with_comments"))
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

	return RTAlerts_Get_IconByType(pAlert->iType, (pAlert->iNumComments != 0));
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
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "police_on_map" );
    	break;

    case RT_ALERT_TYPE_ACCIDENT:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "accident_on_map" );
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "chit_chat_on_map" );
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "loads_on_map" );
        break;

    case RT_ALERT_TYPE_HAZARD:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "hazard_on_map" );
    	break;

    case RT_ALERT_TYPE_PARKING:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "parking_on_map" );
        break;

    case RT_ALERT_TYPE_OTHER:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "other_on_map" );
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
    	RTALERT_MAPICON_ATTACH( icon_name, pAlert, "road_construction_on_map" );
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
        return "real_time_police_alert";
    case RT_ALERT_TYPE_ACCIDENT:
        return "real_time_accident_alert";
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
        return "real_time_police_alert";
    case RT_ALERT_TYPE_ACCIDENT:
        return "real_time_accident_alert";
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

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "Police trap";
    case RT_ALERT_TYPE_ACCIDENT:
        return "Accident";
    default:
        return NULL;
    }
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
    case RT_ALERT_TYPE_ACCIDENT:
        return 2000;
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

    RoadMapSoundList sound_list;
    RTAlert *pAlert;

    sound_list = roadmap_sound_list_create(0);

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return sound_list;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        roadmap_sound_list_add(sound_list, "Police trap");
        break;
    case RT_ALERT_TYPE_ACCIDENT:
        roadmap_sound_list_add(sound_list, "Accident");
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

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return TRUE;
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
    default:
        return FALSE;
    }
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

    default:
        return FALSE;
    }
}

/**
 * Display a popup warning that an alert is on the rout
 * @param pAlert - pointer to the alert
 * @return None
 */
static void OnAlertReRoute(RTAlert *pAlert)
{
    RoadMapSoundList sound_list;

    sound_list = roadmap_sound_list_create(0);
    roadmap_sound_list_add(sound_list, "alert_1");
    roadmap_sound_play_list(sound_list);

    if (!ssd_dialog_activate("rt_alert_on_my_route", NULL))
    {
        create_ssd_dialog(pAlert);
        ssd_dialog_activate("rt_alert_on_my_route", NULL);
    }
    RTAlerts_Popup_By_Id(pAlert->iID);
    ssd_dialog_draw();

}

/**
 * Action to be taken after an alert was added
 * @param pAlert - pointer to the alert
 * @return None
 */
static void OnAlertAdd(RTAlert *pAlert)
{

    if ((gAlertsTable.iCount == 1) && (gTimerActive == FALSE))
    {
        gIdleCount = 0;
        roadmap_main_set_periodic(1000, RTAlerts_Timer);
        gTimerActive = TRUE;
    }

    if (RTAlerts_Is_Reroutable(pAlert) && (!pAlert->bAlertByMe))
    {
        if (navigate_track_enabled())
        {
            if (navigate_is_line_on_route(pAlert->iSquare, pAlert->iLineId, pAlert->iNode1,
                    pAlert->iNode2))
            {
                int azymuth, delta;
                PluginLine line;
                RoadMapPosition alert_pos;
                RoadMapGpsPosition gps_map_pos;
                int steering;
                int distance;

				// check that the alert is within alert distance
                alert_pos.latitude = pAlert->iLatitude;
                alert_pos.longitude = pAlert->iLongitude;

                if (roadmap_navigate_get_current(&gps_map_pos, &line, &steering)
                        != -1)
                {
                    RoadMapPosition gps_pos;
                    gps_pos.latitude = gps_map_pos.altitude;
                    gps_pos.longitude = gps_map_pos.longitude;

					//
	                distance = roadmap_math_distance(&gps_pos, &alert_pos);
					if (distance > REROUTE_MIN_DISTANCE) {
						return;
					}

                    //check that we didnt pass the new alert on the route.
                    azymuth = roadmap_math_azymuth(&gps_pos, &alert_pos);
                    delta = azymuth_delta(azymuth, steering);
                    if (delta < 90 && delta >(-90))
                    {
                        OnAlertReRoute(pAlert);
                    }
                }
                else
                {
                    OnAlertReRoute(pAlert);
                }
            }
        }
    }
}

/**
 * Action to be taken after an alert was removed
 * @param None
 * @return None
 */
static void OnAlertRemove(void)
{
    if ((gAlertsTable.iCount == 0) && gTimerActive)
    {
        roadmap_main_remove_periodic(RTAlerts_Timer) ;
        gTimerActive = FALSE;
    }
}

/**
 * RealTime alerts timer. Checks whether we are standing for few seconds and display popup
 * @param None
 * @return None
 */
static void RTAlerts_Timer(void)
{

    RoadMapGpsPosition pos;
    const char *focus = roadmap_trip_get_focus_name();
    BOOL gps_active;
    int gps_state;


    gps_state = roadmap_gps_reception_state();
    gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);


    roadmap_navigate_get_current(&pos, NULL, NULL);

    if (pos.speed < 0){
       if (gIdleScrolling){
          RTAlerts_Stop_Scrolling();
          RealTime_SetMapDisplayed(TRUE);
          gIdleScrolling = FALSE;
       }
       gIdleCount = 0;
       return;
    }

    if (gIdleScrolling && pos.speed > 5)
    {
        gIdleCount = 0;
        RTAlerts_Stop_Scrolling();
        RealTime_SetMapDisplayed(TRUE);
        gIdleScrolling = FALSE;
        return;
    }

    if ((pos.speed < 2) && (gps_active))
        gIdleCount++;
    else
    {
        gIdleCount = 0;
        if (gIdleScrolling)
        {
            RTAlerts_Stop_Scrolling();
            RealTime_SetMapDisplayed(TRUE);
            gIdleScrolling = FALSE;
        }
    }

    if (!focus){
       gIdleCount = 0;
       return;
    }

    if (strcmp(focus, "GPS"))
    {
       gIdleCount = 0;
        return;
    }

	if ((ssd_dialog_is_currently_active() && (strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg")))){
		//Other dialog is active
		return;
	}

    //focus && !strcmp (focus, "GPS") &&
    if ((gIdleCount == 30) && (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg")))))
    {
      roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
      RTAlerts_Set_Ignore_Max_Distance(FALSE);
      RTAlerts_Scroll_All();
      RealTime_SetMapDisplayed(FALSE);
      gIdleScrolling = TRUE;
    }

    if (gIdleCount > 30)
        gIdleCount = 0;

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
    int scale;

    if (iCenterAround == CENTER_ON_ALERT)
    {
    	  roadmap_trip_set_point("Hold", &AlertPosition);
        roadmap_screen_update_center(&AlertPosition);
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
				 scale = 5000;
		    }
		    roadmap_screen_update_center(&pos);
	    } else {
   		 scale = 5000;
          roadmap_screen_update_center(&AlertPosition);
		 }
	 }

    roadmap_math_set_scale(scale, roadmap_screen_height() / 3);
    roadmap_layer_adjust();
    roadmap_screen_set_orientation_fixed();
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
    int context_save_zoom;
    int line_directions;
    const char *street_name2;
    int count = 0;

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
    }

#ifndef J2ME
    if (sort_method == sort_proximity)
    	qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_proximity);
    else
    	qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_recency);
#endif
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

   if (RTAlerts_Get_Type_By_Id(gCurrentAlertId) == RT_ALERT_TYPE_TRAFFIC_INFO)
         is_auto_jam = TRUE;

   if (RTAlerts_Get_Number_of_Comments(gCurrentAlertId) != 0)
      has_comments = TRUE;

   has_image = RTAlerts_Has_Image( gCurrentAlertId );

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
                           0);

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

   if (has_image || has_comments||is_auto_jam){
      ssd_widget_set_left_softkey_callback(dlg, on_options);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Options"));
   }
   else{
      ssd_widget_set_left_softkey_callback(dlg, real_time_post_alert_comment_sk_cb);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Comment"));
   }


}

static int real_time_hide_sk_cb(SsdWidget widget, const char *new_value, void *context){
	RTAlerts_Cancel_Scrolling();
	return 0;
}

static void set_right_softkey(SsdWidget dlg){
	ssd_widget_set_right_softkey_callback(dlg, real_time_hide_sk_cb);
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
    	RTAlerts_Sort_List(sort_proximity);
    	gIterator = 0; // return to begining
    }

    // if idle scrolling, and alert is bigger than maximal distance,
    // then return to the beginning ( closest alert ).
    if ((pAlert->iDistance > roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist))&&
    	(!gIgnoreAlertMaxDist))
    {
    	gIterator = 0;
    }
    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;
	
	if (!gIgnoreAlertMaxDist){
		roadmap_log(ROADMAP_DEBUG, "RTAlerts_Popup: popping up report automatically, report distance is %d", 
		pAlert->iDistance);
		if (pAlert->iDistance > roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist))
			roadmap_log( ROADMAP_ERROR, 
			"RTAlerts_Popup: popping up report automatically, report distance is %d is bigger than maxDist %d", 
			pAlert->iDistance,roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist));
	}
	
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);

}

const char *RTAlerts_get_title(int iAlertType, int iAlertSubType){

    if (iAlertType == RT_ALERT_TYPE_ACCIDENT)
        return roadmap_lang_get("Accident");
    else if (iAlertType == RT_ALERT_TYPE_POLICE)
        return roadmap_lang_get("Police trap");
    else if (iAlertType == RT_ALERT_TYPE_TRAFFIC_JAM)
		return roadmap_lang_get("Traffic jam");
    else if (iAlertType == RT_ALERT_TYPE_TRAFFIC_INFO){
       if (iAlertSubType == LIGHT_TRAFFIC)
          return roadmap_lang_get("Light traffic");
       else if (iAlertSubType == MODERATE_TRAFFIC)
          return roadmap_lang_get("Moderate traffic");
       else if (iAlertSubType == HEAVY_TRAFFIC)
          return roadmap_lang_get("Heavy traffic");
       else if (iAlertSubType == STAND_STILL_TRAFFIC)
          return roadmap_lang_get("Complete standstill");
       else
          return roadmap_lang_get("Traffic");
    }
    else if (iAlertType == RT_ALERT_TYPE_HAZARD)
		return roadmap_lang_get("Hazard");
    else if (iAlertType == RT_ALERT_TYPE_OTHER)
		return roadmap_lang_get("Other");
    else if (iAlertType == RT_ALERT_TYPE_CONSTRUCTION)
		return roadmap_lang_get("Road construction");
    else if (iAlertType == RT_ALERT_TYPE_PARKING)
      return roadmap_lang_get("Parking");
    else
		return roadmap_lang_get("Chit chat");
}

static void on_popup_close(int exit_code, void* context){

   if (gSavedZoom == -1){
       roadmap_trip_set_focus("GPS");
       roadmap_math_zoom_reset();
   }else{
   	  roadmap_math_set_context(&gSavedPosition, gSavedZoom);
   	  gSavedPosition.latitude = -1;
   	  gSavedPosition.longitude = -1;
   	  gSavedZoom = -1;
   }

   if (gSavedFocus)
      roadmap_trip_set_focus(gSavedFocus);
   gSavedFocus = NULL;

   if (exit_code == dec_close)
   {
	  if (gIdleScrolling){
    	 roadmap_main_remove_periodic(RTAlerts_Timer);
    	 gTimerActive = FALSE;
		 RealTime_SetMapDisplayed(TRUE);
         gIdleScrolling = FALSE;
	  }

	  if (gPopAllTimerActive){
			roadmap_main_remove_periodic(RTAlerts_Popup) ;
			gPopAllTimerActive = FALSE;
		}
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

static int on_add_comment (SsdWidget widget, const char *new_value){
	gPopUpType = POP_UP_ALERT;
    real_time_post_alert_comment_by_id(gCurrentAlertId);
	return 0;
}

static int on_view_comments (SsdWidget widget, const char *new_value){
	RTAlerts_CurrentComments();
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
static int on_view_image( SsdWidget widget, const char *new_value )
{
   RTAlerts_Download_Image( gCurrentAlertId );
   return 0;
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
static void RTAlerts_get_report_info_str( RTAlert *pAlert, char* buf, int buf_len )
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

  	snprintf( buf, buf_len, tmpStr );


    if (timeDiff < 60)
        snprintf( buf  + strlen( buf ), buf_len - strlen( buf ),
                roadmap_lang_get("%d seconds ago"), timeDiff );
    else if ((timeDiff > 60) && (timeDiff < 3600))
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

static void update_popup(SsdWidget popup, RTAlert *pAlert, int iCenterAround){
   char AlertStr[700];
   char ReportedByStr[100];
   SsdWidget button;
   RoadMapPosition position;

   ssd_widget_reset_cache(popup);
   // Set the title
   ssd_widget_set_value( popup, "popuup_text", RTAlerts_get_title(pAlert->iType, pAlert->iSubType) );

   ssd_bitmap_update(ssd_widget_get(popup, "alert_icon"), RTAlerts_Get_Icon(pAlert->iID));

   ssd_widget_set_value(popup, "alert_location", pAlert->sLocationStr);

   RTAlerts_get_report_info_str( pAlert, AlertStr, sizeof( AlertStr ) );
   ssd_widget_set_value(popup, "alert_info", AlertStr);
   ReportedByStr[0] = 0;
   RTAlerts_get_reported_by_string(pAlert,ReportedByStr,sizeof(ReportedByStr));
   ssd_widget_set_value(popup, "reported_by_info", ReportedByStr);
   RTAlerts_update_stars(ssd_widget_get(popup,"position_container"),pAlert);
   RTAlerts_show_space_before_desc(popup,pAlert);
   AlertStr[0] = 0;
   RTAlerts_get_report_distance_str( pAlert, AlertStr, sizeof( AlertStr ) );
   ssd_widget_set_value(popup, "alert_distance", AlertStr);
   if (pAlert->sDescription[0] != 0){
      ssd_widget_set_value(popup, "descripion_text", pAlert->sDescription);
   }
   else{
      ssd_widget_set_value(popup, "descripion_text", "");
   }

#ifdef TOUCH_SCREEN
   button = ssd_widget_get(popup,"Comment_button");
   if (button){
      if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) || (pAlert->iNumComments != 0))
         ssd_widget_hide(button);
      else
         ssd_widget_show(button);
   }
   button = ssd_widget_get( popup, "ViewImage_button");
   // Attachment image
   if (button){
      if ( !pAlert->sImageIdStr[0] )
         ssd_widget_hide(button);
      else
         ssd_widget_show(button);
   }
   button = ssd_widget_get(popup, "ViewComments_button");
   if (button){
      if ( pAlert->iNumComments == 0)
         ssd_widget_hide(button);
      else
         ssd_widget_show(button);
   }

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
    static SsdWidget popup;
    SsdWidget text_con;
    SsdWidget spacer;
    SsdWidget image_con;
    char *icon[3];
    SsdSize size;
    SsdWidget bitmap;
    int width = roadmap_canvas_width() ;
    char ReportedByStr[100];
#ifdef TOUCH_SCREEN
    SsdWidget button;
#endif
    AlertStr[0] = 0;

    if (width > roadmap_canvas_height())
#ifdef IPHONE
       width = 320;
#else
      width = roadmap_canvas_height();
#endif // IPHONE
   
    focus = roadmap_trip_get_focus_name();

    if (gAlertsTable.iCount == 0)
        return;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return;


   position.longitude = pAlert->iLongitude;
   position.latitude = pAlert->iLatitude;
   AlertStr[0] =0;

   if (iCenterAround != CENTER_NONE)
     RTAlerts_zoom(position, iCenterAround);

   if (popup){
      gCurrentAlertId = alertId;
      update_popup(popup, pAlert, iCenterAround);
      return;
   }

	bitmap = ssd_bitmap_new("alert_icon", RTAlerts_Get_Icon(pAlert->iID),SSD_ALIGN_CENTER|SSD_WS_TABSTOP);

	ssd_widget_get_size(bitmap, &size, NULL);
    position_con =
      ssd_container_new ("position_container", "", width-size.width - 40, SSD_MIN_SIZE,0);
    ssd_widget_set_color(position_con, NULL, NULL);

    text_con =
      ssd_container_new ("text_conatiner", "", SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
    ssd_widget_set_color(text_con, NULL, NULL);

    text = ssd_text_new("popuup_text", RTAlerts_get_title(pAlert->iType, pAlert->iSubType), 18, SSD_END_ROW);
    ssd_widget_set_color(text,"#f6a201", NULL);
    ssd_widget_add(position_con, text);
    
    // Display when the alert street name and city
	text = ssd_text_new("alert_location", pAlert->sLocationStr, 16, SSD_END_ROW);
	ssd_widget_set_color(text,"#f6a201", NULL);
	ssd_widget_add(position_con, text);
	AlertStr[0] = 0;

	RTAlerts_get_report_info_str( pAlert, AlertStr, sizeof( AlertStr ) );

    text = ssd_text_new("alert_info", AlertStr, 14,SSD_END_ROW);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(position_con, text);
    ReportedByStr[0] = 0;
    RTAlerts_get_reported_by_string(pAlert,ReportedByStr,sizeof(ReportedByStr));
    text = ssd_text_new("reported_by_info", ReportedByStr, 14,0);
    ssd_widget_set_color(text,"#ffffff", NULL);
    ssd_widget_add(position_con, text);
	RTAlerts_update_stars(position_con, pAlert);
	ssd_widget_add (position_con,
      ssd_container_new ("space_before_descrip", NULL, 0, 10, SSD_END_ROW));
    RTAlerts_show_space_before_desc(position_con,pAlert);
	AlertStr[0] = 0;


    if (pAlert->sDescription[0] != 0){
	    //Display the alert description
    	snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
        	    "%s", pAlert->sDescription);
    	text = ssd_text_new("descripion_text", AlertStr, 14,0);
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


    gCurrentAlertId = alertId;
    gCurrentComment = pAlert->Comment;


    popup = ssd_popup_new("AlertPopUPDlg", "", on_popup_close, SSD_MAX_SIZE, SSD_MIN_SIZE,&position, SSD_ROUNDED_BLACK|SSD_POINTER_LOCATION);
    /* Makes it possible to click in the bottom vicinity of the buttons  */
    ssd_widget_set_click_offsets_ext( popup, 0, 0, 0, 15 );
    
    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( popup, spacer );
    
    image_con =
      ssd_container_new ("IMAGE_container", "", 40, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
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
#ifdef TOUCH_SCREEN

    spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
    ssd_widget_set_color( spacer, NULL, NULL );
    ssd_widget_add( popup, spacer );

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    ssd_widget_add(popup, button);

    button = ssd_button_label("Comment_button", roadmap_lang_get("Comment"), SSD_ALIGN_CENTER, on_add_comment);
    ssd_widget_add(popup, button);

    if ((pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO) || (pAlert->iNumComments != 0)){
       ssd_widget_hide(button);
    }

    button = ssd_button_label("ViewComments_button", roadmap_lang_get("View comments"), SSD_ALIGN_CENTER, on_view_comments);
    ssd_widget_add(popup, button);

    if ( pAlert->iNumComments == 0){
       ssd_widget_hide(button);
       
    }


    icon[0] = "button_image_attachment_up";
    icon[1] = "button_image_attachment_down";
    icon[2] = NULL;
    button = ssd_button_new( "ViewImage_button", "View image", (const char**) &icon[0], 2, SSD_ALIGN_CENTER, on_view_image );
    ssd_widget_add(popup, button);

    // Attachment image
    if ( !pAlert->sImageIdStr[0] )
    {
       ssd_widget_hide(button);
    }

#else
    set_left_softkey(popup->parent, pAlert);
    set_right_softkey(popup->parent);
#endif

   ssd_dialog_activate("AlertPopUPDlg", NULL);
   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }

}


/**
 * Display the pop up of a specific alert
 * @param IId - the ID of the alert to popup
 * @return None
 */
void RTAlerts_Popup_By_Id(int iID)
{
    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {

       const char *focus = roadmap_trip_get_focus_name();
       if (focus && (strcmp(focus, "GPS")))
          roadmap_math_get_context(&gSavedPosition, &gSavedZoom);

       roadmap_screen_hold();
       gSavedFocus = focus;
    }
    RTAlerts_popup_alert(iID, CENTER_ON_ALERT);
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
 * Download callback for the image
 * @param - the ID of the alert
 * @return None
 */
static void RTAlerts_Download_Image_Callback( void* context, int status, const char *image_path  )
{
	RoadMapImage image;
	int alertId = ((ImageDownloadCbContext*)context)->alertId;

	if ( status == 0 )
	{
#ifndef IPHONE
		image = roadmap_jpeg_read_file( NULL, image_path );
#else
		image = roadmap_canvas_load_image(NULL, image_path);
#endif //IPHONE

		if ( image )
			RTAlerts_Show_Image( alertId, image );
		else
			roadmap_log( ROADMAP_ERROR, "Error reading image from file: %d", alertId );
	}
	else
	{
		roadmap_log( ROADMAP_ERROR, "Error downloading image. Alert ID: %d. Status : %d. Path: %s",
				alertId, status, image_path );
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
	ImageDownloadCbContext *context = ( ImageDownloadCbContext* ) malloc( sizeof( ImageDownloadCbContext ) );
	context->alertId = alertId;

	pAlert = RTAlerts_Get_By_ID( alertId );

	roadmap_camera_image_download( pAlert->sImageIdStr, context, RTAlerts_Download_Image_Callback );

}
/**
 * Display the image attached to the alert
 * @param - the ID of the alert
 * @param image - the image to display
 * @return None
 */
static void RTAlerts_Show_Image( int alertId, RoadMapImage image )
{
	static SsdWidget dialog = NULL;
	static SsdWidget image_bitmap = NULL;
    RTAlert *pAlert;
    const char* title;

    pAlert = RTAlerts_Get_By_ID( alertId );

    title = RTAlerts_get_title( pAlert->iType, pAlert->iSubType );

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

    }

    ssd_bitmap_image_update( image_bitmap, image );

    dialog->data = (void*) image;

    ssd_dialog_activate( SSD_RT_ALERT_IMAGE_DLG_NAME, NULL );

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
void RTAlerts_Scroll_All()
{
    RTAlert *pAlert;
    if (!(ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "AlertPopUPDlg"))))
    {
		if (RTAlerts_Is_Empty())
			  return;
        RTAlerts_Sort_List(sort_proximity);
        pAlert = RTAlerts_Get(0);
        if ((pAlert->iDistance > roadmap_config_get_integer(&RoadMapConfigMaxAlertPopDist))&&
    	(!gIgnoreAlertMaxDist))
   		{ 
   			return ; // closest alert is too far away! don't scroll at all. 
   		}
        roadmap_screen_hold();
        gIterator = -1;
        gCurrentAlertId = -1;
        gPopAllTimerActive = TRUE;
        RTAlerts_Popup();
        roadmap_main_set_periodic(6000, RTAlerts_Popup);
    }
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
    	   gSavedZoom = -1;
    	}

   	ssd_dialog_hide_current(dec_cancel);

      roadmap_layer_adjust();
      roadmap_screen_redraw();
    }

    if (gPopAllTimerActive){
       roadmap_main_remove_periodic(RTAlerts_Popup) ;
       gPopAllTimerActive = FALSE;
    }
    gIterator = 0;
}

/**
 * Remove the alert and stop the Alerts timer.
 * @param None
 * @return None
 */
void RTAlerts_Cancel_Scrolling()
{
    RTAlerts_Stop_Scrolling();
    if (gTimerActive)
    	roadmap_main_remove_periodic(RTAlerts_Timer);
    gTimerActive = FALSE;
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

    if (gTimerActive)
        roadmap_main_remove_periodic(RTAlerts_Timer);
	gTimerActive = FALSE;

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
    int iDirection;
    const char * szDescription;
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
    BOOL success;
    const char *desc;
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

    RTAlerts_ShowProgressDlg();
    RTAlerts_ImageUpload();
    success = Realtime_Report_Alert(AlertContext->iType, desc,
            AlertContext->iDirection, gCurrentImageId, roadmap_twitter_is_sending_enabled() );
    if (success){
        ssd_dialog_hide_all(dec_close);
    }
    else
        ssd_dialog_hide_current(dec_close);

    if ( !success )
    	RTAlerts_CloseProgressDlg();

    free(AlertContext);

    return success;
}

/**
 * Show add additi -alert Type, szDescription - alert description, iDirection - direction of the alert
 * @return void
 */
void report_alert(int iAlertType, const char * szDescription, int iDirection)
{

    RTAlertContext *AlertConext = calloc(1, sizeof(RTAlertContext));
    gCurrentImageId[0] = 0;
    AlertConext->iType = iAlertType;
    AlertConext->szDescription = szDescription;
    AlertConext->iDirection = iDirection;

    ssd_dialog_hide_current(dec_close);
#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE)
    ShowEditbox(roadmap_lang_get("Additional Details"), "",
            on_keyboard_closed, (void *)AlertConext, EEditBoxStandard
#if defined(__SYMBIAN32__)
              | EEditBoxAlphaNumeric
#endif
               );
#else
    ssd_show_keyboard_dialog_ext( roadmap_lang_get("Additional Details"), NULL, NULL, NULL, on_keyboard_closed, AlertConext, SSD_KB_DLG_TYPING_LOCK_ENABLE );

#endif
}

static char const *DirectionQuickMenu[] = {

   "mydirection",
#ifndef IPHONE
   RoadMapFactorySeparator,
#endif //IPHONE
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

   TripLocation = roadmap_trip_get_gps_position("AlertSelection");
	if ((TripLocation == NULL) /*|| (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)*/) {
   		roadmap_messagebox ("Error", "Can't find alert position.");
   		return;
    }

#ifdef IPHONE
	roadmap_list_menu_simple (name, NULL, DirectionQuickMenu, NULL, NULL, actions,
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

    report_alert(RT_ALERT_TYPE_POLICE, "" , RT_ALERT_MY_DIRECTION);
}


/**
 * Report an alert for Accident my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_my_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Traffic Jam my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_my_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Hazard my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_my_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Parking my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_parking_my_direction()
{

    report_alert(RT_ALERT_TYPE_PARKING, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Other my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_my_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for constrcution my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_my_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for police on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_police_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_POLICE, "" , RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for accident on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, "" , RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Traffic Jam on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, "", RT_ALERT_OPPSOITE_DIRECTION);
}


/**
 * Report an alert for Hazard on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD, "", RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Parking on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_parking_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_PARKING, "", RT_ALERT_OPPSOITE_DIRECTION);
}
/**
 * Report an alert for Other on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, "", RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Construction on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, "", RT_ALERT_OPPSOITE_DIRECTION);
}

#ifndef IPHONE
///////////////////////////////////////////////////////////////////////////////////////////
void add_real_time_chit_chat()
{
    const RoadMapGpsPosition   *TripLocation;
#ifndef TOUCH_SCREEN
    RTAlertContext *AlertConext;
#endif

    TripLocation = roadmap_trip_get_gps_position("AlertSelection");
    if ((TripLocation == NULL) /*|| (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)*/) {
   		roadmap_messagebox ("Error", "Can't find current street.");
   		return;
    }
#ifdef TOUCH_SCREEN
	report_dialog(RT_ALERT_TYPE_CHIT_CHAT);
#else
    AlertConext = calloc(1, sizeof(RTAlertContext));

    AlertConext->iType = RT_ALERT_TYPE_CHIT_CHAT;
    AlertConext->szDescription = "";
    AlertConext->iDirection = RT_ALERT_MY_DIRECTION;

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined (IPHONE)
    ShowEditbox(roadmap_lang_get("Chat"), "", on_keyboard_closed,
            AlertConext, EEditBoxEmptyForbidden/* | EEditBoxAlphaNumeric */);
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

    success = Realtime_Post_Alert_Comment(pAlert->iID, value, roadmap_twitter_is_sending_enabled());
    if (success){
#ifndef IPHONE
        ssd_dialog_hide_all(dec_close);
#else
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

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE)
    ShowEditbox(roadmap_lang_get("Comment"), "", post_comment_keyboard_callback,
            pAlert, EEditBoxEmptyForbidden/* | EEditBoxAlphaNumeric*/ );
#else
   ssd_show_keyboard_dialog(  roadmap_lang_get("Comment"),
                              NULL,
                              post_comment_keyboard_callback,
                              pAlert);
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
    int context_save_zoom;
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
 * Button callbak for the messagebox indicating an incident on the route.
 * @param widget - the widget that was pressed
 * @param new_value - the value of the widget
 * @return void
 */
static int button_callback(SsdWidget widget, const char *new_value)
{

    if (!strcmp(widget->name, "OK") || !strcmp(widget->name, "Recalculate"))
    {

        ssd_dialog_hide_current(dec_ok);

        if (!strcmp(widget->name, "Recalculate"))
        {
            navigate_main_calc_route();
        }
        RTAlerts_Stop_Scrolling();
        return 1;

    }


    ssd_dialog_hide_current(dec_close);
    return 1;
}

/**
 * Creates and display a messagebox indicating an incident on the route.
 * @param pAlert - pointer to the alert that is on the route
 * @return void
 */
static void create_ssd_dialog(RTAlert *pAlert)
{
    const char *description;
    SsdWidget text;
    SsdWidget dialog = ssd_dialog_new("rt_alert_on_my_route",
            roadmap_lang_get("New Alert"),
            NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

    if (pAlert->iType== RT_ALERT_TYPE_ACCIDENT)
        description = roadmap_lang_get("New accident on your route");
    else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        description = roadmap_lang_get("New traffic jam on your route");
    else
        description = roadmap_lang_get("New incident on your route");

	text = ssd_text_new("text", description, 16, SSD_END_ROW|SSD_WIDGET_SPACE);
   ssd_widget_add(dialog, text);
	text = ssd_text_new("alert_text", pAlert->sLocationStr, 16, SSD_END_ROW);
	ssd_widget_set_color(text,"#9d1508", NULL);
	ssd_widget_add(dialog, text);


   /* Spacer */
   ssd_widget_add(dialog, ssd_container_new("spacer1", NULL, 0, 20,   SSD_END_ROW));

   ssd_widget_add(dialog, ssd_button_label("Recalculate",
            roadmap_lang_get("Recalculate"),
            SSD_WS_TABSTOP|SSD_ALIGN_CENTER, button_callback));

   ssd_widget_add(dialog, ssd_button_label("Cancel",
            roadmap_lang_get("Cancel"),
            SSD_WS_TABSTOP|SSD_ALIGN_CENTER, button_callback));
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

	ssd_widget_set_right_softkey_callback(dlg, Comment_Reply_sk_cb);
	ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Reply"));

	ssd_widget_set_left_softkey_callback(dlg, Comment_Ignore_sk_cb);
	ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Ignore"));

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
#endif

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
	CommentStr[0] = 0;


	if ((roadmap_config_get_integer(&LastCommentAlertIDCfg) == Alert->iID) && (roadmap_config_get_integer(&LastCommentIDCfg) == Comment->iID))
	   return;

	roadmap_config_set_integer(&LastCommentAlertIDCfg, Alert->iID);
	roadmap_config_set_integer(&LastCommentIDCfg, Comment->iID);
	roadmap_config_save(FALSE);

	 dialog =  ssd_popup_new("Comment Pop Up", roadmap_lang_get("Response"),NULL,SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_ROUNDED_BLACK|SSD_HEADER_BLACK|SSD_POINTER_COMMENT);

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
    snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s%s", Alert->sLocationStr, NEW_LINE);

	bitmap = ssd_bitmap_new("alert_icon",RTAlerts_Get_IconByType(Alert->iType, FALSE),0);
	ssd_widget_add(dialog, bitmap);
	text = ssd_text_new ("Comment Str", CommentStr, 14,SSD_END_ROW);
	ssd_widget_set_color(text,"#ffffff", NULL);
	ssd_widget_add(dialog, text);
	CommentStr[0] = 0;

	 snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s %s%s", roadmap_lang_get("by"), roadmap_lang_get(Comment->sPostedBy), NEW_LINE);

	 snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s%s", Comment->sDescription, NEW_LINE);

	text = ssd_text_new ("Comment Str", CommentStr, 14,0);
	ssd_widget_add(dialog, text);
	ssd_widget_set_color(text,"#ffffff", NULL);
    RTAlerts_add_comment_stars(dialog, Comment);

#ifdef TOUCH_SCREEN
	ssd_widget_add(dialog, space(5));
	ssd_widget_add (dialog,
   					ssd_button_label ("Reply", roadmap_lang_get ("Reply"),
                   	SSD_WS_TABSTOP|SSD_ALIGN_CENTER, Comment_Reply_button_callback));
   ssd_widget_add (dialog,
                  ssd_button_label ("Close", roadmap_lang_get ("Close"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, Comment_Close_button_callback));
   ssd_widget_add(dialog, space(5));

   ssd_widget_add(dialog, space(5));
#else
   RTAlerts_Comment_set_softkeys(dialog->parent);
#endif

	ssd_dialog_activate ("Comment Pop Up", NULL);
	ssd_dialog_draw();


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
	return TRUE;
}

int Rtalerts_Delete(int alertId){
   RTAlerts_Remove(alertId);
	return real_time_remove_alert(alertId);
}

int RTAlerts_Check_Same_Street(int record){
   return TRUE;
}

int RTAlerts_HandleAlert(int alertId){
   return FALSE;
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

#ifdef TOUCH_SCREEN
////////////////////////////////////////////////////////////////////
static int report_menu_buttons_callback (SsdWidget widget, const char *new_value) {

   SsdWidget container = widget->parent;
   BOOL success;
   if (!strcmp(widget->name, "Send")){
         const char *description = ssd_widget_get_value(ssd_widget_get(container, roadmap_lang_get("Additional Details")),roadmap_lang_get("Additional Details"));
   		const char *direction = ssd_button_get_name(ssd_widget_get(container, "Direction"));
   		const char *type = ssd_bitmap_get_name(ssd_widget_get(container, "alert_icon"));
   		int alert_direction;
   		if (!strcmp(direction, "mydirection"))
   			alert_direction = RT_ALERT_MY_DIRECTION;
   		else
   			alert_direction = RT_ALERT_OPPSOITE_DIRECTION;

			RTAlerts_ShowProgressDlg();
			RTAlerts_ImageUpload();
         gMinimizeState = RTAlerts_Get_TypeByIconName(type);
   	   success = Realtime_Report_Alert(RTAlerts_Get_TypeByIconName(type), description, alert_direction, gCurrentImageId, roadmap_twitter_is_sending_enabled() );
		    if (success){
		         //gMinimizeState = -1;
		         //roadmap_trip_restore_focus();
	            ssd_dialog_hide_all(dec_ok);		// Why this !!!??? *** AGA ***
		    }
		    else
		    {
		    	RTAlerts_CloseProgressDlg();

		    }
   }

   else if (!strcmp(widget->name, "Hide")){
   		const char *type = ssd_bitmap_get_name(ssd_widget_get(container, "alert_icon"));
   		gMinimizeState = RTAlerts_Get_TypeByIconName(type);
   		ssd_dialog_hide_all(dec_yes);
   }

   else{
         gMinimizeState = -1;
   		roadmap_trip_restore_focus();
   		ssd_dialog_hide_all(dec_close);
   }

   return 1;
}

/////////////////////////////////////////////////////////////////////
static void on_mood_changed(void){

	char *icon[2];

	icon[0] = (char *)roadmap_mood_get();
	icon[1] = NULL;

	ssd_dialog_change_button(roadmap_lang_get ("Mood"), (const char **)&icon[0], 1);
	ssd_dialog_change_text("Mood Text", roadmap_lang_get(roadmap_mood_get_name()));


}

/////////////////////////////////////////////////////////////////////
static int on_mood_select( SsdWidget this, const char *new_value){
	roadmap_mood_dialog(on_mood_changed);
	return 0;
}

/////////////////////////////////////////////////////////////////////
static int on_add_image( SsdWidget this, const char *new_value )
{
#ifndef IPHONE
	static RoadMapImage image_thumbnail = NULL;
	const char *bitmap_names[2] = { NULL, NULL };
	BOOL image_taken;

	// Release the previous thumbnail
	if ( image_thumbnail )
	{
		roadmap_canvas_free_image( image_thumbnail );
		image_thumbnail = NULL;
	}

	image_taken = roadmap_camera_image_alert( &gCurrentImagePath, &image_thumbnail);


	// Replace the thumbnail in the button if the image has been taken successfully
	if ( image_taken )
	{
		roadmap_log( ROADMAP_INFO, "Camera image was successfully taken and stored at %s", gCurrentImagePath );
		if ( image_thumbnail )
		{
			RoadMapImage bitmap_images[1];
			bitmap_names[0] = "";
			bitmap_images[0] = image_thumbnail;
			ssd_button_change_images( this, bitmap_images, bitmap_names, 1 );
		}
		else
		{

			bitmap_names[0] = "picture_added";
			ssd_button_change_icon( this, bitmap_names, 1 );
		}
	}
	else
	{
		*bitmap_names = "add_image_box";
		ssd_button_change_icon( this, bitmap_names, 1 );
		roadmap_log( ROADMAP_WARNING, "The camera image was not captured" );
	}
#endif //IPHONE

	return 0;
}


/////////////////////////////////////////////////////////////////////
static int on_direction( SsdWidget this, const char *new_value){
	char *icon[2];
	char sLocationStr[RT_ALERT_LOCATION_MAX_SIZE+1];
	int iDirection = RT_ALERT_MY_DIRECTION;
	SsdWidget button = ssd_widget_get(this,"Direction" );

	icon[1] = NULL;
	if (!strcmp(ssd_button_get_name(button), roadmap_lang_get("mydirection"))){
		icon[0] = "oppositedirection";
		ssd_dialog_change_button("Direction", (const char **)&icon[0], 1);
		ssd_dialog_change_text("Direction Text", roadmap_lang_get("Opposite direction"));
		iDirection = RT_ALERT_OPPSOITE_DIRECTION;
		get_location_str(&sLocationStr[0], iDirection, NULL);
		ssd_dialog_change_text("Alert position", sLocationStr);
	}
	else{
		icon[0] = "mydirection";
		ssd_dialog_change_button("Direction", (const char **)&icon[0], 1);
		ssd_dialog_change_text("Direction Text", roadmap_lang_get("My direction"));
		iDirection = RT_ALERT_MY_DIRECTION;
		get_location_str(&sLocationStr[0], iDirection, NULL);
		ssd_dialog_change_text("Alert position", sLocationStr);
	}
	return 0;
}



/////////////////////////////////////////////////////////////////////
static void report_dialog(int iAlertType){
	SsdWidget dialog;
   SsdWidget group;
   SsdWidget container;
	SsdWidget text, bitmap, box, button;
	SsdWidget text_box;
	char *icon[2];
	BOOL isLongScreen;
	const char *mood;
	const RoadMapGpsPosition   *TripLocation;
	char sLocationStr[RT_ALERT_LOCATION_MAX_SIZE+1];
	int iDirection = RT_ALERT_MY_DIRECTION;
	int container_height = 40;
   const char *edit_button_r[] = {"edit_right", "edit_right"};
   const char *edit_button_l[] = {"edit_left", "edit_left"};
    gCurrentImageId[0] = 0;
    isLongScreen = (roadmap_canvas_height()>320);

    TripLocation = roadmap_trip_get_gps_position("AlertSelection");
	if ((TripLocation == NULL) /*|| (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)*/) {
   		roadmap_messagebox ("Error", "Can't find location.");
   		return;
    }

	gMinimizeState = -1;

	dialog = ssd_dialog_new ("Report",
                            RTAlerts_get_title(iAlertType, 0),
                            NULL,
                            SSD_CONTAINER_TITLE);

   group = ssd_container_new ("Report", NULL,
                        		SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

	if (isLongScreen){
		ssd_widget_add(group, space(2));
		text = ssd_text_new ("Position saved", roadmap_lang_get("Note: Location and time saved"), -1,SSD_END_ROW);
		ssd_widget_set_color(text, "#206892",NULL);
		ssd_widget_add(group, text);
		ssd_widget_add(group, space(1));
	}


   box = ssd_container_new ("box", NULL,
                        		SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW);
	bitmap = ssd_bitmap_new("alert_icon",RTAlerts_Get_IconByType(iAlertType, FALSE), 0);
	ssd_widget_add(box, bitmap);
	get_location_str(&sLocationStr[0], iDirection, NULL);
	text = ssd_text_new ("Alert position", sLocationStr, -1,SSD_END_ROW|SSD_ALIGN_VCENTER);
	ssd_widget_add(box, text);
	ssd_widget_add(group, box);
    ssd_widget_add(group, space(1));


	text_box = ssd_entry_new( roadmap_lang_get("Additional Details"),"",SSD_WIDGET_SPACE|SSD_END_ROW,0, SSD_MAX_SIZE, SSD_MIN_SIZE,roadmap_lang_get("<Add description>"));
	ssd_entry_set_kb_params( text_box, roadmap_lang_get("Additional Details"), NULL, NULL, NULL, SSD_KB_DLG_TYPING_LOCK_ENABLE );
	ssd_widget_add(group, text_box);

   #ifdef HI_RES_SCREEN
      ssd_widget_add(group, space(4));
   #endif

   box = ssd_container_new ("Direction group", NULL,
                  SSD_MAX_SIZE,container_height,
		  		  SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_TXT_BOX);
	icon[0] = "mydirection";
	icon[1] = NULL;
   container =  ssd_container_new ("space", NULL, 60, SSD_MIN_SIZE,SSD_ALIGN_VCENTER);
	ssd_widget_set_color(container, NULL, NULL);
   ssd_widget_add (container,
		   ssd_button_new ("Direction", roadmap_lang_get("My direction"), (const char **)&icon[0], 1,
		   				   SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER,
		   				   NULL));
	ssd_widget_add(box, container);
	box->callback = on_direction;
	ssd_widget_set_pointer_force_click( box );
	text = ssd_text_new ("Direction Text", roadmap_lang_get("My direction"), -1,SSD_ALIGN_VCENTER);
	ssd_widget_add(box, text);
	ssd_widget_add(group, box);

   #ifdef HI_RES_SCREEN
      ssd_widget_add(group, space(4));
   #endif
	//Mood
    box = ssd_container_new ("Mood group", NULL,
                  SSD_MAX_SIZE,container_height,
		  		  SSD_WIDGET_SPACE|SSD_END_ROW|SSD_CONTAINER_TXT_BOX);
	ssd_widget_set_color (box, "#000000", "#ffffff");
	box->callback = on_mood_select;
   ssd_widget_set_pointer_force_click( box );
	icon[0] = (char *)roadmap_mood_get();
	icon[1] = NULL;
   container =  ssd_container_new ("space", NULL, 60, SSD_MIN_SIZE,SSD_ALIGN_VCENTER);
	ssd_widget_set_color(container, NULL, NULL);
	mood = roadmap_mood_get();
   ssd_widget_add (container,
		   ssd_button_new (roadmap_lang_get ("Mood"), roadmap_lang_get(mood), (const char **)&icon[0], 1,
		   				   SSD_ALIGN_VCENTER|SSD_WS_TABSTOP|SSD_ALIGN_CENTER,
		   				   on_mood_select));
   free((void *)mood);
	ssd_widget_add(box, container);
	ssd_widget_add (box,
         ssd_text_new ("Mood Text", roadmap_lang_get(roadmap_mood_get_name()), -1,
                        SSD_ALIGN_VCENTER));


   if (!ssd_widget_rtl(NULL))
	   button = ssd_button_new ("edit_button", "", &edit_button_r[0], 2,
        	                 SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_mood_select);
   else
	   button = ssd_button_new ("edit_button", "", &edit_button_l[0], 2,
        	                 SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_mood_select);
   if (!ssd_widget_rtl(NULL))
   		ssd_widget_set_offset(button, 10, 0);
   else
		ssd_widget_set_offset(button, -11, 0);
   ssd_widget_add(box, button);

   ssd_widget_add (group, box);

   if (isLongScreen){
    	ssd_widget_add(group, space(1));
   }
    icon[0] = "add_image_box";
    icon[1] = NULL;
    ssd_widget_add (group,
		   ssd_button_new ("add image", "", (const char **)&icon[0], 1,
		   				   SSD_ALIGN_CENTER|SSD_END_ROW,
		   				   on_add_image));
   if (isLongScreen){
   ssd_widget_add(group, space(1));
   }

    ssd_widget_add (group,
   				   ssd_button_label ("Send", roadmap_lang_get ("Send"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, report_menu_buttons_callback));

    ssd_widget_add (group,
   				   ssd_button_label ("Hide", roadmap_lang_get ("Hide"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, report_menu_buttons_callback));
    ssd_widget_add (group,
	   					ssd_button_label ("Cancel", roadmap_lang_get ("Cancel"),
	                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, report_menu_buttons_callback));

    if (isLongScreen){
		ssd_widget_add(group, space(2));
    }

	ssd_widget_add(dialog, group);



	ssd_dialog_activate("Report", NULL);

}
#endif

void RTAlerts_Resert_Minimized(void){
  gMinimizeState = -1;
}

int RTAlerts_Get_Minimize_State(void){
	return gMinimizeState;
}

void RTAlerts_Minimized_Alert_Dialog(void){
	ssd_dialog_activate("Report", NULL);
	ssd_dialog_draw ();
}

static void RTAlerts_ShowProgressDlg(void)
{
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Sending report . . . " ) );
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

static void RTAlerts_ImageUpload(void)
{
#ifndef IPHONE
	if ( gCurrentImagePath )
	{
		roadmap_camera_image_upload( NULL, gCurrentImagePath, gCurrentImageId, NULL );
		RTAlerts_ReleaseImagePath();
	}
#endif //IPHONE
}



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
         }
      }
      success = Realtime_ReportMapProblem(type, description,TripLocation);
      roadmap_trip_restore_focus();
      ssd_dialog_hide_all(dec_close);
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
      if (CheckboxTypeOfError[i] && strcmp(widget->parent->name, CheckboxTypeOfError[i]->name))
         CheckboxTypeOfError[i]->set_data(CheckboxTypeOfError[i], "no");
      else
         CheckboxTypeOfError[i]->set_data(CheckboxTypeOfError[i], "yes");
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
   RoadMapGpsPosition *TripLocation;
   char sLocationStr[200];
   int iDirection = RT_ALERT_MY_DIRECTION;

   int i;
   if (!roadmap_gps_have_reception ()) {
      roadmap_messagebox ("Error",
               "No GPS connection. Make sure you are outdoors. Currently showing approximate location");
      return;
   }

   TripLocation = ( RoadMapGpsPosition *)roadmap_trip_get_gps_position ("AlertSelection");
   if (TripLocation == NULL) {

      int login_state;
      PluginLine line;
      int direction;
      login_state = RealTimeLoginState();
      if (login_state == 0){
            roadmap_messagebox("Error","No network connection, unable to report");
            return;
      }


      TripLocation = malloc (sizeof (*TripLocation));
      if (roadmap_navigate_get_current (TripLocation, &line, &direction) == -1) {
         const RoadMapPosition *GpsPosition;
         GpsPosition = roadmap_trip_get_position ("GPS");
         if ( (GpsPosition != NULL)) {
            TripLocation->latitude = GpsPosition->latitude;
            TripLocation->longitude = GpsPosition->longitude;
            TripLocation->speed = 0;
            TripLocation->steering = 0;
         }
         else {
            free (TripLocation);
            roadmap_messagebox ("Error",
                     "No GPS connection. Make sure you are outdoors. Currently showing approximate location");
            return;
         }
      }
      roadmap_trip_set_gps_position( "AlertSelection", "Selection", "new_alert_marker",TripLocation);

   }

   gMinimizeState = -1;

   dialog = ssd_dialog_new ("MapErrorDlg", roadmap_lang_get ("Map error"),
            on_map_problem_close, SSD_CONTAINER_TITLE);

   group = ssd_container_new ("MapErrorGrp", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
                     | SSD_ROUNDED_WHITE | SSD_POINTER_NONE
                     | SSD_CONTAINER_BORDER);
   ssd_widget_add (group, space (1));

   box = ssd_container_new ("box", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color (box, NULL, NULL);
   bitmap = ssd_bitmap_new ("icon", "map_error_small", 0);
   ssd_widget_add (box, bitmap);
   get_location_str (&sLocationStr[0], iDirection,TripLocation );
   text = ssd_text_new ("Alert position", sLocationStr, -1, SSD_END_ROW
            | SSD_ALIGN_VCENTER);
   ssd_widget_set_color (text, "#206892", NULL);
   ssd_widget_add (box, text);
   ssd_widget_add (group, box);

   ssd_widget_add (group, space (2));
   for (i = 0; i < gMapProblemsCount; i++) {

      box = ssd_container_new ("CheckBoxbox", NULL, SSD_MAX_SIZE, 35,
               SSD_WIDGET_SPACE | SSD_END_ROW | SSD_WS_TABSTOP);

      CheckboxTypeOfError[i] = ssd_checkbox_new (MAP_PROBLEMS_OPTION[gMapProblems[i]], FALSE,
               SSD_ALIGN_VCENTER, checkbox_callback, NULL, NULL,
               CHECKBOX_STYLE_ROUNDED);
      ssd_widget_add (box, CheckboxTypeOfError[i]);

      space_w = ssd_container_new ("spacer1", NULL, 10, 14, 0);
      ssd_widget_set_color (space_w, NULL, NULL);
      ssd_widget_add (box, space_w);

      ssd_widget_add (box, ssd_text_new ("TextCheckbox", roadmap_lang_get(MAP_PROBLEMS_OPTION[gMapProblems[i]]), 14,
               SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE | SSD_END_ROW));

      ssd_widget_add (group, box);
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
			return NULL;
	}
}



//adds the stars icon to the user who reported an alert
void RTAlerts_update_stars(SsdWidget container,  RTAlert *pAlert){
   SsdWidget stars_icon =  ssd_widget_get ( container, "stars_icon");
   if (stars_icon ==NULL){ // init the bitmap, call with 0 in the meanwhile.
   	  stars_icon = ssd_bitmap_new("stars_icon",RTAlerts_Get_Stars_Icon(0), 0);
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
      		 SsdWidget stars_bitmap = ssd_bitmap_new("stars icon", RTAlerts_Get_Stars_Icon(rank),SSD_END_ROW);
      		 ssd_widget_add(container, stars_bitmap);
    }
}

void RTAlerts_Set_Ignore_Max_Distance(BOOL ignore){
	gIgnoreAlertMaxDist = ignore;
}

/**
 * Builds the string containing the reported by info
 */
static void RTAlerts_get_reported_by_string( RTAlert *pAlert, char* buf, int buf_len )
{

   // Display who reported the alert
   if (pAlert->sReportedBy[0] != 0)
   {
    	snprintf(buf, buf_len,
      	      " %s %s  ", roadmap_lang_get("by"), pAlert->sReportedBy );
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

