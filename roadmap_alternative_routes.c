/* roadmap_alternative_routes.c
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
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_screen.h"
#include "roadmap_pointer.h"
#include "roadmap_alternative_routes.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_math.h"
#include "roadmap_trip.h"
#include "roadmap_general_settings.h"
#include "navigate/navigate_main.h"
#include "navigate/navigate_res_dlg.h"
#include "navigate/navigate_bar.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAltRoutes.h"
#include "navigate/navigate_main.h"
#include "roadmap_bar.h"
#include "roadmap_screen_obj.h"
#include "roadmap_object.h"
#define ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME  "alt-routes-suggets-rout-dlg"
#define ALT_ROUTE_ROUTS_DLG_NAME          "alternative-routes-dlg"
#define SUGGEST_DLG_TIMEOUT              15000

#define ALT_CLOSE_CLICK_OFFSETS_DEFAULT {-20, -20, 20, 20 };

static RoadMapCallback gLoginCallBack;

static RoadMapConfigDescriptor RoadMapConfigShowSuggested =
      ROADMAP_CONFIG_ITEM("Alternative Routes", "Show Suggested");

static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Alternative Routes", "Feature enabled");

static RoadMapConfigDescriptor RoadMapConfigLastCheckTimeStamp =
      ROADMAP_CONFIG_ITEM("Alternative Routes", "Last check timestamp");

typedef enum time_of_day {
   tod_morning, tod_afternoon, tod_evening, tod_night
} time_of_day;

static BOOL timerActive = FALSE;

#ifndef TOUCH_SCREEN
// Context menu:
typedef enum alt_routes_context_menu_items {
   alt_rt_cm_drive,
   alt_rt_cm_show_route,
   alt_rt_cm_show_all_routes,
   alt_rt_cm_show_alt_routes,
   alt_rt_cm_dont_ask_me_again,
   alt_rt_cm_route_1,
   alt_rt_cm_route_2,
   alt_rt_cm_route_3,
   alt_rt_cm_compare_routes,
   alt_rt_cm_ignore,
   alt_rt_cm_cancel,
   alt_rt_cm__count,
   alt_rt_cm__invalid
} alt_routes_context_menu_items;

// Suggest Dlg Context menu:
static ssd_cm_item suggest_context_menu_items[] = {
      SSD_CM_INIT_ITEM ( "Show Routes", alt_rt_cm_show_alt_routes),
      SSD_CM_INIT_ITEM ( "Don't ask me again", alt_rt_cm_dont_ask_me_again),
      SSD_CM_INIT_ITEM ( "Ignore", alt_rt_cm_ignore),
};

// Routes Dlg Context menu:
static ssd_cm_item routes_context_menu_items[] = {
      SSD_CM_INIT_ITEM ( "Drive", alt_rt_cm_drive),
      SSD_CM_INIT_ITEM ( "Show Route", alt_rt_cm_show_route),
      SSD_CM_INIT_ITEM ( "Show All Routes", alt_rt_cm_show_all_routes),
};

// Compare Routes Context menu:
static ssd_cm_item compare_routes_context_menu_items[] = {
      SSD_CM_INIT_ITEM ( "Drive", alt_rt_cm_drive),
      SSD_CM_INIT_ITEM ( "Route 1 [1]", alt_rt_cm_route_1),
      SSD_CM_INIT_ITEM ( "Route 2 [2]", alt_rt_cm_route_2),
      SSD_CM_INIT_ITEM ( "Route 3 [3]", alt_rt_cm_route_3),
      SSD_CM_INIT_ITEM ( "All routes [0]", alt_rt_cm_show_all_routes),
};

static BOOL g_context_menu_is_active = FALSE;
static ssd_contextmenu suggest_context_menu = SSD_CM_INIT_MENU( suggest_context_menu_items);
static ssd_contextmenu routes_context_menu = SSD_CM_INIT_MENU( routes_context_menu_items);
static ssd_contextmenu compare_routes_context_menu = SSD_CM_INIT_MENU( compare_routes_context_menu_items);

static int compare_routes_options_sk_cb (SsdWidget widget, const char *new_value, void *context);

#endif//TOUCH_SCREEN

static void kill_timer (void);
static int on_drive_btn_cb (SsdWidget widget, const char *new_value);

static int g_seconds;
static SsdWidget dialog;
static SsdWidget progress;

typedef struct {
   int id;
   NavigateRouteResult *nav_result;
} RoutingContext;


static int override_long_click (RoadMapGuiPoint *point) {
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_alternative_routes_suggest_routes (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigShowSuggested), "yes")){
      return TRUE;
   }

   return FALSE;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_alternative_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int roadmap_alternative_routes_get_last_check_time_stamp(void){
   return roadmap_config_get_integer(&RoadMapConfigLastCheckTimeStamp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_alternative_routes_set_last_check_time_stamp(int time_stamp){
   roadmap_config_set_integer (&RoadMapConfigLastCheckTimeStamp, time_stamp);
   roadmap_config_save(0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_set_suggest_routes (BOOL suggest_at_start) {
   if (suggest_at_start){
      roadmap_config_set (&RoadMapConfigShowSuggested, "yes");
   }else{
      roadmap_config_set (&RoadMapConfigShowSuggested, "no");
   }

   roadmap_config_save (FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget space (int height) {
   SsdWidget space;
   if (roadmap_screen_is_hd_screen ()){
      height *= 2;
   }

   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color (space, NULL, NULL);
   return space;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_pointer_down (SsdWidget this, const RoadMapGuiPoint *point) {

   if (!this)
      return 0;

   ssd_widget_pointer_down_force_click (this, point);

   if (!this->tab_stop)
      return 0;

   if (!this->in_focus)
      ssd_dialog_set_focus (this);
   ssd_dialog_draw ();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void suggest_route_dialog_close (int exit_code) {
   kill_timer ();
   ssd_dialog_hide (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, dec_ok);
   if (exit_code == dec_close) {
      roadmap_trip_remove_point ("Destination");
      roadmap_trip_remove_point ("Departure");
      navigate_main_stop_navigation ();
   }
   roadmap_screen_redraw ();
}

#ifndef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static int Drive_sk_cb(SsdWidget widget, const char *new_value, void *context) {
   on_drive_btn_cb(NULL,NULL);
   return 1;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
static void update_button (void) {
   SsdWidget button;
   char button_txt[20];
   g_seconds--;

   if (g_seconds < 0) {
      on_drive_btn_cb(NULL,NULL);
      return;
   }

   button = ssd_widget_get (dialog, "Drive_button");
   if (g_seconds)
      snprintf (button_txt, sizeof(button_txt),"%s (%d)", roadmap_lang_get ("Drive"), g_seconds);
   else {
      char *dlg_name;
      snprintf (button_txt, sizeof(button_txt), "%s", roadmap_lang_get ("Drive"));
      dlg_name = ssd_dialog_currently_active_name ();
      if (!dlg_name || strcmp (dlg_name, ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME))
         ssd_dialog_set_focus (button);
   }
#ifdef TOUCH_SCREEN
   if (button)
      ssd_button_change_text (button, button_txt);
#else
   ssd_widget_set_right_softkey_callback(dialog, Drive_sk_cb);
   ssd_widget_set_right_softkey_text(dialog, button_txt);
   ssd_dialog_refresh_current_softkeys();
#endif
   roadmap_screen_redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void kill_timer (void) {
   if (timerActive)
      roadmap_main_remove_periodic (update_button);
   timerActive = FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static void set_timer(void){
   roadmap_main_set_periodic (1000, update_button);
   timerActive = TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_close_btn_cb (SsdWidget widget, const char *new_value) {
   roadmap_alternative_routes_set_last_check_time_stamp((int)time(NULL));
   RealtimeAltRoutes_Route_CancelRequest();
   suggest_route_dialog_close (dec_close);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_alt_routes_btn_cb (SsdWidget widget, const char *new_value) {

   AltRouteTrip *pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      roadmap_log (ROADMAP_ERROR,"on_alt_routes_btn_cb - pAltRoute is NULL");
      return 0;
   }
   navigate_main_stop_navigation();
   ssd_dialog_hide (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, dec_ok);
   kill_timer ();

   ssd_progress_msg_dialog_show( roadmap_lang_get( "Calculating alternative routes, please wait..." ) );

   RealtimeAltRoutes_Route_Request (pAltRoute->iTripId, &pAltRoute->srcPosition, &pAltRoute->destPosition, MAX_ROUTES);

   return 1;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_drive_btn_cb (SsdWidget widget, const char *new_value) {
   AltRouteTrip *pAltRoute;
   address_info ai;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   suggest_route_dialog_close (dec_ok);

   if (!pAltRoute){
       return 1;
   }
   ai.city = NULL;
   ai.country = NULL;
   ai.house = NULL;
   ai.state= NULL;
   ai.street = NULL;
   navigate_main_set_dest_pos(&pAltRoute->destPosition);
   Realtime_ReportOnNavigation(&pAltRoute->destPosition, &ai);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int route_select (RoutingContext *context) {
   address_info ai;
   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute){
      roadmap_log (ROADMAP_ERROR,"route_select - pAltRoute is NULL");
      return 0;
   }

   if (!context){
      roadmap_log (ROADMAP_ERROR,"route_select - context is NULL");
      return 0;
   }
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   navigate_main_set_outline (0, context->nav_result->geometry.points,
                  context->nav_result->geometry.num_points, context->nav_result->alt_id, FALSE);
   roadmap_math_set_min_zoom(-1);
   navigate_main_set_route(context->nav_result->alt_id);
   navigate_route_select(context->nav_result->alt_id);
   ssd_dialog_hide_all (dec_close);
   roadmap_log (ROADMAP_INFO,"on_route_selected selecting route alt_id=%d" , pAltRoute->pRouteResults[0].alt_id);
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Please wait..." ) );

   ai.city = NULL;
   ai.country = NULL;
   ai.house = NULL;
   ai.state= NULL;
   ai.street = NULL;
   Realtime_ReportOnNavigation(&pAltRoute->destPosition, &ai);

   suggest_route_dialog_close (dec_ok);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_route_selected (SsdWidget widget, const RoadMapGuiPoint *point) {

   RoutingContext *context;
   context = (RoutingContext *) widget->context;
#ifndef IPHONE_NATIVE
   return route_select (context);
#else
   return roadmap_alternative_route_select (context->id);
#endif

}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_route_show_list (SsdWidget widget, const char *new_value) {
   ssd_dialog_hide_current (dec_close);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_show_route_highlight_dlg_closed (int exit_code, void* context) {
   int offset = -50;
   navigate_main_set_outline (0, NULL, 0, -1, FALSE);
   navigate_main_set_outline (1, NULL, 0, -1, FALSE);
   navigate_main_set_outline (2, NULL, 0, -1, FALSE);
   roadmap_math_set_min_zoom(-1);

   roadmap_trip_set_focus ("GPS");
   roadmap_pointer_unregister_long_click(override_long_click);
   roadmap_object_enable_short_click();
#ifdef TOUCH_SCREEN
#ifndef IPHONE_NATIVE
   if (roadmap_screen_is_hd_screen())
      offset = -110;
   roadmap_screen_obj_global_offset(0, offset);
   roadmap_bottom_bar_show ();
#endif //IPHONE_NATIVE
#endif

}

//////////////////////////////////////////////////////////////////////////////////////////////////
int on_routes_selection_route (SsdWidget widget, const RoadMapGuiPoint *point) {
   RoutingContext *context;
   char name[20];
   widget->force_click  = FALSE;

   navigate_main_set_outline (0, NULL, 0, -1, FALSE);
   navigate_main_set_outline (1, NULL, 0, -1, FALSE);
   navigate_main_set_outline (2, NULL, 0, -1, FALSE);
   roadmap_math_set_min_zoom(-1);

   context = (RoutingContext *) widget->context;

   highligh_selection(context->id);


   navigate_main_set_outline (context->id, context->nav_result->geometry.points,
                     context->nav_result->geometry.num_points, context->id, TRUE);
   sprintf (name, roadmap_lang_get ("Route %d"), context->id + 1);
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);

   ssd_dialog_change_text("title_text",name );
   roadmap_screen_redraw();
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void keypressed_showroute(int route){
   char name[20];
   int i;
   RoutingContext *context;
   SsdWidget dialog = ssd_dialog_get_currently_active();

   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute){
       roadmap_log (ROADMAP_ERROR,"keypressed_showroute - pAltRoute is NULL");
       return;
   }

   if (route >= RealtimeAltRoutes_Get_Num_Routes()){
      return;
   }

   navigate_main_set_outline (0, NULL, 0, -1, FALSE);
   navigate_main_set_outline (1, NULL, 0, -1, FALSE);
   navigate_main_set_outline (2, NULL, 0, -1, FALSE);
   roadmap_math_set_min_zoom(-1);

   highligh_selection(route);
   if (route == -1){
      for (i =0; i<RealtimeAltRoutes_Get_Num_Routes(); i++){
         navigate_main_set_outline (i, pAltRoute->pRouteResults[i].geometry.points,
                        pAltRoute->pRouteResults[i].geometry.num_points, pAltRoute->pRouteResults[i].alt_id, TRUE);
      }
      dialog->context = NULL;
      ssd_dialog_change_text("title_text",roadmap_lang_get ("Compare routes") );
   }else{
      navigate_main_set_outline (route, pAltRoute->pRouteResults[route].geometry.points,
                     pAltRoute->pRouteResults[route].geometry.num_points, pAltRoute->pRouteResults[route].alt_id, TRUE);
      sprintf (name, roadmap_lang_get ("Route %d"), route + 1);
      ssd_dialog_change_text("title_text",name );
      context = malloc (sizeof(RoutingContext));
      context->id = route;
      context->nav_result = &pAltRoute->pRouteResults[route];
      dialog->context = context;
#ifndef TOUCH_SCREEN
      ssd_widget_set_left_softkey_callback(dialog, compare_routes_options_sk_cb);
      ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
      ssd_dialog_refresh_current_softkeys();
#endif
   }
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);

   roadmap_screen_redraw();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL OnKeyPressed( SsdWidget this, const char* utf8char, uint32_t flags)
{

   if( !this)
      return FALSE;


   switch(*utf8char)
   {
      case VK_Arrow_up:
         roadmap_screen_hold ();
         roadmap_screen_move_up();
         break;

      case VK_Arrow_down:
         roadmap_screen_hold ();
         roadmap_screen_move_down();
         break;

      case VK_Arrow_left:
         roadmap_screen_hold ();
         roadmap_screen_move_left();
         break;

      case VK_Arrow_right:
         roadmap_screen_hold ();
         roadmap_screen_move_right();
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


      case '0':
         keypressed_showroute(-1);
         break;

      case '1':
         keypressed_showroute(0);
         break;

      case '2':
         keypressed_showroute(1);
         break;

      case '3':
         keypressed_showroute(2);
         break;
   }

   return FALSE;
}

static roadmap_input_type get_input_type  (SsdWidget widget){
   return inputtype_numeric;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int show_route (RoutingContext *context) {
   SsdWidget dialog, container;
   char title[20];
   AltRouteTrip *pAltRoute;
#ifdef TOUCH_SCREEN
   int offset = 50;
   SsdWidget right_titlle_button;
   const char *right_buttons[] = { "route_list_e", "route_list_p" };
#endif
   navigate_main_set_outline (context->id, context->nav_result->geometry.points,
                  context->nav_result->geometry.num_points, context->id, TRUE);

   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute)
      return 0;
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);

#ifdef TOUCH_SCREEN
#ifndef IPHONE_NATIVE
   roadmap_bottom_bar_hide ();
#endif //IPHONE_NATIVE
#endif
   navigate_bar_set_mode (FALSE);
   sprintf (title, roadmap_lang_get ("Route %d"), context->id + 1);
   roadmap_trip_set_focus ("Alt-Routes");

   dialog = ssd_dialog_new ("Route-Highligh-dlg", title, on_show_route_highlight_dlg_closed,
                  SSD_CONTAINER_TITLE | SSD_DIALOG_TRANSPARENT | SSD_DIALOG_FLOAT);
#ifdef TOUCH_SCREEN
   if (roadmap_screen_is_hd_screen())
       offset = 110;
   roadmap_screen_obj_global_offset(0, offset);
#else
   dialog->context = context;
   ssd_widget_set_left_softkey_callback(dialog, compare_routes_options_sk_cb);
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
   ssd_dialog_refresh_current_softkeys();
#endif
   roadmap_pointer_register_long_click (override_long_click, POINTER_HIGHEST);
   roadmap_object_disable_short_click();

   //Add Empty container for handling keys
   container = ssd_container_new("empty","", SSD_MIN_SIZE, 0,SSD_WS_TABSTOP|SSD_WS_DEFWIDGET);
   ssd_widget_set_color(container, "#ffffff", "#ffffff");
   container->key_pressed = OnKeyPressed;
   ssd_widget_add(dialog, container);
   container->get_input_type = get_input_type;


   ssd_dialog_activate ("Route-Highligh-dlg", NULL);
#ifdef TOUCH_SCREEN
   right_titlle_button = ssd_dialog_right_title_button ();
   ssd_widget_show (right_titlle_button);
   ssd_button_change_icon (right_titlle_button, right_buttons, 2);
   right_titlle_button->callback = on_route_show_list;
   add_routes_selection(dialog);
#endif
   highligh_selection(context->id);
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);

   roadmap_screen_redraw();
   roadmap_screen_unfreeze ();
   return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_route_show (SsdWidget widget, const char *new_value) {
   RoutingContext *context;
   context = (RoutingContext *) widget->context;
   return show_route (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_route_show_all (SsdWidget widget, const char *new_value) {
   SsdWidget dialog, container;
   int i;
   int num_routes;
   AltRouteTrip *pAltRoute;
#ifdef TOUCH_SCREEN
   SsdWidget right_titlle_button;
   int offset = 50;
   const char *right_buttons[] = { "route_list_e", "route_list_p" };
#endif

   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      roadmap_log (ROADMAP_ERROR,"on_route_show_all - pAltRoute is NULL");
      return 1;
   }

   num_routes = RealtimeAltRoutes_Get_Num_Routes();

   for (i = 0; i < num_routes; i++) {
      NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[i];
      navigate_main_set_outline (i, pRouteResults->geometry.points,
                     pRouteResults->geometry.num_points, pRouteResults->alt_id, TRUE);
   }
   dialog = ssd_dialog_new ("Route-Compare-dlg", roadmap_lang_get ("Compare routes"),
                  on_show_route_highlight_dlg_closed, SSD_CONTAINER_TITLE | SSD_DIALOG_TRANSPARENT
                                 | SSD_DIALOG_FLOAT);
#ifdef TOUCH_SCREEN
   if (roadmap_screen_is_hd_screen())
       offset = 110;
   roadmap_screen_obj_global_offset(0, offset);
#endif
   roadmap_pointer_register_long_click (override_long_click, POINTER_HIGHEST);
   roadmap_object_disable_short_click();
   //Add Empty container for handling keys
   container = ssd_container_new("empty","", SSD_MIN_SIZE, 0,SSD_WS_TABSTOP|SSD_WS_DEFWIDGET);
   ssd_widget_set_color(container, "#ffffff", "#ffffff");
   container->key_pressed = OnKeyPressed;
   container->get_input_type = get_input_type;
   ssd_widget_add(dialog, container);

   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   roadmap_trip_set_focus ("Alt-Routes");
#ifdef TOUCH_SCREEN
#ifndef IPHONE_NATIVE
   roadmap_bottom_bar_hide ();
#endif //IPHONE_NATIVE
#endif
   navigate_bar_set_mode (FALSE);
   ssd_dialog_activate ("Route-Compare-dlg", NULL);
#ifdef TOUCH_SCREEN
   right_titlle_button = ssd_dialog_right_title_button ();
   ssd_widget_show (right_titlle_button);
   ssd_button_change_icon (right_titlle_button, right_buttons, 2);
   right_titlle_button->callback = on_route_show_list;
   add_routes_selection(dialog);
#else
   ssd_widget_set_left_softkey_callback(dialog, compare_routes_options_sk_cb);
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
   ssd_dialog_refresh_current_softkeys();

#endif
   highligh_selection(-1);
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);

   roadmap_screen_unfreeze ();
   roadmap_screen_redraw();

   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Not static because of IPHONE code
void highligh_selection(int route){
   SsdWidget container, text ;
   char container_name[20];
   int i;
   SsdWidget dialog = ssd_dialog_get_currently_active();
   int num_routes = RealtimeAltRoutes_Get_Num_Routes ();

   container = ssd_widget_get(dialog, "All");
   text = ssd_widget_get(container, "AllTxt");

   if (route != -1){
      if (text)
         ssd_widget_set_color(text, "#bebebe","#ffffff");
   }
   else{
      if (text)
         ssd_widget_set_color(text, "#ffffff","#ffffff");
   }

   for (i = 0; i < num_routes; i++){

      sprintf (container_name, "%d_route", i + 1);
      container = ssd_widget_get(dialog, container_name);
      if (container)
         container->short_click = on_routes_selection_route;
      text = ssd_widget_get(container, "TimeTxt1");
      if (text)
         ssd_widget_set_color(text, "#bebebe","#ffffff");
      text = ssd_widget_get(container, "TimeTxt2");
      if (text)
         ssd_widget_set_color(text, "#bebebe","#ffffff");

   }

   if (route != -1){
      sprintf (container_name, "%d_route", route + 1);
      container = ssd_widget_get(dialog, container_name);
      if (container)
         container->short_click = on_route_selected;

      text = ssd_widget_get(container, "TimeTxt1");
      if (text)
         ssd_widget_set_color(text, "#ffffff","#ffffff");
      text = ssd_widget_get(container, "TimeTxt2");
      if (text)
         ssd_widget_set_color(text, "#ffffff","#ffffff");
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_routes_selection_all (SsdWidget widget, const char *new_value) {
   AltRouteTrip *pAltRoute;
   int i;
   int num_routes;
   widget->force_click  = FALSE;

   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      roadmap_log (ROADMAP_ERROR,"on_routes_selection_all - pAltRoute is NULL");
      return 1;
   }

   highligh_selection(-1);
   num_routes = RealtimeAltRoutes_Get_Num_Routes();

   for (i = 0; i < num_routes; i++) {
      NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[i];
      navigate_main_set_outline (i, pRouteResults->geometry.points,
                     pRouteResults->geometry.num_points, pRouteResults->alt_id, TRUE);
   }
#ifndef IPHONE_NATIVE
   ssd_dialog_change_text("title_text",roadmap_lang_get ("Compare routes") );
#endif
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);
   roadmap_screen_redraw();

   return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void add_routes_selection(SsdWidget dialog){
   SsdWidget container, box, box2, bitmap, text;
   int i, width, height;
   int minutes;
   int all_container_width;
   int num_routes = RealtimeAltRoutes_Get_Num_Routes ();
   int container_width = roadmap_canvas_width();
   if (roadmap_canvas_height() < roadmap_canvas_width())
      container_width = roadmap_canvas_height();

   if (roadmap_screen_is_hd_screen()){
      height = 90;
      all_container_width = 80;
   }else{
      height = 60;
      all_container_width = 56;
   }
#ifdef IPHONE_NATIVE
   all_container_width = 50;

   dialog = ssd_dialog_new("RouteSelection",NULL,NULL, SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
                  SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);
   ssd_widget_set_size(dialog, 320, SSD_MIN_SIZE);

   container = ssd_container_new ("buttons_bar", "", SSD_MAX_SIZE, 48, 0);
#else
   container = ssd_container_new ("buttons_bar", "", SSD_MAX_SIZE, height,  SSD_CONTAINER_BORDER|
                                                     SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);
#endif

   ssd_widget_set_color (container, NULL, NULL);

#ifndef IPHONE_NATIVE
   width = (container_width - all_container_width - 20) / (num_routes);
#else
   width = (300 - all_container_width) / num_routes;
#endif

   box = ssd_container_new ("All", "", all_container_width, height-6, 0);
   ssd_widget_set_color (box, NULL, NULL);

   bitmap = ssd_bitmap_new ("all_routes", "all_routes", SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
   ssd_widget_add (box, bitmap);
   bitmap = ssd_bitmap_new("separator", "separator", SSD_ALIGN_RIGHT);
   ssd_widget_add(box, bitmap);
   //box->pointer_down = on_pointer_down;
   ssd_widget_set_pointer_force_click( box );
   box->callback = on_routes_selection_all;
   ssd_widget_add (container, box);

   for (i = 0; i < num_routes; i++) {
      NavigateRouteResult *nav_result;
      AltRouteTrip *pAltRoute;
      RoutingContext *context;
      char icon[20];
      char msg[300];

      sprintf (icon, "%d_route", i + 1);
      msg[0] = 0;

      nav_result = RealtimeAltRoutes_Get_Route_Result (i);
      pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
      minutes = (int)(nav_result->total_time / 60.0);
      snprintf (msg , sizeof (msg), "%d",minutes);
      box = ssd_container_new (icon, "", width, 48, 0);

      ssd_dialog_add_vspace(box, 1, 0);
      ssd_dialog_add_hspace(box, 2, 0);
      ssd_widget_set_color (box, NULL, NULL);
      bitmap = ssd_bitmap_new ("1", icon, 0);
      ssd_widget_add (box, bitmap);
      ssd_dialog_add_hspace(box, 3, 0);

      if (i != (num_routes -1)){
         bitmap = ssd_bitmap_new("separator", "separator", SSD_ALIGN_RIGHT);
         ssd_widget_add(box, bitmap);
      }

      box2 = ssd_container_new ("1", "", SSD_MAX_SIZE, SSD_MIN_SIZE, 0);
      ssd_widget_set_color (box2, NULL, NULL);

      if (minutes < 100)
         text = ssd_text_new("TimeTxt1", msg, 24, SSD_ALIGN_CENTER|SSD_END_ROW);
      else
         text = ssd_text_new("TimeTxt1", msg, 24, SSD_ALIGN_CENTER|SSD_END_ROW);
      ssd_widget_set_color(text, "#bebebe","#ffffff");
      ssd_widget_add (box2, text);

      text = ssd_text_new("TimeTxt2", roadmap_lang_get("min."), 14, SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT);
      ssd_widget_set_color(text, "#bebebe","#ffffff");
      ssd_widget_add (box2, text);

      ssd_widget_add(box, box2);


      context = malloc (sizeof(RoutingContext));
      context->id = i;
      context->nav_result = nav_result;

      box->context = (void *) context;
      //box->pointer_down = on_pointer_down;
      ssd_widget_set_pointer_force_click( box );
      box->short_click = on_routes_selection_route;

      ssd_widget_add (container, box);
   }

   ssd_widget_add (dialog, container);
#ifdef IPHONE_NATIVE
   ssd_dialog_activate("RouteSelection", NULL);
#endif
}



#ifndef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_compare_routes_option_selected (BOOL made_selection, ssd_cm_item_ptr item, void* context) {
   alt_routes_context_menu_items selection;

   g_context_menu_is_active = FALSE;

   roadmap_screen_unfreeze();
   if (!made_selection)
      return;

   selection = (alt_routes_context_menu_items) item->id;
   switch (selection) {

      case alt_rt_cm_drive:
         route_select (context);
         break;

      case alt_rt_cm_route_1:
         keypressed_showroute(0);
         break;

      case alt_rt_cm_route_2:
         keypressed_showroute(1);
         break;

      case alt_rt_cm_route_3:
         keypressed_showroute(2);
         break;

      case alt_rt_cm_show_all_routes:
         keypressed_showroute(-1);
         break;

      case alt_rt_cm_cancel:
         g_context_menu_is_active = FALSE;
         roadmap_screen_redraw();
         break;

      default:
         break;
   }
   g_context_menu_is_active = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_routes_option_selected (BOOL made_selection, ssd_cm_item_ptr item, void* context) {
   alt_routes_context_menu_items selection;

   g_context_menu_is_active = FALSE;

   if (!made_selection)
      return;

   selection = (alt_routes_context_menu_items) item->id;

   switch (selection) {

      case alt_rt_cm_drive:
         route_select ((RoutingContext *) context);
         break;

      case alt_rt_cm_show_route:
         show_route ((RoutingContext *) context);
         break;

      case alt_rt_cm_show_all_routes:
         on_route_show_all (NULL, NULL);
         break;

      case alt_rt_cm_cancel:
         g_context_menu_is_active = FALSE;
         roadmap_screen_redraw();
         break;

      default:
         break;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int compare_routes_options (SsdWidget widget, const char *new_value) {
   RoutingContext *context;
   int menu_x;
   int num_routes;
   BOOL have_2_routes;
   BOOL have_3_routes;
   BOOL can_drive ;
   AltRouteTrip *pAltRoute;

   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      return 0;
   }

   num_routes = RealtimeAltRoutes_Get_Num_Routes();

   context = (RoutingContext *) widget->context;
   kill_timer ();
   if (ssd_widget_rtl (NULL))
      menu_x = SSD_X_SCREEN_RIGHT;
   else
      menu_x = SSD_X_SCREEN_LEFT;

   have_2_routes = (num_routes > 1);
   have_3_routes = (num_routes > 2);
   can_drive = (context != NULL);

   ssd_contextmenu_show_item( &compare_routes_context_menu,
                              alt_rt_cm_route_1,
                              have_2_routes,
                              FALSE);

   ssd_contextmenu_show_item( &compare_routes_context_menu,
                              alt_rt_cm_route_2,
                              have_2_routes,
                              FALSE);

   ssd_contextmenu_show_item( &compare_routes_context_menu,
                              alt_rt_cm_route_3,
                              have_3_routes,
                              FALSE);

   ssd_contextmenu_show_item( &compare_routes_context_menu,
                              alt_rt_cm_show_all_routes,
                              have_2_routes && can_drive,
                              FALSE);

   ssd_contextmenu_show_item( &compare_routes_context_menu,
                              alt_rt_cm_drive,
                              can_drive,
                              FALSE);

   ssd_context_menu_show (menu_x, // X
                  SSD_Y_SCREEN_BOTTOM, // Y
                  &compare_routes_context_menu, on_compare_routes_option_selected, context, dir_default, 0, TRUE);

   g_context_menu_is_active = TRUE;
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int routes_options (SsdWidget widget, const char *new_value) {
   RoutingContext *context;
   int menu_x;
   int num_routes;
   BOOL have_2_routes;
   AltRouteTrip *pAltRoute;
   BOOL is_focus_on_route;

   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
       return 0;
   }

   num_routes = RealtimeAltRoutes_Get_Num_Routes();

   context = (RoutingContext *) widget->context;
   is_focus_on_route = (context != NULL);

   kill_timer ();
   if (ssd_widget_rtl (NULL))
      menu_x = SSD_X_SCREEN_RIGHT;
   else
      menu_x = SSD_X_SCREEN_LEFT;

   have_2_routes = (num_routes > 1);


   ssd_contextmenu_show_item( &routes_context_menu,
                               alt_rt_cm_show_route,
                               is_focus_on_route,
                               FALSE);

   ssd_contextmenu_show_item( &routes_context_menu,
                               alt_rt_cm_drive,
                               is_focus_on_route,
                               FALSE);

   ssd_context_menu_show (menu_x, // X
                  SSD_Y_SCREEN_BOTTOM, // Y
                  &routes_context_menu, on_routes_option_selected, context, dir_default, 0, TRUE);

   g_context_menu_is_active = TRUE;
   return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int routes_options_sk_cb (SsdWidget widget, const char *new_value, void *context) {
   SsdWidget selected;
   selected = ssd_dialog_get_focus ();
   routes_options (selected, NULL);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int compare_routes_options_sk_cb (SsdWidget widget, const char *new_value, void *context) {
   compare_routes_options (widget, NULL);
   return 1;
}

#endif //TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static int dont_show_callback (SsdWidget widget, const char *new_value) {
   roadmap_alternative_routes_set_suggest_routes (FALSE);
   roadmap_trip_remove_point ("Destination");
   roadmap_trip_remove_point ("Departure");
   suggest_route_dialog_close (dec_close);

   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static time_of_day tod (void) {
   timeStruct current = navigate_main_get_current_time ();
   if ( (current.hours >= 4) && (current.hours <= 11))
      return tod_morning;
   else if ( (current.hours > 11) && (current.hours <= 16))
      return tod_afternoon;
   else if ( (current.hours > 16) && (current.hours <= 22))
      return tod_evening;
   return tod_night;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget get_bitmap (void) {
   static SsdWidget bitmap;
   char bitmap_name[30];

   if ( (tod () == tod_morning) || (tod () == tod_afternoon)) {
      strcpy (bitmap_name, "sun");
   }
   else {
      strcpy (bitmap_name, "moon");
   }

   bitmap = ssd_bitmap_new ("bg_image", bitmap_name, SSD_ALIGN_CENTER | SSD_WIDGET_SPACE);
   ssd_widget_set_offset (bitmap, 0, -15);
   return bitmap;

}

#ifndef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static int Close_sk_cb(SsdWidget widget, const char *new_value, void *context) {
   on_close_btn_cb (NULL,NULL);
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected( BOOL made_selection,
               ssd_cm_item_ptr item,
               void* context)
{
   alt_routes_context_menu_items selection;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
   return;

   selection = (alt_routes_context_menu_items)item->id;

   switch( selection)
   {

      case alt_rt_cm_drive:
      on_drive_btn_cb(NULL, NULL);
      break;

      case alt_rt_cm_show_alt_routes:
      on_alt_routes_btn_cb(NULL, NULL);
      break;

      case alt_rt_cm_dont_ask_me_again:
      dont_show_callback(NULL, NULL);
      break;

      case alt_rt_cm_ignore:
      on_close_btn_cb (NULL,NULL);
      break;

      default:
      break;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int Options_sk_cb(SsdWidget widget, const char *new_value, void *context) {

   int menu_x;
   BOOL have_routes;

   have_routes = (RealtimeAltRoutes_Get_Num_Routes() > 0);

   kill_timer();
   if (ssd_widget_rtl (NULL))
   menu_x = SSD_X_SCREEN_RIGHT;
   else
   menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item( &suggest_context_menu,
                              alt_rt_cm_show_alt_routes,
                              have_routes,
                              FALSE);

   ssd_context_menu_show( menu_x, // X
                  SSD_Y_SCREEN_BOTTOM, // Y
                  &suggest_context_menu,
                  on_option_selected,
                  NULL,
                  dir_default,
                  0,
                  TRUE);

   g_context_menu_is_active = TRUE;
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_alternative_routes_suggest_route_dialog_set_softkeys(SsdWidget dlg) {

   ssd_widget_set_right_softkey_callback(dlg, Close_sk_cb);
   ssd_widget_set_right_softkey_text(dlg, roadmap_lang_get("Close"));

   ssd_widget_set_left_softkey_callback(dlg, Options_sk_cb);
   ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Options"));

}

#endif //TOUCH_SCREEN

//////////////////////////////////////////////////////////////////////////////////////////////////
static void update_progress(void){
   static int precentage = 0;
   if (precentage < 100)
      precentage += 5;
   else
      precentage = 0;
   ssd_progress_set_value(progress, precentage);
   roadmap_screen_redraw();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void start_progress(){
   roadmap_main_set_periodic(100, update_progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void stop_progress(){
   roadmap_main_remove_periodic(update_progress);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_suggest_dlg_close (int exit_code, void* context){
   stop_progress();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_suggest_dlg_canceled (int exit_code, void* context){
   RealtimeAltRoutes_Route_CancelRequest();
   roadmap_trip_remove_point ("Destination");
   roadmap_trip_remove_point ("Departure");
   navigate_main_stop_navigation ();
   stop_progress();
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_suggest_route_dialog () {
   char *dlg_name;
   SsdWidget bitmap, container, button, dont_show_container, progress_con, inner_con;
   SsdWidget text;
   char msg[250];
   AltRouteTrip *pAltRoute;
   int width;
   int num_trips;
   int row_height = 40;
   int progress_height = 100;

#ifdef IPHONE_NATIVE
   width = 280;
#else
   width = roadmap_canvas_width() - 40;
   if (roadmap_canvas_height() < roadmap_canvas_width())
      width = roadmap_canvas_height() - 40;
#endif

   if (navigate_main_nav_resumed ()) {
      roadmap_log (ROADMAP_ERROR,"roadmap_alternative_routes_suggest_route_dialog Navigation already resumed " );
      return;
   }

   if (roadmap_screen_is_hd_screen ()) {
      row_height = 60;
   }

   if (is_screen_wide()){
      progress_height = 60;
   }


   num_trips = RealtimeAltRoutes_Count ();
   if (num_trips < 1) {
      roadmap_log (ROADMAP_ERROR,"roadmap_alternative_routes_suggest_route_dialog num_trips < 1 " );
      return;
   }

   pAltRoute = RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      roadmap_log (ROADMAP_ERROR,"roadmap_alternative_routes_suggest_route_dialog record 0 is NULL");
      return;
   }

   dlg_name = ssd_dialog_currently_active_name ();
   if (dlg_name && !strcmp (dlg_name, ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME))
   {
      stop_progress();

      // Update ETA Widget
      navigate_res_update_ETA_widget(dialog, pAltRoute->iTripDistance, pAltRoute->iTripLenght, pAltRoute->pRouteResults[0].description, FALSE);

      // Show ETA Widget
      navigate_res_show_ETA_widget(dialog);

#ifdef TOUCH_SCREEN
      // Emable Drive button
      button = ssd_widget_get (dialog, "Drive_button");
      if (button)
         ssd_button_enable(button);
#endif

      progress_con = ssd_widget_get(dialog, "progress container");
      if (progress_con)
         ssd_widget_hide(progress_con);

      //Set Timer on drive button
      g_seconds = SUGGEST_DLG_TIMEOUT/1000;
      set_timer();
      ssd_dialog_set_callback(on_suggest_dlg_close);
      roadmap_alternative_routes_set_last_check_time_stamp((int)time(NULL));
      roadmap_screen_redraw();
      return;
   }
   else{
      if (RealtimeAltRoutes_Get_Num_Routes() > 0) {
         roadmap_trip_remove_point ("Destination");
         roadmap_trip_remove_point ("Departure");
         navigate_main_stop_navigation ();
         return; //user already closed the dialog
      }
   }


   dialog = ssd_dialog_new (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, "", on_suggest_dlg_canceled, SSD_DIALOG_FLOAT
                           | SSD_ALIGN_CENTER | SSD_ROUNDED_CORNERS | SSD_ROUNDED_BLACK | SSD_ALIGN_VCENTER | SSD_CONTAINER_BORDER);

   ssd_widget_add (dialog, space (5));

   inner_con = ssd_container_new ("bg container", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                                    SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW );
   ssd_widget_set_color(inner_con, NULL, NULL);

   // BG image
#ifdef TOUCH_SCREEN
   if (!is_screen_wide()){
      container = ssd_container_new ("bg container", NULL, 20, 30,
            SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW );
      ssd_widget_set_color(container, NULL, NULL);
      bitmap = get_bitmap();
      ssd_widget_add (container, bitmap);
      ssd_widget_add (inner_con, container);
   }
#endif
   if (tod() == tod_morning)
      snprintf(msg, sizeof(msg), "%s ", roadmap_lang_get ("Good morning"));
   else if (tod() == tod_evening)
      snprintf(msg, sizeof(msg), "%s ", roadmap_lang_get ("Good evening"));
   else
      snprintf(msg, sizeof(msg), "%s ", roadmap_lang_get ("Hello"));

   text = ssd_text_new ("Welcome TXT", msg, 18, 0);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add (inner_con, text);
   ssd_dialog_add_hspace(inner_con, 5, 0);

   text = ssd_text_new ("UserName TXT", RealTime_GetUserName(), 18, SSD_END_ROW);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add (inner_con, text);

   ssd_widget_add (inner_con, space (2));
   ssd_widget_add(inner_con, ssd_separator_new("sep", 0));
   ssd_widget_add (inner_con, space (3));
   snprintf(msg, sizeof(msg), "%s:", roadmap_lang_get ("Driving to"));
   text = ssd_text_new ("Alt-Sugg-group", msg, 18, 0);
   ssd_text_set_color(text,"#ffffff");
   ssd_widget_add (inner_con, text);
   ssd_dialog_add_hspace(inner_con, 5, 0);
   snprintf(msg, sizeof(msg), " %s?", roadmap_lang_get (pAltRoute->sDestinationName));
   text = ssd_text_new ("Alt-Sugg-group", msg, 18, SSD_END_ROW);
   ssd_text_set_color(text,"#f6a203");
   ssd_widget_add (inner_con, text);
   ssd_widget_add (inner_con, space (3));

   ssd_widget_add(inner_con, navigate_res_ETA_widget(pAltRoute->iTripDistance, pAltRoute->iTripLenght, "", FALSE, TRUE, on_alt_routes_btn_cb));
   navigate_res_hide_ETA_widget(inner_con);

   // Progress image
    progress_con = ssd_container_new ("progress container", NULL, SSD_MAX_SIZE, progress_height, SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW );
    ssd_widget_set_color(progress_con, NULL, NULL);
    ssd_widget_add (progress_con, space (2));
    text = ssd_text_new ("Progress_text", roadmap_lang_get("Calculating alternative routes..."), 18, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
    ssd_text_set_color(text, "#ffffff");
    ssd_widget_add(progress_con, text);

    progress = ssd_progress_new("progress",0,SSD_ALIGN_VCENTER);
    ssd_progress_set_value(progress, 0);
    ssd_widget_add(progress_con, progress);
    start_progress();
    ssd_widget_add(inner_con, progress_con);

#ifdef TOUCH_SCREEN
   //Add buttons
   ssd_widget_add (inner_con, space (5));

   button = ssd_button_label("Drive_button", roadmap_lang_get("Drive"), SSD_ALIGN_CENTER|SSD_WS_TABSTOP, on_drive_btn_cb);
   ssd_button_disable(button);
   ssd_widget_add(inner_con, button);

   button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER|SSD_WS_TABSTOP|SSD_WS_DEFWIDGET|SSD_END_ROW, on_close_btn_cb);
   ssd_widget_add(inner_con, button);

   // Dont show
   dont_show_container = ssd_container_new ("Alt-Sugg-group2", NULL, SSD_MAX_SIZE, row_height-3,
                                            SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_WS_TABSTOP );
   ssd_widget_set_color(dont_show_container, NULL, NULL);

   text = ssd_text_new ("Dont-Suggest-Txt", roadmap_lang_get ("Don't auto-learn my frequent routes"), 14,
   SSD_END_ROW | SSD_ALIGN_VCENTER|SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text, "#f6a203");

   //ssd_widget_set_offset (text, 10, 0);
   ssd_widget_add (dont_show_container, text);
   dont_show_container->callback = dont_show_callback;
   ssd_widget_add (inner_con, dont_show_container);
#else
   roadmap_alternative_routes_suggest_route_dialog_set_softkeys(dialog);
#endif

   ssd_widget_add (dialog, inner_con);
   ssd_dialog_activate (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, NULL);
   roadmap_screen_redraw();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_alertnative_routes_after_login (void) {
   RoadMapPosition *position;
   RoadMapPosition temp_position;
   BOOL has_reception;
   int last_check;
   int time_now = (int) time(NULL);

   if (navigate_main_nav_resumed ()) {
      roadmap_log (ROADMAP_DEBUG,"roadmap_alertnative_routes_after_login, Navigation already resumed " );
      if (gLoginCallBack)
         (*gLoginCallBack) ();
      return;
   }

   if (navigate_main_state() == 0){
      roadmap_log (ROADMAP_DEBUG,"roadmap_alertnative_routes_after_login, already Navigating" );
      if (gLoginCallBack)
         (*gLoginCallBack) ();
      return;
   }

   if (navigate_main_is_calculating_route()){
      roadmap_log (ROADMAP_DEBUG,"roadmap_alertnative_routes_after_login, already calculating route" );
      if (gLoginCallBack)
         (*gLoginCallBack) ();
      return;
   }

   last_check = roadmap_alternative_routes_get_last_check_time_stamp();
   if ( (last_check != -1) && ((time_now -last_check ) < (3600)) ){
          roadmap_log (ROADMAP_DEBUG,"roadmap_alertnative_routes_after_login, already checked %d minutes ago" , (time_now -last_check )/60 );
          if (gLoginCallBack)
             (*gLoginCallBack) ();
          return;
   }

   has_reception = (roadmap_gps_reception_state () != GPS_RECEPTION_NONE)
                        && (roadmap_gps_reception_state () != GPS_RECEPTION_NA);

   if (has_reception)
      position = (RoadMapPosition*) roadmap_trip_get_position ("GPS");
   else
      position = (RoadMapPosition*) roadmap_trip_get_position ("Location");

   if ( (position == NULL) || IS_DEFAULT_LOCATION( position )) {
      temp_position.latitude = -1;
      temp_position.longitude = -1;
      position = &temp_position;
   }


   roadmap_log (ROADMAP_INFO,"roadmap_alertnative_routes_after_login FindTrip (%d,%d) has_reception=%d ",position->latitude, position->longitude, has_reception );

   Realtime_TripServer_FindTrip (position);
   if (gLoginCallBack)
      (*gLoginCallBack) ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_show_routes_dlg_closed (int exit_code, void* context) {
   int i;
   char box_name[10];

   int num_routes = RealtimeAltRoutes_Get_Num_Routes ();

   for (i = 0; i < num_routes; i++) {
      SsdWidget box;
      sprintf (box_name, "%d_route", i + 1);
      box = ssd_widget_get(ssd_dialog_get_currently_active(), box_name);
      if (box && box->context)
         free(box->context);

   }

   if ((exit_code == dec_ok) || (exit_code == dec_cancel)) {
      roadmap_trip_remove_point ("Destination");
      roadmap_trip_remove_point ("Departure");
      navigate_bar_set_mode (FALSE);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int routes_short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if ( (widget->children != NULL) && (!ssd_widget_short_click (widget->children, point))) {
      on_route_selected (widget, NULL);
   }
   else {
      widget->force_click = FALSE;
   }

   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_route_get_waypoint (int distance, RoadMapPosition *way_point) {
   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      const RoadMapPosition *from;
      from = roadmap_trip_get_focus_position ();
      *way_point = *from;
      return;
   }
   *way_point = pAltRoute->destPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_route_get_src (RoadMapPosition *way_point) {
   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      navigate_get_waypoint (-1, way_point);
      return;
   }
   *way_point = pAltRoute->srcPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef IPHONE_NATIVE
void roadmap_alternative_routes_routes_dialog (void) {
   int i;
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   SsdWidget r_box;
   SsdWidget text;
#ifdef TOUCH_SCREEN
   SsdWidget right_titlle_button;
   const char *right_buttons[] = { "route_map_e", "route_map_p" };
#endif
   int num_routes = RealtimeAltRoutes_Get_Num_Routes ();
   int next_container_width = 40;

   if (roadmap_screen_is_hd_screen ()) {
      next_container_width = 60;
   }

   ssd_progress_msg_dialog_hide ();

   if (num_routes == 0) {
      roadmap_log (ROADMAP_ERROR,"roadmap_alternative_routes_routes_dialog - num_route is 0");
      return;
   }

   dialog = ssd_dialog_new (ALT_ROUTE_ROUTS_DLG_NAME, roadmap_lang_get ("Compare routes"),
                  on_show_routes_dlg_closed, SSD_CONTAINER_TITLE);

   group = ssd_container_new ("alt_group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE
                              | SSD_ALIGN_CENTER | SSD_END_ROW );
   ssd_widget_set_color(group, NULL, NULL);

   ssd_widget_add (group, space (5));
   text = ssd_text_new ("recommend-txt", roadmap_lang_get ("Recommended routes:"), 18, SSD_END_ROW);
   ssd_widget_add (group, text);
   ssd_widget_add (group, space (5));

   for (i = 0; i < num_routes; i++) {
      AltRouteTrip *pAltRoute;
      NavigateRouteResult *nav_result;
      SsdWidget bitmap;
      SsdWidget icon_container;
      SsdWidget text_container, text_container1,text_container2;
      SsdWidget image_con2;
      RoutingContext *context;
      char icon[20];
      char msg[300];
      SsdWidget button_next;
      int route_row_height = 110;
      int icon_cont_width = 50;
      const char *next_button_icon[4] = { "edit_left", "edit_left", "edit_right", "edit_right" };

      if (roadmap_screen_is_hd_screen ()) {
         route_row_height = 160;
         icon_cont_width = 75;
      }

      nav_result = RealtimeAltRoutes_Get_Route_Result (i);
      pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);


      sprintf (icon, "%d_route", i + 1);
      r_box = ssd_container_new ("r_box", NULL, SSD_MAX_SIZE, route_row_height, SSD_WIDGET_SPACE
                     | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE
                     | SSD_CONTAINER_BORDER );

      box = ssd_container_new (icon, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE, SSD_WIDGET_SPACE
                     | SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER | SSD_WS_TABSTOP);

      box->pointer_down = on_pointer_down;
      box->short_click = routes_short_click;
#ifndef TOUCH_SCREEN
      box->callback = routes_options;
#endif
      context = malloc (sizeof(RoutingContext));
      context->id = i;
      context->nav_result = nav_result;
      box->context = (void *) context;
      icon_container = ssd_container_new ("icon_container", NULL, icon_cont_width,
                     route_row_height - 6, SSD_ALIGN_VCENTER);
      ssd_widget_set_color (icon_container, NULL, NULL);
      bitmap = ssd_bitmap_new (icon, icon, SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER);
      ssd_widget_add (icon_container, bitmap);
      ssd_widget_add (box, icon_container);

      text_container = ssd_container_new ("text_container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, 0);
      ssd_widget_set_color (text_container, NULL, NULL);

      text_container1 = ssd_container_new ("text_container1", NULL, SSD_MAX_SIZE, route_row_height/2, 0);
      ssd_widget_set_color (text_container1, NULL, NULL);

      if (nav_result->origin){
         text = ssd_text_new ("route-msg-txt", roadmap_lang_get("(your frequent)"), 12, SSD_ALIGN_RIGHT);
         ssd_widget_add (text_container1, text);
      }

      msg[0] = 0;
      snprintf (msg , sizeof (msg), "%d %s",
                     (int)(nav_result->total_time / 60.0), roadmap_lang_get ("min."));

      text = ssd_text_new ("route-msg-txt", msg, 20, SSD_END_ROW);
      ssd_widget_add (text_container1, text);

      msg[0] = 0;
      snprintf (msg + strlen (msg), sizeof (msg) - strlen (msg), "%d.%d %s",
					 roadmap_math_to_trip_distance(nav_result->total_length), roadmap_math_to_trip_distance_tenths(nav_result->total_length)%10,
                     roadmap_lang_get (roadmap_math_trip_unit ()));
      text = ssd_text_new ("route-msg-txt", msg, 16, SSD_END_ROW);
      ssd_widget_add (text_container1, text);

      ssd_widget_add (text_container1, space (5));

      text_container2 = ssd_container_new ("text_container", NULL, SSD_MAX_SIZE, route_row_height/2, 0);
      ssd_widget_set_color (text_container2, NULL, NULL);
      msg[0] = 0;
      snprintf (msg, sizeof (msg), "%s: %s",
                      roadmap_lang_get("Via"),
                      roadmap_lang_get (nav_result->description));
      text = ssd_text_new ("route-name-txt", msg, 16,
                     SSD_END_ROW);
      ssd_text_set_color(text, "#6a6a6a");
      ssd_widget_add (text_container2, text);
#ifdef TOUCH_SCREEN
      image_con2 = ssd_container_new ("next_icon_container", NULL, next_container_width,
                     route_row_height, SSD_TAB_CONTROL | SSD_ALIGN_VCENTER | SSD_ALIGN_RIGHT);
      ssd_widget_set_color (image_con2, NULL, NULL);
      if (ssd_widget_rtl (NULL)) {
         button_next = ssd_button_new ("next_icon", "next_icon", &next_button_icon[0], 2,
                        SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER, NULL);
      }
      else {
         button_next = ssd_button_new ("next_icon", "next_icon", &next_button_icon[2], 2,
                        SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER, NULL);
      }
      image_con2->callback = on_route_show;
      image_con2->context = (void *) context;
      ssd_widget_add (image_con2, button_next);
      ssd_widget_add (box, image_con2);
#endif
      ssd_widget_add (text_container, text_container1);
      ssd_widget_add (text_container, text_container2);
      ssd_widget_add (box, text_container);

      ssd_widget_add (r_box, box);
      ssd_widget_add (group, r_box);

   }

   ssd_widget_add (group, space (5));
   ssd_widget_add (dialog, group);

   ssd_widget_add (dialog, space(2));
   if (num_routes == 1)
      text = ssd_text_new("DisclaimerTxt",roadmap_lang_get("No valid alternatives were found for this destination"), -1, SSD_END_ROW|SSD_WS_TABSTOP);
   else
      text = ssd_text_new("DisclaimerTxt",roadmap_lang_get("Recommended route may take a bit more time but has less turns and junctions"), -1, SSD_END_ROW|SSD_WS_TABSTOP);
   ssd_widget_add (dialog, text);

#ifdef TOUCH_SCREEN
   ssd_dialog_activate (ALT_ROUTE_ROUTS_DLG_NAME, NULL);
   right_titlle_button = ssd_dialog_right_title_button ();
   ssd_widget_show (right_titlle_button);
   ssd_button_change_icon (right_titlle_button, right_buttons, 2);
   right_titlle_button->callback = on_route_show_all;
#else
   ssd_widget_set_left_softkey_callback(dialog, routes_options_sk_cb);
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
#endif
   ssd_dialog_activate (ALT_ROUTE_ROUTS_DLG_NAME, NULL);
   ssd_dialog_draw();
   roadmap_screen_redraw();

}
#endif //IPHONE_NATIVE
//////////////////////////////////////////////////////////////////////////////////////////////////
void request_route (void) {
   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute){
      roadmap_log (ROADMAP_ERROR,"roadmap_alternative_routes - request_route ApltRoute is NULL");
      return;
   }

   roadmap_main_remove_periodic (request_route);

   roadmap_log (ROADMAP_WARNING,"roadmap_alternative_routes - request_route for %s (id=%d)" ,pAltRoute->sDestinationName,pAltRoute->iTripId);
   RealtimeAltRoutes_TripRoute_Request (pAltRoute->iTripId, &pAltRoute->srcPosition, &pAltRoute->destPosition, 1);
   roadmap_alternative_routes_suggest_route_dialog ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_suggested_trip (void) {
   roadmap_main_set_periodic (200, request_route);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_init (void) {

   roadmap_config_declare_enumeration ("user", &RoadMapConfigShowSuggested, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "yes",
                  "no", NULL);

   roadmap_config_declare ("user", &RoadMapConfigLastCheckTimeStamp, "-1", NULL);

   if (!roadmap_alternative_feature_enabled ()) {
      roadmap_log (ROADMAP_INFO,"Alternative routes - feature disabled" );
      return;
   }

   if (!roadmap_alternative_routes_suggest_routes ()) {
      roadmap_log (ROADMAP_DEBUG,"Alternative routes - Suggest routes off " );
      return;
   }

   gLoginCallBack = Realtime_NotifyOnLogin (roadmap_alertnative_routes_after_login);

}


BOOL roadmap_alternative_routes_suggest_dlg_active(void){
   char *dlg_name;
   dlg_name = ssd_dialog_currently_active_name ();
   if (dlg_name && !strcmp (dlg_name, ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME))
      return TRUE;
   else
      return FALSE;
}
