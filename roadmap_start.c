/* roadmap_start.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   (c) Copyright 2002, 2003 Pascal F. Martin
 *   (c) Copyright 2008 Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   void roadmap_start (int argc, char **argv);
 */

#include <stdlib.h>
#include <string.h>

#ifdef ROADMAP_DEBUG_HEAP
#include <mcheck.h>
#endif

#include "roadmap.h"
#include "roadmap_copyright.h"
#include "roadmap_dbread.h"
#include "roadmap_math.h"
#include "roadmap_string.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_sunrise.h"

#include "roadmap_address.h"
#include "roadmap_spawn.h"
#include "roadmap_path.h"
#include "roadmap_net.h"
#include "roadmap_io.h"
#include "roadmap_state.h"
#include "roadmap_object.h"
#include "roadmap_voice.h"
#include "roadmap_gps.h"
#include "roadmap_car.h"
#include "roadmap_canvas.h"
#include "roadmap_map_settings.h"
#include "roadmap_download_settings.h"
#include "roadmap_preferences.h"
#include "roadmap_coord.h"
#include "roadmap_crossing.h"
#include "roadmap_sprite.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_adjust.h"
#include "animation/roadmap_animation.h"
#include "roadmap_screen.h"
#include "roadmap_view.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_label.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_copy.h"
#include "roadmap_httpcopy.h"
#include "roadmap_download.h"
#include "roadmap_factory.h"
#include "roadmap_res.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_help.h"
#include "roadmap_pointer.h"
#include "roadmap_sound.h"
#include "roadmap_prompts.h"
#include "roadmap_lang.h"
#include "roadmap_skin.h"
#include "roadmap_start.h"
#include "roadmap_state.h"
#include "roadmap_layer.h"
#include "roadmap_trip.h"
#include "roadmap_line_route.h"
#include "roadmap_search.h"
#include "roadmap_alerter.h"
#include "roadmap_bar.h"
#include "roadmap_device.h"
#include "roadmap_power.h"
#include "roadmap_softkeys.h"
#include "roadmap_border.h"
#include "roadmap_mood.h"
#include "roadmap_ticker.h"
#include "roadmap_message_ticker.h"
#include "roadmap_warning.h"
#include "roadmap_general_settings.h"
#include "roadmap_social.h"
#include "roadmap_foursquare.h"
#include "roadmap_camera_image.h"
#include "roadmap_welcome_wizard.h"
#include "roadmap_tripserver.h"
#include "roadmap_analytics.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertsList.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "Realtime/RealtimeTrafficInfoPlugin.h"
#include "Realtime/RealtimePrivacy.h"
#include "Realtime/RealtimeAltRoutes.h"
#include "Realtime/RealtimeTrafficDetection.h"
#include "Realtime/RealtimeSystemMessage.h"
#include "roadmap_geo_config.h"
#include "roadmap_device_events.h"
#include "roadmap_alternative_routes.h"
#include "roadmap_debug_info.h"
#include "roadmap_map_download.h"
#include "roadmap_ticker.h"
#include "roadmap_reminder.h"
#include "roadmap_splash.h"
#include "roadmap_browser.h"
#include "roadmap_scoreboard.h"
#include "roadmap_social_image.h"
#include "roadmap_groups.h"
#include "roadmap_groups_settings.h"
#include "Realtime/RealtimeBonus.h"
#include "Realtime/RealtimeExternalPoi.h"
#include "roadmap_speedometer.h"
#include "roadmap_recommend.h"

#ifdef SSD
#include "ssd/ssd_widget.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_login.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_popup.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32

ssd_contextmenu_ptr s_main_menu = NULL;
ssd_contextmenu_ptr s_alerts_menu = NULL;

#ifdef  TOUCH_SCREEN
const char*   grid_menu_labels[256];
#endif  //  TOUCH_SCREEN

#endif


#include "navigate/navigate_main.h"
#include "editor/editor_main.h"
#include "editor/track/editor_track_main.h"
#include "editor/editor_screen.h"
#include "editor/db/editor_db.h"
#include "editor/static/update_range.h"
#include "editor/static/add_alert.h"
#include "editor/static/edit_marker.h"
#include "editor/static/notes.h"
#include "editor/export/editor_report.h"
#include "editor/export/editor_download.h"
#include "editor/track/editor_track_main.h"
#include "editor/export/editor_sync.h"
#include "editor/editor_points.h"
#include "editor/static/editor_street_bar.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeTrafficInfo.h"
#include "roadmap_keyboard.h"
#include "roadmap_phone_keyboard.h"
#include "roadmap_input_type.h"
#include "address_search/address_search.h"
#include "address_search/local_search.h"
#include "address_search/single_search.h"
#include "roadmap_geo_location_info.h"
#include "roadmap_scoreboard.h"
#ifdef IPHONE
#include "iphone/roadmap_location.h"
#include "iphone/roadmap_settings.h"
#include "iphone/roadmap_list_menu.h"
#include "iphone/roadmap_introduction.h"
#include "iphone/roadmap_urlscheme.h"
#include "iphone/roadmap_media_player.h"
#include "roadmap_scoreboard.h"
#endif //IPHONE

static const char *RoadMapMainTitle = "Waze";

#define MAX_ACTIONS 200

static int RoadMapStartFrozen = 0;

static RoadMapDynamicString RoadMapStartGpsID;

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralUserUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                        ROADMAP_CONFIG_ITEM("General", "Keyboard");

static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");

static RoadMapConfigDescriptor RoadMapConfigMapCredit =
                        ROADMAP_CONFIG_ITEM("Map", "Credit");

static RoadMapConfigDescriptor RoadMapConfigFirstTimeUse =
                        ROADMAP_CONFIG_ITEM("General", "First use");

static RoadMapConfigDescriptor RoadMapConfigClosedProperly =
                        ROADMAP_CONFIG_ITEM("General", "Closed properly");

static RoadMapConfigDescriptor RoadMapConfigRateUsShown =
                        ROADMAP_CONFIG_ITEM("General", "Rate us shown");
static RoadMapConfigDescriptor RoadMapConfigRateUsTime =
                        ROADMAP_CONFIG_ITEM("General", "Rate us time");
static RoadMapConfigDescriptor RoadMapConfigRateUsVer =
                        ROADMAP_CONFIG_ITEM("General", "Rate us ver");
static RoadMapConfigDescriptor RoadMapConfigRateUsMinPeriod =
                        ROADMAP_CONFIG_ITEM("General", "Rate us min period"); //Min period since last time "rate us" shown
static RoadMapConfigDescriptor RoadMapConfigFBShareMinPeriod =
                        ROADMAP_CONFIG_ITEM("General", "FB Share min period"); //Min period to show FB share after "rate us" shown
static RoadMapConfigDescriptor RoadMapConfigFBShareShown =
                        ROADMAP_CONFIG_ITEM("General", "FB Share shown");

extern RoadMapConfigDescriptor RoadMapConfigGeneralLogLevel;

static RoadMapMenu LongClickMenu;
#ifndef SSD
static RoadMapMenu QuickMenu;
#endif

static RoadMapStartSubscriber  RoadMapStartSubscribers = NULL;
static RoadMapScreenSubscriber roadmap_start_prev_after_refresh = NULL;

static RoadMapCallback StartNextLoginCb = NULL;
static BOOL RoadMapStartAfterCrash = FALSE;
static BOOL RoadMapStartInitialized = FALSE;

typedef struct tag_RoadMapActionOverride
{
   char *name;
   char *new_label;
   char *new_icon;
}  RoadMapActionOverride;

#define MAX_OVERRIDE 20

static RoadMapActionOverride RoadMapActionsOverride[MAX_OVERRIDE + 1] = {
   {NULL, NULL, NULL}
};


/* The menu and toolbar callbacks: --------------------------------------- */

static void roadmap_start_periodic (void);
static BOOL on_key_pressed( const char* utf8char, uint32_t flags);
void roadmap_confirmed_exit(void);
void start_zoom_side_menu(void);
void start_rotate_side_menu(void);
void start_map_options_side_menu(void);
void start_alerts_menu(void);
void start_map_updates_menu(void);
void display_shortcuts(void);
static int bottom_bar_status(void);
RoadMapAction *roadmap_start_find_action_un_const (const char *name);
void mark_location(void);
void save_location(void);
void gps_network_status(void);
void roadmap_start_continue(void);
static void viewMyPoints(void);
static void roadmap_start_hide_current_dialog ( void );

#ifndef IPHONE_NATIVE
void roadmap_help_menu(void);
#endif
static void roadmap_start_console (void) {

   const char *url = roadmap_gps_source();

   if (url == NULL) {
      roadmap_spawn ("roadgps", "");
   } else {
      char arguments[1024];
      snprintf (arguments, sizeof(arguments), "--gps=%s", url);
      roadmap_spawn ("roadgps", arguments);
   }
}

static void roadmap_start_purge (void) {
   roadmap_history_purge (10);
}

