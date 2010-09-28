/*
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_groups.h"
#include "roadmap_browser.h"
#include "roadmap_canvas.h"
#include "roadmap_bar.h"
#include "roadmap_res.h"
#include "roadmap_message_ticker.h"
#include "roadmap_plugin.h"
#include "roadmap_navigate.h"

#include "roadmap_trip.h"
#include "roadmap_res_download.h"
#include "roadmap_messagebox.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertsList.h"
#include "roadmap_start.h"
#include "ssd/ssd_list.h"

#define MAX_GROUPS 200

//////////////////////////////////////////////////////////////////////////////////////////////////
static RoadMapConfigDescriptor RoadMapConfigGroupsPopUpReports =
      ROADMAP_CONFIG_ITEM("Groups", "Pop-up reports");

static RoadMapConfigDescriptor RoadMapConfigGroupsShowWazers =
      ROADMAP_CONFIG_ITEM("Groups", "Show wazers");

static RoadMapConfigDescriptor RoadMapConfigGroupsURL =
      ROADMAP_CONFIG_ITEM("Groups", "URL");

static RoadMapConfigDescriptor RoadMapConfigGroupsFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Groups", "Feature Enabled");

static RoadMapConfigDescriptor RoadMapConfigGroupsTipShown =
      ROADMAP_CONFIG_ITEM("Groups", "Tip Shown");

//////////////////////////////////////////////////////////////////////////////////////////////////
static int  g_NumFollowingGroups = 0;
static char g_ActiveGroupName[GROUP_NAME_MAX_SIZE];
static char g_ActiveGroupIcon[GROUP_ICON_MAX_SIZE];
static char g_ActiveGroupWazerIcon[GROUP_ICON_MAX_SIZE];

static char *g_FollowingGroupNames[MAX_GROUPS];
static char *g_FollowingGroupIcons[MAX_GROUPS];

static char *g_SelectedGroupName;
static char *g_SelectedGroupIcon;

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_groups_get_popup_config(void){

   const char *conf_value;
   conf_value = roadmap_config_get(&RoadMapConfigGroupsPopUpReports);
   if (!strcmp(conf_value, POPUP_REPORT_VAL_NONE)) return POPUP_REPORT_NONE;
   else if (!strcmp(conf_value, POPUP_REPORT_VAL_FOLLOWING_GROUPS)) return POPUP_REPORT_FOLLOWING_GROUPS;
   else if (!strcmp(conf_value, POPUP_REPORT_VAL_ONLY_MAIN_GROUP)) return POPUP_REPORT_ONLY_MAIN_GROUP;
   else return POPUP_REPORT_NONE;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_popup_config(const char *popup_val){

   roadmap_config_set(&RoadMapConfigGroupsPopUpReports,popup_val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_groups_get_show_wazer_config(void){

   const char *conf_value;
   conf_value = roadmap_config_get(&RoadMapConfigGroupsShowWazers);
   if (!strcmp(conf_value, SHOW_WAZER_GROUP_VAL_ALL)) return SHOW_WAZER_GROUP_ALL;
   else if (!strcmp(conf_value, SHOW_WAZER_GROUP_VAL_FOLLOWING)) return SHOW_WAZER_GROUP_FOLLOWING;
   else if (!strcmp(conf_value, SHOW_WAZER_GROUP_VAL_MAIN)) return SHOW_WAZER_GROUP_MAIN;
   else return SHOW_WAZER_GROUP_MAIN;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_show_wazer_config(const char *show_wazer_val){

   roadmap_config_set(&RoadMapConfigGroupsShowWazers,show_wazer_val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_num_following(int num_groups){

   g_NumFollowingGroups = num_groups;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_groups_get_num_following(void){

   return g_NumFollowingGroups;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_active_group_name(char *name){

   if (name && *name)
      strncpy(g_ActiveGroupName, name, sizeof(g_ActiveGroupName));
   else
      g_ActiveGroupName[0] = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_active_group_name(void){

   return &g_ActiveGroupName[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_active_group_icon(char *name){

   if (name && *name){

      strncpy(g_ActiveGroupIcon, name, sizeof(g_ActiveGroupIcon));

      snprintf(g_ActiveGroupWazerIcon, GROUP_ICON_MAX_SIZE, "wazer_%s", name);

      // preload my active groups icon
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, name) == NULL)
         roadmap_res_download(RES_DOWNLOAD_IMAGE, name, NULL, "",FALSE, 0, NULL, NULL );

      // preload my active groups wazer icon
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, g_ActiveGroupWazerIcon) == NULL)
         roadmap_res_download(RES_DOWNLOAD_IMAGE, g_ActiveGroupWazerIcon, NULL, "",FALSE, 0, NULL, NULL );
   }
   else{
      g_ActiveGroupIcon[0] = 0;
      g_ActiveGroupWazerIcon[0] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_add_following_group_icon(int index, char *name){

   if (name && *name){
      char temp[GROUP_ICON_MAX_SIZE];
      snprintf(temp, GROUP_ICON_MAX_SIZE, "wazer_%s", name);

      // preload follwing groups icons
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, name) == NULL)
          roadmap_res_download(RES_DOWNLOAD_IMAGE, name, NULL, "",FALSE, 0, NULL, NULL );
   }

   if (index > MAX_GROUPS)
      return;

   if (g_FollowingGroupIcons[index] != NULL)
      free(g_FollowingGroupIcons[index]);

   if (name && *name){
      g_FollowingGroupIcons[index] = strdup(name);
   }
   else{
      g_FollowingGroupIcons[index] = strdup("groups_default_icons");
   }


}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_group_get_following_name(int index){
   return g_FollowingGroupNames[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_group_get_following_icon(int index){
   return g_FollowingGroupIcons[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_add_following_group_name(int index, char *name){

   if (index > MAX_GROUPS)
      return;

   if (g_FollowingGroupNames[index] != NULL)
      free(g_FollowingGroupNames[index]);

   g_FollowingGroupNames[index] = strdup(name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_active_group_icon(void){

   return &g_ActiveGroupIcon[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_active_group_wazer_icon(void){

   return &g_ActiveGroupWazerIcon[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_url(void){
   return roadmap_config_get(&RoadMapConfigGroupsURL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_groups_feature_enabled (void) {

   if (0 == strcmp (roadmap_config_get (&RoadMapConfigGroupsFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL roadmap_groups_tip_shown (void) {

   if (0 == strcmp (roadmap_config_get (&RoadMapConfigGroupsTipShown), "yes")){
      return TRUE;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_groups_set_tip_shown (void) {

   roadmap_config_set(&RoadMapConfigGroupsTipShown,"yes");
   roadmap_config_save(0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_groups_display_groups_supported (void) {

//#if (defined(IPHONE) || defined(ANDROID))
   return TRUE;
//#else
//   return FALSE;
//#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_init(void){


   roadmap_config_declare_enumeration ("user", &RoadMapConfigGroupsPopUpReports, NULL, POPUP_REPORT_VAL_NONE, POPUP_REPORT_VAL_FOLLOWING_GROUPS,POPUP_REPORT_VAL_ONLY_MAIN_GROUP, NULL);

   roadmap_config_declare_enumeration ("user", &RoadMapConfigGroupsShowWazers, NULL, SHOW_WAZER_GROUP_VAL_FOLLOWING, SHOW_WAZER_GROUP_VAL_MAIN, SHOW_WAZER_GROUP_VAL_ALL, NULL);

   roadmap_config_declare_enumeration ("user", &RoadMapConfigGroupsTipShown, NULL, "no", "yes", NULL);

   roadmap_config_declare( "preferences", &RoadMapConfigGroupsURL, "", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigGroupsFeatureEnabled, NULL, "yes", "no", NULL);

   memset( g_ActiveGroupName, 0, sizeof(g_ActiveGroupName));
   memset( g_ActiveGroupIcon, 0, sizeof(g_ActiveGroupIcon));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_term(void){

}

static void append_current_location( char* buffer)
{
   char  float_string_longitude[32];
    char  float_string_latitude [32];
    PluginLine line;
    int direction;

    RoadMapGpsPosition   MyLocation;

    if( roadmap_navigate_get_current( &MyLocation, &line, &direction) != -1)
    {
       convert_int_coordinate_to_float_string( float_string_longitude, MyLocation.longitude);
       convert_int_coordinate_to_float_string( float_string_latitude , MyLocation.latitude);

       sprintf( buffer, "&lon=%s&lat=%s", float_string_longitude, float_string_latitude);
    }
    else{
       const RoadMapPosition *Location;
       Location = roadmap_trip_get_position( "Location" );
       if ( (Location != NULL) && !IS_DEFAULT_LOCATION( Location ) ){
          convert_int_coordinate_to_float_string( float_string_longitude, Location->longitude);
          convert_int_coordinate_to_float_string( float_string_latitude , Location->latitude);

          sprintf( buffer, "&lon=%s&lat=%s", float_string_longitude, float_string_latitude);
       }
       else{
          roadmap_log( ROADMAP_DEBUG, "address_search::no location used");
          sprintf( buffer, "&lon=0&lat=0");
       }
    }
}
///////////////////////////////////////////////////////////////
static const char * create_group_url(const char *group_name) {
   static char url[1024];
   snprintf(url, sizeof(url),"%s?sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&gotogroup=%s&client_version=%s&web_version=%s&lang=%s",
            roadmap_groups_get_url(),
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID,
            roadmap_canvas_width(),
            roadmap_canvas_height() - roadmap_bar_bottom_height(),
            group_name,
            roadmap_start_version(),
            BROWSER_WEB_VERSION,
            roadmap_lang_get_system_lang());
   append_current_location(url + strlen(url));
   return &url[0];
}

///////////////////////////////////////////////////////////////
static const char *create_url(void) {
   static char url[1024];
   snprintf(url, sizeof(url),"%s?sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&client_version=%s&web_version=%s&lang=%s",
            roadmap_groups_get_url(),
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID,
            roadmap_canvas_width(),
            roadmap_canvas_height() - roadmap_bar_bottom_height(),
            roadmap_start_version(),
            BROWSER_WEB_VERSION,
            roadmap_lang_get_system_lang());
   append_current_location(url + strlen(url));
   return &url[0];
}
/*************************************************************************************************
 * void roadmap_groups_browser_btn_close_cb( void )
 * Custom button callback - Groups browser
 *
 */
