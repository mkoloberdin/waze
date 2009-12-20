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
#include "roadmap_alternative_routes.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_math.h"
#include "roadmap_trip.h"
#include "navigate/navigate_main.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAltRoutes.h"

#define ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME  "alt-routes-suggets-rout-dlg"
#define ALT_ROUTE_ROUTS_DLG_NAME          "alternative-routes-dlg"
#define SUGGEST_DLG_TIMEOUT              15000
#define ROW_HEIGH                        46
#define ROUTE_ROW_HEIGHT                 70
#define ALT_CLOSE_CLICK_OFFSETS_DEFAULT {-20, -20, 20, 20 };

static SsdClickOffsets sgCloseOffsets = ALT_CLOSE_CLICK_OFFSETS_DEFAULT
;

static RoadMapCallback gLoginCallBack;

static RoadMapConfigDescriptor RoadMapConfigShowSuggested =
      ROADMAP_CONFIG_ITEM("Alternative Routes", "Show Suggested");

static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Alternative Routes", "Feature enabled");

static RoadMapCallback AlternativeRoutesCallback = NULL;

typedef struct {
   int                     id;
   NavigateRouteResult     *nav_result;
} RoutingContext;

/////////////////////////////////////////////////////////////////////
static SsdWidget space (int height) {
   SsdWidget space;
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color (space, NULL, NULL);
   return space;
}

