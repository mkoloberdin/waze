/* RealtimeTrafficDetection.c - Manage real time traffic detection
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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

#include "../roadmap.h"
#include "../roadmap_main.h"
#include "../roadmap_config.h"
#include "../roadmap_lang.h"
#include "../roadmap_line_route.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_navigate.h"
#include "../roadmap_math.h"
#include "../roadmap_ticker.h"
#include "../roadmap_trip.h"
#include "../roadmap_street.h"
#include "../roadmap_res.h"
#include "../editor/editor_points.h"
#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_bitmap.h"
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_confirm_dialog.h"
#include "../ssd/ssd_contextmenu.h"
#include "../ssd/ssd_checkbox.h"
#include "../navigate/navigate_main.h"
#include "Realtime.h"
#include "RealtimeTrafficDetection.h"
#include "RealtimeTrafficInfo.h"

typedef struct RealtimeTrafficDetection_Context{
   PluginLine *line;
}RealtimeTrafficDetection_Context;

#ifndef TOUCH_SCREEN
// Context menu:
typedef enum alt_routes_context_menu_items {
   rttd_cm_yes,
   rttd_cm_no,
   rttd_cm_dont_ask,
   rttd_cm_cancel,
   rttd_cm__count,
   rttd_cm__invalid
} alt_routes_context_menu_items;

// Suggest Dlg Context menu:
static ssd_cm_item rt_context_menu_items[] = {
      SSD_CM_INIT_ITEM ( "Yes", rttd_cm_yes),
      SSD_CM_INIT_ITEM ( "No", rttd_cm_no),
      SSD_CM_INIT_ITEM ( "Don't ask me again", rttd_cm_dont_ask),
      SSD_CM_INIT_ITEM ( "Cancel",            rttd_cm_cancel),
};

static BOOL g_context_menu_is_active = FALSE;
static ssd_contextmenu rttd_context_menu = SSD_CM_INIT_MENU( rt_context_menu_items);
#endif

static RoadMapCallback gLoginCallBack;

#define TRAFFIC_DETECTION_DLG_NAME "TrafficDetectionDlg"
#define TRAFFIC_DLG_TIMEOUT 15
static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Feature enabled");

static RoadMapConfigDescriptor RoadMapConfigAskMe =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Ask me");

static RoadMapConfigDescriptor RoadMapConfigMaxTimesPerSession =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Max times per session");

static RoadMapConfigDescriptor RoadMapConfigNumSecondsToAsk =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Number of seconds to ask");

static RoadMapConfigDescriptor RoadMapConfigMinSegmentLength =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Min segment length");

static RoadMapConfigDescriptor RoadMapConfigDetectionSpeed =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Speed");

static RoadMapConfigDescriptor RoadMapConfigMaxRoadType =
      ROADMAP_CONFIG_ITEM("Traffic Detection", "Max Road type");

static int g_seconds;
static BOOL gTimerActive = FALSE;
static void RealtimeTrafficDetection_Timer(void);
static void TarfficDetectedDlgTimer(void);

//////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeTrafficDetection_MaxTimesPerSession(void){
   return roadmap_config_get_integer(&RoadMapConfigMaxTimesPerSession);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeTrafficDetection_NumSecondsToAsk(void){
   return roadmap_config_get_integer(&RoadMapConfigNumSecondsToAsk);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeTrafficDetection_MinSegmentLength(void){
   return roadmap_config_get_integer(&RoadMapConfigMinSegmentLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeTrafficDetection_DetectionSpeed(void){
   return roadmap_config_get_integer(&RoadMapConfigDetectionSpeed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeTrafficDetection_MaxRoadType(void){
   return roadmap_config_get_integer(&RoadMapConfigMaxRoadType);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL RealtimeTrafficDetection_FeatureEnabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL RealtimeTrafficDetection_AskMe (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigAskMe), "yes")){
      return TRUE;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void RealtimeTrafficDetection_DontAskMeAgain (BOOL ask_me) {
   if (ask_me == FALSE)
      roadmap_config_set (&RoadMapConfigAskMe, "no");
   else
      roadmap_config_set (&RoadMapConfigAskMe, "yes");
   roadmap_config_save(0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RealtimeTrafficDetection_OnRes (RealtimeTrafficDetection_Res *res) {
   if (res->iPoints > 0){
      editor_points_add_new_points(res->iPoints);
      editor_points_display_new_points_timed(res->iPoints, 5,confirm_event);
   }

   if (res->msg[0] != 0){;

      if (res->iPoints > 0)
         roadmap_messagebox_timeout(res->title, res->msg, 5);
      else
         roadmap_messagebox(res->title, res->msg);
   }

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onYesButtonSelected (SsdWidget widget, const char *new_value) {
   RoadMapGpsPosition GPS_position;
   PluginLine line;
   int direction;
   int from_node = -1;
    int to_node = -1;

   Realtime_ReportTraffic(TRAFFIC_VALUE_YES);
   ssd_dialog_hide_current(dec_close);

#ifdef TOUCH_SCREEN
   if (roadmap_navigate_get_current(&GPS_position, &line, &direction) != -1){
      if (line.plugin_id != -1){
            roadmap_square_set_current (line.square);
            if (ROUTE_DIRECTION_AGAINST_LINE != direction)
               roadmap_line_point_ids (line.line_id, &from_node, &to_node);
            else
               roadmap_line_point_ids (line.line_id, &to_node, &from_node);
      }
      roadmap_trip_set_gps_and_nodes_position( "AlertSelection", "Selection", "new_alert_marker",
                                              &GPS_position, from_node, to_node );
      add_real_time_alert_for_traffic_jam();


   }
#endif // TOUCH_SCREEN
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onNoButtonSelected (SsdWidget widget, const char *new_value) {
   Realtime_ReportTraffic(TRAFFIC_VALUE_NO);
   ssd_dialog_hide_current(dec_close);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onCloseButtonSelected (SsdWidget widget, const char *new_value) {
   Realtime_ReportTraffic(TRAFFIC_VALUE_NO_ANSWER);
   ssd_dialog_hide_current(dec_close);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onDontAskMeAgainSelected(SsdWidget widget, const char *new_value){
   //ssd_dialog_hide_current(dec_close);
   const char *val = ssd_dialog_get_data( "DontAsk" );
   if (!strcmp(val, "yes")){
      RealtimeTrafficDetection_DontAskMeAgain(FALSE);
      roadmap_main_remove_periodic(RealtimeTrafficDetection_Timer);
   }
   else{
      RealtimeTrafficDetection_DontAskMeAgain(TRUE);
      roadmap_main_set_periodic(1000, RealtimeTrafficDetection_Timer);
   }



   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onSettings(SsdWidget widget, const char *new_value){
   ssd_widget_show(ssd_widget_get(widget->parent,"DontAsk.Container" ));
   ssd_widget_hide(widget);
   if (gTimerActive){
      roadmap_main_remove_periodic(TarfficDetectedDlgTimer);
      gTimerActive = FALSE;
   }
   return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
static void TarfficDetectedDlgTimer(void){
   g_seconds --;
   if (g_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), TRAFFIC_DETECTION_DLG_NAME))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close_button" );
         if (g_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
#ifdef TOUCH_SCREEN
         if (button)
            ssd_button_change_text(button,button_txt );
#else
         ssd_widget_set_right_softkey_text(dialog, button_txt);
         ssd_dialog_refresh_current_softkeys();
#endif
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }
   onCloseButtonSelected(NULL, NULL);

   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void onTarfficDetectedDlgClose  (int exit_code, void* context){
   if (gTimerActive)
      roadmap_main_remove_periodic(TarfficDetectedDlgTimer);
   gTimerActive = FALSE;
}

#ifndef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected( BOOL made_selection,
               ssd_cm_item_ptr item, void* context){

   alt_routes_context_menu_items selection;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
     return;

   selection = (alt_routes_context_menu_items)item->id;

   switch( selection)
   {
       case rttd_cm_yes:
          onYesButtonSelected(NULL, NULL);
          break;

       case rttd_cm_no:
          onNoButtonSelected(NULL, NULL);
          break;

       case rttd_cm_dont_ask:
          onDontAskMeAgainSelected(NULL, NULL);
          break;

       default:
          break;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int Options_sk_cb(SsdWidget widget, const char *new_value, void *context) {

   int menu_x;

   if (ssd_widget_rtl (NULL))
   menu_x = SSD_X_SCREEN_RIGHT;
   else
   menu_x = SSD_X_SCREEN_LEFT;

   if (gTimerActive)
      roadmap_main_remove_periodic(TarfficDetectedDlgTimer);
   gTimerActive = FALSE;


   ssd_context_menu_show( menu_x, // X
                  SSD_Y_SCREEN_BOTTOM, // Y
                  &rttd_context_menu,
                  on_option_selected,
                  NULL,
                  dir_default,
                  0,
                  TRUE);

   g_context_menu_is_active = TRUE;
   return 1;
}
#endif // TOUCH_SCREEN

//////////////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget createTarfficDetectedDlg(void){
   SsdWidget dialog = NULL;
   SsdWidget text = NULL;
   SsdWidget box = NULL;
   SsdWidget box2 = NULL;
   SsdWidget checkbox =  NULL;
   char msg[256];

   char *icon[3];

   dialog = ssd_dialog_new (TRAFFIC_DETECTION_DLG_NAME, "", onTarfficDetectedDlgClose,
          SSD_PERSISTENT|SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
          SSD_ALIGN_CENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK|SSD_POINTER_NONE);

//   box = ssd_container_new ("Text.Container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
//   ssd_widget_set_color(box, NULL, NULL);
//   bitmap = ssd_bitmap_new("Traffic", "alert_icon_traffic_jam", SSD_ALIGN_VCENTER);
//   ssd_widget_add(box, bitmap);
//   ssd_widget_add(dialog, box);

   box2 = ssd_container_new ("Text.Container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
   ssd_widget_set_color(box2, NULL, NULL);
   sprintf(msg, "%s %s", roadmap_lang_get("Hey"), RealTime_GetUserName());
   text = ssd_text_new("TextTitle", msg, 16, SSD_END_ROW);
   ssd_text_set_color(text, "#ffffff");
   ssd_dialog_add_hspace(box2,5,0);
   ssd_widget_add(box2, text);
   ssd_dialog_add_vspace(box2, 5, 0);
   text = ssd_text_new("TextTitle", roadmap_lang_get("Are you in traffic?"), 18, SSD_END_ROW);
   ssd_text_set_color(text, "#f6a201");
   ssd_dialog_add_hspace(box2,5,0);
   ssd_widget_add(box2, text);
   ssd_dialog_add_vspace(box2, 2, 0);
   text = ssd_text_new("TextTitle", roadmap_lang_get("We're detecting a slow down"), 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text, "#f6a201");
   ssd_dialog_add_hspace(box2,5,0);
   ssd_widget_add(box2, text);
   ssd_dialog_add_vspace(box2, 10, 0);
   ssd_widget_add(dialog, box2);
#ifdef TOUCH_SCREEN
   box = ssd_container_new ("Buttons.Container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
   ssd_widget_set_color(box, NULL, NULL);

   icon[0] = "button_small_up";
   icon[1] = "button_small_down";
   icon[2] = NULL;

   ssd_widget_add (box,
         ssd_button_label_custom( "Yes", roadmap_lang_get ("Yes"),SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, onYesButtonSelected,
               (const char **)icon, 2, "#FFFFFF", "#FFFFFF",14) );

   ssd_widget_add (box,
         ssd_button_label_custom( "No", roadmap_lang_get ("No"),SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, onNoButtonSelected,
               (const char **)icon, 2, "#FFFFFF", "#FFFFFF",14) );

   ssd_widget_add (box,
         ssd_button_label ("Close_button", roadmap_lang_get("Close"),
                        SSD_ALIGN_CENTER| SSD_ALIGN_VCENTER | SSD_WS_TABSTOP,
                        onCloseButtonSelected));

   ssd_widget_add(dialog, box);

   box = ssd_container_new ("DontAsk.Container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_set_color(box,NULL, NULL);
   text = ssd_text_new("DontAsk", roadmap_lang_get("Don't ask me again"), 12, SSD_END_ROW|SSD_ALIGN_VCENTER);
   checkbox = ssd_checkbox_new ( "DontAsk", FALSE,  0, onDontAskMeAgainSelected, NULL, NULL, CHECKBOX_STYLE_DEFAULT ) ;
   ssd_text_set_color(text, "#ffffff");
   //box->callback = onDontAskMeAgainSelected;
   ssd_dialog_add_hspace(box,5,0);
   ssd_widget_add(box, checkbox);
   ssd_dialog_add_hspace(box,5,0);
   ssd_widget_add(box, text);
   ssd_widget_add(dialog, box);
   ssd_widget_hide(box);

   box = ssd_container_new ("DontAsk.Container2", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW|SSD_WS_TABSTOP|SSD_ALIGN_CENTER);
   ssd_widget_set_color(box,NULL, NULL);
   text = ssd_text_new("Settings", roadmap_lang_get("Settings"), 12, SSD_END_ROW|SSD_ALIGN_VCENTER|SSD_TEXT_NORMAL_FONT);
   box->callback = onSettings;
   ssd_text_set_color(text, "#ffffff");
   ssd_dialog_add_hspace(box,5,0);
   ssd_widget_add(box, text);
   ssd_widget_add(dialog, box);

#else
   ssd_widget_set_left_softkey_callback(dialog, Options_sk_cb);
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
   ssd_widget_set_right_softkey_text(dialog, roadmap_lang_get("Close"));
   ssd_dialog_refresh_current_softkeys();
#endif
   return dialog;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void showTarfficDetectedDlg(void *context){

   static SsdWidget dialog = NULL;
   static RoadMapSoundList list = NULL;

   if (!dialog){
      dialog = createTarfficDetectedDlg();
   }else{
#ifdef TOUCH_SCREEN
      ssd_button_change_text(ssd_widget_get(dialog, "Close_button"),roadmap_lang_get ("Close") );
#else
      ssd_widget_set_right_softkey_text(dialog, roadmap_lang_get("Close"));
#endif
   }
   ssd_dialog_activate(TRAFFIC_DETECTION_DLG_NAME, NULL);

   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "ping");
      roadmap_res_get (RES_SOUND, 0, "ping");
   }
   roadmap_sound_play_list (list);

   g_seconds = TRAFFIC_DLG_TIMEOUT;
   roadmap_main_set_periodic(1000, TarfficDetectedDlgTimer);
   gTimerActive = TRUE;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int line_extended_length (const PluginLine *line) {

   RoadMapPosition p1;
   RoadMapPosition p2;
   int length = 0;

   roadmap_street_extend_line_ends(line, &p1, &p2, FLAG_EXTEND_BOTH, NULL, NULL);
   length += roadmap_math_distance (&p1, &p2);

   return length;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
static void RealtimeTrafficDetection_Timer(void)
{

    RoadMapGpsPosition pos;
    BOOL gps_active;
    int gps_state;
    static int idleCount = 0;
    static int numTimesAsked = 0;
    RealtimeTrafficDetection_Context context;
    PluginLine line;
    int direction;
    int speed;
    int length;
    static BOOL drove = FALSE;



    gps_state = roadmap_gps_reception_state();
    gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);

    // Check that we have GPS
    if (!gps_active)
       return;

    // Check Speed
    if (-1 == roadmap_navigate_get_current(&pos, &line, &direction)){
       return;
    }

    speed = roadmap_math_to_speed_unit(pos.speed);

    if (pos.speed < 0){
        idleCount = 0;
        return;
    }

    // Check that the user did some driving before
    if (!drove){
       if (speed > 30){
          drove = TRUE;
          return;
       }
       else{
          return;
       }
    }

    // Check that we are logged in
    if (!RealTimeLoginState()){
       roadmap_log (ROADMAP_DEBUG, "Realtime Traffic detection. Not logged in.");
       return;
    }

    // Check Speed
    if (speed > RealtimeTrafficDetection_DetectionSpeed()){
        idleCount = 0;
        return;
    }

    // Check road type
    if (line.cfcc > RealtimeTrafficDetection_MaxRoadType()){
       idleCount = 0;
       return;
    }

    // Check that we are on a jam
    if (!roadmap_navigate_is_jammed( &line, &direction)){
       roadmap_log (ROADMAP_DEBUG, "Realtime Traffic detection. We are near the end of the segment.");
       idleCount = 0;
       return;
    }

     // Check that we are not near the destination
    if (navigate_main_near_destination()){
       roadmap_log (ROADMAP_DEBUG, "Realtime Traffic detection. We are near the destination.");
       idleCount = 0;
       return;
    }

    // Check Line legth
    roadmap_square_set_current(line.square);
    length = line_extended_length (&line);
    if (length < RealtimeTrafficDetection_MinSegmentLength()){
       roadmap_log (ROADMAP_DEBUG, "Realtime Traffic detection. Segment legth is to short id=%d length=%d.", line.line_id, length);
       idleCount = 0;
       return;
    }


    if (RTTrafficInfo_Get_Line(line.line_id, line.square, (direction == ROUTE_DIRECTION_AGAINST_LINE)) != -1){
       idleCount = 0;
       return;
    }

    idleCount++;
    if (idleCount == RealtimeTrafficDetection_NumSecondsToAsk()){
       if (numTimesAsked < RealtimeTrafficDetection_MaxTimesPerSession()){
          roadmap_log (ROADMAP_DEBUG, "Realtime Traffic detection. JAM detected on line id=%d.", line.line_id);
          context.line = &line;
          showTarfficDetectedDlg((void *)&context);
          numTimesAsked++;
          drove = FALSE;
       }
       else{
          roadmap_main_remove_periodic(RealtimeTrafficDetection_Timer);

       }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void RealtimeTrafficDetection_AfterLogin (void) {
   if (gLoginCallBack)
      (*gLoginCallBack) ();
   roadmap_main_set_periodic(1000, RealtimeTrafficDetection_Timer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeTrafficDetection_Init(void){

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "yes",
                  "no", NULL);

   roadmap_config_declare_enumeration ("user", &RoadMapConfigAskMe, NULL, "yes",
                  "no", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMaxTimesPerSession, "2", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigNumSecondsToAsk, "30", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMinSegmentLength, "150", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigDetectionSpeed, "15", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMaxRoadType, "3", NULL);


   if (RealtimeTrafficDetection_FeatureEnabled() && RealtimeTrafficDetection_AskMe()){
      gLoginCallBack = Realtime_NotifyOnLogin (RealtimeTrafficDetection_AfterLogin);
   }
   else{
      roadmap_log (ROADMAP_INFO, "Realtime Traffic detection disabled.");
   }
}