void roadmap_groups_browser_btn_close_cb( void )
{
   ssd_dialog_hide_current( dec_ok );

}
//////////////////////////////////////////////////////////////////////////////////////////////////
static void configure_browser( RMBrowserAttributes* attrs )
{
   RMBrTitleAttributes *title_attrs;

   roadmap_browser_reset_attributes( attrs );

   /*
    * Note: Platform that uses cutomized ssd browser has to implement
    * roadmap_groups_browser_btn_XXX_cb for the desired buttons at the title bar
    * Now only android implements it. AGA
    */

   title_attrs = &attrs->title_attrs;
   title_attrs->title = "Groups";

   // Close button
   roadmap_browser_set_button_attrs( title_attrs, BROWSER_FLAG_TITLE_BTN_RIGHT2, "Close",
         roadmap_groups_browser_btn_close_cb, "button_small_up", "button_small_down" );

#ifdef ANDROID
   // Home button
   roadmap_browser_set_button_attrs( title_attrs, BROWSER_FLAG_TITLE_BTN_LEFT1, "Home",
         roadmap_groups_browser_btn_home_cb, "button_small_up", "button_small_down" );
   // Back button
   roadmap_browser_set_button_attrs( title_attrs, BROWSER_FLAG_TITLE_BTN_LEFT2, "Back",
         roadmap_groups_browser_btn_back_cb, "button_small_up", "button_small_down" );
#endif
}