static void roadmap_start_show_destination (void) {
    roadmap_trip_set_focus ("Destination");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_location (void) {
    roadmap_trip_set_focus ("Address");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps (void) {
    roadmap_screen_hold ();
    roadmap_trip_set_focus ("GPS");
    roadmap_screen_refresh ();
    roadmap_state_refresh ();//Added by AviR

}

static void roadmap_start_hold_map (void) {
   roadmap_start_periodic (); /* To make sure the map is current. */
   roadmap_screen_hold ();
}

static void roadmap_start_rotate (void) {
    roadmap_screen_rotate (10);
}

static void roadmap_start_counter_rotate (void) {
    roadmap_screen_rotate (-10);
}


static int about_callbak (SsdWidget widget, const char *new_value) {
   if (strcmp(widget->name, "exit")) {
      ssd_dialog_hide ("about_exit", dec_ok);
   } else {
      roadmap_main_exit();
   }

   return 0;
}

static void about_exit_dialog(char *exit_msg){

   SsdWidget dialog;
   SsdWidget text;
   SsdWidget button;

   if ( !ssd_dialog_exists( "about_exit" ) )
   {
	   dialog = ssd_dialog_new ( "about_exit", "", NULL,
			 SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
			 SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);

	   ssd_widget_set_color (dialog, "#000000", "#ff0000000");
	   ssd_widget_add (dialog,
		  ssd_container_new ("spacer1", NULL, 0, 20, SSD_END_ROW));

	   text =  ssd_text_new ("text", exit_msg, 14, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT);
	   ssd_text_set_color(text,"#ffffff");
	   ssd_widget_add (dialog,text);

	   /* Spacer */
	   ssd_widget_add (dialog,
 	   ssd_container_new ("spacer2", NULL, 0, 20, SSD_END_ROW|SSD_START_NEW_ROW));
           button = ssd_button_label ("exit", roadmap_lang_get ("Turn off"),
                                 SSD_ALIGN_CENTER|SSD_WS_DEFWIDGET|
                                 SSD_WS_TABSTOP,
                                 about_callbak);
      ssd_widget_add(button, ssd_bitmap_new("turn_off", "turn_off", 0));
      ssd_widget_add(dialog, button);

      ssd_widget_add (dialog,
                      ssd_button_label ("cancel", roadmap_lang_get ("Cancel"),
                                        SSD_ALIGN_CENTER|SSD_WS_DEFWIDGET|
                                        SSD_WS_TABSTOP,
                                        about_callbak));
   }

   ssd_dialog_activate("about_exit", NULL);
   roadmap_screen_redraw();
}

static void roadmap_start_about_exit (void) {
#ifdef IPHONE_NATIVE
	roadmap_main_show_root(0);
#endif //IPHONE_NATIVE

   char exit_msg[500];
#ifdef IPHONE

   sprintf (exit_msg, "%s\n%s",
            roadmap_lang_get("Are you sure you want to turn off waze?"),
            roadmap_lang_get("FYI waze automatically shuts down when not used in background"));
#else
   sprintf (exit_msg, "%s",
            roadmap_lang_get("Are you sure you want to turn off waze?"));
#endif

   about_exit_dialog(exit_msg);
}

static void roadmap_start_export_data (void) {

}

static void roadmap_start_export_reset (void) {

   //editor_export_reset_dirty ();
}

static void roadmap_start_download_map (void) {

   //editor_download_update_map (NULL);
}

static void roadmap_start_create_trip (void) {

    roadmap_trip_new ();
}

static void roadmap_start_open_trip (void) {

    roadmap_trip_load (NULL, 0);
}

static void roadmap_start_save_trip (void) {

    roadmap_trip_save (roadmap_trip_current());
}

static void roadmap_start_save_trip_as (void) {

    roadmap_trip_save (NULL);
}

static void roadmap_start_trip (void) {

    roadmap_trip_start ();
}

static void roadmap_start_trip_resume (void) {

    roadmap_trip_resume ();
}

static void roadmap_start_trip_reverse (void) {

    roadmap_trip_reverse ();
}

static void roadmap_start_navigate (void) {

    navigate_main_calc_route ();
}


void save_destination_to_history(){
   int number;
   int layers[128];
   int layers_count;
   RoadMapNeighbour neighbours[2];
   PluginStreetProperties properties;
   static const char *argv[ahi__count];
   char  temp[15];
   RoadMapPosition *position =  (RoadMapPosition *)roadmap_trip_get_position("Destination");
   if (!position)
      return;

   layers_count = roadmap_layer_all_roads(layers, 128);
   roadmap_math_set_context(position, 20);
   number = roadmap_street_get_closest(position, 0, layers, layers_count,
                     1, &neighbours[0], 1);
   if (number <= 0)
      return;
   roadmap_plugin_get_street_properties (&neighbours[0].line, &properties, 0);
   roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
   argv[0] = strdup(properties.address);
   argv[1] = strdup(properties.street);
   argv[2] = strdup(properties.city);
   argv[3] = ""; //state
   argv[4] = "";
   sprintf(temp, "%d", position->latitude);
   argv[5] = strdup(temp);
   sprintf(temp, "%d", position->longitude);
   argv[6] = strdup(temp);
   argv[ahi_synced] = "false";
   roadmap_history_add ('A', argv);
   roadmap_history_save();
}

static void roadmap_start_set_destination (void) {
   roadmap_analytics_log_event (ANALYTICS_EVENT_NAVIGATE, ANALYTICS_EVENT_INFO_SOURCE,  "MAP" );
    roadmap_trip_set_selection_as ("Destination");
    ssd_dialog_hide_current(dec_close);
    roadmap_screen_refresh();
    navigate_main_route ();
    save_destination_to_history();
}

static void roadmap_start_set_departure (void) {

    roadmap_trip_set_selection_as ("Departure");
    ssd_dialog_hide_current(dec_close);
    roadmap_screen_refresh();
}

static void roadmap_start_set_waypoint (void) {

    const char *id = roadmap_display_get_id ("Selected Street");

    if (id != NULL) {
       roadmap_trip_set_selection_as (id);
       roadmap_screen_refresh();
    }
}

static void roadmap_start_delete_waypoint (void) {

    roadmap_trip_remove_point (NULL);
}

static int roadmap_start_no_download (int fips) {

   if (! roadmap_download_blocked (fips)) {
      roadmap_log (ROADMAP_WARNING, "cannot open map database usc%05d", fips);
      roadmap_download_block (fips);
   }
   return 0;
}

static void roadmap_start_toggle_download (void) {

   if (roadmap_download_enabled()) {

      roadmap_download_subscribe_when_done (NULL);
      roadmap_locator_declare (&roadmap_start_no_download);

   } else {

      static int ProtocolInitialized = 0;

      if (! ProtocolInitialized) {

         /* PLUGINS NOT SUPPORTED YET.
          * roadmap_plugin_load_all
          *      ("download", roadmap_download_subscribe_protocol);
          */

         roadmap_copy_init (roadmap_download_subscribe_protocol);
         roadmap_httpcopy_init (roadmap_download_subscribe_protocol);

         ProtocolInitialized = 1;
      }

      roadmap_download_subscribe_when_done (roadmap_screen_redraw);
      //roadmap_locator_declare (roadmap_download_get_county);
      roadmap_download_unblock_all ();
   }

   roadmap_screen_redraw ();
}


static void roadmap_start_detect_receiver (void) {

    roadmap_gps_detect_receiver ();
}

static void roadmap_confirmed_start_sync_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    editor_report_markers ();
    export_sync ();
}

static void roadmap_start_sync_data (void) {
    ssd_confirm_dialog ("Warning", "Sync requires large amount of data, continue?", TRUE, roadmap_confirmed_start_sync_callback , NULL);
}


static void roadmap_start_quick_menu (void);


#ifdef IPHONE
void start_home  (void);
void start_help_support  (void);
#endif

#ifdef   TESTING_MENU
void do_nothing(void)
{}
#endif   // TESTING_MENU


/* The RoadMap menu and toolbar items: ----------------------------------- */

/* This table lists all the RoadMap actions that can be initiated
 * fom the user interface (a sort of symbol table).
 * Any other part of the user interface (menu, toolbar, etc..)
 * will reference an action.
 */
RoadMapAction RoadMapStartActions[MAX_ACTIONS + 1] = {

   {"preferences", "Preferences", "Preferences", "P",
      "Open the preferences editor", roadmap_preferences_edit},
#ifndef IPHONE
   {"gpsconsole", "GPS Console", "Console", "C",
      "Start the GPS console application", roadmap_start_console},
#endif
   {"mutevoice", "Mute Voice", "Mute", NULL,
      "Mute all voice annoucements", roadmap_voice_mute},

   {"enablevoice", "Enable Voice", "Mute Off", NULL,
      "Enable all voice annoucements", roadmap_voice_enable},

   {"nonavigation", "Disable Navigation", "Nav Off", NULL,
      "Disable all navigation functions", roadmap_navigate_disable},

   {"navigation", "Enable Navigation", "Nav On", NULL,
      "Enable all GPS-based navigation functions", roadmap_navigate_enable},

   {"logtofile", "Log to File", "Log", NULL,
      "Save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"nolog", "Disable Log", "No Log", NULL,
      "Do not save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"purgelogfile", "Purge Log File", "Purge", NULL,
      "Delete the current postmortem log file", roadmap_log_purge},

   {"purgehistory", "Purge History", "Forget", NULL,
      "Remove all but the 10 most recent addresses", roadmap_start_purge},

   {"quit", "Quit", NULL, NULL,
      "Quit RoadMap", roadmap_confirmed_exit},

   {"zoomin", "Zoom In", "+", NULL,
      "Enlarge the central part of the map", roadmap_screen_zoom_in},

   {"zoomout", "Zoom Out", "-", NULL,
      "Show a larger area", roadmap_screen_zoom_out},

   {"zoom1", "Normal Size", ":1", NULL,
      "Set the map back to the default zoom level", roadmap_screen_zoom_reset},

   {"up", "Up", "N", NULL,
      "Move the map view upward", roadmap_screen_move_up},

   {"left", "Left", "W", NULL,
      "Move the map view to the left", roadmap_screen_move_left},

   {"right", "Right", "E", NULL,
      "Move the map view to the right", roadmap_screen_move_right},

   {"down", "Down", "S", NULL,
      "Move the map view downward", roadmap_screen_move_down},

   {"toggleview", "Toggle view mode", "M", NULL,
      "Toggle view mode 2D / 3D", roadmap_screen_toggle_view_mode},

   {"toggleskin", "Toggle skin", "", NULL,
      "Toggle skin (day / night)", roadmap_skin_toggle},

   {"toggleorientation", "Toggle orientation mode", "", NULL,
      "Toggle orientation mode dynamic / fixed",
      roadmap_screen_toggle_orientation_mode},

   {"IncHorizon", "Increase Horizon", "I", NULL,
      "Increase the 3D horizon", roadmap_screen_increase_horizon},

   {"DecHorizon", "Decrease Horizon", "DI", NULL,
      "Decrease the 3D horizon", roadmap_screen_decrease_horizon},

   {"clockwise", "Rotate Clockwise", "R+", "K",
      "Rotate the map view clockwise", roadmap_start_rotate},

   {"counterclockwise", "Rotate Counter-Clockwise", "R-", "J",
      "Rotate the map view counter-clockwise", roadmap_start_counter_rotate},

   {"hold", "Hold Map", "Hold", "H",
      "Hold the map view in its current position", roadmap_start_hold_map},

#ifndef SSD
   {"address", "Address...", "Addr", "A",
      "Show a specified address", roadmap_address_location_by_city},
#else
   {"address", "Address...", "Addr", "A",
      "Show a specified address", roadmap_address_history},

   {"dialog_hide_current", "", NULL, NULL,
      "", roadmap_start_hide_current_dialog },
#endif

   {"intersection", "Intersection...", "X", NULL,
      "Show a specified street intersection", roadmap_crossing_dialog},

   {"position", "Position...", "P", NULL,
      "Show a position at the specified coordinates", roadmap_coord_dialog},

   {"destination", "Destination", "D", NULL,
      "Show the current destination point", roadmap_start_show_destination},

   {"gps", "GPS Position", "GPS", "G",
      "Center the map on the current GPS position", roadmap_start_show_gps},

   {"location", "Location", "L", NULL,
      "Center the map on the last selected location",
      roadmap_start_show_location},

   {"mapdownload", "Map Download", "Download", NULL,
      "Enable/Disable the map download mode", roadmap_start_toggle_download},

   {"mapdiskspace", "Map Disk Space", "Disk", NULL,
      "Show the amount of disk space occupied by the maps",
      roadmap_download_show_space},

   {"deletemaps", "Delete Maps...", "Delete", "Del",
      "Delete maps that are currently visible", roadmap_download_delete},

   {"newtrip", "New Trip", "New", NULL,
      "Create a new trip", roadmap_start_create_trip},

   {"opentrip", "Open Trip", "Open", "O",
      "Open an existing trip", roadmap_start_open_trip},

   {"savetrip", "Save Trip", "Save", "S",
      "Save the current trip", roadmap_start_save_trip},
#ifndef IPHONE
   {"savescreenshot", "Make a screenshot of the map", "Screenshot", "Y",
      "Make a screenshot of the current map under the trip name",
      roadmap_trip_save_screenshot},

   {"savetripas", "Save Trip As...", "Save As", "As",
      "Save the current trip under a different name",
      roadmap_start_save_trip_as},

   {"starttrip", "Start Trip", "Start", NULL,
      "Start tracking the current trip", roadmap_start_trip},

   {"stoptrip", "Stop Trip", "Stop", NULL,
      "Stop tracking the current trip", roadmap_trip_stop},

   {"resumetrip", "Resume Trip", "Resume", NULL,
      "Resume the trip (keep the existing departure point)",
      roadmap_start_trip_resume},

   {"returntrip", "Return Trip", "Return", NULL,
      "Start the trip back to the departure point",
      roadmap_start_trip_reverse},
#endif
   {"setasdeparture", "Set as Departure", NULL, NULL,
      "Set the selected street block as the trip's departure",
      roadmap_start_set_departure},

   {"setasdestination", "Set as Destination", NULL, NULL,
      "Set the selected street block as the trip's destination",
      roadmap_start_set_destination},

   {"navigate", "Navigate", NULL, NULL,
      "Calculate route",
      roadmap_start_navigate},

   {"addaswaypoint", "Add as Waypoint", "Waypoint", "W",
      "Set the selected street block as waypoint", roadmap_start_set_waypoint},

   {"deletewaypoints", "Delete Waypoints...", "Delete...", NULL,
      "Delete selected waypoints", roadmap_start_delete_waypoint},

   {"full", "Full Screen", "Full", "F",
      "Toggle the window full screen mode (depends on the window manager)",
      roadmap_main_toggle_full_screen},

   {"about", "About", NULL, NULL,
      "Show information about Waze", roadmap_help_about},

   {"about_exit", "About", NULL, NULL,
      "Show Waze version and exit", roadmap_start_about_exit},

   {"exportdata", "Export Data", NULL, NULL,
      "Export editor data", roadmap_start_export_data},

   {"resetexport", "Reset export data", NULL, NULL,
      "Reset export data", roadmap_start_export_reset},

   {"updatemap", "Update map", NULL, NULL,
      "Export editor data", roadmap_start_download_map},

   {"detectreceiver", "Detect GPS receiver", NULL, NULL,
      "Auto-detect GPS receiver", roadmap_start_detect_receiver},

   {"sync", "Sync", NULL, NULL,
      "Sync map and data", roadmap_start_sync_data},

   {"quickmenu", "Open quick menu", NULL, NULL,
      "Open quick menu", roadmap_start_quick_menu},

   {"alertsmenu", "alerts menu", NULL, NULL,
     "alerts menu", start_alerts_menu},

   {"updaterange", "Update house number", NULL, NULL,
      "Update house number", update_range_dialog},

   {"viewmarkers", "View markers", NULL, NULL,
      "View / Edit markers", edit_markers_dialog},

   {"addquicknote", "Add a quick note", NULL, NULL,
      "Add a quick note", editor_notes_add_quick},

   {"addeditnote", "Add a note", NULL, NULL,
      "Add a note and open edit dialog", editor_notes_add_edit},

   {"addvoicenote", "Add a voice note", NULL, NULL,
      "Add a voice note", editor_notes_add_voice},

#ifdef IPHONE
   {"geoinfo", "Waze support in your area", NULL,  NULL,
      "waze support in your area", roadmap_geo_location_info_show},

   {"home", "Home", NULL, NULL,
      "Home / about button", start_home},

   {"guided_tour", "Watch the guided tour", NULL,  NULL,
      "Watch the guided tour", roadmap_help_guided_tour},

   {"what_to_expect", "What to expect", NULL,  NULL,
      "What to expect", roadmap_help_what_to_expect},

   {"nutshell", "Waze in a nutshell", NULL,  NULL,
      "waze in a nutshell", roadmap_help_nutshell},

   {"introduction", "Learn more about waze", NULL,  NULL,
      "Learn more about waze", roadmap_introduction_show},

   {"media_player", "Go to my music player", NULL,  NULL,
      "Go to my music player", roadmap_media_player},

   {"media_player_toggle", "Toggle play/pause", NULL,  NULL,
      "Toggle play/pause", roadmap_media_player_toggle},

   {"showmorebar", "Show more bar", NULL,  NULL,
      "Show more bar", roadmap_more_bar_show},

   {"hidemorebar", "Hide more bar", NULL,  NULL,
      "Hide more bar", roadmap_more_bar_hide},
#endif

   {"minimize", "Minimize application", NULL, NULL,
      "Close app window and keep in background", roadmap_main_minimize},

   {"speedcam", "Speed cam", NULL,  NULL,
      "Add a speed camera", add_speed_cam_alert},

   {"redlightcam", "Red light cam", NULL,  NULL,
         "Add a red light camera", add_red_light_cam_alert},

   {"add_cam", "Speed cams", NULL,  NULL,
               "Speed cams", add_cam_dlg},

   {"reportpolice", "Police", NULL,  NULL,
      "Report a police", add_real_time_alert_for_police},

   {"reportaccident", "Accident", NULL,  NULL,
      "Report an accident", add_real_time_alert_for_accident},

   {"reporttrafficjam", "Traffic jam", NULL,  NULL,
      "Report a traffic jam", add_real_time_alert_for_traffic_jam},

   {"reporthazard", "Hazard", NULL,  NULL,
      "Report a hazard condition", add_real_time_alert_for_hazard},

   {"reportother", "Other", NULL,  NULL,
      "Report other", add_real_time_alert_for_other},

   {"road_construction", "Road construction", NULL,  NULL,
      "Report road construction", add_real_time_alert_for_construction},

   {"reportparking", "Available parking", NULL,  NULL,
		"Report parking", add_real_time_alert_for_parking},

   {"real_time_alerts_list", "Real Time Alerts", NULL,  NULL,
      "Real Time Alerts",RealtimeAlertsList},

   {"real_time_alerts_list_group", "Real Time Alerts", NULL,  NULL,
         "Real Time Alerts",roadmap_groups_alerts_action},

   {"next_real_time_alerts", "Next real time alert", NULL,  NULL,
         "Next real time alert",RTAlerts_Scroll_Next},

   {"prev_real_time_alerts", "Previous real time alert", NULL,  NULL,
         "Previous real time alert",RTAlerts_Scroll_Prev},

   {"cancel_real_time_scroll", "Cancel real time scroll", NULL,  NULL,
         "Cancel real time scroll", RTAlerts_Cancel_Scrolling},

   {"reportincident", "Chit chat", NULL,  NULL,
      "Report an incident", add_real_time_chit_chat},

   {"show_wazer_nearby", "Show wazer nearby", NULL,  NULL,
      "Show wazer nearby", Realtime_ShowWazerNearby},

   {"poi_nearby", "POI nearby", NULL,  NULL,
         "POI nearby", RealtimeExternalPoi_OnShowPoiNearByPressed},

   {"toggle_navigation_guidance", "Toggle navigation guidance", NULL,  NULL,
         "Toggle navigation guidance", toggle_navigation_guidance},

   {"navigation_guidance_on", "Unmute", NULL,  NULL,
               "Unmute", navigation_guidance_on},

   {"navigation_guidance_off", "Mute", NULL,  NULL,
                     "Mute", navigation_guidance_off},

   {"settingsmenu", "Settings menu", NULL, NULL,
            "Settings menu", start_settings_quick_menu},

   {"select_car", "Select your car", NULL,  NULL,
               "Select your car", roadmap_car},

   {"send_comment", "Send a comment on alert", NULL,  NULL,
                           "Send a comment on alert", real_time_post_alert_comment},
#ifndef IPHONE
   {"left_softkey", "left soft key", NULL,  NULL,
                           "left soft key", roadmap_softkeys_left_softkey_callback},

   {"right_softkey", "right soft key", NULL,  NULL,
                           "right soft key", roadmap_softkeys_right_softkey_callback},

   {"zoom", "zoom", NULL,  NULL,
                     "zoom", start_zoom_side_menu},

   {"rotate", "rotate", NULL,  NULL,
                     "rotate", start_rotate_side_menu},
#endif //!IPHONE
   {"toggle_traffic", "Toggle traffic info", NULL,  NULL,
                           "Toggle traffic info", RealtimeRoadToggleShowTraffic},

   {"search_menu", "Drive to", NULL,  NULL,
                           "Drive to", roadmap_search_menu},

   {"view", "View menu", NULL,  NULL,
                           "View menu", roadmap_view_menu},

   {"commute_view", "Commute view", "Commute view", NULL,
      "Commute view", roadmap_view_commute},

   {"navigation_view", "Navigation view", "Navigation view", NULL,
      "Navigation view", roadmap_view_navigation},

   {"privacy_settings", "Privacy settings", NULL,  NULL,
                        "Privacy settings", RealtimePrivacySettings},

   {"general_settings", "General settings", NULL,  NULL,
                        "General settings", roadmap_general_settings_show},

   {"stop_navigate", "Stop navigation", NULL,  NULL,
                        "Stop navigation", navigate_main_stop_navigation_menu},

   {"recalc_route", "Recalculate route", NULL,  NULL,
                          "Recalculate route", navigate_main_recalculate_route},

   {"nav_menu", "Nav navigation", NULL,  NULL,
                         "Nav menu", navigate_menu},
#ifdef SSD
   {"login_details", "Login details", NULL, NULL,
                  "Login details", roadmap_login_details_dialog_show},
#endif

   {"search_history", "Recent searches", "Recent searches", NULL,
      "Recent searches", search_menu_search_history},

   {"search_favorites", "My favorites", "My favorites", NULL,
      "My favorites", search_menu_search_favorites},

   {"search_address", "New address", "New address", NULL,
      "New address", search_menu_search_address},

   {"search_local", "Local search", "Local search ", NULL,
      "Local search", search_menu_search_local},

   {"single_search", "Drive to", "Drive to", NULL,
         "Drive to", search_menu_single_search},

   {"single_search_menu", "Address or Place", "Address or Place", NULL,
               "Search address or Place", search_menu_single_search},

   {"scoreboard", "Scoreboard", NULL,  NULL,
      "Scoreboard", roadmap_scoreboard},

#if defined (IPHONE) || defined (_WIN32) || defined(ANDROID)
   {"search_ab", "Contacts", "Contacts", NULL,
         "Contacts", roamdmap_search_address_book},
#endif
#ifdef IPHONE
   {"scoreboard_menu", "Scoreboard menu", NULL,  NULL,
         "Scoreboard menu", roadmap_scoreboard},
   {"recommend", "Spread the word", NULL,  NULL,
         "Spread the word", roadmap_recommend},
   {"appstorereview", "Review us on Appstore", NULL,  NULL,
      "Review us on Appstore", roadmap_recommend_appstore},
   {"chompreview", "Review us on Chomp", NULL,  NULL,
      "Review us on Chomp", roadmap_recommend_chomp},
   {"updateproperties", "Update details", NULL,  NULL,
      "Update details", editor_street_bar_update_properties},
   {"recommendbyemail", "Email your friends", NULL,  NULL,
      "Email your friends", roadmap_recommend_email},
#endif //IPHONE
   {"facebookshare", "Share on Facebook", NULL,  NULL,
      "Share on Facebook", roadmap_facebook_share},

   {"scoreboard_menu", "Scoreboard", NULL,  NULL,
      "Scoreboard", roadmap_scoreboard},

   {"search_marked_locations", "Saved locations", "Saved locations", NULL,
         "Saved locations", search_menu_my_saved_places},


   {"geo_reminders", "Geo-Reminders", "Geo-Reminders", NULL,
               "Geo-Reminders", search_menu_geo_reminders},

   {"show_me", "Me on map", "Me on map", NULL,
   	"Me on map", show_me_on_map},
#ifndef IPHONE
   {"show_shortcuts", "Shortcuts", NULL,  NULL,
         "Shortcuts", display_shortcuts},
#endif //!IPHONE
   {"recommend_waze", "Recommend Waze to a friend", NULL, NULL,
   		"Recommend Waze to a friend", RecommentToFriend},

   {"view_points", "View my points", NULL, NULL,
		"View my points", viewMyPoints},

   {"ticker_menu", "Show points ticker", NULL, NULL,
   		"Show points ticker", viewMyPoints},

   {"my_points_menu", "My points", NULL, NULL,
		"My points", roadmap_start_my_points_menu },

	{"menu_ticker", "View my points", NULL, NULL,
		"View my points", viewMyPoints},


   {"help_menu", "Help/Support", NULL, NULL,
   		"Help/Support", roadmap_help_menu},

   {"map_settings", "Map", NULL, NULL,
   		"Map", roadmap_map_settings_show},

   {"download_settings", "Data Usage", NULL, NULL,
   		"Data Usage", roadmap_download_settings_show},

   {"help", "Help", NULL, NULL,
   		"Help",  roadmap_open_help},

   {"navigation_list", "Navigation list", NULL, NULL,
   		"Navigation list", navigate_main_list},

  {"map_updates_menu", "Update map", NULL, NULL,
      "Open map updates menu", start_map_updates_menu},

  {"nav_list_hide", "Hide navigation list popup", NULL, NULL,
      "Hide navigation list popup ", navigate_main_list_hide},


  {"more_menu", "More", NULL, NULL,
      "More", start_more_menu},

  {"current_comments", "Current Alerts comments", NULL, NULL,
      "Current Alerts comments", RTAlerts_CurrentComments},

  {"mood_dialog", "Change mood", NULL, NULL,
      "Change mood", roadmap_mood},

  {"minimized_alert_dialog", "Alert dialog", NULL, NULL,
      "Alert dialog",  RTAlerts_Minimized_Alert_Dialog},

  {"mark_location", "mark location", NULL, NULL,
      "mark location",  mark_location},

  {"save_location", "Save location", NULL, NULL,
          "save location",  roadmap_reminder_save_location},

  {"screen_touched_off", "screen_touched_off", NULL, NULL,
      "screen_touched_off",  roadmap_screen_touched_off},

  {"map_error", "Report map problem", NULL,  NULL,
         "Map error", RTAlerts_report_map_problem},


  {"submit_logs", "Send logs", NULL,  NULL,
          "Send logs", roadmap_debug_info_submit},

  {"show_ticker", "Show ticker", NULL,  NULL,
                  "Show ticker", roadmap_ticker_show},

  {"hide_ticker", "Hide ticker", NULL,  NULL,
                  "Hide ticker", roadmap_ticker_hide},

  {"gps_net_stat", "Gps & Network status", NULL, NULL,
                 "Gps & Network status", gps_network_status },

  {"add_reminder", "Add Geo-Reminder", NULL, NULL,
                  "Add Geo-Reminder", roadmap_reminder_add },

  {"foursquare_checkin", "Check-in with Foursquare", NULL, NULL,
                  "Check-in with Foursquare", roadmap_foursquare_checkin },

  {"groups", "Groups", NULL, NULL,
                   "Groups", roadmap_groups_show },

  {"group_alerts", "Group alerts", NULL, NULL,
                   "Group alerts", roadmap_groups_alerts_action },

  {"group_settings", "Groups", NULL, NULL,
                     "Groups", roadmap_groups_settings },

  {"my_coupons", "My Coupons", NULL, NULL,
                     "My Coupons", RealtimeExternalPoi_MyCouponsDlg },

  {"exit", "Quit", NULL, NULL,
                     "Exit application", roadmap_main_exit},

                  #ifdef   TESTING_MENU
   {"dummy_entry", "Dummy popup", "", NULL,
      "Dummy popup...", do_nothing},
   //
   {"dummy_entry1", "DE1", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry2", "DE2", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry3", "DE3", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry4", "DE4", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry5", "DE5", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry6", "DE6", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry7", "DE7", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry8", "DE8", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry9", "DE9", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry10", "DE10", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry11", "DE11", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry12", "DE12", "", NULL,
      "Dummy entry...", do_nothing},
#endif   // TESTING_MENU

   {NULL, NULL, NULL, NULL, NULL, NULL}
};


#ifdef UNDER_CE
static const char *RoadMapStartCfgActions[] = {

   "preferences",
   "mutevoice",
   "enablevoice",
   "quit",
   "zoomin",
   "zoomout",
   "zoom1",
   "up",
   "left",
   "right",
   "down",
   "toggleview",
   "toggleorientation",
   "IncHorizon",
   "DecHorizon",
   "clockwise",
   "counterclockwise",
   "hold",
   "address",
   "destination",
   "gps",
   "location",
   "full",
   "sync",
   "quickmenu",
   "updaterange",
   "viewmarkers",
   "addquicknote",
   "addeditnote",
   "addvoicenote",
#ifdef IPHONE
   "toggle_location",
#endif
   NULL
};
#endif


#ifdef J2ME
static const char *RoadMapStartMenu[] = {
#ifdef RIMAPI
   "address",
   "gps",
   "hold",
   "toggleview",
   "toggleorientation",
	"setasdestination",
   "detectreceiver",
   "traffic",
   "preferences",

   RoadMapFactorySeparator,

   "quit",
#else
   "about",
   "address",
   "gps",
   "hold",
   "toggleview",
   "toggleorientation",
   "traffic",
   "togglegpsrecord",
   "uploadj2merecord",
   "detectreceiver",
   "destination",
   "preferences",
   "quit",
#endif
   NULL
};
#else

static const char *RoadMapStartMenu[] = {

   ROADMAP_MENU "File",

   "preferences",
   "gpsconsole",

   RoadMapFactorySeparator,

   "sync",
   "exportdata",
   "updatemap",

   RoadMapFactorySeparator,
   "mutevoice",
   "enablevoice",
   "nonavigation",
   "navigation",

   RoadMapFactorySeparator,

   "logtofile",
   "nolog",
   "purgehistory",

   RoadMapFactorySeparator,

   "quit",


   ROADMAP_MENU "View",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",
   "toggleorientation",
   "IncHorizon",
   "DecHorizon",
   "toggleview",
   "full",

   RoadMapFactorySeparator,

   "clockwise",
   "counterclockwise",

   RoadMapFactorySeparator,

   "hold",


   ROADMAP_MENU "Find",

   "address",
   "intersection",
   "position",

   RoadMapFactorySeparator,

   "destination",
   "gps",

   RoadMapFactorySeparator,

   "mapdownload",
   "mapdiskspace",
   "deletemaps",


   ROADMAP_MENU "Trip",

   "newtrip",
   "opentrip",
   "savetrip",
   "savetripas",
   "savescreenshot",

   RoadMapFactorySeparator,

   "starttrip",
   "stoptrip",
   "resumetrip",
   "resumetripnorthup",
   "returntrip",
   "navigate",

   RoadMapFactorySeparator,

   "setasdestination",
   "setasdeparture",
   "addaswaypoint",
   "deletewaypoints",

   ROADMAP_MENU "Tools",

   "detectreceiver",

   ROADMAP_MENU "Help",

   RoadMapFactoryHelpTopics,

   RoadMapFactorySeparator,

   "about",

   NULL
};
#endif


static char const *RoadMapStartToolbar[] = {

   "destination",
   "location",
   "gps",
   "hold",

   RoadMapFactorySeparator,

   "counterclockwise",
   "clockwise",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",

   RoadMapFactorySeparator,

   "full",
   "quit",

   NULL,
};


static char const *RoadMapStartLongClickMenu[] = {

   "setasdeparture",
   "setasdestination",

   NULL,
};


#ifndef SSD
static char const *RoadMapStartQuickMenu[] = {

   "address",
   RoadMapFactorySeparator,
   "detectreceiver",
   "preferences",
   "about",
   "quit",

   NULL,
};
#endif

static char const *RoadMapAlertMenu[] = {
   NULL,
};

#ifndef SSD
static char const *RoadMapSettingsMenu[] = {
   "login_details",
   "traffic",
   "select_car",
   NULL,
};
#endif

void display_shortcuts(){

#ifdef TOUCH_SCREEN
	return;
#endif
	ssd_dialog_hide_all(dec_close);

	if (roadmap_display_is_sign_active("Shortcuts")){
		roadmap_display_hide("Shortcuts");
		roadmap_screen_redraw();
		return;
	}
	if (is_screen_wide())
		roadmap_activate_image_sign("Shortcuts", "shortcuts_wide");
	else
		roadmap_activate_image_sign("Shortcuts", "shortcuts");
}

#ifndef UNDER_CE
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "left",
   "Button-Right"    ROADMAP_MAPPED_TO "right",
   "Button-Up"       ROADMAP_MAPPED_TO "up",
   "Button-Down"     ROADMAP_MAPPED_TO "down",

   /* These binding are for the iPAQ buttons: */
   "Button-Menu"     ROADMAP_MAPPED_TO "zoom1",
   "Button-Contact"  ROADMAP_MAPPED_TO "zoomin",
   "Button-Calendar" ROADMAP_MAPPED_TO "zoomout",
   "Button-Start"    ROADMAP_MAPPED_TO "quit",

   /* These binding are for regular keyboards (case unsensitive !): */
   "+"               ROADMAP_MAPPED_TO "zoomin",
   "-"               ROADMAP_MAPPED_TO "zoomout",
   "A"               ROADMAP_MAPPED_TO "address",
   "B"               ROADMAP_MAPPED_TO "returntrip",
   /* C Unused. */
   "D"               ROADMAP_MAPPED_TO "destination",
   "E"               ROADMAP_MAPPED_TO "deletemaps",
   "F"               ROADMAP_MAPPED_TO "full",
   "G"               ROADMAP_MAPPED_TO "gps",
   "H"               ROADMAP_MAPPED_TO "hold",
   "I"               ROADMAP_MAPPED_TO "intersection",
   "J"               ROADMAP_MAPPED_TO "counterclockwise",
   "K"               ROADMAP_MAPPED_TO "clockwise",
   "L"               ROADMAP_MAPPED_TO "location",
   "M"               ROADMAP_MAPPED_TO "mapdownload",
   "N"               ROADMAP_MAPPED_TO "newtrip",
   "O"               ROADMAP_MAPPED_TO "opentrip",
   "P"               ROADMAP_MAPPED_TO "stoptrip",
   "Q"               ROADMAP_MAPPED_TO "quit",
   "R"               ROADMAP_MAPPED_TO "zoom1",
   "S"               ROADMAP_MAPPED_TO "starttrip",
   "T"             ROADMAP_MAPPED_TO "next_real_time_alerts",
   "U"               ROADMAP_MAPPED_TO "gpsnorthup",
   "V"             ROADMAP_MAPPED_TO "prev_real_time_alerts",
   "W"               ROADMAP_MAPPED_TO "addaswaypoint",
   "X"               ROADMAP_MAPPED_TO "intersection",
   "Y"               ROADMAP_MAPPED_TO "savesscreenshot",
   /* Z Unused. */
   NULL
};

