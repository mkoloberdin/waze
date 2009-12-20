/* RealtimeAlertsList.c - manage the Real Time Alerts list display
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009 Avi R.
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
 * SYNOPSYS:
 *
 *   See RealtimeAlertsList.h
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_dialog.h"

#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertsList.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "Realtime/RealtimeTrafficInfo.h"
#include "iphone/roadmap_list_menu.h"
#include "iphone/roadmap_iphonelist_menu.h"
#include "iphone/widgets/iphoneLabel.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_path.h"
#include "roadmap_main.h"

#ifdef STREAM_TEST
#include "roadmap_sound_stream.h"
#endif //STREAM_TEST


// Context menu:
static ssd_cm_item      context_menu_items[] = 
{
   SSD_CM_INIT_ITEM  ( "Show on map",        rtl_cm_show),
   SSD_CM_INIT_ITEM  ( "Delete",             rtl_cm_delete),
   SSD_CM_INIT_ITEM  ( "View comments",      rtl_cm_view_comments),
   SSD_CM_INIT_ITEM  ( "Post Comment",       rtl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Sort by proximity",  rtl_cm_sort_proximity),
   SSD_CM_INIT_ITEM  ( "Sort by recency",    rtl_cm_sort_recency),
   SSD_CM_INIT_ITEM  ( "Exit_key",           rtl_cm_exit),
   SSD_CM_INIT_ITEM  ( "Cancel",      	     rtl_cm_cancel),
};
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

static void populate_list();
static void free_list();

static SsdWidget   tabs[tab__count];

typedef struct AlertList_s{
	char *type_str[MAX_ALERTS_ENTRIES];
   char *location_str[MAX_ALERTS_ENTRIES];
	char *detail[MAX_ALERTS_ENTRIES];
	char *distance[MAX_ALERTS_ENTRIES];
	char *unit[MAX_ALERTS_ENTRIES];
	char *label[MAX_ALERTS_ENTRIES];
	char *value[MAX_ALERTS_ENTRIES];
	char *icon[MAX_ALERTS_ENTRIES];
	char *attachment[MAX_ALERTS_ENTRIES];
	int type[MAX_ALERTS_ENTRIES];
	int iDistnace[MAX_ALERTS_ENTRIES];
	UIView *view[MAX_ALERTS_ENTRIES];
	int iCount;
}AlertList;

static AlertList gAlertListTable;
static AlertList gTabAlertList; 
 
static   real_time_tabs		g_active_tab = tab_all;
static   alert_sort_method g_list_sorted = sort_proximity;

static int g_alert_type = -1;

static UIColor *gColorType = NULL;
static UIColor *gColorUnit = NULL;

static CGRect gRectIcon = {10.0f, 10.0f, 40.0f, 40.0f};
static CGRect gRectType = {65.0f, 2.0f, 220.0f, 23.0f};
static CGRect gRectLocation = {65.0f, 25.0f, 220.0f, 46.0f};
static CGRect gRectDetail = {65.0f, 77.0f, 220.0f, 50.0f};
static CGRect gRectAttachment = {35.0f, 10.0f, 40.0f, 50.0f};
static CGRect gRectDistance = {15.0f, 50.0f, 35.0f, 15.0f};
static CGRect gRectUnit = {15.0f, 65.0f, 35.0f, 20.0f};

#define ALERT_LIST_HEIGHT 130

static int initialized = 0;
//static int fresh_list = 0;


#define MAX_IMG_ITEMS 100

typedef struct ImageList_s{
	char *icon[MAX_IMG_ITEMS];
	UIImage *image[MAX_IMG_ITEMS];
	int iCount;
}ImageList;

static ImageList gImageList;


///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBack(SsdWidget widget, const char *new_value,
        const void *value, void *context)
{
    int alertId;

	/* fix this if we have "scroll list" as the first item on the table
    if (value == gAlertListTable.value[0])
    {
        RTAlerts_Scroll_All();
        ssd_generic_list_dialog_hide_all();
        return TRUE;
    }
	 */

    alertId = atoi((const char*)value);
    if (RTAlerts_Get_Number_of_Comments(alertId) == 0)
    {
        //ssd_generic_list_dialog_hide_all();
		roadmap_main_show_root(0);
        RTAlerts_Popup_By_Id(alertId);
    }
    else
    {
        //ssd_generic_list_dialog_hide();
        RealtimeAlertCommentsList(alertId);
    }
	
	//free_list();
	
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void delete_callback(int exit_code, void *context){
    int alertId;
    BOOL success;
    
    if (exit_code != dec_yes)
         return;
    
    alertId = atoi((const char*)context);
    success = real_time_remove_alert(alertId);
    if (success)
        roadmap_messagebox("Delete Alert", "Your request was sent to the server");
    
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsDeleteCallBack(SsdWidget widget, const char *new_value,
        const void *value, void *context)
{
   char message[200];
    
/*
    if (!new_value[0] || !strcmp(new_value,
            roadmap_lang_get("Scroll Through Alerts on the map")))
    {
         return TRUE;
    }*/
	
	roadmap_main_show_root(NO);
	int i;
	for (i = 0; i < gAlertListTable.iCount; ++i) {
		if (gAlertListTable.value[i] == value) break;
	}

	 sprintf(message,"%s\n%s\n%s",roadmap_lang_get("Delete Alert:"), gAlertListTable.type_str[i], gAlertListTable.location_str[i]);
   
    ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value); 

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
UIImage *getImage (char *icon) {
	int i;
	
	for (i = 0; i < gImageList.iCount; ++i) {
		if (strcmp(icon, gImageList.icon[i]) == 0)
			return (gImageList.image[i]);
	}
	
	if (i >= MAX_IMG_ITEMS)
		return NULL;
	
	gImageList.image[i] = roadmap_iphoneimage_load(icon);
	gImageList.icon[i] = strdup(icon);
	gImageList.iCount++;
	
	return (gImageList.image[i]);
}

///////////////////////////////////////////////////////////////////////////////////////////
UIView *RealtimeAlertsListViewForCell (void *value, CGRect viewRect, UIView *reuseView) {
	int i;
	int index;
	UIImage *img;
	CGRect rect;
	UIImageView *imgView = NULL;
	iphoneLabel *label = NULL;
	UIView *view = NULL;
	UIView *cellView = reuseView;
	
	enum tags {
		ICON_TAG = 1,
		TYPE_TAG,
      LOCATION_TAG,
		DETAIL_TAG,
		DISTANCE_TAG,
		UM_TAG,
		ATTCH_ICON_TAG
	};
	
	for (i = 0; i < gAlertListTable.iCount; ++i) {
		if (gAlertListTable.value[i] == value) break;
	}
	
	index = i;
	
	if (index == gAlertListTable.iCount)
		return NULL;
	
	
	if (cellView == NULL) {
		cellView = [[UIView alloc] initWithFrame:viewRect];
      [cellView setAutoresizesSubviews:YES];
      [cellView setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		
		// Create alert icon
		imgView = [[UIImageView alloc] initWithFrame:gRectIcon];
		imgView.tag = ICON_TAG;
		[cellView addSubview:imgView];
		[imgView release];
		
		// Create alert type
		label = [[iphoneLabel alloc] initWithFrame:gRectType];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:1];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setTextColor:gColorType];
		[label setFont:[UIFont boldSystemFontOfSize:20]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = TYPE_TAG;
		[cellView addSubview:label];
		[label release];
      
      // Create alert location
		label = [[iphoneLabel alloc] initWithFrame:gRectLocation];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:2];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setFont:[UIFont boldSystemFontOfSize:20]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = LOCATION_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert details
		label = [[iphoneLabel alloc] initWithFrame:gRectDetail];
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
		[label setNumberOfLines:3];
		[label setLineBreakMode:UILineBreakModeWordWrap];
		[label setFont:[UIFont systemFontOfSize:14]];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = DETAIL_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert distance
		label = [[iphoneLabel alloc] initWithFrame:gRectDistance];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setFont:[UIFont boldSystemFontOfSize:16]];
		[label setAdjustsFontSizeToFitWidth:YES];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = DISTANCE_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Create alert um
		label = [[iphoneLabel alloc] initWithFrame:gRectUnit];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setTextColor:gColorUnit];
		[label setFont:[UIFont systemFontOfSize:14]];
      [label setBackgroundColor:[UIColor clearColor]];
		label.tag = UM_TAG;
		[cellView addSubview:label];
		[label release];
		
		// Creqte attachment icon
		imgView = [[UIImageView alloc] initWithFrame:gRectAttachment];
		imgView.tag = ATTCH_ICON_TAG;
		[cellView addSubview:imgView];
	}
	
	for (i = 0; i < [[cellView subviews] count]; i++) {
		view = (UIView *) [[cellView subviews] objectAtIndex:i];
		
		switch (view.tag) {
			case ICON_TAG:
				//Set alert icon
				img = getImage(gAlertListTable.icon[index]);
				[(UIImageView *)view setImage:img];
				if (img) {
					rect = gRectIcon;
					rect.size = [img size];
					rect.origin.x = gRectIcon.origin.x + (gRectIcon.size.width - rect.size.width)/2;
					[view setFrame:rect];
				}
				break;
			case TYPE_TAG:
				//set alert description
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.type_str[index]]];
				break;
         case LOCATION_TAG:
				//set alert description
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.location_str[index]]];
				break;
			case DETAIL_TAG:
				//set alert details
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.detail[index]]];
				break;
			case DISTANCE_TAG:
				//set alert distance
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.distance[index]]];
				break;
			case UM_TAG:
				//set alert um
				[(iphoneLabel *)view setText:[NSString stringWithUTF8String:gAlertListTable.unit[index]]];
				break;
			case ATTCH_ICON_TAG:
				//set attachment icon
				if (gAlertListTable.attachment[index]) {
					img = getImage(gAlertListTable.attachment[index]);
					[(UIImageView *)view setImage:img];
					if (img) {
						rect = gRectAttachment;
						rect.size = [img size];
						rect.origin.x = gRectAttachment.origin.x + (gRectAttachment.size.width - rect.size.width)/2;
						[view setFrame:rect];
					}
				} else {
					[(UIImageView *)view setImage:NULL];
				}
				break;
			default:
				break;
		}
	}
	
	return cellView;
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsListShow(void)
{
	const char *title;

	
	switch (g_alert_type) {
		case -1:
			title = "All";
			break;
		case RT_ALERT_TYPE_POLICE:
			title = "Police";
			break;
		case RT_ALERT_TYPE_TRAFFIC_JAM:
			title = "Traffic";
			break;
		case RT_ALERT_TYPE_ACCIDENT:
			title = "Accidents";
			break;
		case RT_ALERT_TYPE_OTHER:
			title = "Other";
			break;
		default:
			title = "Events";
			break;
	}
	roadmap_list_menu_custom (roadmap_lang_get(title), gTabAlertList.iCount,
							  (const void **)&gTabAlertList.value[0],
							  RealtimeAlertsListCallBack, RealtimeAlertsDeleteCallBack,
							  NULL, RealtimeAlertsListViewForCell, NULL, ALERT_LIST_HEIGHT, FALSE);
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBackTabs(SsdWidget widget, const char *new_value,
        const void *value)
{

    int alertId;
	
    if (!new_value[0] || !strcmp(new_value,
            roadmap_lang_get("Scroll Through Alerts on the map")))
    {
        RTAlerts_Scroll_All();
        ssd_generic_list_dialog_hide_all();
        return TRUE;
    }

    alertId = atoi((const char*)value);
    if (RTAlerts_Get_Number_of_Comments(alertId) == 0)
    {
        ssd_generic_list_dialog_hide_all();
        RTAlerts_Popup_By_Id(alertId);
    }
    else
    {
        ssd_generic_list_dialog_hide();
        RealtimeAlertCommentsList(alertId);
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsDeleteCallBackTabs(SsdWidget widget, const char *new_value,
        const void *value)
{
   char message[200];
    

    if (!new_value[0] || !strcmp(new_value,
            roadmap_lang_get("Scroll Through Alerts on the map")))
    {
         return TRUE;
    }

	 sprintf(message,"%s\n%s",roadmap_lang_get("Delete Alert:"), new_value);
   
    ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value); 

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int count_tab(int tab, int num, int types,...){
	int i;
	int count = 0;
	va_list ap;
	int type;
   
	for ( i=0; i< gAlertListTable.iCount; i++){
		int j;
		va_start(ap, types);
		type = types;
		for (j = 0; j < num; j++ ) {
			if (gAlertListTable.type[i] == type || tab == tab_all) {
            count++;
			}
			type = va_arg(ap, int);
		}
		va_end(ap);
   }
   
   return count;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_tab(int tab, int num, int types,...){
	int i;
	int count = 0;
	va_list ap;
	int type;
		
	for ( i=0; i< gAlertListTable.iCount; i++){
		int j;
		va_start(ap, types);
		type = types;
		for (j = 0; j < num; j++ ) {
			if (gAlertListTable.type[i] == type || tab == tab_all) {
				gTabAlertList.type_str[count] = gAlertListTable.type_str[i];
            gTabAlertList.location_str[count] = gAlertListTable.location_str[i];
				gTabAlertList.detail[count] = gAlertListTable.detail[i];
				gTabAlertList.distance[count] = gAlertListTable.distance[i];
				gTabAlertList.unit[count] = gAlertListTable.unit[i];
				gTabAlertList.value[count] = gAlertListTable.value[i];
				gTabAlertList.icon[count] = gAlertListTable.icon[i];
				gTabAlertList.type[count] = gAlertListTable.type[i];
				gTabAlertList.iDistnace[count] = gAlertListTable.iDistnace[i];
            
				count++;
			}
			type = va_arg(ap, int);
		}
		va_end(ap);
	 }
	 gTabAlertList.iCount = count;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_list(){
   int iCount = -1;
   int iAlertCount;
   RTAlert *alert;
   char AlertStr[700];
   char str[100];
   int distance = -1;
   RoadMapGpsPosition CurrentPosition;
   RoadMapPosition position, current_pos;
   const RoadMapPosition *gps_pos;
   PluginLine line;
   int Direction;
   int distance_far;
   char location_str[100];
   char dist_str[100];
   char unit_str[20];
	char detail_str[RT_ALERT_DESCRIPTION_MAXSIZE + 50 + 1]; // 50 for additional text
   int i;
   RoadMapPosition context_save_pos;
   int context_save_zoom;
	time_t now;
	int timeDiff;
	
	//UIView *cellView = NULL;
	
    
    AlertStr[0] = 0;
    str[0] = 0;
	detail_str[0] = 0;

   for (i=0; i<MAX_ALERTS_ENTRIES; i++)
   {
		gAlertListTable.type_str[i] = NULL;
      gAlertListTable.location_str[i] = NULL;
		gAlertListTable.detail[i] = NULL;
		gAlertListTable.distance[i] = NULL;
		gAlertListTable.unit[i] = NULL;
      gAlertListTable.value[i] = NULL;
      gAlertListTable.icon[i] = NULL;
		gAlertListTable.attachment[i] = NULL;
   }
   
	
	//fresh_list = 1;

    /*if (iCount == -1)
    {
        gAlertListTable.description[0]
                = (char *)roadmap_lang_get("Scroll Through Alerts on the map");
        gAlertListTable.icon[0] = "globe";
        gAlertListTable.type[0] = -1;
		gAlertListTable.value[0] = "-1";
		gAlertListTable.detail[0] = " ";
    }
    iCount = 1;
	*/
	iCount = 0;//TODO: do we need the "scroll on map" item?
    
    iAlertCount = RTAlerts_Count();

 	 roadmap_math_get_context(&context_save_pos, &context_save_zoom);

    while ((iAlertCount > 0) && (iCount < MAX_ALERTS_ENTRIES))    {

        AlertStr[0] = 0;

        //alert = RTAlerts_Get(iCount -1);
        alert = RTAlerts_Get(iCount);
		
        position.longitude = alert->iLongitude;
        position.latitude = alert->iLatitude;

        roadmap_math_set_context((RoadMapPosition *)&position, 20);

       if (alert->iType == RT_ALERT_TYPE_ACCIDENT)
          snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr), "%s", roadmap_lang_get("Accident"));
       else if (alert->iType == RT_ALERT_TYPE_POLICE)
          snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr), "%s", roadmap_lang_get("Police"));
       else if (alert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
          snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr), "%s", roadmap_lang_get("Traffic jam"));
       else if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO){
          if (alert->iSubType == LIGHT_TRAFFIC)
             snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                      - strlen(AlertStr), "%s", roadmap_lang_get("Light traffic"));
          else if (alert->iSubType == MODERATE_TRAFFIC)
             snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                      - strlen(AlertStr), "%s", roadmap_lang_get("Moderate traffic"));
          else if (alert->iSubType == HEAVY_TRAFFIC)
             snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                      - strlen(AlertStr), "%s", roadmap_lang_get("Heavy traffic"));
          else if (alert->iSubType == STAND_STILL_TRAFFIC)
             snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                      - strlen(AlertStr), "%s", roadmap_lang_get("Complete standstill"));
          else
             snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                      - strlen(AlertStr), "%s", roadmap_lang_get("Traffic"));
       }
        else if (alert->iType == RT_ALERT_TYPE_HAZARD)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
					 - strlen(AlertStr), "%s", roadmap_lang_get("Hazard"));        
        else if (alert->iType == RT_ALERT_TYPE_OTHER)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
					 - strlen(AlertStr), "%s", roadmap_lang_get("Other"));        
        else if (alert->iType == RT_ALERT_TYPE_CONSTRUCTION)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
					 - strlen(AlertStr), "%s", roadmap_lang_get("Road construction"));        
        else if (alert->iType == RT_ALERT_TYPE_PARKING)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
					 - strlen(AlertStr), "%s", roadmap_lang_get("Parking"));        
        else
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
					 - strlen(AlertStr), "%s",
					 roadmap_lang_get("Chit chat"));

		
        if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction)
                == -1)
        {
            // check the distance to the alert
            gps_pos = roadmap_trip_get_position("Location");
            if (gps_pos != NULL)
            {
                current_pos.latitude = gps_pos->latitude;
                current_pos.longitude = gps_pos->longitude;
            }
            else
            {
                current_pos.latitude = -1;
                current_pos.longitude = -1;
            }
        }
        else
        {
            current_pos.latitude = CurrentPosition.latitude;
            current_pos.longitude = CurrentPosition.longitude;
        }

        if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
        {
            distance = roadmap_math_distance(&current_pos, &position);
            distance_far = roadmap_math_to_trip_distance(distance);

            if (distance_far > 0)
            {
                int tenths = roadmap_math_to_trip_distance_tenths(distance);
                snprintf(dist_str, sizeof(dist_str), "%d.%d", distance_far, tenths
                        % 10);
                snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_lang_get(roadmap_math_trip_unit()));
            }
            else
            {
                snprintf(dist_str, sizeof(dist_str), "%d", distance);
                snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_lang_get(roadmap_math_distance_unit()));
            }
        }

		snprintf(location_str, sizeof(location_str), "%s", alert->sLocationStr);


        snprintf(str, sizeof(str), "%d", alert->iID);
		
		snprintf(detail_str, sizeof(detail_str), "%s", alert->sDescription);
		
		// Display when the alert was generated
		now = time(NULL);
		timeDiff = (int)difftime(now, (time_t)alert->iReportTime);
		if (timeDiff <0)
			timeDiff = 0;
		
		if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
    		snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 "\n%s", roadmap_lang_get("Updated "));
		else
    		snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 "\n%s",roadmap_lang_get("Reported "));
		
		
		if (timeDiff < 60)
			snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 roadmap_lang_get("%d seconds ago"), timeDiff);
		else if ((timeDiff > 60) && (timeDiff < 3600))
			snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 roadmap_lang_get("%d minutes ago"), timeDiff/60);
		else
			snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 roadmap_lang_get("%2.1f hours ago"), (float)timeDiff
					 /3600);
		
		if (alert->sReportedBy[0] != 0)
			snprintf(detail_str + strlen(detail_str), sizeof(detail_str)
					 - strlen(detail_str),
					 " %s %s",roadmap_lang_get("by"), alert->sReportedBy);

       gAlertListTable.type_str[iCount] = strdup(AlertStr);
       gAlertListTable.location_str[iCount] = strdup(location_str);
       gAlertListTable.detail[iCount] = strdup(detail_str);
       gAlertListTable.distance[iCount] = strdup(dist_str);
       gAlertListTable.unit[iCount] = strdup(unit_str);
       gAlertListTable.value[iCount] = strdup(str);
       gAlertListTable.icon[iCount] = strdup(RTAlerts_Get_Icon(alert->iID));
       if (RTAlerts_Has_Image(alert->iID))
          gAlertListTable.attachment[iCount] = strdup ("attachment");
       //else
       //	gAlertListTable.attachment[iCount] = "";
       gAlertListTable.type[iCount] = alert->iType;
       gAlertListTable.iDistnace[iCount] = distance;
       
       
       
       iCount++;
       iAlertCount--;
    }
   
	 gAlertListTable.iCount = iCount;
   
    roadmap_math_set_context(&context_save_pos, context_save_zoom);
}