static void on_close_cb(void){
   Realtime_SendCurrenScreenEdges();
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_show(void){
   RMBrowserAttributes attrs;
   int flags;

   if (!roadmap_groups_feature_enabled())
      return;

   flags = BROWSER_FLAG_TITLE_BTN_LEFT1|BROWSER_FLAG_TITLE_BTN_LEFT2|BROWSER_FLAG_TITLE_BTN_RIGHT2;

   attrs.on_close_cb = on_close_cb;

   configure_browser( &attrs );


   roadmap_browser_show_extended( create_url(), flags, &attrs );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_show_group(const char *group_name){
   RMBrowserAttributes attrs;
   int flags;

   if (!roadmap_groups_feature_enabled())
      return;

   if (!(group_name && *group_name))
      return;

   flags = BROWSER_FLAG_TITLE_BTN_LEFT1|BROWSER_FLAG_TITLE_BTN_LEFT2|BROWSER_FLAG_TITLE_BTN_RIGHT2;
   attrs.on_close_cb = on_close_cb;
   configure_browser( &attrs );
   roadmap_browser_show_extended( create_group_url( group_name ), flags, &attrs );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_alerts_action(void){
   if (!roadmap_groups_feature_enabled())
      return;


   if (!roadmap_groups_tip_shown() && (roadmap_groups_get_num_following() > 0)){
      if (roadmap_groups_get_popup_config() == POPUP_REPORT_NONE){
         roadmap_message_ticker_show(roadmap_lang_get("Waze Groups Tip"),roadmap_lang_get("Want group messages to pop-up to you? Go to Settings >Groups"), "group_settings", -1);
      }

      roadmap_groups_set_tip_shown();
   }

   RTAlerts_clear_group_counter();
   RealtimeAlertsListGroup();
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_selected_group_name(void){
   return g_SelectedGroupName;
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_selected_group_icon(void){
   return g_SelectedGroupIcon;
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_following_group_name(int index){
   if (index > MAX_GROUPS)
      return NULL;

   return g_FollowingGroupNames[index];
}

///////////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_groups_get_following_group_icon(int index){
   if (index > MAX_GROUPS)
      return NULL;

   return g_FollowingGroupIcons[index];
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_selected_group_name(const char *name){
   g_SelectedGroupName = (char *)name;
}

///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_set_selected_group_icon(const char *icon){
   g_SelectedGroupIcon = (char *)icon;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int groups_callback (SsdWidget widget, const char *new_value, const void *value) {

   RoadMapCallback cb = (RoadMapCallback)widget->parent->context;
   char *Groupname = (char *)value;
//   roadmap_mood_set(value);
   if (*Groupname){
      int i;
      g_SelectedGroupName = Groupname;
      if (!strcmp(Groupname, g_ActiveGroupName)){
         g_SelectedGroupIcon = g_ActiveGroupIcon;
      }
      else{
         for (i = 0; i < roadmap_groups_get_num_following()-1; i++){
            if (!strcmp(g_FollowingGroupNames[i], Groupname)){
               g_SelectedGroupIcon = g_FollowingGroupIcons[i];
               break;
            }
         }
      }
   }
   else{
      g_SelectedGroupName = "";
      g_SelectedGroupIcon = "";
   }

   ssd_dialog_hide ( "groupDlg", dec_close );

   if (cb)
         (*cb)();

   return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
void roadmap_groups_dialog (RoadMapCallback callback) {
   int row_height = 60;
   SsdWidget groupDlg;
   SsdWidget list;
   int flags = 0;
   int i;
   int count;
   const char *active_name;
   static char *labels[MAX_GROUPS] ;
   static void *values[MAX_GROUPS] ;
   static void *icons[MAX_GROUPS];
   int num_following;

   if (roadmap_screen_is_hd_screen()){
      row_height = 80;
   }

#ifdef OPENGL
    flags |= SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE;
#endif
    groupDlg   = ssd_dialog_new ( "groupDlg", roadmap_lang_get ("Groups"), NULL, SSD_CONTAINER_TITLE);
    groupDlg->context = (void *)callback;
    list = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, inputtype_none, flags, NULL);
    ssd_list_resize ( list, row_height );

    active_name = roadmap_groups_get_active_group_name();
    count = roadmap_groups_get_num_following();
    num_following = roadmap_groups_get_num_following();
    if (active_name[0] != 0){
       values[0] = (void *)roadmap_groups_get_active_group_name();
       icons[0]   = (void *) roadmap_groups_get_active_group_icon();
       labels[0] = (char *)active_name;
       num_following -= 1;
    }
    else{
       values[0] = "";
       icons[0]   = NULL;
       labels[0] = (char *)roadmap_lang_get("No group");
    }

    for (i = 0; i< num_following; i++){
       values[i+1] = (void *)roadmap_group_get_following_name(i);
       icons[i+1]   = (void *) roadmap_group_get_following_icon(i);
       labels[i+1] = (char *) roadmap_group_get_following_name(i);
    }

    if (active_name[0] != 0){
       values[count] = "";
       icons[count]   = NULL;
       labels[count] = (char *)roadmap_lang_get("No group");
    }

    ssd_list_populate (list, count+1, (const char **)labels, (const void **)values, (const char **)icons, NULL, groups_callback, NULL, FALSE);
    ssd_widget_add (groupDlg, list);
    ssd_dialog_activate ("groupDlg", NULL);
    ssd_dialog_draw ();

}