#else
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "counterclockwise",
   "Button-Right"    ROADMAP_MAPPED_TO "clockwise",
   "Button-Up"       ROADMAP_MAPPED_TO "zoomin",
   "Button-Down"     ROADMAP_MAPPED_TO "zoomout",

   "Button-App1"     ROADMAP_MAPPED_TO "",
   "Button-App2"     ROADMAP_MAPPED_TO "",
   "Button-App3"     ROADMAP_MAPPED_TO "",
   "Button-App4"     ROADMAP_MAPPED_TO "",

   NULL
};
#endif


BOOL get_menu_item_names(  const char*          menu_name,
                           ssd_contextmenu_ptr  parent,
                           const char*          items[],
                           int*                 count)
{
   int                  i;
   int                  item_count = 0;
   ssd_contextmenu_ptr  menu  = NULL;
   RoadMapAction*       action= NULL;

   if( count)
      (*count) = 0;

   if( !parent)
   {
      assert(0);
      return FALSE;
   }

   if( menu_name && (*menu_name))
   {
      for( i=0; i<parent->item_count; i++)
      {
         if( CONTEXT_MENU_FLAG_POPUP & parent->item[i].flags)
         {
            action = (RoadMapAction*)parent->item[i].action;
            if( action && !strcmp( menu_name, action->name))
            {
               menu = parent->item[i].popup;
               break;
            }

            if( get_menu_item_names( menu_name, parent->item[i].popup, items, count))
               return TRUE;
         }
      }
   }
   else
      menu = parent;

   if( !menu)
      return FALSE;

   for( i=0; i<menu->item_count; i++)
   {
      action = (RoadMapAction*)menu->item[i].action;
      assert( action);
      if (!(menu->item[i].flags & CONTEXT_MENU_FLAG_HIDDEN)){
         items[item_count] = action->name;
         item_count++;
      }
   }
   items[i] = NULL;

   if( count)
      (*count) = item_count;

   return TRUE;
}