static void free_list() {
	int i;
	
	for (i = 0; i < gAlertListTable.iCount; ++i) {
		if (gAlertListTable.type_str[i]) {
			free (gAlertListTable.type_str[i]);
			gAlertListTable.type_str[i] = NULL;
		}
      if (gAlertListTable.location_str[i]) {
			free (gAlertListTable.location_str[i]);
			gAlertListTable.location_str[i] = NULL;
		}
		if (gAlertListTable.detail[i]) {
			free (gAlertListTable.detail[i]);
			gAlertListTable.detail[i] = NULL;
		}
		if (gAlertListTable.distance[i]) {
			free (gAlertListTable.distance[i]);
			gAlertListTable.distance[i] = NULL;
		}
		if (gAlertListTable.unit[i]) {
			free (gAlertListTable.unit[i]);
			gAlertListTable.unit[i] = NULL;
		}
		if (gAlertListTable.value[i]) {
			free (gAlertListTable.value[i]);
			gAlertListTable.value[i] = NULL;
		}
		if (gAlertListTable.icon[i]) {
			free (gAlertListTable.icon[i]);
			gAlertListTable.icon[i] = NULL;
		}
		if (gAlertListTable.attachment[i]) {
			free (gAlertListTable.attachment[i]);
			gAlertListTable.attachment[i] = NULL;
		}
	}
	
	gAlertListTable.iCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget create_list(int tab_id){
	SsdWidget list;
	char tab_name[20];
	sprintf(tab_name, "list %d", tab_id);
	list = ssd_list_new (tab_name, SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, 0, NULL);
#ifdef IPHONE
	ssd_list_resize (list, 73);
#elif defined (TOUCH_SCREEN)	
	ssd_list_resize (list, 66);
#else
	ssd_list_resize (list, 63);
#endif
	
	return list;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_tab_gain_focus(int tab)
{
	switch (tab){
		case tab_all:
		    //populate_first_tab();
			//TODO: no need to filter if we want to show all
			populate_tab(tab_all,1,-1);
			break;
		case tab_police:
			populate_tab(tab_police,1,RT_ALERT_TYPE_POLICE);
			break;
		case tab_traffic_jam:
   			populate_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM);
   			break;
   		case tab_accidents:
   			populate_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT);
   			break;
   		case tab_others:
   			populate_tab(tab_others,5, RT_ALERT_TYPE_CHIT_CHAT, RT_ALERT_TYPE_HAZARD,
						 RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING);
   			break;
   			
			default:
				break;
	} 
	g_active_tab = tab;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void clear_lists(){
	int i;
      for (i=0; i< tab__count;i++){
	   	ssd_list_populate (tabs[i], 0, NULL, NULL,NULL, NULL,RealtimeAlertsListCallBackTabs, RealtimeAlertsDeleteCallBackTabs, TRUE);
      }
}


///////////////////////////////////////////////////////////////////////////////////////////
static int on_type_selected (SsdWidget widget, const char* selection,const void *value, void* context) {
	int type = (int) value - 10;
	
   /*
	RTAlerts_Sort_List(sort_proximity);
	 
	 RTAlerts_Stop_Scrolling();
	 
	 populate_list();
    */
	
	switch (type){
		case tab_all:
		    //populate_first_tab();
			populate_tab(tab_all,1,0);
			g_alert_type = -1;
			break;
		case tab_police:
			populate_tab(tab_police,1,RT_ALERT_TYPE_POLICE);
			g_alert_type = RT_ALERT_TYPE_POLICE;
			break;
		case tab_traffic_jam:
         populate_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM);
			g_alert_type = RT_ALERT_TYPE_TRAFFIC_JAM;
         break;
      case tab_accidents:
         populate_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT);
			g_alert_type = RT_ALERT_TYPE_ACCIDENT;
         break;
      case tab_others:
         populate_tab(tab_others,5, RT_ALERT_TYPE_CHIT_CHAT, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING);
			g_alert_type = RT_ALERT_TYPE_OTHER;
         break;
#ifdef STREAM_TEST
      case tab_stream:
         roadmap_radio();
         roadmap_main_show_root(0);
         return 1;
         
         break;
#endif //STREAM_TEST
         
		default:
			break;
	} 
		
	RealtimeAlertsListShow();
	
	return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
