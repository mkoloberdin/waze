/* RealtimeAlertsList.c - manage the Real Time Alerts list display
 *
 * LICENSE:
 *
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
#include "roadmap_groups.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_confirm_dialog.h"
#include "navigate/navigate_main.h"
#include "Realtime.h"
#include "RealtimeAlerts.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"
#include "RealtimeTrafficInfo.h"

#define ALERT_ROW_HEIGHT 120

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Show on map",        rtl_cm_show),
   SSD_CM_INIT_ITEM  ( "View image",         rtl_cm_view_image),
   SSD_CM_INIT_ITEM  ( "Report irrelevant",  rtl_cm_delete),
   SSD_CM_INIT_ITEM  ( "View comments",      rtl_cm_view_comments),
   SSD_CM_INIT_ITEM  ( "Add comment",        rtl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Sort by proximity",  rtl_cm_sort_proximity),
   SSD_CM_INIT_ITEM  ( "Sort by recency",    rtl_cm_sort_recency),
   SSD_CM_INIT_ITEM  ( "Report abuse",       rtl_cm_report_abuse),
   SSD_CM_INIT_ITEM  ( "On route",           rtl_cm_on_route),
   SSD_CM_INIT_ITEM  ( "My Groups",          rtl_cm_filter_group),
   SSD_CM_INIT_ITEM  ( "Show Group",        rtl_cm_show_group),
   SSD_CM_INIT_ITEM  ( "Around me",                rtl_cm_show_all),
   SSD_CM_INIT_ITEM  ( "Cancel",             rtl_cm_cancel),
};
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

static void populate_list();
static void set_softkeys( SsdWidget dialog);

static SsdWidget tabs[tab__count];
static AlertList gAlertListTable;
static AlertList gTabAlertList;

static   BOOL                             g_context_menu_is_active= FALSE;
static   real_time_tabs    g_active_tab = tab_all;
static   alert_sort_method g_list_sorted = sort_proximity;
static   alert_filter g_list_filter = filter_none;

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBack(SsdWidget widget, const char *new_value,
        const void *value, void *context)
{

    int alertId;

    alertId = atoi((const char*)value);
    if (RTAlerts_Get_Number_of_Comments(alertId) == 0)
    {
        ssd_generic_list_dialog_hide_all();
        RTAlerts_Popup_By_Id(alertId,FALSE);
    }
    else
    {
        ssd_generic_list_dialog_hide();
        RealtimeAlertCommentsList(alertId);
    }
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
        roadmap_messagebox_timeout("Delete Alert", "Your request was sent to the server",5);

}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsDeleteCallBack(SsdWidget widget, const char *new_value,
        const void *value, void *context)
{
   char message[200];


    sprintf(message,"%s\n%s",roadmap_lang_get("Delete Alert:"), new_value);

    ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value);

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertsListCallBackTabs(SsdWidget widget, const char *new_value,
        const void *value)
{

    int alertId;
    roadmap_screen_set_Xicon_state(FALSE);

    alertId = atoi((const char*)value);
    if (RTAlerts_Get_Number_of_Comments(alertId) == 0)
    {
        ssd_generic_list_dialog_hide_all();
        RTAlerts_Popup_By_Id(alertId,FALSE);
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


    sprintf(message,"%s\n%s",roadmap_lang_get("Delete Alert:"), new_value);

    ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value);

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_tab(int tab, int num, int types,...){
   int i;
   int count = 0;
   va_list ap;
   int type;
   SsdWidget widget;
   for ( i=0; i< gAlertListTable.iCount; i++){
      int j;
      va_start(ap, types);
      type = types;
      for (j = 0; j < num; j++ ) {

         if (gAlertListTable.type[i] == type) {
            gTabAlertList.labels[count] = gAlertListTable.labels[i];
            gTabAlertList.values[count] = gAlertListTable.values[i];
            gTabAlertList.icons[count] = gAlertListTable.icons[i];
            // Icons widgets
            widget = gAlertListTable.icons_widget[i];
            if ( widget && widget->parent )
            {
            	ssd_widget_remove( widget->parent, widget );
            }
            gTabAlertList.icons_widget[count] = widget;

            // Labels widgets
			widget = gAlertListTable.labels_widget[i];
            if ( widget && widget->parent )
            {
            	ssd_widget_remove( widget->parent, widget );
            }
			gTabAlertList.labels_widget[count] = widget;

            count++;
         }
         type = va_arg(ap, int);
      }
      va_end(ap);
    }
    gTabAlertList.iCount = count;
    ssd_widget_show(tabs[tab]);
    ssd_list_populate_widgets (tabs[tab], count, (const char **)&gTabAlertList.labels[0], &gTabAlertList.labels_widget[0], (const void **)&gTabAlertList.values[0],&gTabAlertList.icons_widget[0], NULL,RealtimeAlertsListCallBackTabs, RealtimeAlertsDeleteCallBackTabs, TRUE);

    if ((gAlertListTable.iCount == 0) && (g_list_filter == filter_on_route) && (navigate_main_state() == -1)){
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_show(msg);
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_hide(msg);
       ssd_widget_hide(tabs[tab]);
    }
    else if (RealTimeLoginState() && (roadmap_groups_get_num_following() == 0) && (g_list_filter == filter_group)){
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_show(msg);
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_hide(msg);
       ssd_widget_hide(tabs[tab]);
    }
    else{
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_hide(msg);
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_hide(msg);
    }


}
static void clear_List()
{
	int i;

	for ( i = 1; i <  gAlertListTable.iCount; i++ )
	{
		if ( gAlertListTable.icons[i] )
		{
			free( gAlertListTable.icons[i] );
            gAlertListTable.icons[i] = NULL;
		}
		if ( gAlertListTable.labels[i] )
		{
			free( gAlertListTable.labels[i] );
            gAlertListTable.labels[i] = NULL;
		}
		if ( gAlertListTable.values[i] )
		{
			free( gAlertListTable.values[i] );
            gAlertListTable.values[i] = NULL;
		}

		if ( gAlertListTable.icons_widget[i] )
		{
			ssd_widget_free( gAlertListTable.icons_widget[i], TRUE, TRUE );
			gAlertListTable.icons_widget[i] = NULL;
		}
		if ( gAlertListTable.labels_widget[i] )
		{
			ssd_widget_free( gAlertListTable.labels_widget[i], TRUE, TRUE );
			gAlertListTable.labels_widget[i] = NULL;
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////
static char *count_tab(int tab, int num, int types,...){
   int i;
   int count = 0;
   va_list ap;
   int type;
   static char str[5];

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
   sprintf(str,"%d", count);
   return &str[0];
}
///////////////////////////////////////////////////////////////////////////////////////////
static void set_counts(void){
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_all",count_tab(tab_all,1,0));
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_police",count_tab(tab_police,1,RT_ALERT_TYPE_POLICE));
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_loads",count_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM));
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_accidents",count_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT));
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_chit_chats",count_tab(tab_chit_chat,1, RT_ALERT_TYPE_CHIT_CHAT));
   ssd_menu_set_right_text(ALERTS_LIST_TITLE, "report_list_other",count_tab(tab_others,5, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING, RT_ALERT_TYPE_DYNAMIC));
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_list(){
    int iCount = -1;
    int iAlertCount;
    RTAlert *alert;
    char AlertStr[700];
    char TempStr[200];
    char str[100];
    int distance = -1;
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition position, current_pos;
    const RoadMapPosition *gps_pos;
    PluginLine line;
    int Direction;
    int distance_far;
    char dist_str[100];
    char unit_str[20];
    int i;
    RoadMapPosition context_save_pos;
    int context_save_zoom;
    SsdWidget icon_w, text_w, icon_attachment;
    SsdWidget icon_container;
    SsdWidget attachment_container;
    SsdSize  size_alert_icon;
    static SsdSize size_attach_icon = {-1, -1};
    int   image_container_width = 70;
    int   offset_y = 0;
    int   num_addOns;
    int   index = 0;
    const char *button[2];
    SsdWidget widget;
    SsdWidget text;
    int group_flag = 0;

    for (i=0; i<MAX_ALERTS_ENTRIES; i++)
    {
        gAlertListTable.icons[i] = NULL;
    }

    iCount = 0;

    iAlertCount = RTAlerts_Count();

    if ( iAlertCount > 0 )
    {
    	clear_List();
    }

    roadmap_math_get_context(&context_save_pos, &context_save_zoom);

    while ((iAlertCount > 0) && (iCount < MAX_ALERTS_ENTRIES))    {

        widget = ssd_container_new("list", "list_row", SSD_MIN_SIZE, SSD_MAX_SIZE, 0);
        ssd_widget_set_color(widget, NULL, NULL);

        AlertStr[0] = 0;
        str[0] = 0;

        alert = RTAlerts_Get(index);

        if (g_list_filter == filter_on_route){
           if (!RTAlerts_isAlertOnRoute (alert->iID)){
              iAlertCount--;
              index++;
              continue;
           }
        }
        else  if (g_list_filter == filter_group){
           if (alert->iGroupRelevance == GROUP_RELEVANCE_NONE){
              iAlertCount--;
              index++;
              continue;
           }
        }
        position.longitude = alert->iLongitude;
        position.latitude = alert->iLatitude;

        roadmap_math_set_context((RoadMapPosition *)&position, 20);

        if (alert->iType == RT_ALERT_TYPE_ACCIDENT)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                    - strlen(AlertStr), "%s", roadmap_lang_get("Accident"));
        else if (alert->iType == RT_ALERT_TYPE_POLICE)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                    - strlen(AlertStr), "%s", roadmap_lang_get("Police trap"));
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
        else if ((alert->iType == RT_ALERT_TYPE_DYNAMIC) && (alert->pAlertTitle))
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                 - strlen(AlertStr), "%s", roadmap_lang_get(alert->pAlertTitle));
        else
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                    - strlen(AlertStr), "%s",
                    roadmap_lang_get("Chit chat"));

        text = ssd_text_new("alert_txt", AlertStr, 16, SSD_END_ROW);
        ssd_widget_set_color(text,"#f77507", NULL);
        AlertStr[0] = 0;
        ssd_widget_add(widget, text);
        if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction)
                == -1)
        {
            gps_pos = roadmap_trip_get_position("Location");
            if ((gps_pos != NULL) && !IS_DEFAULT_LOCATION( gps_pos ) ){
               current_pos.latitude = gps_pos->latitude;
               current_pos.longitude = gps_pos->longitude;
            }
            else
            {
               // check the distance to the alert
               gps_pos = roadmap_trip_get_position("GPS");
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
        }
        else
        {
            current_pos.latitude = CurrentPosition.latitude;
            current_pos.longitude = CurrentPosition.longitude;
        }

        if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
        {
            int tenths;
            distance = roadmap_math_distance(&current_pos, &position);
            distance_far = roadmap_math_to_trip_distance(distance);

            tenths = roadmap_math_to_trip_distance_tenths(distance);
            snprintf(dist_str, sizeof(str), "%d.%d", distance_far, tenths
                     % 10);
            snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_lang_get(roadmap_math_trip_unit()));

        }

        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "\n%s", alert->sLocationStr);

        text = ssd_text_new("alert_txt", AlertStr, 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
        ssd_widget_set_color(text,"#f77507", NULL);
        AlertStr[0] = 0;
        ssd_widget_add(widget, text);

        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "\n%s", alert->sDescription);

        ssd_dialog_add_vspace(widget, 5
              , 0);
        text = ssd_text_new("alert_txt", AlertStr, 14, SSD_END_ROW);
        AlertStr[0] = 0;
        ssd_widget_add(widget, text);


        if (alert->sGroup[0] == 0)
           group_flag = SSD_ALIGN_BOTTOM;
        else{
           group_flag = 0;
        }

        if (group_flag != 0){
           char tempStr1[200];
           char tempStr2[200];
           AlertStr[0] = 0;
           tempStr1[0] = 0;
           tempStr2[0] = 0;
           RTAlerts_get_report_info_str(alert,tempStr1,sizeof(tempStr1));

           if (alert->sReportedBy[0] != 0){
              RTAlerts_get_reported_by_string(alert,&tempStr2[0],sizeof(tempStr2));
           }
           snprintf(AlertStr, sizeof(AlertStr), "%s %s", tempStr1, tempStr2);
           text = ssd_text_new("reported_by_info", AlertStr, 12,SSD_TEXT_NORMAL_FONT|group_flag);
           ssd_widget_add(widget, text);
        }
        else{
           AlertStr[0] = 0;
           snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr), "\n %s: %s", roadmap_lang_get("group"), alert->sGroup);
           text = ssd_text_new("GroupStr", AlertStr, 14,SSD_ALIGN_BOTTOM);
           ssd_widget_add(widget, text);
        }
        snprintf(str, sizeof(str), "%d", alert->iID);

        //      if (labels[iCount]) free (labels[iCount]);
        gAlertListTable.labels[iCount] = strdup(str);

        gAlertListTable.values[iCount] = strdup( str );
        gAlertListTable.icons[iCount] = strdup(RTAlerts_Get_Icon(alert->iID));

        if ( roadmap_screen_is_hd_screen() )
        {
			image_container_width = 110;
			offset_y = 10;
        }

        /* Persistent widget - can be released only when forced */
        icon_container = ssd_container_new ("alert_icon_container", NULL, image_container_width,  SSD_MAX_SIZE,  SSD_PERSISTENT|SSD_ALIGN_VCENTER );
        ssd_widget_set_color(icon_container, NULL, NULL);
        button[0] = strdup(RTAlerts_Get_Icon(alert->iID));
        button[1] = NULL;

        icon_w =  ssd_bitmap_new ("icon", RTAlerts_Get_Icon(alert->iID), SSD_ALIGN_CENTER|SSD_ALIGN_CENTER);
        ssd_widget_get_size( icon_w, &size_alert_icon, NULL );
        //ssd_widget_set_offset(icon_w, 0, offset_y);
        ssd_widget_add(icon_container, icon_w);


        num_addOns = RTAlerts_Get_Number_Of_AddOns(alert->iID);
        for (i = 0 ; i < num_addOns; i++){
           const char *AddOn = RTAlerts_Get_AddOn(alert->iID, i);
           if (AddOn)
              ssd_bitmap_add_overlay(icon_w,AddOn);
        }

        if ( size_attach_icon.width == -1 )
        {
            icon_attachment =  ssd_bitmap_new ( "Image Attachment", "attachment", SSD_END_ROW );
            ssd_widget_get_size( icon_attachment, &size_attach_icon, NULL );
            ssd_widget_free( icon_attachment, TRUE, FALSE );
        }

        // Add attachment container always.Preserve the icon height
        /* Persistent widget - can be released only when forced */
        ssd_widget_get_size( icon_w, &size_alert_icon, NULL );
        attachment_container = ssd_container_new ( "atachment_icon_container", NULL, size_attach_icon.width,
                       size_alert_icon.height,  SSD_END_ROW );
        ssd_widget_set_color( attachment_container, NULL, NULL);
        // Bitmap is added if image is attached
        if ( alert->sImageIdStr[0] )
        {
        	icon_attachment =  ssd_bitmap_new ( "Image Attachment", "attachment", SSD_END_ROW );
            ssd_widget_add( attachment_container, icon_attachment );
        }
        // Add container in any case for the proper alignment.
        ssd_widget_add( icon_container, attachment_container );

        text_w = ssd_text_new ("Alert Distance", dist_str, 16, SSD_ALIGN_CENTER|SSD_END_ROW);
        ssd_widget_set_offset(text_w, 0, offset_y);
        ssd_widget_add(icon_container, text_w);

        text_w = ssd_text_new ("Alert Distance units", unit_str, 12,SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT|SSD_END_ROW);
        ssd_widget_set_offset(text_w, 0, offset_y);
        ssd_widget_add(icon_container, text_w);

        if (alert->sGroup[0] != 0){
           icon_w =  ssd_bitmap_new ("group_icon",alert->sGroupIcon, SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM);
           ssd_widget_add(icon_container, icon_w);
        }

        gAlertListTable.icons_widget[iCount] = icon_container;
        gAlertListTable.labels_widget[iCount] = widget;

        gAlertListTable.type[iCount] = alert->iType;
        gAlertListTable.iDistnace[iCount] = distance;
        iCount++;
        iAlertCount--;
        index++;
    }

    gAlertListTable.iCount = iCount;

    roadmap_math_set_context(&context_save_pos, context_save_zoom);
}