ssd_cm_item_ptr roadmap_start_get_menu_item( const char*          menu_name,
											 const char*          item_name,
											 ssd_contextmenu_ptr  parent )
{
   int                  i;
   ssd_contextmenu_ptr  menu  = NULL;
   RoadMapAction*       action= NULL;
   ssd_cm_item_ptr 		item;

   if( !parent)
   {
      return FALSE;
   }

   if( menu_name && (*menu_name))
   {
	   // Find the pop menu
      for( i=0; i<parent->item_count; i++)
      {
         if( CONTEXT_MENU_FLAG_POPUP & parent->item[i].flags)
         {
            action = (RoadMapAction*)parent->item[i].action;
            if( action && !strcmp( menu_name, action->name))
            {
               menu = parent->item[i].popup;
               break;
            }

            if(  (item = roadmap_start_get_menu_item( menu_name, item_name, parent->item[i].popup ) ) )
               return item;
         }
      }
   }
   else
      menu = parent;

   if( !menu)
      return FALSE;

   // Pass through the items and set the icon
   for( i=0; i < menu->item_count; i++ )
   {
	  item = &menu->item[i];
      action = (RoadMapAction*) item->action;
      if ( action->name && !strcmp( action->name, item_name ) )
      {
    	  return item;
      }
   }

   return NULL;
}



#ifndef  TOUCH_SCREEN
void  on_contextmenu_closed(  BOOL              made_selection,
                              ssd_cm_item_ptr   item,
                              void*             context)
{
   if( made_selection)
   {
      RoadMapAction* action = (RoadMapAction*)item->action;
      action->callback();
   }
}
#endif   // !TOUCH_SCREEN

static void roadmap_start_quick_menu (void) {

#ifdef  TOUCH_SCREEN
   int   count;
#else
   int   menu_x;
#endif   // !TOUCH_SCREEN

   if( !s_main_menu)
   {
      s_main_menu = roadmap_factory_load_menu("quick.menu", RoadMapStartActions);

      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }

   // Dynamically set the local search attributes before menu activation
   search_menu_set_local_search_attrs();

#ifndef  TOUCH_SCREEN
   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item__by_action_name(s_main_menu, "download_settings", roamdmap_map_download_enabled(), TRUE);

   ssd_contextmenu_show_item__by_action_name(s_main_menu, "stop_navigate", navigate_main_state() == 0, TRUE);

   ssd_contextmenu_show_item__by_action_name(s_main_menu, "navigation_list", navigate_main_state() == 0, TRUE);

   ssd_contextmenu_show_item__by_action_name(s_main_menu, "my_coupons", RealtimeExternalPoi_MyCouponsEnabled(), TRUE);

   ssd_contextmenu_set_separator(s_main_menu, "navigation_list");

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           s_main_menu,
                           on_contextmenu_closed,
                           NULL,
                           dir_default,
                           0,
                           FALSE);

#else
   // TOUCH SCREEN:

#ifdef SSD
   if( !get_menu_item_names( NULL, s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }

   ssd_menu_activate("Main Menu",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_TITLE,
                     DIALOG_ANIMATION_NONE);
#else
   #error Need to port next menu to use 'ssd_contextmenu_ptr'
   // NOT SSD
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("quick",
                                     RoadMapStartQuickMenu,
                                     RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif   // SSD

#endif   // !TOUCH_SCREEN
}

void on_closed_alerts_quick_menu (int exit_code, void* context){
	if ((exit_code == dec_cancel) ||( exit_code == dec_close) || (exit_code == dec_ok)){
		roadmap_trip_restore_focus();
	}
}

static void  updateRecordRoadsLabel(){
	const char * label = roadmap_lang_get(editor_track_main_getRecordRoadsLabel());
	RoadMapAction * recordRoadsAction = roadmap_start_find_action_un_const("togglenewroads");
	recordRoadsAction->label_long = label;
#ifdef TOUCH_SCREEN
	ssd_menu_set_label_long_text("Update map", "togglenewroads", label);
#else
	ssd_menu_set_label_long_text("map_updates", "togglenewroads", label);
#endif
}


void start_map_updates_menu(void){
   updateRecordRoadsLabel();
#ifdef IPHONE
	roadmap_list_menu_simple ("Update map", "map_updates", RoadMapAlertMenu, NULL, NULL, NULL, NULL,
							RoadMapStartActions,
							SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL);
#elif defined(TOUCH_SCREEN)
		ssd_menu_activate ("Update map", "map_updates", RoadMapAlertMenu, NULL, NULL,
   		  				    RoadMapStartActions,
    							 SSD_CONTAINER_TITLE,DIALOG_ANIMATION_NONE);
#else
	ssd_list_menu_activate ("map_updates", "map_updates", RoadMapAlertMenu, NULL,
   		  				        RoadMapStartActions,
    							SSD_DIALOG_FLOAT|SSD_DIALOG_VERTICAL );
#endif
}


static void mark_location_callback(int exit_code, void *context){
    char **argv = (char **)context;
    if (exit_code != dec_yes){
         show_me_on_map();
         roadmap_trip_remove_point("Marked_Location");
         roadmap_trip_restore_focus();
         free(argv[0]);
         free(argv[1]);
         free(argv[2]);
         free(argv[5]);
         free(argv[6]);
         return;
    }
    roadmap_history_add ('S', (const char **)argv);
    roadmap_history_save();
    roadmap_trip_save (roadmap_trip_current());
    ssd_dialog_hide_current(dec_close);
    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[5]);
    free(argv[6]);
}

int network_status_callbak(SsdWidget widget, const char *new_value){
   ssd_dialog_hide ("gsp_net_status", dec_ok);
   return 0;
}

