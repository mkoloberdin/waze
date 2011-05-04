/* navigate_res_dlg.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *f
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_gps.h"
#include "roadmap_main.h"
#include "roadmap_screen.h"
#include "roadmap_lang.h"
#include "roadmap_math.h"
#include "roadmap_trip.h"
#include "roadmap_alternative_routes.h"
#include "navigate_res_dlg.h"
#include "navigate_main.h"
#include "navigate_route.h"
#include "navigate_bar.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "roadmap_general_settings.h"
#include "roadmap_alternative_routes.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAltRoutes.h"

static int g_seconds;
static SsdWidget dialog;

#define NAVIAGTE_RES_DLG_NAME "Navigate_Res_Dlg"
static void update_button (void);
static BOOL g_is_distance_display = TRUE;

#define SWITCH_ETA_TIMER 3000

/////////////////////////////////////////////////////////////////////
static SsdWidget space (int height) {
   SsdWidget space;
   //if (roadmap_screen_is_hd_screen())
//      height *= 2;
   height = ADJ_SCALE(height);
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color (space, NULL, NULL);
   return space;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void kill_timer (void) {

   roadmap_main_remove_periodic (update_button);
   roadmap_main_remove_periodic(navigate_res_dlg_switch_eta_display);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void navigate_res_dlg_close (int exit_code, void* context) {

   if (exit_code != dec_ok){
      roadmap_trip_remove_point("Destination");
      roadmap_trip_remove_point("Departure");

   }

   if (exit_code != dec_cancel) {
      navigate_bar_set_mode(1);
   }

}

static void close_res_dlg(void){
   kill_timer ();
   ssd_dialog_hide (NAVIAGTE_RES_DLG_NAME, dec_ok);
   navigate_main_start_navigating();
   roadmap_screen_redraw ();
}

/////////////////////////////////////////////////////////////////////
static int on_drive_btn_cb (SsdWidget widget, const char *new_value){
   close_res_dlg();
   return 1;
}

/////////////////////////////////////////////////////////////////////
static int on_alt_routes_btn_cb(SsdWidget widget, const char *new_value){
   const RoadMapPosition *from;
   RoadMapPosition to;
   AltRouteTrip route;
   RealtimeAltRoutes_Clear();
   from =navigate_main_get_src_position ();
   navigate_get_waypoint(-1, &to);
   kill_timer();
   ssd_dialog_hide(NAVIAGTE_RES_DLG_NAME, dec_ok);
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Calculating alternative routes, please wait..." ) );
   RealtimeAltRoutes_Init_Record(&route);
   route.srcPosition = *from;
   route.destPosition = to;
   route.iTripId = -1;
   navigate_main_stop_navigation();
   roadmap_trip_set_point ("Destination", &route.destPosition);
   roadmap_trip_set_point ("Departure", &route.srcPosition);
   RealtimeAltRoutes_Add_Route(&route);
   RealtimeAltRoutes_Route_Request (-1, from, &to, MAX_ROUTES);
   return 1;
}

/////////////////////////////////////////////////////////////////////
static void switch_to_eta_display(void){
   SsdWidget ETA_cont;
   SsdWidget TimeSidt_cont;

   TimeSidt_cont = ssd_widget_get(dialog,"time_container" );
   if (TimeSidt_cont)
      ssd_widget_hide(TimeSidt_cont);

   TimeSidt_cont = ssd_widget_get(dialog,"distance_container" );
   if (TimeSidt_cont)
      ssd_widget_hide(TimeSidt_cont);

   TimeSidt_cont = ssd_widget_get(dialog,"distance_container_spacer" );
   if (TimeSidt_cont)
      ssd_widget_hide(TimeSidt_cont);

   ETA_cont = ssd_widget_get(dialog,"ETA_container" );
   if (ETA_cont)
      ssd_widget_show(ETA_cont);

   roadmap_screen_refresh();

}

/////////////////////////////////////////////////////////////////////
static void switch_to_time_distance_display(void){
   SsdWidget ETA_cont;
   SsdWidget TimeSidt_cont;

   TimeSidt_cont = ssd_widget_get(dialog,"time_container" );
   if (TimeSidt_cont)
      ssd_widget_show(TimeSidt_cont);

   TimeSidt_cont = ssd_widget_get(dialog,"distance_container" );
   if (TimeSidt_cont)
      ssd_widget_show(TimeSidt_cont);

   TimeSidt_cont = ssd_widget_get(dialog,"distance_container_spacer" );
   if (TimeSidt_cont)
      ssd_widget_show(TimeSidt_cont);

   ETA_cont = ssd_widget_get(dialog,"ETA_container" );
   if (ETA_cont)
      ssd_widget_hide(ETA_cont);

   roadmap_screen_refresh();

}

/////////////////////////////////////////////////////////////////////
void navigate_res_dlg_switch_eta_display(void){

   if (g_is_distance_display){
      g_is_distance_display = FALSE;
      switch_to_eta_display();
   }
   else{
      g_is_distance_display = TRUE;
      switch_to_time_distance_display();
   }


}
/////////////////////////////////////////////////////////////////////
static void update_button (void) {
   SsdWidget button;
   char button_txt[20];
   char *dlg_name;
   g_seconds--;

   if (g_seconds < 0){
      close_res_dlg();
      return;
   }

#ifdef TOUCH_SCREEN
   button = ssd_widget_get(dialog,"Drive_button" );
   if (g_seconds)
      sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Drive"), g_seconds);
   else{
      sprintf(button_txt, "%s", roadmap_lang_get ("Drive"));
      dlg_name = ssd_dialog_currently_active_name();
      if (dlg_name && !strcmp(dlg_name, NAVIAGTE_RES_DLG_NAME))
         ssd_dialog_set_focus(button);
   }
   ssd_button_change_text(button, button_txt);
#else
   if (g_seconds)
      sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Drive"), g_seconds);
   else
      sprintf(button_txt, "%s", roadmap_lang_get ("Drive"));

   dlg_name = ssd_dialog_currently_active_name();
   if (dlg_name && !strcmp(dlg_name, NAVIAGTE_RES_DLG_NAME)){
      ssd_widget_set_right_softkey_text(ssd_dialog_get_currently_active(), button_txt);
      ssd_dialog_refresh_current_softkeys();
   }
#endif
   roadmap_screen_redraw ();
}
void navigate_res_hide_ETA_widget(SsdWidget container){
   SsdWidget ETA_widget;
   if (container == NULL)
      return;

   ETA_widget = ssd_widget_get(container,"ETA_Time_Dist_container");
   if (ETA_widget)
      ssd_widget_hide(ETA_widget);
}

void navigate_res_show_ETA_widget(SsdWidget container){
   SsdWidget ETA_widget;
   if (container == NULL)
      return;

   ETA_widget = ssd_widget_get(container,"ETA_Time_Dist_container");
   if (ETA_widget)
      ssd_widget_show(ETA_widget);
}
void navigate_res_update_ETA_widget(SsdWidget dlg, SsdWidget container, int iRouteDistance, int iRouteLenght, const char *via, BOOL showDistance){
   SsdWidget text;
   timeStruct ETA_struct;
   timeStruct curTime ;
   timeStruct timeToDest;
   char str[100];
   char unit_str[20];
   char msg[250];

   dialog = dlg;
   curTime = navigate_main_get_current_time();

   timeToDest.hours = iRouteLenght / 3600;
   timeToDest.minutes =  (iRouteLenght % 3600) / 60;
   timeToDest.seconds = iRouteLenght % 60;
   ETA_struct = navigate_main_calculate_eta(curTime,timeToDest);
   g_is_distance_display = TRUE;

   navigate_main_get_distance_str(iRouteDistance, &str[0], sizeof(str), &unit_str[0], sizeof(unit_str));

   snprintf(msg, sizeof(msg), "%d",   (int)(iRouteLenght/60.0));
   text = ssd_widget_get (container, "ETA_W_Minutes_Text");
   ssd_text_set_text(text, msg);

   msg[0] = 0;
   snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
                    "%s",roadmap_lang_get("ETA"));
   text = ssd_widget_get (container, "ETA_Text");
   if (text)
      ssd_text_set_text(text, msg);

   msg[0] = 0;
   snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
                    "%d:%02d",
                     ETA_struct.hours, ETA_struct.minutes);
   text = ssd_widget_get (container, "ETA_W_ETA_TIME_Text");
   if (text)
      ssd_text_set_text(text, msg);



   if (showDistance){
      text = ssd_widget_get (container, "ETA_W_Distance_Text");
      if (text)
         ssd_text_set_text(text, str);

      text = ssd_widget_get (container, "ETA_W_Distance_Unit_Text");
      if (text)
         ssd_text_set_text(text, unit_str);
   }

   // VIA Text
   if (via && (via[0] != 0)){
      msg[0] = 0;
      snprintf (msg, sizeof (msg), "%s: %s",
                   roadmap_lang_get("Via"),
                   roadmap_lang_get (via));
      text = ssd_widget_get (container, "ETA_W_VIA_Text");
      if (text)
         ssd_text_set_text(text, msg);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
SsdWidget navigate_res_ETA_widget(int iRouteDistance, int iRouteLenght, const char *via, BOOL showDistance, BOOL showAltBtn, SsdCallback callback){
   SsdWidget ETA_Time_Dist_container, inner_container, button, text;
   SsdWidget time_dist_container;
   SsdWidget time_container;
   SsdWidget distance_container;
   SsdWidget ETA_container;
   SsdWidget container;
   SsdWidget spacer;
   char *icon[3];
   int inner_width;
   int font_size;
   int width = SSD_MAX_SIZE;

#ifdef IPHONE_NATIVE
   width = ADJ_SCALE(300);
#else
   width = roadmap_canvas_width() - ADJ_SCALE(40);
   if (roadmap_canvas_height() < roadmap_canvas_width())
      width = roadmap_canvas_height() - ADJ_SCALE(40);
#endif


   ETA_Time_Dist_container = ssd_container_new ("ETA_Time_Dist_container", NULL, width, SSD_MIN_SIZE,
                  SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW  );
   ssd_widget_set_color(ETA_Time_Dist_container, NULL, NULL);

   inner_width = width  ;

   inner_container = ssd_container_new ("inner container", NULL, inner_width, SSD_MIN_SIZE,
                  SSD_WIDGET_SPACE | SSD_END_ROW  );
   ssd_widget_set_color(inner_container,NULL, NULL);

   // VIA Text
   text = ssd_text_new ("ETA_W_VIA_Text", "", 18, SSD_TEXT_NORMAL_FONT|SSD_END_ROW|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#b6b6b6");
   ssd_widget_add (inner_container, text);
   ssd_dialog_add_vspace(inner_container,5,0);

   time_dist_container= ssd_container_new ("time_dist_container", NULL, inner_width-10, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW |SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER );
   ssd_widget_set_color(time_dist_container,"#00000087", "#00000087");
   ssd_widget_set_offset(time_dist_container, ADJ_SCALE(-2),0);
   ssd_widget_add(inner_container, time_dist_container);


   time_container = ssd_container_new ("time_container", NULL, (inner_width-10)/2-4, SSD_MIN_SIZE,
            SSD_WIDGET_SPACE | SSD_ALIGN_CENTER  | SSD_ALIGN_VCENTER );
   ssd_widget_set_color(time_container,NULL, NULL);
   ssd_widget_add(time_dist_container, time_container);


   if (showDistance){
      spacer = ssd_container_new("distance_container_spacer", "",1, ADJ_SCALE(45), SSD_ALIGN_VCENTER);
      ssd_widget_set_color(spacer,"#747474", "#747474");
      ssd_widget_add(time_dist_container, spacer);

      distance_container = ssd_container_new ("distance_container", NULL, (inner_width-10)/2-4, ADJ_SCALE(70),
               SSD_WIDGET_SPACE | SSD_ALIGN_CENTER );
      ssd_widget_set_color(distance_container,NULL, NULL);
      ssd_widget_add(time_dist_container, distance_container);
   }

#ifdef IPHONE_NATIVE
//   ssd_widget_add (inner_container, space (7));
#endif
   container = ssd_container_new ("container__", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
               SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER  );
   ssd_widget_set_color(container, NULL, NULL);
   font_size = 40;
   text = ssd_text_new ("ETA_W_Minutes_Text", "", font_size, SSD_ALIGN_VCENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_text_set_use_height_factor(text, FALSE);
   ssd_widget_set_offset(text, 0, ADJ_SCALE(-4));
   ssd_widget_add (container, text);
   ssd_dialog_add_hspace(container,ADJ_SCALE(3), 0);
   font_size = 16;
   text = ssd_text_new ("ETA__W_Min_text", roadmap_lang_get ("min."), font_size, SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text,"#ffffff");
   ssd_text_set_use_height_factor(text, FALSE);
   ssd_widget_set_offset(text, 0, ADJ_SCALE(23));
   ssd_widget_add (container, text);
   ssd_widget_add (time_container, container);
   if (showDistance){
      container = ssd_container_new ("container__", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                     SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER  );
      ssd_widget_set_color(container, NULL, NULL);
      text = ssd_text_new ("ETA_W_Distance_Text", "", 40, SSD_ALIGN_VCENTER);
      ssd_text_set_color(text,"#ffffff");
      ssd_text_set_use_height_factor(text, FALSE);
      ssd_widget_set_offset(text, 0, ADJ_SCALE(-4));
      ssd_widget_add (container, text);
      ssd_dialog_add_hspace(container,ADJ_SCALE(3), 0);
      text = ssd_text_new ("ETA_W_Distance_Unit_Text", "", 16, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
      ssd_text_set_color(text,"#ffffff");
      ssd_text_set_use_height_factor(text, FALSE);
      ssd_widget_set_offset(text, 0, ADJ_SCALE(23));
      ssd_widget_add (container, text);
      ssd_widget_add (distance_container, container);
  }

   // Add Alternatives buttons
#ifdef TOUCH_SCREEN
   if (roadmap_alternative_feature_enabled() && RealTimeLoginState () && showAltBtn){
      icon[0] = "alternative_button";
      icon[1] = "alternative_button_s";
      icon[2] = NULL;
      button = ssd_button_new("Alt_button", "Alt",(const char**) &icon[0], 2, SSD_ALIGN_RIGHT|SSD_WS_TABSTOP|SSD_ALIGN_VCENTER, callback);
      ssd_widget_add(time_dist_container, button);
   }
#endif
   ssd_widget_add (inner_container, space (2));

   ETA_container = ssd_container_new ("ETA_container", NULL, SSD_MIN_SIZE, ADJ_SCALE(70),
            SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER );
   ssd_widget_set_color(ETA_container,NULL, NULL);
   ssd_widget_add(time_dist_container, ETA_container);

   text = ssd_text_new ("ETA_Text", "", 16, SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text,"#ffffff");
   ssd_text_set_use_height_factor(text, FALSE);
   ssd_widget_set_offset(text, 0, ADJ_SCALE(23));
   ssd_widget_add (ETA_container, text);
   ssd_dialog_add_hspace(ETA_container, ADJ_SCALE(5),SSD_ALIGN_CENTER);

   text = ssd_text_new ("ETA_W_ETA_TIME_Text", "", 40, SSD_END_ROW|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_set_offset(text, 0, ADJ_SCALE(-4));
   ssd_text_set_use_height_factor(text, FALSE);
   ssd_widget_add (ETA_container, text);
   ssd_widget_hide(ETA_container);

   ssd_widget_add(ETA_Time_Dist_container, inner_container);
   navigate_res_update_ETA_widget(dialog, ETA_Time_Dist_container, iRouteDistance, iRouteLenght, via, showDistance);
   return ETA_Time_Dist_container;

}


#ifndef TOUCH_SCREEN
static int Drive_sk_cb(SsdWidget widget, const char *new_value, void *context){
   ssd_dialog_hide(NAVIAGTE_RES_DLG_NAME, dec_close);
   kill_timer();
   return 1;
}

static int Alternatives_sk_cb(SsdWidget widget, const char *new_value, void *context){
   on_alt_routes_btn_cb(NULL, NULL);
}

static void navigate_res_dlg_set_softkeys(SsdWidget dlg){

   ssd_widget_set_right_softkey_callback(dlg, Drive_sk_cb);
   ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Drive"));

   if (roadmap_alternative_feature_enabled() && RealTimeLoginState()){
      ssd_widget_set_left_softkey_callback(dlg, Alternatives_sk_cb);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Alternatives"));
   }

}
#endif //TOUCH_SCREEN

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_close_btn_cb (SsdWidget widget, const char *new_value) {
   navigate_main_stop_navigation();
   kill_timer();
   ssd_dialog_hide_all(dec_cancel);
   return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void navigate_res_dlg (int NavigateFlags, const char *pTitleText, int iRouteDistance, int iRouteLenght, const char *via, int iTimeOut, BOOL show_disclaimer) {

   SsdWidget button;
   SsdWidget text;
   const char *small_button_icon[]   = {"button_small_up", "button_small_down", "button_small_up"};

   dialog = ssd_dialog_new (NAVIAGTE_RES_DLG_NAME, "", navigate_res_dlg_close, SSD_DIALOG_FLOAT
                  | SSD_ALIGN_CENTER | SSD_ROUNDED_CORNERS | SSD_ROUNDED_BLACK | SSD_ALIGN_VCENTER | SSD_CONTAINER_BORDER);

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space (3));
   if (pTitleText && !is_screen_wide()){
      text = ssd_text_new ("Title TXT", pTitleText, 22, SSD_ALIGN_CENTER);
      ssd_text_set_color(text, "#f7a200");
      ssd_widget_add (dialog, text);
      ssd_widget_add (dialog, space (5));
      ssd_widget_add(dialog, ssd_separator_new("sep", 0));
      ssd_widget_add (dialog, space (3));
   }
#endif

   ssd_widget_add(dialog, navigate_res_ETA_widget(iRouteDistance, iRouteLenght, via, TRUE, FALSE, NULL));
   if ((NavigateFlags & CHANGED_DESTINATION) && !is_screen_wide()) {
      ssd_widget_add (dialog, space (3));
      text = ssd_text_new("NearestDestText", roadmap_lang_get ("Unable to provide route to destination. Taking you to nearest location."), -1, SSD_END_ROW|SSD_ALIGN_CENTER);
      ssd_text_set_color(text, "#b6b6b6");
      ssd_widget_add(dialog, text);
      ssd_widget_add (dialog, space (3));
   }

   if ((NavigateFlags & CHANGED_DEPARTURE) && !is_screen_wide()) {
      ssd_widget_add (dialog, space (3));
      text = ssd_text_new("NearestDestText", roadmap_lang_get ("Showing route using alternative departure point."), -1, SSD_END_ROW|SSD_ALIGN_CENTER);
      ssd_text_set_color(text, "#b6b6b6");
      ssd_widget_add(dialog, text);
      ssd_widget_add (dialog, space (3));
   }

   if (show_disclaimer && !is_screen_wide()){
      text = ssd_text_new("text", roadmap_lang_get("Note: route may not be optimal, but waze learns quickly..."), 12, SSD_TEXT_NORMAL_FONT|SSD_END_ROW|SSD_WIDGET_SPACE|SSD_START_NEW_ROW|SSD_ALIGN_CENTER);
      ssd_text_set_color(text, "#b6b6b6");
      ssd_widget_add(dialog, text);
   }

   //Add buttons
#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog, space (5));
   ssd_widget_add(dialog, ssd_separator_new("sep", 0));
   ssd_widget_add (dialog, space (5));
   button = ssd_button_label("Drive_button", roadmap_lang_get("Drive"), SSD_ALIGN_CENTER|SSD_WS_TABSTOP|SSD_WS_DEFWIDGET, on_drive_btn_cb);
   if (button)
      ssd_widget_set_color(ssd_widget_get(button,"label"),"#f7a200", "#ffffff" );
   ssd_widget_add(dialog, button);

   if (roadmap_alternative_feature_enabled() && RealTimeLoginState()){
      button = ssd_button_label("Alt_button", roadmap_lang_get("Routes"), SSD_ALIGN_CENTER|SSD_WS_TABSTOP|SSD_WS_DEFWIDGET, on_alt_routes_btn_cb);
      ssd_widget_add(dialog, button);
   }

   button = ssd_button_label_custom("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_close_btn_cb, small_button_icon, 3,"#ffffff", "#ffffff",14);
   ssd_widget_add(dialog, button);
#else
   navigate_res_dlg_set_softkeys(dialog);
#endif


   g_seconds = iTimeOut/1000 ;
   navigate_bar_set_mode(0);
   roadmap_main_set_periodic (1000, update_button);

   g_is_distance_display = TRUE;
   roadmap_main_set_periodic (SWITCH_ETA_TIMER, navigate_res_dlg_switch_eta_display);

   ssd_dialog_activate (NAVIAGTE_RES_DLG_NAME, NULL);
   if (!roadmap_screen_refresh ())
      roadmap_screen_redraw ();
}