static void	show_types_list(){
	const char* tabs_titles[tab__count]; //for iPhone, these are not actually tabs...
	const char* tabs_icons[tab__count];
	const char* tabs_values[tab__count];
   const char* tabs_item_count[tab__count];
   char str_count[20];
   
   RTAlerts_Sort_List(sort_proximity);
   
   RTAlerts_Stop_Scrolling();
   
   populate_list();
	
	//careful with this list, must fill exactly tab__count number of items
	tabs_titles[tab_all] = roadmap_lang_get("All");
	tabs_icons[tab_all] = "report_list_all";
	tabs_values[tab_all] = (void *)(tab_all + 10); //TODO: find better id
   sprintf (str_count,"%d",count_tab(tab_all,1,0));
   tabs_item_count[tab_all] = strdup(str_count);
   
	tabs_titles[tab_police] = roadmap_lang_get("Police");
	tabs_icons[tab_police] = "report_list_police";
	tabs_values[tab_police] = (void *)(tab_police + 10);
   sprintf (str_count,"%d",count_tab(tab_police,1,RT_ALERT_TYPE_POLICE));
   tabs_item_count[tab_police] = strdup(str_count);
   
	tabs_titles[tab_traffic_jam] = roadmap_lang_get("Traffic");
	tabs_icons[tab_traffic_jam] = "report_list_loads";
	tabs_values[tab_traffic_jam] = (void *)(tab_traffic_jam + 10);
   sprintf (str_count,"%d",count_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM));
   tabs_item_count[tab_traffic_jam] = strdup(str_count);
   
	tabs_titles[tab_accidents] = roadmap_lang_get("Accidents");
	tabs_icons[tab_accidents] = "report_list_accidents";
	tabs_values[tab_accidents] = (void *)(tab_accidents + 10);
   sprintf (str_count,"%d",count_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT));
   tabs_item_count[tab_accidents] = strdup(str_count);
   
	tabs_titles[tab_others] = roadmap_lang_get("Other");
	tabs_icons[tab_others] = "report_list_other";
	tabs_values[tab_others] = (void *)(tab_others + 10);
   sprintf (str_count,"%d",count_tab(tab_others,5, RT_ALERT_TYPE_CHIT_CHAT, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING));
   tabs_item_count[tab_others] = strdup(str_count);