void gps_network_status(void){
   char message[300];
   SsdWidget dialog;
   SsdWidget text;
   SsdWidget bitmap;
   SsdWidget container;
   SsdSize dlg_size;
   int gps_state = roadmap_gps_reception_state();
   int net_state = RealTimeLoginState();

   dialog = ssd_dialog_new ("gsp_net_status", "", NULL,
                            SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
                            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


   ssd_widget_add (dialog,
                  ssd_container_new ("spacer1", NULL, 0, 5, SSD_END_ROW));

   message[0] = 0;
   if ((gps_state == GPS_RECEPTION_NONE) || (gps_state == GPS_RECEPTION_NA)){
     ssd_widget_get_size( dialog, &dlg_size, NULL );
     ssd_dialog_add_hspace(dialog,5,0);
	  container = ssd_container_new ("title", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, 0);
	  ssd_widget_set_color(container, NULL, NULL);
	  bitmap = ssd_bitmap_new("top_satellite_off", "TS_top_satellite_off", SSD_ALIGN_VCENTER);
	  ssd_widget_add (container,bitmap);
	  ssd_dialog_add_hspace(container,5,0);
	  text =  ssd_text_new ("text", roadmap_lang_get("No GPS reception"), 16, SSD_END_ROW|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
	  ssd_text_set_color(text,"#ffffff");

	  ssd_widget_add (container,text);
	  ssd_widget_add (dialog, container);

	  text =  ssd_text_new ("text", roadmap_lang_get("Make sure you are outdoors. Currently showing approximate location using cell ID."), 16, SSD_END_ROW|SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (dialog,text);
   }
   else if (gps_state == GPS_RECEPTION_POOR){
	  char msg[150];
	  ssd_dialog_add_hspace(dialog,5,0);
	  container = ssd_container_new ("title", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
	  ssd_widget_set_color(container, NULL, NULL);
	  bitmap = ssd_bitmap_new("TS_top_satellite_poor", "TS_top_satellite_poor", SSD_ALIGN_VCENTER);
	  ssd_widget_add (container,bitmap);
	  ssd_dialog_add_hspace(container,5,0);
	  msg[0] = 0;
#if !IPHONE
//	  sprintf(msg, "%s \n(%d %s)", roadmap_lang_get("Poor GPS reception"), roadmap_gps_satelite_count(), roadmap_lang_get("satellites"));
	  sprintf(msg, "%s", roadmap_lang_get("Poor GPS reception"));
#else
	  sprintf(msg, "%s", roadmap_lang_get("Poor GPS reception"));
#endif
	  text =  ssd_text_new ("text", msg, 16, SSD_END_ROW|SSD_ALIGN_VCENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (container,text);
	  ssd_widget_add (dialog, container);

	  text =  ssd_text_new ("text", roadmap_lang_get("Make sure you are outdoors."), 16, SSD_END_ROW|SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (dialog,text);
   }
   else if (gps_state == GPS_RECEPTION_GOOD){
	  char msg[150];
	  ssd_dialog_add_hspace(dialog,5,0);
	  container = ssd_container_new ("title", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
	  ssd_widget_set_color(container, NULL, NULL);
	  bitmap = ssd_bitmap_new("TS_top_satellite_on", "TS_top_satellite_on", SSD_ALIGN_VCENTER);
	  ssd_widget_add (container,bitmap);
	  ssd_dialog_add_hspace(container,5,0);

	  sprintf(msg, "%s", roadmap_lang_get("Good GPS reception"));

	  text =  ssd_text_new ("text", msg, 16, SSD_END_ROW|SSD_ALIGN_VCENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (container,text);

	  ssd_widget_add (dialog, container);
   }

   ssd_dialog_add_vspace(dialog, 10, SSD_END_ROW);
   ssd_widget_add(dialog, ssd_separator_new("sep", SSD_END_ROW));

   if (net_state == 0){
	  ssd_dialog_add_hspace(dialog,5,0);
	  container = ssd_container_new ("title", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
	  ssd_widget_set_color(container, NULL, NULL);
	  bitmap = ssd_bitmap_new("TS_top_not_connected", "TS_top_not_connected", SSD_ALIGN_VCENTER);
	  ssd_widget_add (container,bitmap);
	  ssd_dialog_add_hspace(container,5,0);

	  text =  ssd_text_new ("text", roadmap_lang_get("No network connection"), 16, SSD_END_ROW|SSD_ALIGN_VCENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (container,text);
	  ssd_widget_add (dialog, container);

	  text =  ssd_text_new ("text", roadmap_lang_get("Functionality and navigation may be impaired."), 16, SSD_END_ROW|SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
	  ssd_text_set_color(text,"#ffffff");
	  ssd_widget_add (dialog,text);

   }
   else{
      ssd_dialog_add_hspace(dialog,5,0);
	   container = ssd_container_new ("title", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
		ssd_widget_set_color(container, NULL, NULL);
	   bitmap = ssd_bitmap_new("TS_top_connected", "TS_top_connected", SSD_ALIGN_VCENTER);
	   ssd_widget_add (container,bitmap);
	   ssd_dialog_add_hspace(container,5,0);

	   text =  ssd_text_new ("text", roadmap_lang_get("Connected to waze"), 16, SSD_END_ROW|SSD_ALIGN_VCENTER);
	   ssd_text_set_color(text,"#ffffff");
	   ssd_widget_add (container,text);
	   ssd_widget_add (dialog, container);

   }
   ssd_dialog_add_vspace(dialog, 10, SSD_END_ROW);
   ssd_widget_add (dialog,
                  ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                                    SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_DEFWIDGET|
                                    SSD_WS_TABSTOP,
                                    network_status_callbak));

   ssd_dialog_activate("gsp_net_status", NULL);

   roadmap_screen_redraw();

}

void mark_location(void){
    PluginLine line;
    int direction;
    RoadMapGpsPosition 			*CurrentGpsPoint;
    const RoadMapPosition 		*GpsPosition;
    BOOL has_position = FALSE;

	CurrentGpsPoint = malloc(sizeof(*CurrentGpsPoint));
	roadmap_check_allocated(CurrentGpsPoint);

	if (roadmap_navigate_get_current
		(CurrentGpsPoint, &line, &direction) == -1) {
		BOOL has_reception = (roadmap_gps_reception_state() != GPS_RECEPTION_NONE) && (roadmap_gps_reception_state() != GPS_RECEPTION_NA);
		GpsPosition = roadmap_trip_get_position ("GPS");
	   if ((GpsPosition != NULL) && (has_reception)){
	   	int number;
	      int layers[128];
	      int layers_count;
	      RoadMapNeighbour neighbours[2];
	      layers_count = roadmap_layer_all_roads(layers, 128);
	   	roadmap_math_set_context((RoadMapPosition *)GpsPosition, 20);
	   	number = roadmap_street_get_closest(GpsPosition, 0, layers, layers_count,
	   	   	            1, &neighbours[0], 1);
	   	if (number > 0){
	   		CurrentGpsPoint->latitude = GpsPosition->latitude;
	   		CurrentGpsPoint->longitude = GpsPosition->longitude;
	   		CurrentGpsPoint->speed = 0;
	   		CurrentGpsPoint->steering = 0;
	   		has_position = TRUE;
	   		line = neighbours[0].line;
	   	}else{
	          free(CurrentGpsPoint);
	   	}
	   } else{
          free(CurrentGpsPoint);
      }
    }
    else{
    	has_position = TRUE;
    }

    if (has_position){
      PluginStreetProperties properties;
      static const char *argv[reminder_hi__count];
      char	temp[15];
      char message[250];
      roadmap_plugin_get_street_properties (&line, &properties, 0);
      roadmap_history_declare ('S', reminder_hi__count);
      argv[0] = strdup(properties.address);
      argv[1] = strdup(properties.street);
      argv[2] = strdup(properties.city);
      argv[3] = ""; //state
 	   argv[4] = "";
 	   sprintf(temp, "%d", CurrentGpsPoint->latitude);
 	   argv[5] = strdup(temp);
 	   sprintf(temp, "%d", CurrentGpsPoint->longitude);
 	   argv[6] = strdup(temp);
 	   argv[7] = "";
 	   argv[8] = "";
 	   argv[9] = "";
 	   argv[10] = "";
      roadmap_trip_set_gps_position ("Marked_Location", "", "mark_location_pin", CurrentGpsPoint);
      roadmap_trip_set_focus("Marked_Location");
      roadmap_layer_adjust();
      roadmap_screen_refresh ();
      sprintf(message, roadmap_lang_get("Would you like to add  (%s %s, %s) to Saved Locations?"), argv[1], argv[0], argv[2]);
 	   ssd_confirm_dialog ("Save location", message, TRUE, mark_location_callback , (void *)&argv[0]);

    }
    else{
      roadmap_messagebox_timeout("No GPS reception","Sorry, there's no GPS reception in this location. Make sure you are outdoors",5);
    }
}

void save_location(void){
    PluginStreetProperties properties;
    static const char *argv[reminder_hi__count];
    char  temp[15];
    char message[250];
    int number;
    int layers[128];
    int layers_count;
    RoadMapNeighbour neighbours[2];
    RoadMapPosition *position =  (RoadMapPosition *)roadmap_trip_get_position("Selection");
    RoadMapGpsPosition gps_pos;
    memcpy( &gps_pos, position, sizeof( RoadMapPosition ) ); /* Note: Other fields are not initialized */

    if (!position)
       return;

    layers_count = roadmap_layer_all_roads(layers, 128);
    roadmap_math_set_context(position, 20);
    number = roadmap_street_get_closest(position, 0, layers, layers_count,
                      1, &neighbours[0], 1);
    if (number <= 0)
       return;
    roadmap_plugin_get_street_properties (&neighbours[0].line, &properties, 0);

   roadmap_history_declare ('S', reminder_hi__count);
   argv[0] = strdup(properties.address);
   argv[1] = strdup(properties.street);
   argv[2] = strdup(properties.city);
   argv[3] = ""; //state
   argv[4] = "";
   sprintf(temp, "%d", position->latitude);
   argv[5] = strdup(temp);
   sprintf(temp, "%d", position->longitude);
   argv[6] = strdup(temp);
   argv[7] = "";
   argv[8] = "";
   argv[9] = "";
   argv[10] = "";
   roadmap_trip_set_gps_position( "Marked_Location", "", "mark_location_pin", &gps_pos );
   roadmap_trip_set_focus("Marked_Location");
   roadmap_layer_adjust();
   roadmap_screen_refresh ();
   sprintf(message, roadmap_lang_get("Would you like to add  (%s %s, %s) to Saved Locations?"), argv[1], argv[0], argv[2]);
   ssd_confirm_dialog ("Save location", message, TRUE, mark_location_callback , (void *)&argv[0]);

}

void start_alerts_menu_delayed(void){
#ifdef OPENGL
   //roadmap_screen_stop_glow();
#endif
   ssd_dialog_hide("LocationSavedDlg", dec_close);
   roadmap_main_remove_periodic(start_alerts_menu_delayed);
#ifdef IPHONE
   const char* additional_item;
   if (roadmap_foursquare_is_enabled())
      additional_item = "foursquare_checkin";
   else
      additional_item = "";


   roadmap_list_menu_simple ("alerts","alerts", RoadMapAlertMenu, additional_item,
                             NULL, NULL, on_closed_alerts_quick_menu,
                     RoadMapStartActions, SSD_DIALOG_ADDITION_BELOW);
#elif defined(TOUCH_SCREEN)
   ssd_menu_activate ("alerts","alerts", RoadMapAlertMenu,  roadmap_foursquare_create_alert_menu(),on_closed_alerts_quick_menu,
                                RoadMapStartActions,SSD_CONTAINER_TITLE|SSD_DIALOG_ADDITION_BELOW,0);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_next (SsdWidget widget, const char *new_value){
   return 1;
}

void location_saved_popup_closed (int exit_code, void* context){
   focus_on_me();
}


void alerts_popup_msg(void){
   RoadMapGuiPoint offset = {0,0};
   RoadMapPosition position;
   const RoadMapGpsPosition *gps_pos;

   roadmap_main_remove_periodic(alerts_popup_msg);

   gps_pos = roadmap_trip_get_gps_position("AlertSelection");
   if (gps_pos){
      position.latitude = gps_pos->latitude;
      position.longitude = gps_pos->longitude;

#ifdef OPENGL
   //roadmap_screen_start_glow( &position,1, &offset);
#endif

      ssd_popup_show_float("LocationSavedDlg",
                           roadmap_lang_get("Location and time saved"),
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &position,
                           ADJ_SCALE(-36),
                           location_saved_popup_closed,
                           on_next,
                           NULL,
                           NULL);
      roadmap_main_set_periodic(600,start_alerts_menu_delayed );
   }
   else{
      roadmap_main_set_periodic(50,start_alerts_menu_delayed );
   }
}


void start_alerts_menu(void){

    PluginLine line;
    int direction;
    RoadMapGpsPosition 			  *CurrentGpsPoint;
    const RoadMapPosition 		*GpsPosition;
    const char *menu[2] ={"alerts_offline", "alerts"};


    int menu_file;
    BOOL has_position = FALSE;
    BOOL has_reception = roadmap_gps_have_reception();
    if (ssd_dialog_is_currently_active()) {
		if ((!strcmp(ssd_dialog_currently_active_name(),menu[0] )) || (!strcmp(ssd_dialog_currently_active_name(),menu[1])) ){
			on_closed_alerts_quick_menu(dec_close, NULL);
			ssd_dialog_hide_current(dec_close);
			roadmap_screen_refresh ();
			return;
		}
	}
   menu_file = RealTimeLoginState();
   if (menu_file == 0){
      if (!has_reception)
         roadmap_messagebox_timeout("No Network & GPS","Sorry, you need Network & GPS connection when reporting an event. Please re-try later",5);
      else
         roadmap_messagebox_timeout("No Network Connection","Sorry, you have no network connection right now. Please try later",5);
      return;
   }

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
                     //roadmap_messagebox_timeout("No GPS reception","Sorry, there's no GPS reception in this location. Make sure you are outdoors",5);
                     has_position = FALSE;
                     //return;
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
    	free(CurrentGpsPoint);
    	roadmap_screen_redraw();
    }

    RTAlerts_Reset_Minimized();
#if defined(TOUCH_SCREEN)
   roadmap_main_set_periodic(50,alerts_popup_msg );
#else

		ssd_list_menu_activate (menu[menu_file], menu[menu_file], RoadMapAlertMenu, on_closed_alerts_quick_menu,
   		  				        RoadMapStartActions,
    							SSD_DIALOG_FLOAT|SSD_DIALOG_VERTICAL);
#endif
}

static int bottom_bar_status(void){

	const char *focus;

	focus = roadmap_trip_get_focus_name();
	if (focus && !strcmp(focus, "GPS"))
		return 1;

	return 0;
}



void start_more_menu (void) {

   int   count;

   static ssd_contextmenu_ptr s_more_menu = NULL;

   if( !s_more_menu)
   {
      s_more_menu = roadmap_factory_load_menu("more.menu", RoadMapStartActions);

      if( !s_more_menu)
      {
         return;
      }
   }

#ifdef TOUCH_SCREEN
   ssd_contextmenu_show_item__by_action_name(s_more_menu, "my_coupons", RealtimeExternalPoi_MyCouponsEnabled(), TRUE);
   if( !get_menu_item_names( NULL, s_more_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }
	if (ssd_dialog_is_currently_active()) {
		if (!strcmp(ssd_dialog_currently_active_name(),"More" )){
			ssd_dialog_hide_current(dec_close);
			roadmap_screen_refresh ();
			return;
		}
	}


   ssd_menu_activate("More",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_BLACK|SSD_ROUNDED_CORNERS, DIALOG_ANIMATION_FROM_BOTTOM);
#endif
}

#ifndef IPHONE_NATIVE
void roadmap_help_menu(void){
#ifdef TOUCH_SCREEN
#ifdef SSD
   int count;

   if( !s_main_menu)
   {
   s_main_menu = roadmap_factory_load_menu("help.menu", RoadMapStartActions);

      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }

   if( !get_menu_item_names( "help_menu", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }


#ifdef TOUCH_SCREEN
   ssd_menu_activate("Help/Support",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_TITLE,DIALOG_ANIMATION_NONE);
#else
   ssd_menu_activate("Help/Support",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU,DIALOG_ANIMATION_NONE);
#endif

#else
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("help_menu",
                                  RoadMapSettingsMenu,
                                         RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif
#endif   // TOUCH_SCREEN

}
#endif //!IPHONE_NATIVE


void start_settings_quick_menu (void) {

#ifdef IPHONE
	roadmap_settings();
	return;
#endif //IPHONE

#ifdef TOUCH_SCREEN
#ifdef SSD
   int count;

   roadmap_analytics_log_event(ANALYTICS_EVENT_SETTINGS, NULL, NULL);

   if( !s_main_menu)
   {
	s_main_menu = roadmap_factory_load_menu("quick.menu", RoadMapStartActions);

      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }

   ssd_contextmenu_show_item__by_action_name(s_main_menu, "download_settings", TRUE, TRUE);

   if( !get_menu_item_names( "settingsmenu", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }

#ifdef TOUCH_SCREEN
   ssd_menu_activate("Settings menu",
                     NULL,
                     grid_menu_labels,
                     create_quick_setting_menu(),
                     quick_settins_exit,
                     RoadMapStartActions,
                     SSD_CONTAINER_TITLE, DIALOG_ANIMATION_NONE);
#else
   ssd_menu_activate("Settings menu",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU, DIALOG_ANIMATION_NONE);
#endif

#else
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("settings",
                                  RoadMapSettingsMenu,
                                         RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif
#endif   // TOUCH_SCREEN
}

#ifdef IPHONE
void start_home (void) {
	if (roadmap_main_is_root()) {
		roadmap_start_about_exit();
	} else {
		roadmap_main_show_root(1);
	}
}
#endif //IPHONE

void start_side_menu (const char           			 *name,
				      const char           			 *items[],
            		  const RoadMapAction  *actions,
            		  int                   flags)
{
   ssd_menu_activate (name, NULL, items, NULL, NULL,
          actions,
    SSD_DIALOG_FLOAT|SSD_DIALOG_VERTICAL|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|flags, DIALOG_ANIMATION_NONE);
}

#ifndef TOUCH_SCREEN
static char const *MapOptionsMenu[] = {
   "zoom",
   RoadMapFactorySeparator,
   "rotate",
   RoadMapFactorySeparator,
   "toggleview",
   RoadMapFactorySeparator,
   "toggleskin",
   NULL,
};
#endif

void start_map_options_side_menu(){
#ifndef TOUCH_SCREEN
	start_side_menu("Map Options Menu", MapOptionsMenu, RoadMapStartActions,0);
#endif
}

static char const *ZoomMenu[] = {
   "zoomin",
   RoadMapFactorySeparator,
   "zoomout",
   NULL,
};

void start_zoom_side_menu(){
	start_side_menu("Zoom Menu", ZoomMenu, RoadMapStartActions,0);
}

static char const *RotateMenu[] = {
   "clockwise",
   RoadMapFactorySeparator,
   "counterclockwise",
   NULL,
};

void start_rotate_side_menu(){
	start_side_menu("Rotate Menu", RotateMenu, RoadMapStartActions,0);
}

#ifdef UNDER_CE
static void roadmap_start_init_key_cfg (void) {

   const char **keys = RoadMapStartKeyBinding;
   RoadMapConfigDescriptor config = ROADMAP_CONFIG_ITEM("KeyBinding", "");

   while (*keys) {
      char *text;
      char *separator;
      const RoadMapAction *this_action;
      const char **cfg_actions = RoadMapStartCfgActions;
      RoadMapConfigItem *item;

      text = strdup (*keys);
      roadmap_check_allocated(text);

      separator = strstr (text, ROADMAP_MAPPED_TO);
      if (separator != NULL) {

         const char *new_config = NULL;
         char *p;
         for (p = separator; *p <= ' '; --p) *p = 0;

         p = separator + strlen(ROADMAP_MAPPED_TO);
         while (*p && (*p <= ' ')) ++p;

         this_action = roadmap_start_find_action (p);

         config.name = text;
         config.reference = NULL;

         if (this_action != NULL) {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config, NULL,
                    roadmap_lang_get (this_action->label_long), NULL);

            if (strcmp(roadmap_lang_get (this_action->label_long),
                     roadmap_config_get (&config))) {

               new_config = roadmap_config_get (&config);
            }

            roadmap_config_add_enumeration_value (item, "");
         } else {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config, NULL, "", NULL);

            if (strlen(roadmap_config_get (&config))) {
               new_config = roadmap_config_get (&config);
            }
         }

         while (*cfg_actions) {
            const RoadMapAction *cfg_action =
                        roadmap_start_find_action (*cfg_actions);
            if (new_config &&
                  !strcmp(new_config,
                          roadmap_lang_get (cfg_action->label_long))) {
               new_config = cfg_action->name;
            }

            roadmap_config_add_enumeration_value
                     (item, roadmap_lang_get (cfg_action->label_long));
            cfg_actions++;
         }

         if (new_config != NULL) {
            char str[100];
            snprintf(str, sizeof(str), "%s %s %s", text, ROADMAP_MAPPED_TO,
                                                   new_config);
            *keys = strdup(str);
         }
      }

      keys++;
   }
}
#endif


static void roadmap_start_set_unit (void) {

   const char *unit = roadmap_config_get (&RoadMapConfigGeneralUserUnit);

   if (!strcmp(unit,"default")){
      unit = roadmap_config_get (&RoadMapConfigGeneralUnit);
   }

   if (strcmp (unit, "imperial") == 0) {

      roadmap_math_use_imperial();

   } else if (strcmp (unit, "metric") == 0) {

      roadmap_math_use_metric();

   } else {
      roadmap_log (ROADMAP_ERROR, "%s is not a supported unit", unit);
      roadmap_math_use_imperial();
   }
}


static int RoadMapStartGpsRefresh = 0;

static void roadmap_gps_update
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *gps_position) {

   static int RoadMapSynchronous = -1;

   if (RoadMapStartFrozen) {

      RoadMapStartGpsRefresh = 0;

   } else {

/*
      roadmap_object_move (RoadMapStartGpsID, gps_position);

      roadmap_trip_set_mobile ("GPS", gps_position);
      */

      roadmap_log_reset_stack ();

      roadmap_trip_set_point( "Location", (const RoadMapPosition*) gps_position );
      roadmap_navigate_locate (gps_position, gps_time);

      roadmap_log_reset_stack ();

      if (RoadMapSynchronous) {

         if (RoadMapSynchronous < 0) {
            RoadMapSynchronous = roadmap_option_is_synchronous ();
         }

         RoadMapStartGpsRefresh = 0;

         roadmap_screen_refresh();
         roadmap_log_reset_stack ();

      } else {

         RoadMapStartGpsRefresh = 1;
      }
   }
}


static void roadmap_start_periodic (void) {

   roadmap_spawn_check ();

   if (RoadMapStartGpsRefresh) {

      RoadMapStartGpsRefresh = 0;

      roadmap_screen_refresh();
      roadmap_log_reset_stack ();
   }
}


static void roadmap_start_add_gps (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_gps_input);
}

static void roadmap_start_remove_gps (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_set_timeout (RoadMapCallback callback) {

   roadmap_main_set_periodic (3000, callback);
}


static int roadmap_start_long_click (RoadMapGuiPoint *point) {

   RoadMapPosition position;

   roadmap_math_to_position (point, &position, 1);
   roadmap_trip_set_point ("Selection", &position);

   if (LongClickMenu != NULL) {
      roadmap_main_popup_menu (LongClickMenu, point->x, point->y);
   }

   return 1;
}


static void roadmap_start_realtime (void) {
#if defined(__SYMBIAN32__) || defined(IPHONE_NATIVE)
   roadmap_main_remove_periodic(roadmap_start_realtime);
#endif
   if(!Realtime_Initialize())
      roadmap_log( ROADMAP_ERROR, "roadmap_start() - 'Realtime_Initialize()' had failed");

   /* Disabled now. There is no delayed wizard on start */
   /* roadmap_welcome_wizard(); */

}

static void roadmap_start_window (void) {

   roadmap_main_new (RoadMapMainTitle,
                     roadmap_option_width("Main"),
                     roadmap_option_height("Main"));

#ifndef J2ME
   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);

   LongClickMenu = roadmap_factory_menu ("long_click",
                                         RoadMapStartLongClickMenu,
                                         RoadMapStartActions);

   roadmap_pointer_register_long_click
      (roadmap_start_long_click, POINTER_DEFAULT);

#endif

   roadmap_main_add_canvas ();

   roadmap_main_show ();

   roadmap_splash_display ();

   roadmap_gps_register_link_control
      (roadmap_start_add_gps, roadmap_start_remove_gps);

   roadmap_gps_register_periodic_control
      (roadmap_start_set_timeout, roadmap_main_remove_periodic);
}


const char *roadmap_start_get_title (const char *name) {

   static char *RoadMapMainTitleBuffer = NULL;

   int length;


   if (name == NULL) {
      return RoadMapMainTitle;
   }

   length = strlen(RoadMapMainTitle) + strlen(name) + 4;

   if (RoadMapMainTitleBuffer != NULL) {
         free(RoadMapMainTitleBuffer);
   }
   RoadMapMainTitleBuffer = malloc (length);

   if (RoadMapMainTitleBuffer != NULL) {

      strcpy (RoadMapMainTitleBuffer, RoadMapMainTitle);
      strcat (RoadMapMainTitleBuffer, ": ");
      strcat (RoadMapMainTitleBuffer, name);
      return RoadMapMainTitleBuffer;
   }

   return name;
}


static void roadmap_start_after_refresh (void) {

   if (roadmap_download_enabled()) {

      RoadMapGuiPoint download_point = {0, 20};

      download_point.x = roadmap_canvas_width() - 20;
      if (download_point.x < 0) {
         download_point.x = 0;
      }
      roadmap_sprite_draw
         ("Download", &download_point, 0 - roadmap_math_get_orientation());
   }

   if (roadmap_start_prev_after_refresh) {
      (*roadmap_start_prev_after_refresh) ();
   }
}


static void roadmap_start_usage (const char *section) {

   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
   roadmap_factory_usage (section, RoadMapStartActions);
}


void roadmap_start_freeze (void) {

   RoadMapStartFrozen = 1;
   RoadMapStartGpsRefresh = 0;

   roadmap_screen_freeze ();
}

void roadmap_start_unfreeze (void) {

   RoadMapStartFrozen = 0;
   roadmap_screen_unfreeze ();
}

void roadmap_start_screen_refresh (int refresh) {

   if (refresh) {
      roadmap_screen_unfreeze ();
   } else {
      roadmap_screen_freeze ();
   }
}

int roadmap_start_is_frozen (void) {

   return RoadMapStartFrozen;
}
/*
 * Send application to run in background
 */
void roadmap_start_app_run_bg( void )
{
	// AGA DEBUG Reduce level
	roadmap_log( ROADMAP_INFO, "Going background" );

	roadmap_screen_set_background_run( TRUE );

	roadmap_res_invalidate( RES_BITMAP );

	roadmap_log( ROADMAP_INFO, "Canvas shutting down!" );

	roadmap_canvas_shutdown();
}

/*
 * Send application to run in foreground
 */
void roadmap_start_app_run_fg( void )
{
	// AGA DEBUG Reduce level
	roadmap_log( ROADMAP_INFO, "Going foreground" );

	roadmap_screen_set_background_run( FALSE );

	roadmap_screen_redraw();
}



static void roadmap_start_set_left_softkey(const char *name, const char*str, RoadMapCallback callback){
	static Softkey s;
	strcpy(s.text, str);
	s.callback = callback;
	roadmap_softkeys_set_left_soft_key(name, &s);

}

static void roadmap_start_set_right_softkey(const char *name, const char *str, RoadMapCallback callback){
	static Softkey s;
	strcpy(s.text, str);
	s.callback = callback;
	roadmap_softkeys_set_right_soft_key(name, &s);

}

int roadmap_start_get_first_time_use(){
 return roadmap_config_get_integer(&RoadMapConfigFirstTimeUse);
}


const char * roadmap_start_get_map_credit(void){
  return roadmap_config_get(&RoadMapConfigMapCredit);
}

static void roadmap_start_upload (void){
   if (StartNextLoginCb) {
      StartNextLoginCb ();
      StartNextLoginCb = NULL;
   }

   editor_sync_upload();

}

static void roadmap_confirmed_send_log_callback(int exit_code, void *context){
   if (exit_code == dec_yes)
      roadmap_debug_info_submit_confirmed();

   roadmap_start_upload ();
}

static BOOL should_show_rate_screen (void) {
#if defined(IPHONE_NATIVE) || defined(ANDROID)
   int min_period, last_time;

   if (!roadmap_config_match(&RoadMapConfigRateUsShown, "yes") &&
       time (NULL) - roadmap_start_get_first_time_use() > 7*24*60*60)
      return TRUE;

   min_period = roadmap_config_get_integer(&RoadMapConfigRateUsMinPeriod);
   last_time = roadmap_config_get_integer(&RoadMapConfigRateUsTime);

   if (!roadmap_config_match(&RoadMapConfigRateUsVer, roadmap_start_version()) && //not same version
       roadmap_config_match(&RoadMapConfigRateUsShown, "yes") && // shown at least once before
       min_period > 0 && //rate us period enabled
       time (NULL) - last_time >= min_period) //period elapsed
      return TRUE;
#endif

   return FALSE;
}

static BOOL should_show_fb_share_screen (void) {
#if defined(IPHONE_NATIVE)
   int min_period, rate_us_time;

   min_period = roadmap_config_get_integer(&RoadMapConfigFBShareMinPeriod);
   rate_us_time = roadmap_config_get_integer(&RoadMapConfigRateUsTime);

   if (!roadmap_config_match(&RoadMapConfigFBShareShown, "yes") &&  //not show yet
       roadmap_config_match(&RoadMapConfigRateUsShown, "yes") && //already shown rate us
       roadmap_config_match(&RoadMapConfigRateUsVer, roadmap_start_version()) && //rated this version
       min_period > 0 && //FB share period enabled
       time (NULL) - rate_us_time >= min_period) //period elapsed
      return TRUE;
#endif

   return FALSE;
}

void roadmap_start_login_cb (void){
   if (RoadMapStartAfterCrash) {
      if (!roadmap_screen_is_any_dlg_active()){
         ssd_confirm_dialog_custom_timeout("", "We seem to have encountered a bug. Help us improve by sending us an error report.",
                                          FALSE, roadmap_confirmed_send_log_callback, NULL,
                                          roadmap_lang_get("Send"), roadmap_lang_get("Cancel"),5);
      }
      else {
         roadmap_start_upload();
      }
   } else {
#if defined(IPHONE_NATIVE) || defined(ANDROID)
      if ( should_show_rate_screen()) {
         if (roadmap_config_match(&RoadMapConfigRateUsShown, "skipped_once")) {
            if (!roadmap_config_match(&RoadMapConfigRateUsVer, "") &&
                !roadmap_config_match(&RoadMapConfigRateUsVer, roadmap_start_version())) { //new version
               roadmap_recommend_rate_us(roadmap_start_upload, RATE_SCREEN_TYPE_NEW_VER);
            } else {
               roadmap_recommend_rate_us(roadmap_start_upload, RATE_SCREEN_TYPE_FIRST_TIME);
            }
            roadmap_config_set(&RoadMapConfigRateUsVer, roadmap_start_version());
            roadmap_config_set_integer(&RoadMapConfigRateUsTime, (int)time (NULL));
            roadmap_config_set(&RoadMapConfigRateUsShown, "yes");
         } else {
            roadmap_config_set(&RoadMapConfigRateUsShown, "skipped_once");
            roadmap_start_upload();
         }
         roadmap_config_save(FALSE);
      } else if (should_show_fb_share_screen()) {
         roadmap_recommend_rate_us(roadmap_start_upload, RATE_SCREEN_TYPE_FB_SHARE);
         roadmap_config_set(&RoadMapConfigFBShareShown, "yes");
      } else
#endif
      {
         roadmap_start_upload();
      }
   }
}

static int roadmap_start_closed_properly () {
   return strcmp(roadmap_config_get (&RoadMapConfigClosedProperly),"no");
}

static void roadmap_start_set_closed_properly (const char* val) {
   roadmap_config_set (&RoadMapConfigClosedProperly, val);
   roadmap_config_save (0);
}

extern int do_alloc_trace;

void roadmap_start_after_intro_screen(void){
   // Start 'realtime':
#ifdef __SYMBIAN32__
   roadmap_main_set_periodic(4000, roadmap_start_realtime);
#elif defined (IPHONE_NATIVE)
   roadmap_main_set_periodic(1000, roadmap_start_realtime);
#else
   roadmap_start_realtime();
#endif

}


void roadmap_start (int argc, char **argv) {



#ifdef J2ME
	printf("In roadmap_start!\n");
#endif
#ifdef ROADMAP_DEBUG_HEAP
   /* Do not forget to set the trace file using the env. variable MALLOC_TRACE,
    * then use the mtrace tool to analyze the output.
    */
   mtrace();
#endif
   roadmap_config_initialize   ();

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, NULL, "imperial", "metric", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigGeneralUserUnit, NULL, "default", "imperial", "metric", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralKeyboard, NULL, "yes", "no", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryMain, "800x600", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigMapPath, "", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigMapCredit, "", NULL);

   roadmap_config_declare
      ("user", &RoadMapConfigFirstTimeUse, "-1", NULL);

   roadmap_config_declare_enumeration
      ("session", &RoadMapConfigClosedProperly, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigRateUsShown, NULL, "no", "yes", "skipped_once", NULL);

   roadmap_config_declare
      ("user", &RoadMapConfigRateUsTime, "-1", NULL);

   roadmap_config_declare
      ("user", &RoadMapConfigRateUsVer, "", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigRateUsMinPeriod, "-1", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigFBShareMinPeriod, "-1", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigFBShareShown, NULL, "no", "yes", NULL);

   roadmap_option (argc, argv, roadmap_start_usage);
   roadmap_spawn_initialize (argv != NULL ? argv[0] : NULL);


   if (roadmap_start_get_first_time_use() == -1){
      int time_now = (int)time (NULL);
      roadmap_config_set_integer (&RoadMapConfigFirstTimeUse, time_now);
      roadmap_analytics_log_event (ANALYTICS_EVENT_FIRST_USE, NULL, NULL);
      roadmap_config_save (0);
   }

   if (!roadmap_config_match(&RoadMapConfigRateUsShown, "no") &&
       roadmap_config_match(&RoadMapConfigRateUsVer, "")){
      int time_now = (int)time (NULL);
      roadmap_config_set_integer (&RoadMapConfigRateUsTime, time_now);
      roadmap_config_set(&RoadMapConfigRateUsVer, "2.0");
      roadmap_config_save (0);
   }


   roadmap_net_initialize      ();
   roadmap_device_events_init  ();
#if 0
   roadmap_log_init();
#endif
   roadmap_log_register_msgbox (roadmap_messagebox);
   roadmap_option_initialize   ();
   roadmap_alerter_initialize  ();
   roadmap_math_initialize     ();
   roadmap_trip_initialize     ();
   roadmap_pointer_initialize  ();
#ifdef OPENGL
   roadmap_animation_initialize();
#endif
   roadmap_screen_initialize   ();
   roadmap_fuzzy_initialize    ();
   roadmap_trip_server_init    ();
   roadmap_alternative_routes_init();
   roadmap_navigate_initialize ();
   roadmap_label_initialize    ();
   roadmap_display_initialize  ();
   roadmap_warning_initialize  ();
   roadmap_gps_initialize      ();
   roadmap_history_initialize  ();
   roadmap_adjust_initialize   ();
   roadmap_device_initialize   ();
   roadmap_power_initialize    ();
   roadmap_login_initialize	   ();
   roadmap_map_settings_init   ();
   roadmap_download_settings_init ();
#ifdef IPHONE
   roadmap_location_initialize ();
#endif //IPHONE
   roadmap_start_set_title (roadmap_lang_get ("Waze"));
   roadmap_gps_register_listener (&roadmap_gps_update);

   RoadMapStartGpsID = roadmap_string_new("GPS");

   roadmap_object_add (roadmap_string_new("RoadMap"),
                       RoadMapStartGpsID,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       0,
                       NULL);


   roadmap_path_set("maps", roadmap_config_get(&RoadMapConfigMapPath));

#if UNDER_CE
   roadmap_start_init_key_cfg ();
#endif

   roadmap_math_restore_zoom ();
   roadmap_start_window      ();
   roadmap_border_initialize();
   roadmap_speedometer_initialize();

   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
   roadmap_label_activate    ();
   roadmap_sprite_initialize ();

   roadmap_screen_set_initial_position ();

#ifndef J2ME
   roadmap_history_load ();
#endif

   roadmap_help_initialize ();

   roadmap_locator_declare (&roadmap_start_no_download);

   roadmap_start_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (roadmap_start_after_refresh);

   if (RoadMapStartSubscribers) RoadMapStartSubscribers (ROADMAP_START_INIT);

   /* due to the automatic sync on WinCE, the editor plugin must register
     * first
     */
    editor_main_initialize ();
    editor_points_initialize();

    roadmap_state_add ("navigation_guidance_state", navigation_guidance_state);
    roadmap_state_add ("real_time_state", RealTimeLoginState);
    roadmap_state_add ("alerts_state", RTAlerts_State);
    roadmap_state_add ("groups_state", RTAlerts_get_group_state);
    roadmap_state_add ("skin_state", roadmap_skin_state);
    roadmap_state_add ("skin_state_screen_touched", roadmap_skin_state_screen_touched);
    roadmap_state_add ("privacy_state", RealtimePrivacyState);
    roadmap_state_add ("softkeys_state", roadmap_softkeys_orientation);
    roadmap_state_add ("wide_screen", is_screen_wide);
    roadmap_state_add ("navigation_list_pop_up", navigate_main_is_list_displaying);
    roadmap_state_add ("screen_touched_state", roadmap_screen_touched_state);
    roadmap_state_add ("screen_not_touched_state", roadmap_screen_not_touched_state);
    roadmap_state_add ("bottom_bar", bottom_bar_status);
    roadmap_state_add ("reply_pop_up", RTAlerts_is_reply_popup_on);
    roadmap_state_add ("pop_up_has_comments", RTAlerts_CurrentAlert_Has_Comments);
    roadmap_state_add ("mood_state", roadmap_mood_state);
    roadmap_state_add ("alert_minimized", RTAlerts_Get_Minimize_State);
    roadmap_state_add ("navigation_state", navigate_main_state);
    roadmap_state_add ("ticker_state", roadmap_ticker_state);
    roadmap_state_add ("user_addon_state", Realtime_AddonState);
    roadmap_state_add ("wazer_nearby_state", Realtime_WazerNearbyState);
#ifdef IPHONE
    roadmap_state_add ("media_player_state", roadmap_media_player_state);
    roadmap_state_add ("more_bar_state", roadmap_bar_more_state);
#endif //IPHONE

    roadmap_state_add ("editor_shortcut", editor_track_shortcut);
    roadmap_state_add ("top_bar_exit_state", roadmap_bar_top_bar_exit_state );
    roadmap_state_add ("alt_routes", navigate_main_alt_routes_display);

    roadmap_start_set_left_softkey("Menu","Menu", roadmap_start_quick_menu );

    roadmap_trip_restore_focus ();

    roadmap_gps_open ();

    // Set the input mode for the starting screen to the numeric
    roadmap_input_type_set_mode( inputtype_numeric );
    roadmap_start_set_right_softkey("Report", "Report", start_alerts_menu);
    roadmap_geo_config(roadmap_start_continue);
}


void roadmap_start_continue(void)   {



   if (! roadmap_trip_load (roadmap_trip_current(), 1)) {
      roadmap_start_create_trip ();
   }
   roadmap_analytics_init();
   roadmap_general_settings_init();
   roadmap_splash_download_init();
   roadmap_prompts_init        ();
   roadmap_voice_initialize    (); //AFTER GEO
   roadmap_download_initialize (); //AFTER GEO
   roadmap_lang_initialize     (); //AFTER GEO
   roadmap_net_mon_initialize  ();
   roadmap_phone_keyboard_init ();

   roadmap_sound_initialize    (); //AFTER GEO

   roadmap_start_set_unit (); //AFTER GEO
   roadmap_social_initialize();//AFTER GEO
   roadmap_foursquare_initialize();//AFTER GEO
   single_search_init();//AFTER GEO
   roadmap_mood_init();
   roadmap_bar_initialize();
   roadmap_screen_obj_initialize ();
   roadmap_ticker_initialize   ();
   roadmap_message_ticker_initialize   ();
   roadmap_reminder_init();

   roadmap_social_image_initialize();
   roadmap_groups_init();
   RealtimeTrafficDetection_Init();

   if (strcmp(roadmap_trip_get_focus_name(), "GPS"))
   		roadmap_screen_add_focus_on_me_softkey();


   RTTrafficInfo_Init();//AFTER GEO
   navigate_main_initialize ();
   roadmap_camera_image_initialize();//AFTER GEO
   roadmap_recorder_voice_initialize();//AFTER GEO

   if (!roadmap_start_closed_properly ()) {
      roadmap_log (ROADMAP_ERROR, "Waze did not close properly");
      RoadMapStartAfterCrash = TRUE;
      Realtime_CheckDumpOfflineAfterCrash();
   }
   roadmap_start_set_closed_properly("no");

   RoadMapStartInitialized = TRUE;

   StartNextLoginCb = Realtime_NotifyOnLogin (roadmap_start_login_cb);

   roadmap_term_of_use(roadmap_start_after_intro_screen);

   roadmap_screen_restore_view();


#ifdef J2ME
   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);
#endif


   roadmap_skin_init();

   // Register for keyboard callback:
   roadmap_keyboard_register_to_event__key_pressed( on_key_pressed);


   roadmap_main_set_periodic (500, roadmap_start_periodic);

   //do_alloc_trace = 1;


}


void roadmap_start_exit (void) {
   if (!RoadMapStartInitialized)
      return;


   roadmap_analytics_term();

   // Terminate 'realtime' engine:
   Realtime_Terminate();

   single_search_term();

   // Un-register from keyboard callback:
   roadmap_keyboard_unregister_from_event__key_pressed( on_key_pressed);

    roadmap_warning_shutdown();
    navigate_main_shutdown ();
    roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
    roadmap_plugin_shutdown ();
#ifndef EMBEDDED_CE
    roadmap_sound_shutdown ();
    roadmap_net_shutdown ();
#endif
    roadmap_history_save ();
    roadmap_screen_shutdown ();
#ifndef __SYMBIAN32__
    roadmap_start_save_trip ();
#endif
#ifdef J2ME
    roadmap_config_save (1);
#else
    roadmap_config_save (0);
#endif
    editor_main_shutdown ();
#ifndef __SYMBIAN32__
    roadmap_db_end ();
#endif
    roadmap_gps_shutdown ();
    roadmap_res_shutdown ();
    roadmap_device_events_term();
    roadmap_phone_keyboard_term();
#ifndef EMBEDDED_CE
    roadmap_camera_image_shutdown();
#endif
    roadmap_border_shutdown();
    roadmap_social_image_terminate();
    roadmap_groups_term();
#ifndef J2ME
    roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
#endif
   roadmap_start_set_closed_properly("yes");
   RoadMapStartInitialized = FALSE;
}


const RoadMapAction *roadmap_start_find_action (const char *name) {

   const RoadMapAction *actions = RoadMapStartActions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, name) == 0) return actions;
      ++actions;
   }

   return NULL;
}

RoadMapAction *roadmap_start_find_action_un_const (const char *name) {

   RoadMapAction *actions = RoadMapStartActions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, name) == 0) return actions;
      ++actions;
   }

   return NULL;
}



void roadmap_start_set_title (const char *title) {
   RoadMapMainTitle = title;
}


RoadMapStartSubscriber roadmap_start_subscribe
                                 (RoadMapStartSubscriber handler) {

   RoadMapStartSubscriber previous = RoadMapStartSubscribers;

   RoadMapStartSubscribers = handler;

   return previous;
}


int roadmap_start_add_action (const char *name, const char *label_long,
                              const char *label_short, const char *label_terse,
                              const char *tip, RoadMapCallback callback) {

   int i;
   RoadMapAction action;

   action.name = name;
   action.label_long = label_long;
   action.label_short = label_short;
   action.label_terse = label_terse;
   action.tip = tip;
   action.callback = callback;

   for (i=0; i<MAX_ACTIONS; i++) {
      if (RoadMapStartActions[i].name == NULL) break;
   }

   if (i == MAX_ACTIONS) {
      roadmap_log (ROADMAP_ERROR, "Too many actions.");
      return -1;
   }

   RoadMapStartActions[i] = action;
   RoadMapStartActions[i+1].name = NULL;

   return 0;
}

void roadmap_start_override_action (const char *name, const char *new_label, const char *new_icon) {
   int i;

   for (i=0; i<MAX_OVERRIDE; i++) {
      if (RoadMapActionsOverride[i].name && strcmp(RoadMapActionsOverride[i].name, name) == 0) {
         free (RoadMapActionsOverride[i].new_label);
         free (RoadMapActionsOverride[i].new_icon);
         break;
      }
      if (RoadMapActionsOverride[i].name == NULL) break;
   }

   if (i == MAX_OVERRIDE) {
      roadmap_log (ROADMAP_ERROR, "Too many override actions.");
      return;
   }

   if (RoadMapActionsOverride[i].name == NULL) {
      RoadMapActionsOverride[i].name = strdup(name);
      RoadMapActionsOverride[i+1].name = NULL;
   }

   RoadMapActionsOverride[i].new_label = strdup(new_label);
   RoadMapActionsOverride[i].new_icon = strdup(new_icon);
}

BOOL roadmap_start_get_override_action (const char *name, char **new_label, char **new_icon) {
   int i;

   for (i=0; i<MAX_OVERRIDE; i++) {
      if (RoadMapActionsOverride[i].name && strcmp(RoadMapActionsOverride[i].name, name) == 0) {
         *new_label = RoadMapActionsOverride[i].new_label;
         *new_icon = RoadMapActionsOverride[i].new_icon;
         return TRUE;
      }
   }

   return FALSE;
}


void roadmap_start_context_menu (const RoadMapGuiPoint *point) {

#ifdef TOUCH_SCREEN
#ifdef SSD
   int count;

   if( !s_main_menu)
   {
      assert(0);
      return;
   }

   if( !get_menu_item_names( "Context", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }

   ssd_menu_activate("Context",
                     NULL,
                     grid_menu_labels,
                     NULL,
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU, DIALOG_ANIMATION_NONE);
#else
   if (LongClickMenu == NULL) return;

   roadmap_main_popup_menu (LongClickMenu, point->x, point->y);
#endif
#endif
}


#ifdef SSD
void roadmap_start_popup_menu (const char *name,
                               const char *items[],
                               PFN_ON_DIALOG_CLOSED on_dialog_closed,
                               const RoadMapGuiPoint *point,
                               int animation) {

    ssd_menu_activate (name, "", items,  NULL,on_dialog_closed, RoadMapStartActions,
                      SSD_ALIGN_CENTER|SSD_DIALOG_FLOAT|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK|SSD_ALIGN_VCENTER, animation);
}
#endif


#ifdef SSD
void roadmap_start_hide_menu (const char *name) {
   ssd_menu_hide (name);
}

static void roadmap_start_hide_current_dialog ( void ) {
#ifndef IPHONE_NATIVE
   ssd_dialog_hide_current( dec_ok );
#else
   roadmap_main_pop_view(TRUE);
#endif //IPHONE_NATIVE
}


#endif //SSD


void roadmap_start_redraw (void) {
   roadmap_screen_redraw ();
}


static void roadmap_confirmed_exit_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    roadmap_main_exit();
}

void roadmap_confirmed_exit(){
#ifdef J2ME
   roadmap_main_exit();
#else
   ssd_confirm_dialog ("", "Exit?", TRUE, roadmap_confirmed_exit_callback , NULL);
#endif
}

static BOOL on_key_pressed( const char* utf8char, uint32_t flags)
{
   BOOL  key_handled = TRUE;

   if( ssd_dialog_is_currently_active())
      return FALSE;

   if( flags & KEYBOARD_VIRTUAL_KEY)
   {
      switch( *utf8char)
      {
         case VK_Back:
            break;   // return TRUE so application will not be closed...

         case VK_Arrow_up:
            roadmap_screen_hold ();
            roadmap_screen_move_up();
            roadmap_screen_set_Xicon_state(TRUE);
            break;

         case VK_Arrow_down:
            roadmap_screen_hold ();
            roadmap_screen_move_down();
            roadmap_screen_set_Xicon_state(TRUE);
            break;

         case VK_Arrow_left:
            roadmap_screen_hold ();
            roadmap_screen_move_left();
            roadmap_screen_set_Xicon_state(TRUE);
            break;

         case VK_Arrow_right:
            roadmap_screen_hold ();
            roadmap_screen_move_right();
            roadmap_screen_set_Xicon_state(TRUE);
            break;

         case VK_Softkey_left:
            roadmap_softkeys_left_softkey_callback();
            break;

         case VK_Softkey_right:
            roadmap_softkeys_right_softkey_callback();
            break;

         case VK_Call_Start:
            roadmap_device_call_start_callback();
            break;

         default:
            key_handled = FALSE;

      }
   }
   else
   {
      if( flags & KEYBOARD_ASCII)
      {
         switch( *utf8char)
         {
            case ENTER_KEY:
               if(!strcmp(roadmap_trip_get_focus_name(),"GPS")){ // we are focused on car
              	  mark_location();
               }else{
#ifndef TOUCH_SCREEN
               	  if(roadmap_screen_is_xicon_open()){
					  RoadMapGuiPoint position;

					  position.x = roadmap_canvas_width()/2;
	               	  position.y = roadmap_canvas_height()/2;
	               	  roadmap_pointer_force_click(KEY_BOARD_PRESS,&position);
               	  }
#endif
               }
               break;

            case SPACE_KEY:
            case '0':
               search_menu_search_favorites();
               break;

            case 'd':
            case 'D':
            case '5':
               roadmap_screen_toggle_view_mode();
               break;

            case 's':
            case 'S':
               roadmap_skin_toggle();
               break;

            case 'n':
            case 'N':
               roadmap_address_history();
               break;

            case 'h':
            case 'H':
               //show_keyboard_hotkeys_map();
               break;

            case 'q':
            case 'Q':
            case '9':
               roadmap_confirmed_exit();
               return TRUE;
               break;

            case 'm':
            case 'M':
                roadmap_start_quick_menu();
               break;

            case 'v':
            case 'V':
            case '4':
               RTAlerts_Scroll_Prev();
               break;

            case 't':
            case 'T':
            case '6':
               RTAlerts_Scroll_Next();
               break;


		    case '7':
		       start_map_updates_menu();
		       break;

            case 'y':
            case 'Y':
            case '8':
               search_menu_single_search();
               break;

            case 'z':
            case 'Z':
               roadmap_softkeys_right_softkey_callback();
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

            case 'k':
            case 'K':
               roadmap_start_rotate();
               break;

            case 'j':
            case 'J':
               roadmap_start_counter_rotate();
               break;

            case '1':
               roadmap_screen_hold ();
               roadmap_screen_toggle_orientation_mode();
               break;

            case '2':
               roadmap_skin_toggle();
               break;

			case '3':
			   toggle_navigation_guidance();
			   break;

            default:
               key_handled = FALSE;
         }
      }
      else
         key_handled = FALSE;
   }

   if( key_handled)
      roadmap_screen_redraw();

   return key_handled;
}

const char* roadmap_start_version() {
   static char version[64];
   static int done = 0;

   if( done)
      return version;

   sprintf( version,
            "%d.%d.%d.%d",
            CLIENT_VERSION_MAJOR,
            CLIENT_VERSION_MINOR,
            CLIENT_VERSION_SUB,
            CLIENT_VERSION_CFG );

   done = TRUE;
   return version;
}

/*
 * This function resets the debug mode (verbose level/enables csv tracker
 * for the internal company/QA usage only
 */
void roadmap_start_reset_debug_mode()
{
	static int initial_debug_level = DEFAULT_LOG_LEVEL;
	const char *log_levels[5] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };

	char msg_text[128];
	if ( roadmap_verbosity() > ROADMAP_MESSAGE_DEBUG )
	{
		sprintf( msg_text, "Log level is set to %s", log_levels[ROADMAP_MESSAGE_DEBUG-1] );
		roadmap_option_set_verbosity( ROADMAP_MESSAGE_DEBUG );
		if ( !roadmap_gps_csv_tracker_get_enable() )
		{
			roadmap_gps_csv_tracker_set_enable( TRUE );
			roadmap_gps_csv_tracker_initialize();
		}
	}
	else
	{
		sprintf( msg_text, "Log level is set to %s", log_levels[initial_debug_level-1] );
		roadmap_option_set_verbosity( initial_debug_level );
		roadmap_gps_csv_tracker_set_enable( FALSE );
		roadmap_gps_csv_tracker_shutdown();
	}
	roadmap_config_set_integer( &RoadMapConfigGeneralLogLevel, roadmap_verbosity() );
	roadmap_messagebox( "DEBUG MODE", msg_text );
}


void roadmap_start_my_points_menu(void)
{
}

static void viewMyPoints(void){
   int view_my_points_popup_time = 5000;
#ifdef SSD
	ssd_dialog_hide_current(dec_close);
#endif
	roadmap_ticker_set_suppress_hide(TRUE);
	roadmap_ticker_show();
	roadmap_main_set_periodic(view_my_points_popup_time,roadmap_ticker_hide);
	roadmap_main_set_periodic(view_my_points_popup_time,roadmap_ticker_supress_hide);
}