///////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget create_list(int tab_id){
   SsdWidget list;
   char tab_name[20];
   sprintf(tab_name, "list %d", tab_id);
   list = ssd_list_new (tab_name, SSD_MIN_SIZE, SSD_MIN_SIZE, inputtype_none, 0, NULL);
   ssd_widget_set_color(list, "#ffffff", "#000000");
   ssd_list_resize (list, ALERT_ROW_HEIGHT);

   return list;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void populate_first_tab(){
	int i;
	SsdWidget widget;
	/*
	 * Update the parent of the widget that it pass to another list
	 */
    for (i = 0; i < gAlertListTable.iCount; i++ )
    {
		  // Icons widgets
          widget = gAlertListTable.icons_widget[i];
          if ( widget && widget->parent )
          {
          	ssd_widget_remove( widget->parent, widget );
          }

          // Labels widgets
		  widget = gAlertListTable.labels_widget[i];
          if ( widget && widget->parent )
          {
          	ssd_widget_remove( widget->parent, widget );
          }
    }
    ssd_widget_show(tabs[0]);
    ssd_list_populate_widgets (tabs[0], gAlertListTable.iCount, (const char **)&gAlertListTable.labels[0], &gAlertListTable.labels_widget[0], (const void **)&gAlertListTable.values[0],&gAlertListTable.icons_widget[0], NULL,RealtimeAlertsListCallBackTabs, RealtimeAlertsDeleteCallBackTabs, TRUE);
    if ((gAlertListTable.iCount == 0) && (g_list_filter == filter_on_route) && (navigate_main_state() == -1)){
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_show(msg);
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_hide(msg);
       ssd_widget_hide(tabs[0]);
    }
    else if (RealTimeLoginState() && (roadmap_groups_get_num_following() == 0) && (g_list_filter == filter_group)){
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_show(msg);
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_hide(msg);
       ssd_widget_hide(tabs[0]);
    }
    else{
       SsdWidget dialog;
       SsdWidget msg;
       dialog = ssd_dialog_get_currently_active();
       msg = ssd_widget_get(dialog, "no_group_msg");
       if (msg)
          ssd_widget_hide(msg);
       msg = ssd_widget_get(dialog, "no_route_msg");
       if (msg)
          ssd_widget_hide(msg);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_tab_gain_focus(int tab)
{
   switch (tab){
      case tab_all:
          populate_first_tab();
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
      case tab_chit_chat:
            populate_tab(tab_chit_chat,1, RT_ALERT_TYPE_CHIT_CHAT);
            break;
      case tab_others:
            populate_tab(tab_others,5, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING, RT_ALERT_TYPE_DYNAMIC);
            break;

      default:
            break;
   }
   g_active_tab = tab;
}


///////////////////////////////////////////////////////////////////////////////////////////
#ifndef TOUCH_SCREEN
static SsdTcCtx   create_tab_control(){
   SsdTcCtx tabcontrol;
   const char* tabs_titles[tab__count];
   int i;

   tabs_titles[tab_all] = roadmap_lang_get("All");
   tabs_titles[tab_police] = roadmap_lang_get("Police traps");
   tabs_titles[tab_traffic_jam] = roadmap_lang_get("Traffic jams");
   tabs_titles[tab_accidents] = roadmap_lang_get("Accidents");
   tabs_titles[tab_chit_chat] = roadmap_lang_get("Chit Chats");
   tabs_titles[tab_others] = roadmap_lang_get("Others");

   for (i=0; i< tab__count;i++)
      tabs[i] = create_list(i);



    RTAlerts_Sort_List(sort_proximity);

    RTAlerts_Stop_Scrolling();

    populate_list();

   tabcontrol = ssd_tabcontrol_new(
                              ALERTS_LIST_TITLE,
                              roadmap_lang_get(ALERTS_LIST_TITLE),
                              NULL,
                              NULL,
                              on_tab_gain_focus,
                              NULL,
                              tabs_titles,
                              tabs,
                              tab__count,
                              0);
    set_softkeys( ssd_tabcontrol_get_dialog(tabcontrol));

    populate_first_tab();

    return tabcontrol;
}


///////////////////////////////////////////////////////////////////////////////////////////
static SsdTcCtx  get_tabcontrol()
{
   static SsdTcCtx   tabcontrol = NULL;

   if( tabcontrol){
      g_list_sorted = sort_proximity;
      g_list_filter = filter_none;
      RTAlerts_Sort_List(sort_proximity);
      RTAlerts_Stop_Scrolling();
      populate_list();
      populate_first_tab();
      ssd_tabcontrol_set_active_tab(tabcontrol, tab_all);

      return tabcontrol;
   }
   else{
      tabcontrol =  create_tab_control();
      return tabcontrol;
   }
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////
static void create_lists(){
   const char* tabs_titles[tab__count];
   int i;

   tabs_titles[tab_all] = roadmap_lang_get("All");
   tabs_titles[tab_police] = roadmap_lang_get("Police traps");
   tabs_titles[tab_traffic_jam] = roadmap_lang_get("Traffic jams");
   tabs_titles[tab_accidents] = roadmap_lang_get("Accidents");
   tabs_titles[tab_chit_chat] = roadmap_lang_get("Chit Chats");
   tabs_titles[tab_others] = roadmap_lang_get("Others");

   for (i=0; i< tab__count;i++)
      tabs[i] = create_list(i);

    RTAlerts_Sort_List(sort_proximity);

    RTAlerts_Stop_Scrolling();

    populate_list();

    populate_first_tab();
}

static void report_list(){

    RTAlerts_Stop_Scrolling();
   RTAlerts_Sort_List(sort_proximity);

   /*
    * AGA TODO :: Add configuration if create the list all the time
    */
   // populate_list();
   create_lists();

//   if (tabs[0] == NULL){
//      populate_list();
//      create_lists();
//   }else{
//      populate_list();
//   }
}

int on_button_show_all (SsdWidget widget, const char *new_value){
   SsdWidget button;
   g_list_filter = filter_none;
   populate_list();
   on_tab_gain_focus(g_active_tab);
   ssd_button_disable(widget);
   button = ssd_widget_get(widget->parent,"On_route_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"Groups_button");
   ssd_button_enable(button);
   return 1;
}

int on_button_show_on_route (SsdWidget widget, const char *new_value){
   SsdWidget button;
   g_list_filter = filter_on_route;
   populate_list();
   on_tab_gain_focus(g_active_tab);
   ssd_button_disable(widget);
   button = ssd_widget_get(widget->parent,"All_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"Groups_button");
   ssd_button_enable(button);
   return 1;
}

int on_button_show_group (SsdWidget widget, const char *new_value){
   SsdWidget button;

   g_list_filter = filter_group;
   populate_list();
   on_tab_gain_focus(g_active_tab);
   ssd_button_disable(widget);
   button = ssd_widget_get(widget->parent,"All_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"On_route_button");
   ssd_button_enable(button);
   return 1;
}
int on_menu_button_show_all (SsdWidget widget, const char *new_value){
   SsdWidget button;
   g_list_filter = filter_none;
   populate_list();
   set_counts();
   ssd_button_disable(widget);
   button = ssd_widget_get(widget->parent,"On_route_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"Groups_button");
   ssd_button_enable(button);
   return 1;
}

int on_menu_button_show_on_route (SsdWidget widget, const char *new_value){
   SsdWidget button;
   g_list_filter = filter_on_route;
   populate_list();
   set_counts();
   ssd_button_disable(widget);
   button = ssd_widget_get(widget->parent,"All_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"Groups_button");
   ssd_button_enable(button);
   return 1;
}

int on_menu_button_show_group (SsdWidget widget, const char *new_value){
   SsdWidget button;
   g_list_filter = filter_group;
   ssd_button_disable(widget);
   populate_list();
   set_counts();
   button = ssd_widget_get(widget->parent,"All_button");
   ssd_button_enable(button);
   button = ssd_widget_get(widget->parent,"On_route_button");
   ssd_button_enable(button);
   return 1;
}


static void add_no_route_msg(SsdWidget dialog){
   SsdWidget group, group2;
   SsdWidget text;

   group = ssd_container_new ("no_route_msg", NULL,
            SSD_MAX_SIZE, SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color(group, NULL, NULL);

   group2 = ssd_container_new ("no_route_msg.txt_container", NULL,
            SSD_MIN_SIZE, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(group2, NULL, NULL);

   text = ssd_text_new ("empty list text", roadmap_lang_get("No route"), 22,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_text_set_color(text, "#24323a");
   ssd_widget_add (group2, text);
   ssd_dialog_add_vspace(group2, 20, 0);
   text = ssd_text_new ("empty list text", roadmap_lang_get("You should be in navigation mode to view events on your route"), 18,SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#24323a");
   ssd_widget_add (group2, text);
   ssd_widget_add( group, group2);
   ssd_widget_add( dialog, group);
   ssd_widget_hide(group);

}

static void add_no_group_msg(SsdWidget dialog){
   SsdWidget group, group2;
   SsdWidget text;

   group = ssd_container_new ("no_group_msg", NULL,
            SSD_MAX_SIZE, SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_set_color(group, NULL, NULL);

   group2 = ssd_container_new ("no_group_msg.txt_container", NULL,
            SSD_MIN_SIZE, SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(group2, NULL, NULL);

   text = ssd_text_new ("no_group_msg text1", roadmap_lang_get("No groups"), 22,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_text_set_color(text, "#24323a");
   ssd_widget_add (group2, text);
   ssd_dialog_add_vspace(group2, 20, 0);
   text = ssd_text_new ("no_group_msg text2", roadmap_lang_get("You're not following any groups - but you should!"), 16,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_text_set_color(text, "#24323a");
   ssd_widget_add (group2, text);
   ssd_dialog_add_vspace(group2, 5, SSD_END_ROW);
   text = ssd_text_new ("no_group_msg text2", roadmap_lang_get("Go to 'Groups' in main menu to find or create a group"), 16,SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#24323a");
   ssd_widget_add (group2, text);
   ssd_widget_add( group, group2);
   ssd_widget_add( dialog, group);
   ssd_widget_hide(group);

}

static void show_list(const char *name, const char *title, SsdWidget list){
   SsdWidget dialog;
   SsdWidget filter_container;
   SsdWidget button, button2, button3;
   SsdWidget container, text;


   dialog = ssd_dialog_new (name,
                            title,
                            NULL,
                            SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
    container = ssd_container_new ("buttons_cont", NULL, SSD_MIN_SIZE,  SSD_MIN_SIZE,  SSD_ALIGN_CENTER |SSD_ALIGN_VCENTER);
    ssd_widget_set_color(container, NULL, NULL);
    filter_container = ssd_container_new ("conatiner", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                         SSD_WIDGET_SPACE|SSD_END_ROW|SSD_POINTER_NONE);
    ssd_widget_set_color(filter_container,NULL, NULL);
    button = ssd_button_label("All_button", roadmap_lang_get("Around me"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_button_show_all);
    ssd_widget_add(container, button);
    ssd_dialog_add_hspace(container, 5, 0);
    button2 = ssd_button_label("On_route_button", roadmap_lang_get("On route"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_button_show_on_route);
    ssd_widget_add(container, button2);
    ssd_dialog_add_hspace(container, 5, 0);
    button3 = ssd_button_label("Groups_button", roadmap_lang_get("My Groups"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_button_show_group);
    ssd_widget_add(container, button3);

    if (g_list_filter == filter_on_route){
       text = ssd_widget_get(button2, "label");
       if (text)
          ssd_widget_set_color(text, "#c0c0c0", "#c0c0c0");
    }
    else if (g_list_filter == filter_none){
       text = ssd_widget_get(button, "label");
       if (text)
          ssd_widget_set_color(text, "#c0c0c0", "#c0c0c0");
    }else{
       text = ssd_widget_get(button3, "label");
       if (text)
          ssd_widget_set_color(text, "#c0c0c0", "#c0c0c0");
    }

    ssd_widget_add(filter_container,container);
    ssd_widget_add(dialog, filter_container);

#endif
   ssd_widget_add(dialog, list);
   add_no_route_msg(dialog);
   add_no_group_msg(dialog);
   if (gAlertListTable.iCount == 0)
      ssd_widget_hide(list);

   if ((gAlertListTable.iCount == 0) && (g_list_filter == filter_on_route) && (navigate_main_state() == -1)){
      SsdWidget msg;
      msg = ssd_widget_get(dialog, "no_route_msg");
      if (msg)
         ssd_widget_show(msg);
      msg = ssd_widget_get(dialog, "no_group_msg");
      if (msg)
         ssd_widget_hide(msg);
   }
   else if (RealTimeLoginState() && (roadmap_groups_get_num_following() == 0) && (g_list_filter == filter_group)){
      SsdWidget msg;
      msg = ssd_widget_get(dialog, "no_group_msg");
      if (msg)
         ssd_widget_show(msg);
      msg = ssd_widget_get(dialog, "no_route_msg");
      if (msg)
         ssd_widget_hide(msg);
   }
   else{
      SsdWidget dialog;
      SsdWidget msg;
      dialog = ssd_dialog_get_currently_active();
      msg = ssd_widget_get(dialog, "no_group_msg");
      if (msg)
         ssd_widget_hide(msg);
      msg = ssd_widget_get(dialog, "no_route_msg");
      if (msg)
         ssd_widget_hide(msg);
   }

   set_softkeys(dialog);
   ssd_dialog_activate(name, NULL);

}

void report_list_all(void){
   SsdWidget list;

   report_list();
   populate_first_tab();
   list = tabs[tab_all];
   g_active_tab = tab_all;

   show_list("report_list_all",
             roadmap_lang_get ("All"),
             list);
}

void report_list_police(void){
   SsdWidget list;

   report_list();
   populate_tab(tab_police,1,RT_ALERT_TYPE_POLICE);
   list = tabs[tab_police];
   g_active_tab = tab_police;
   show_list ("report_list_police",
               roadmap_lang_get ("Police traps"),
            list);
}

void report_list_loads(void){
   SsdWidget list;

   report_list();
   populate_tab(tab_traffic_jam,2, RT_ALERT_TYPE_TRAFFIC_INFO, RT_ALERT_TYPE_TRAFFIC_JAM);
   list = tabs[tab_traffic_jam];
   g_active_tab = tab_traffic_jam;
   show_list ("report_list_loads",
               roadmap_lang_get ("Traffic"),
            list);
}

void report_list_accidents(void){
   SsdWidget list;

   report_list();
   populate_tab(tab_accidents,1, RT_ALERT_TYPE_ACCIDENT);
   list = tabs[tab_accidents];
   g_active_tab = tab_accidents;
   show_list ("report_list_accidents",
               roadmap_lang_get ("Accidents"),
            list);
}

void report_list_chit_chats(void){
   SsdWidget list;

   report_list();
   populate_tab(tab_chit_chat,1, RT_ALERT_TYPE_CHIT_CHAT);
   list = tabs[tab_chit_chat];
   g_active_tab = tab_chit_chat;
   show_list ("report_list_chit_chats",
               roadmap_lang_get ("Chit Chats"),
            list);
}
void report_list_other(void){
   SsdWidget list;

   report_list();
   populate_tab(tab_others,5, RT_ALERT_TYPE_HAZARD,RT_ALERT_TYPE_OTHER, RT_ALERT_TYPE_CONSTRUCTION, RT_ALERT_TYPE_PARKING, RT_ALERT_TYPE_DYNAMIC);
   list = tabs[tab_others];
   g_active_tab = tab_others;
   show_list ("report_list_other",
               roadmap_lang_get ("Other"),
            list);
}

RoadMapAction RoadMapAlertListActions[20] = {

   {"report_list_all", "All", "All reports", NULL,
      "All reports", report_list_all},

   {"report_list_police", "Police traps", "Police traps", NULL,
      "Police traps", report_list_police},

   {"report_list_loads", "Traffic", "Traffic", NULL,
      "Traffic", report_list_loads},

   {"report_list_accidents", "Accidents", "Accidents", NULL,
      "Accidents", report_list_accidents},

   {"report_list_chit_chats", "Chit Chats", "Chit Chats", NULL,
         "Chit Chats", report_list_chit_chats},

   {"report_list_other", "Other", "Other", NULL,
      "Other", report_list_other}
};


static SsdWidget create_filter_container(void){
   static SsdWidget filter_container = NULL;
   SsdWidget button, button2, button3;
   SsdWidget container, text;

   if (filter_container)
      return filter_container;

   container = ssd_container_new ("buttons_cont", NULL, SSD_MIN_SIZE,  SSD_MIN_SIZE,  SSD_ALIGN_CENTER |SSD_ALIGN_VCENTER);
   ssd_widget_set_color(container, NULL, NULL);
   filter_container = ssd_container_new ("conatiner", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                        SSD_WIDGET_SPACE|SSD_END_ROW|SSD_POINTER_NONE);
   ssd_widget_set_color(filter_container,NULL, NULL);
   button = ssd_button_label("All_button", roadmap_lang_get("Around me"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_menu_button_show_all);
   ssd_widget_add(container, button);
   ssd_dialog_add_hspace(container, 5, 0);
   button2 = ssd_button_label("On_route_button", roadmap_lang_get("On route"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_menu_button_show_on_route);
   ssd_widget_add(container, button2);
   ssd_dialog_add_hspace(container, 5, 0);
   button3 = ssd_button_label("Groups_button", roadmap_lang_get("My Groups"), SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, on_menu_button_show_group);
   ssd_widget_add(container, button3);


   ssd_widget_add(filter_container,container);

   return filter_container;
}


static void ShowListMenu(){
   SsdWidget filter_container;
#ifndef TOUCH_SCREEN
   SsdTcCtx tabcontrol;
#endif
   if (ssd_dialog_is_currently_active()) {
      if (!strcmp(ssd_dialog_currently_active_name(),ALERTS_LIST_TITLE )){
         ssd_dialog_hide_current(dec_close);
         roadmap_screen_refresh ();
         return;
      }
   }

#ifdef TOUCH_SCREEN
  filter_container = create_filter_container();
  ssd_menu_activate (ALERTS_LIST_TITLE, "report_list", NULL, filter_container, NULL,
                             RoadMapAlertListActions,SSD_CONTAINER_TITLE);

  ssd_button_enable(ssd_widget_get(filter_container, "All_button"));
  ssd_button_enable(ssd_widget_get(filter_container, "On_route_button"));
  ssd_button_enable(ssd_widget_get(filter_container, "Groups_button"));

  if (g_list_filter == filter_group)
     ssd_widget_set_color(ssd_widget_get(ssd_widget_get(filter_container, "Groups_button"),"label"), "#c0c0c0", "#c0c0c0");
  else if (g_list_filter == filter_on_route)
     ssd_widget_set_color(ssd_widget_get(ssd_widget_get(filter_container, "On_route_button"),"label"), "#c0c0c0", "#c0c0c0");
  else
     ssd_widget_set_color(ssd_widget_get(ssd_widget_get(filter_container, "All_button"),"label"), "#c0c0c0", "#c0c0c0");

  populate_list();
  set_counts();
#else
   tabcontrol =  get_tabcontrol();
   ssd_tabcontrol_show(tabcontrol);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsList(){
   g_list_filter = filter_none;
   ShowListMenu();
}

///////////////////////////////////////////////////////////////////////////////////////////
void RealtimeAlertsListGroup(void){
   g_list_filter = filter_group;
   ShowListMenu();

   if (RealTimeLoginState() && (roadmap_groups_get_num_following() == 0)){
      report_list_all();
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_delete(){
   char message[300];
   SsdWidget list = tabs[g_active_tab];
   const void *value = ssd_list_selected_value ( list);
   const char* string   = ssd_list_selected_string( list);
   int alertId = atoi((const char*)string);


   if (value != NULL){

       sprintf(message,"%s\n%s",roadmap_lang_get("Delete Alert:"),(const char *)RTAlerts_Get_LocationStrByID(alertId));

       ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)value);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
      SsdWidget list = tabs[g_active_tab];
      const void *value = ssd_list_selected_value ( list);
      if (value != NULL){
         int alertId = atoi((const char*)value);
         ssd_generic_list_dialog_hide_all();
         RTAlerts_Popup_By_Id(alertId,FALSE);
     }

}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_view_image(){
      SsdWidget list = tabs[g_active_tab];
      const void *value = ssd_list_selected_value ( list);
      if (value != NULL){
         int alertId = atoi((const char*)value);
        RTAlerts_Download_Image( alertId );
     }

}

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

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
   SsdWidget list = tabs[g_active_tab];
   const void *value = ssd_list_selected_value ( list);
   if (value != NULL){
      int alertId = atoi((const char*)value);
      real_time_post_alert_comment_by_id(alertId);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse_confirm_dlg_callback(int exit_code, void *context){
   if (exit_code == dec_yes){
      int alertId = atoi((const char*)context);
      Realtime_ReportAbuse(alertId, -1);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_report_abuse(){
   SsdWidget list = tabs[g_active_tab];
   const void *value = ssd_list_selected_value ( list);
   if (value != NULL){
      ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), FALSE,report_abuse_confirm_dlg_callback, (void *)value, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_sort_proximity(){
   g_list_sorted =sort_proximity;
   RTAlerts_Sort_List(sort_proximity);
   populate_list();
   on_tab_gain_focus(g_active_tab);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_sort_recency(){
   g_list_sorted =sort_recency;
   RTAlerts_Sort_List(sort_recency);
   populate_list();
   on_tab_gain_focus(g_active_tab);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_filter_none(){
   g_list_filter = filter_none;
   populate_list();
   on_tab_gain_focus(g_active_tab);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_filter_on_route(){
   g_list_filter = filter_on_route;
   populate_list();
   on_tab_gain_focus(g_active_tab);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_filter_group(){
   g_list_filter = filter_group;
   populate_list();
   on_tab_gain_focus(g_active_tab);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_open_group(){
   SsdWidget list = tabs[g_active_tab];
   const void *value = ssd_list_selected_value ( list);
   if (value != NULL){
      RTAlert *pAlert;
      int alertId = atoi((const char*)value);
      pAlert = RTAlerts_Get_By_ID(alertId);
      if (pAlert){
         roadmap_groups_show_group(pAlert->sGroup);
      }
   }

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
      case rtl_cm_show:
         on_option_show();
         break;

      case rtl_cm_view_image:
         on_option_view_image();
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
      case rtl_cm_report_abuse:
         on_option_report_abuse();
         break;

      case rtl_cm_show_all:
         on_option_filter_none();
         break;

      case rtl_cm_on_route:
         on_option_filter_on_route();
         break;

      case rtl_cm_filter_group:
         on_option_filter_group();
         break;

      case rtl_cm_show_group:
         on_option_open_group();
         break;

      case rtl_cm_cancel:
         g_context_menu_is_active = FALSE;
         roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   BOOL has_comments = FALSE;
   BOOL is_empty = FALSE;
   BOOL is_selected = TRUE;
   BOOL is_auto_jam = FALSE;
   SsdWidget list ;
   const void *value;
   BOOL add_cancel = FALSE;
   BOOL add_image_view = FALSE;
   BOOL alert_by_me = FALSE;
   BOOL is_touch = FALSE;
   BOOL can_show_group = FALSE;


#ifdef TOUCH_SCREEN
   add_cancel = TRUE;
   is_touch = TRUE;
   roadmap_screen_refresh();
#endif

   can_show_group = roadmap_groups_display_groups_supported();

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
      const char *group_name;
      int alertId = atoi((const char*)value);
      if (RTAlerts_Get_Number_of_Comments(alertId) != 0)
         has_comments = TRUE;

      alert_by_me = RTAlerts_isByMe(alertId);
      add_image_view = RTAlerts_Has_Image( alertId );

      if (RTAlerts_Get_Type_By_Id(alertId) == RT_ALERT_TYPE_TRAFFIC_INFO)
         is_auto_jam = TRUE;

      group_name = RTAlerts_Get_GroupName_By_Id(alertId);
      if (group_name && *group_name)
         can_show_group &= TRUE;
      else
         can_show_group = FALSE;
   }
   else
      is_selected = FALSE;

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_show,
                              !is_empty && is_selected,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_view_image,
                              !is_empty && is_selected && add_image_view,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_add_comments,
                              !is_empty && is_selected && !is_auto_jam ,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_report_abuse,
                              !is_empty && is_selected && !is_auto_jam && !alert_by_me,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_delete,
                              !is_empty && is_selected,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_view_comments,
                              !is_empty && is_selected && has_comments ,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_sort_proximity,
                              (g_list_sorted == sort_recency) && !is_empty ,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_sort_recency,
                              (g_list_sorted == sort_proximity && !is_empty),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_show_all,
                              (g_list_filter != filter_none && !is_touch),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_on_route,
                              (g_list_filter != filter_on_route && !is_empty & !is_touch),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_filter_group,
                              (g_list_filter != filter_group && !is_empty && !is_touch ),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              rtl_cm_show_group,
                              !is_empty && is_selected && can_show_group,
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
                           dir_default,
                           0,
                           TRUE);

   g_context_menu_is_active = TRUE;

   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void set_softkeys( SsdWidget dialog)
{
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_options);
}