#ifdef STREAM_TEST
   tabs_titles[tab_stream] = roadmap_lang_get("Radio road reports");
	tabs_icons[tab_stream] = "radio";
	tabs_values[tab_stream] = (void *)(tab_stream + 10);
   tabs_item_count[tab_stream] = "";
#endif //STREAM_TEST

	roadmap_list_menu_generic("Events", tab__count, tabs_titles, (const void**)tabs_values, tabs_icons, tabs_item_count, 0, on_type_selected, NULL, NULL, NULL, NULL, 60, 0, 2);
	
   free ((char*)tabs_item_count[tab_all]);
   free ((char*)tabs_item_count[tab_police]);
   free ((char*)tabs_item_count[tab_traffic_jam]);
   free ((char*)tabs_item_count[tab_accidents]);
   free ((char*)tabs_item_count[tab_others]);
#ifdef STREAM_TEST
   //free ((char*)tabs_item_count[tab_stream]);
#endif //STREAM_TEST
}

///////////////////////////////////////////////////////////////////////////////////////////
void init_colors(){
	gColorType = [[UIColor alloc] initWithRed:1.0f green:0.45 blue:0.0f alpha:1.0f]; //orange
	//gColorDescription = [UIColor orangeColor];
	//gColorDescriptionSel = [UIColor colorWithRed:70.2f green:0.0f blue:0.0f alpha:1.0f]; //red
	//gColorCellSel = [UIColor colorWithRed:64.7f green:84.7f blue:0.0f alpha:1.0f]; //light green
	gColorUnit = [UIColor lightGrayColor];
	//gColorUnitSel = [UIColor colorWithRed:22.0f green:29.0f blue:0.0f alpha:1.0f]; //green
	
	
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsList(){
	if (!initialized) {
		init_colors();
		initialized = 1;
		gImageList.iCount = 0;
	} else {
		free_list(); //TODO: we better release the list once we are done with it...
	}
	show_types_list();
}




//////////////////////////////////////
////// OPTIONS

/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_delete(){
	char message[300];
	SsdWidget list = tabs[g_active_tab];
	const void *value = ssd_list_selected_value ( list);
	const char* string   = ssd_list_selected_string( list);
	
	if (value != NULL){
	 	
		 sprintf(message,"%s\n%s",roadmap_lang_get("Delete Alert:"),(const char *)string);
	   
	     ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value);
	} 
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
	 	SsdWidget list = tabs[g_active_tab];
	 	const void *value = ssd_list_selected_value ( list);
	 	if (value != NULL){
	 		    int alertId = atoi((const char*)value);
		       ssd_generic_list_dialog_hide_all();
      		  RTAlerts_Popup_By_Id(alertId);
     }
	 		
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show_comments(){
	 	SsdWidget list = tabs[g_active_tab];
	 	const void *value = ssd_list_selected_value ( list);
	 	if (value != NULL){
	 		    int alertId = atoi((const char*)value);
    			if (RTAlerts_Get_Number_of_Comments(alertId) != 0)
    			{
			        ssd_generic_list_dialog_hide();
        			  RealtimeAlertCommentsList(alertId);
    			}
     }
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
		SsdWidget list = tabs[g_active_tab];
	 	const void *value = ssd_list_selected_value ( list);
	 	if (value != NULL){
	 		    int alertId = atoi((const char*)value);
	 		    real_time_post_alert_comment_by_id(alertId);
	 	}
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_sort_proximity(){
	g_list_sorted =sort_proximity;
	RTAlerts_Sort_List(sort_proximity);
	clear_lists();
    populate_list();
    on_tab_gain_focus(g_active_tab);
}
 */
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_sort_recency(){
	g_list_sorted =sort_recency;
	RTAlerts_Sort_List(sort_recency);
	clear_lists();
    populate_list();
    on_tab_gain_focus(g_active_tab);
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)                                  
{
   real_time_list_context_menu_items   selection;
   int                                 exit_code = dec_ok;
   
   g_context_menu_is_active = FALSE;
   
   if( !made_selection)
      return;  
   
   selection = (real_time_list_context_menu_items)item->id;
 
   switch( selection)
   {
      case rtl_cm_show:
      	on_option_show();
         break;
         
      case rtl_cm_delete:
      	on_option_delete();
         break;
         
      case rtl_cm_view_comments:
      	on_option_show_comments();
         break;
         
      case rtl_cm_add_comments:
         on_option_add_comment();
         break;
         
      case rtl_cm_sort_proximity:
      on_option_sort_proximity();
         break;
         
      case rtl_cm_sort_recency:
      	on_option_sort_recency();
         break;
         
         
      case rtl_cm_exit:
         exit_code = dec_cancel;
      	ssd_dialog_hide_all( exit_code);   
      	roadmap_screen_refresh ();
         break;
      case rtl_cm_cancel:
	  	g_context_menu_is_active = FALSE;
	  	roadmap_screen_refresh ();
	  	break;	

      default:
         break;
   }
}
*/
/*
///////////////////////////////////////////////////////////////////////////////////////////

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
	int menu_x;
	BOOL has_comments = FALSE;
	BOOL is_empty = FALSE;
	BOOL is_selected = TRUE;
	SsdWidget list ;
	const void *value;
	BOOL add_cancel = FALSE;
	BOOL add_exit = TRUE;
	

#ifdef TOUCH_SCREEN
   add_cancel = TRUE;
   add_exit = FALSE;
   roadmap_screen_refresh();
#endif

   is_empty = ((g_active_tab == 0) && (gAlertListTable.iCount < 2)) || ((g_active_tab != 0) && (gTabAlertList.iCount < 1));
   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      g_context_menu_is_active = FALSE;
      return 0;
   }

	list = tabs[g_active_tab];
	value = ssd_list_selected_value ( list);
	if (value != NULL){
	 	int alertId = atoi((const char*)value);
    	if (RTAlerts_Get_Number_of_Comments(alertId) != 0)
    		has_comments = TRUE;
    		
	}
	else
		is_selected = FALSE;
	
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_show,
                              !is_empty && is_selected, 
                              FALSE);
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_add_comments,
                              !is_empty && is_selected, 
                              FALSE);
                              
                              
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_delete,
                              !is_empty && is_selected, 
                              FALSE);
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_view_comments,
                              !is_empty && is_selected && has_comments, 
                              FALSE);

   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_sort_proximity,
                              (g_list_sorted == sort_recency) && !is_empty, 
                              FALSE);
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_sort_recency,
                              (g_list_sorted == sort_proximity && !is_empty), 
                              FALSE);
                              
   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_exit,
                              add_exit, 
                              FALSE);

   ssd_contextmenu_show_item( &context_menu, 
                              rtl_cm_cancel,
                              add_cancel, 
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
                           dir_default); 
 
   g_context_menu_is_active = TRUE; 

   return 0;
}
 */