/////////////////////////////////////////////////////////////////////
static int on_pointer_down (SsdWidget this, const RoadMapGuiPoint *point) {

   ssd_widget_pointer_down_force_click (this, point);

   if (!this->tab_stop)
      return 0;

   if (!this->in_focus)
      ssd_dialog_set_focus (this);
   ssd_dialog_draw ();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void kill_timer (void) {

   if (AlternativeRoutesCallback) {
      roadmap_main_remove_periodic (AlternativeRoutesCallback);
      AlternativeRoutesCallback = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void suggest_route_dialog_close (void) {
   kill_timer ();
   ssd_dialog_hide (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, dec_ok);

   roadmap_screen_redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int dont_show_callback (SsdWidget widget, const char *new_value) {
   roadmap_alternative_routes_set_suggest_routes (FALSE);

   suggest_route_dialog_close ();
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_trip_selected (SsdWidget widget, const char *new_value) {
   AltRouteTrip *altRout;
   altRout = (AltRouteTrip *) widget->context;


   roadmap_trip_set_point ("Destination", &altRout->destPosition);
   roadmap_trip_set_point ("Departure", &altRout->srcPosition);
//   const RoadMapPosition * pos = roadmap_trip_get_position ("GPS");
   ssd_progress_msg_dialog_show ("Please wait...");
   RealtimeAltRoutes_Route_Request (altRout->iTripId, &altRout->srcPosition, &altRout->destPosition);
   kill_timer ();

   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_alternative_routes_suggest_routes (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigShowSuggested), "yes"))
      return TRUE;
   return FALSE;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_alternative_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes"))
      return TRUE;
   return FALSE;

}
//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_set_suggest_routes (BOOL suggest_at_start) {
   if (suggest_at_start)
      roadmap_config_set (&RoadMapConfigShowSuggested, "yes");
   else
      roadmap_config_set (&RoadMapConfigShowSuggested, "no");
   roadmap_config_save (TRUE);

}

#ifdef TOUCH_SCREEN
//////////////////////////////////////////////////////////////////////////////////////////////////
static int close_button_callback (SsdWidget widget, const char *new_value) {

   ssd_dialog_hide_current (dec_close);
   return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_suggest_route_dialog () {
   SsdWidget dialog;
   SsdWidget trips_box, trip_container, dont_show_conatiner;
   SsdWidget btn_close;
   SsdWidget bitmap;
   SsdWidget text;
#ifdef TOUCH_SCREEN
   const char *close_icons[] = { "rm_quit", "rm_quit_pressed" };
#endif
   int num_trips;
   int i;

   dialog = ssd_dialog_new (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, "", NULL, SSD_DIALOG_FLOAT
                  | SSD_ALIGN_CENTER | SSD_ROUNDED_CORNERS | SSD_ALIGN_VCENTER | SSD_CONTAINER_BORDER);

#ifdef TOUCH_SCREEN
   btn_close = ssd_button_new ("close", "", close_icons, 2, SSD_ALIGN_RIGHT, close_button_callback);
   ssd_widget_set_click_offsets (btn_close, &sgCloseOffsets);
   ssd_widget_add (dialog, btn_close);
#endif
   bitmap = ssd_bitmap_new ("route-bitmap", "routes", SSD_ALIGN_CENTER | SSD_END_ROW);
   ssd_widget_add (dialog, bitmap);
   ssd_widget_add (dialog, space (5));
   text = ssd_text_new ("Alt-Sugg-group", roadmap_lang_get (
                  "Would you like to see the fastest route to:"), 18, SSD_END_ROW
                  | SSD_ALIGN_CENTER);
   ssd_widget_add (dialog, text);
   ssd_widget_add (dialog, space (5));

   num_trips = RealtimeAltRoutes_Count ();

   trips_box = ssd_container_new ("Alt-Sugg-trip_container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                  SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_ROUNDED_CORNERS
                                 | SSD_ROUNDED_WHITE | SSD_CONTAINER_BORDER);
   for (i = 0; i < num_trips; i++) {
      AltRouteTrip *altRout;
      SsdWidget icon_container;
      SsdWidget bitmap;
      SsdWidget button;
      const char* bitmap_name;
      const char *buttons[2];
      const char *edit_button[] = {"edit_right", "edit_left"};

      icon_container = ssd_container_new ("icon_container", NULL, ROW_HEIGH, ROW_HEIGH - 6, SSD_ALIGN_VCENTER);
      ssd_widget_set_color (icon_container, NULL, NULL);
      trip_container = ssd_container_new ("Alt-Sugg-trip_container", NULL, SSD_MAX_SIZE, ROW_HEIGH,
                     SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_WS_TABSTOP);
      altRout = RealtimeAltRoutes_Get_Record (i);

      if (!strcmp (roadmap_lang_get (altRout->sDestinationName), roadmap_lang_get ("home")) || !strcmp (roadmap_lang_get (altRout->sDestinationName),"home")) {
         bitmap_name = "home";
      }
      else if (!strcmp (roadmap_lang_get (altRout->sDestinationName), roadmap_lang_get ("work")) || !strcmp (roadmap_lang_get (altRout->sDestinationName), "work")) {
         bitmap_name = "work";
      }
      else {
         bitmap_name = "favorite";
      }
      bitmap = ssd_bitmap_new ("TripIcon", bitmap_name, SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER);
      ssd_widget_add (icon_container, bitmap);
      ssd_widget_add (trip_container, icon_container);
      text = ssd_text_new ("TripNameTxt", roadmap_lang_get (altRout->sDestinationName), 18,
                     SSD_END_ROW | SSD_ALIGN_VCENTER);

      trip_container->pointer_down = on_pointer_down;
      trip_container->callback = on_trip_selected;
      trip_container->context = (void *) altRout;
      if (!ssd_widget_rtl(NULL)){
               buttons[0] = edit_button[0];
               buttons[1] = edit_button[0];
      }else{
               buttons[0] = edit_button[1];
               buttons[1] = edit_button[1];
      }
      button = ssd_button_new ("edit_button", "", &buttons[0], 2,
                                SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_trip_selected);
      ssd_widget_add(trip_container, button);
      ssd_widget_add (trip_container, text);
      ssd_widget_add (trips_box, trip_container);
      if (i < (num_trips - 1))
         ssd_widget_add (trips_box, ssd_separator_new ("separator", SSD_END_ROW));
   }
   ssd_widget_add (dialog, trips_box);

   ssd_widget_add (dialog, space (5));

   dont_show_conatiner = ssd_container_new ("Alt-Sugg-group2", NULL, SSD_MAX_SIZE, ROW_HEIGH - 6,
                  SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_WS_TABSTOP
                                 | SSD_ROUNDED_CORNERS | SSD_ROUNDED_WHITE | SSD_CONTAINER_BORDER);
   text = ssd_text_new ("Dont-Suggest-Txt", roadmap_lang_get ("Don't ask me again"), 16,
                  SSD_END_ROW | SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER);
   ssd_widget_set_offset (text, 10, 0);
   ssd_widget_add (dont_show_conatiner, text);
   dont_show_conatiner->callback = dont_show_callback;
   dont_show_conatiner->pointer_down = on_pointer_down;
   ssd_widget_add (dialog, dont_show_conatiner);
   ssd_widget_add (dialog, space (5));
   AlternativeRoutesCallback = suggest_route_dialog_close;
   roadmap_main_set_periodic (SUGGEST_DLG_TIMEOUT, suggest_route_dialog_close);
   ssd_dialog_activate (ALT_ROUTE_SUGGEST_ROUTE_DLG_NAME, NULL);
   if (!roadmap_screen_refresh ())
      roadmap_screen_redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_alertnative_routes_after_login (void) {
   RoadMapPosition *position;

   BOOL has_reception = (roadmap_gps_reception_state () != GPS_RECEPTION_NONE)
                  && (roadmap_gps_reception_state () != GPS_RECEPTION_NA);

   if (has_reception)
      position = (RoadMapPosition*) roadmap_trip_get_position ("GPS");
   else
      position = (RoadMapPosition*) roadmap_trip_get_position ("Location");

   if ( (position == NULL) || IS_DEFAULT_LOCATION( position )) {
      position = malloc (sizeof(RoadMapPosition));
      roadmap_check_allocated(position);
      position->latitude = -1;
      position->longitude = -1;
   }

   Realtime_TripServer_FindTrip (position);
   if (gLoginCallBack)
      (*gLoginCallBack) ();
}

void on_show_route_highlight_dlg_closed   (int exit_code, void* context){
   navigate_main_set_outline(NULL, 0, -1);
}

void on_show_routes_dlg_closed   (int exit_code, void* context){
   roadmap_trip_remove_point("Destination");
   roadmap_trip_remove_point("Departure");
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_route_selected (SsdWidget widget, const char *new_value) {
   RoutingContext *context;
   SsdWidget dialog;
   char title[20];
   context = (RoutingContext *)widget->context;
   navigate_main_set_outline(context->nav_result->geometry.points, context->nav_result->geometry.num_points, context->id);
   //navigate_route_select(nav_result->alt_id);

   sprintf(title, roadmap_lang_get("Route %d"), context->id+1);
   dialog = ssd_dialog_new ("Route-Highligh-dlg", title, on_show_route_highlight_dlg_closed,
                           SSD_CONTAINER_TITLE|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_FLOAT);

   ssd_dialog_activate("Route-Highligh-dlg", NULL);
   roadmap_screen_unfreeze();
   roadmap_screen_refresh();
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_routes_dialog (void) {
   int i;
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   SsdWidget text;
   int num_routes = RealtimeAltRoutes_Get_Num_Routes ();

   ssd_progress_msg_dialog_hide ();

   if (num_routes == 0) {
      // todo
   }

   dialog = ssd_dialog_new (ALT_ROUTE_ROUTS_DLG_NAME, roadmap_lang_get ("Routes"), on_show_routes_dlg_closed,
                  SSD_CONTAINER_TITLE);
   group = ssd_container_new ("alt_group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                  SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_ROUNDED_CORNERS
                                 | SSD_ROUNDED_WHITE | SSD_CONTAINER_BORDER);

   ssd_widget_add (group, space (5));
   text = ssd_text_new ("recommend-txt", roadmap_lang_get ("waze proposed route:"), 18, SSD_END_ROW);
   ssd_widget_add (group, text);
   ssd_widget_add (group, space (5));

   for (i = 0; i < num_routes; i++) {
      NavigateRouteResult *nav_result;
      SsdWidget bitmap;
      SsdWidget icon_container;
      SsdWidget text_container;
      RoutingContext *context;
      char icon[20];
      char msg[300];

      nav_result = RealtimeAltRoutes_Get_Route_Result (i);

      if (i == 1) {
         ssd_widget_add (group, space (10));
         ssd_widget_add (group, ssd_separator_new ("separator", SSD_END_ROW));
         ssd_widget_add (group, space (5));
         text = ssd_text_new ("recommend-txt", roadmap_lang_get ("Your routes:"), 18, SSD_END_ROW);
         ssd_widget_add (group, text);
         ssd_widget_add (group, space (5));
      }

      sprintf (icon, "%d_route", i + 1);
      box = ssd_container_new (icon, NULL, SSD_MAX_SIZE, ROUTE_ROW_HEIGHT,
                     SSD_WIDGET_SPACE | SSD_ALIGN_CENTER | SSD_END_ROW | SSD_ROUNDED_CORNERS
                                    | SSD_ROUNDED_WHITE | SSD_CONTAINER_BORDER | SSD_WS_TABSTOP);
      box->pointer_down = on_pointer_down;
      box->callback = on_route_selected;
      context = malloc (sizeof (RoutingContext));
      context->id = i;
      context->nav_result = nav_result;
      box->context = (void *)context;
      icon_container = ssd_container_new ("icon_container", NULL, ROW_HEIGH, ROUTE_ROW_HEIGHT - 6,
                     SSD_ALIGN_VCENTER);
      ssd_widget_set_color (icon_container, NULL, NULL);
      bitmap = ssd_bitmap_new (icon, icon, SSD_ALIGN_CENTER | SSD_ALIGN_VCENTER);
      ssd_widget_add (icon_container, bitmap);
      ssd_widget_add (box, icon_container);

      text_container = ssd_container_new ("text_container", NULL, SSD_MAX_SIZE, ROUTE_ROW_HEIGHT
                     - 10, SSD_ALIGN_VCENTER);
      ssd_widget_set_color (text_container, NULL, NULL);
      msg[0] = 0;
      snprintf (msg + strlen (msg), sizeof (msg) - strlen (msg), "%.1f %s, %.1f %s",
                     nav_result->total_time / 60.0, roadmap_lang_get ("minutes"),
                     nav_result->total_length / 1000.0,
                     roadmap_lang_get (roadmap_math_trip_unit ()));

      text = ssd_text_new ("route-msg-txt", msg, 18, SSD_END_ROW);
      ssd_widget_add (text_container, text);
      ssd_widget_add (text_container, space (5));
      text = ssd_text_new ("route-name-txt", roadmap_lang_get (nav_result->description), 16,
                     SSD_END_ROW);
      ssd_widget_add (text_container, text);
      ssd_widget_add (box, text_container);
      ssd_widget_add (group, box);

   }
   ssd_widget_add (group, space (5));
   ssd_widget_add (dialog, group);
   ssd_dialog_activate (ALT_ROUTE_ROUTS_DLG_NAME, NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_alternative_routes_init (void) {

   //TEMP_AVI
   return;

   roadmap_config_declare_enumeration ("user", &RoadMapConfigShowSuggested, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "yes",
                  "no", NULL);

   if (!roadmap_alternative_feature_enabled ()) {
      roadmap_log (ROADMAP_INFO,"Alternative routes - feature disabled" );
      return;
   }

   if (!roadmap_alternative_routes_suggest_routes ())
   return;

  gLoginCallBack = Realtime_NotifyOnLogin (roadmap_alertnative_routes_after_login);

}

