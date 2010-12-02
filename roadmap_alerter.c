/* roadmap_alerter.c -Handles the alerting of users about nearby alerts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 *   see roadmap_alerter.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_config.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_screen.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_pointer.h"
#include "roadmap_line.h"
#include "roadmap_point.h"
#include "roadmap_line_route.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_main.h"
#include "roadmap_sound.h"
#include "roadmap_alert.h"
#include "roadmap_alerter.h"
#include "roadmap_lang.h"
#include "roadmap_softkeys.h"
#include "roadmap_messagebox.h"
#include "editor/db/editor_db.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_popup.h"
#include "navigate/navigate_main.h"
#include "roadmap_tile.h"


static RoadMapConfigDescriptor AlertsEnabledCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Enable Alerts");

static RoadMapConfigDescriptor AlertsAudioEnabledCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Enable Audio Alerts");

static RoadMapConfigDescriptor MinSpeedToAlertCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Minimum Speed to Alert");

static SsdWidget dialog;

typedef struct {
   int          	active_alert_id;
   int          	distance_to_alert;
   int 	         alert_type;
   int 			   alert_provider;
   int            square;
} active_alert_st;

static int alert_active;
static roadmap_alert_providers RoadMapAlertProviders;

static active_alert_st  the_active_alert, prev_alert;
static BOOL alert_should_be_visible;
static int g_seconds = 0;
static void hide_alert_dialog();

#define AZYMUTH_DELTA  45

#define MAX_DISTANCE_FROM_ROAD	50
#define MINIMUM_POSITION_CHANGE_TO_CHECK_ALERTER 100

#define ALERT 1
#define WARN 2

#ifndef TRUE
#define FALSE              0
#define TRUE               1
#endif



int  config_alerts_enabled (void) {
   return(roadmap_config_match(&AlertsEnabledCfg, "yes"));
}

int  config_audio_alerts_enabled (void) {
   return(roadmap_config_match(&AlertsAudioEnabledCfg, "yes"));
}

int  config_alerts_min_speed (void) {
   return roadmap_config_get_integer (&MinSpeedToAlertCfg);
}

void roadmap_alerter_register(roadmap_alert_provider *provider){
   RoadMapAlertProviders.provider[RoadMapAlertProviders.count] = provider;
   RoadMapAlertProviders.count++;
}


void roadmap_alerter_initialize(void) {

   //minimum speed to check alerts
   roadmap_config_declare
      ("preferences", &MinSpeedToAlertCfg, "10", NULL);

   // Enable/Diable audio alerts
   roadmap_config_declare_enumeration
      ("preferences", &AlertsAudioEnabledCfg, NULL, "yes", "no", NULL);

   // Enable/Diable alerts
   roadmap_config_declare_enumeration
      ("preferences", &AlertsEnabledCfg, NULL, "yes", "no", NULL);

   RoadMapAlertProviders.count = 0;

   alert_should_be_visible = FALSE;
   alert_active = FALSE;
   the_active_alert.active_alert_id = -1;
   the_active_alert.alert_provider = 	-1;

   roadmap_alerter_register(&RoadmapAlertProvider);


}
//return the ID of the active alert
int roadmap_alerter_get_active_alert_id(){
   return the_active_alert.active_alert_id;
}

static void get_street_from_line (int square, int line, const char **street_name) {
   RoadMapStreetProperties properties;
   roadmap_square_set_current(square);
   roadmap_street_get_properties (line, &properties);
   *street_name = roadmap_street_get_street_fename (&properties);
}


static int azymuth_delta (int az1, int az2) {

   int delta;

   delta = az1 - az2;

   while (delta > 180)	 delta -= 360;
   while (delta < -180) delta += 360;

   return delta;
}

int alert_is_on_route (roadmap_alerter_location_info *pInfo, int steering) {

   int rc = 0;
   int from;
   int to;
   RoadMapPosition from_position;
   RoadMapPosition to_position;
   int road_azymuth;
   int delta;
   roadmap_square_set_current (pInfo->square_id);
   roadmap_line_points (pInfo->line_id, &from, &to);
   roadmap_point_position (from, &from_position);
   roadmap_point_position (to, &to_position);
   road_azymuth = roadmap_math_azymuth (&from_position, &to_position);
   delta = azymuth_delta (steering, road_azymuth);
   if (delta >= -90 && delta < 90)
      rc = navigate_is_line_on_route (pInfo->square_id, pInfo->line_id, from, to);
   else
      rc = navigate_is_line_on_route (pInfo->square_id, pInfo->line_id, to, from);
   return rc;
}

/**
 * [IN] alert_position - the position of the alert
 * [IN] alert_location_info - The old location_info of the alert. Will be updated if necessary
 * [OUT] pNew_Info - the location_info of this alert, to be calculated.
 * returns TRUE iff successful in finding relevant location info for this alert
 * - D.F.
 */
BOOL get_alert_location_info(const RoadMapPosition *alert_position,roadmap_alerter_location_info *pNew_Info,
      roadmap_alerter_location_info *alert_location_info){

   int alert_square_time_stamp;
   int layers_count;
   int count;
   int layers[128];
   RoadMapNeighbour neighbours;
   int square;
   int context_save_zoom;
   RoadMapPosition context_save_pos;

   if (alert_location_info){

      // check if this alert was tagged as invalid in the past - Could locate a street next to it, eventhough
      // its tile already exists.
      if (alert_location_info->square_id == ALERTER_SQUARE_ID_INVALID)
         return FALSE;

      // if alert info is already initialized and has an up to date timestamp, use its info.
      if(alert_location_info->square_id!=ALERTER_SQUARE_ID_UNINITIALIZED){
         alert_square_time_stamp = roadmap_square_version(alert_location_info->square_id);
         if(alert_square_time_stamp==alert_location_info->time_stamp){
            pNew_Info->line_id = alert_location_info->line_id;
            pNew_Info->square_id = alert_location_info->square_id;
            pNew_Info->time_stamp = alert_location_info->time_stamp;
            return TRUE;
         }
      }
   }

   // did not return so far - so no cached valid location info was found, need to cacluate it.
   layers_count =  roadmap_layer_all_roads (layers, 128);
   if (layers_count > 0) {
      square = roadmap_tile_get_id_from_position(0,alert_position);
      if(!roadmap_square_set_current(square)){
         return FALSE; // do no have this alert's tile, return FALSE;
      }

      roadmap_math_get_context(&context_save_pos, &context_save_zoom);
      roadmap_math_set_context((RoadMapPosition *)alert_position, 20);
      count = roadmap_street_get_closest(alert_position, 0, layers, layers_count, 1, &neighbours, 1);
      roadmap_math_set_context(&context_save_pos, context_save_zoom);
      if(count>0&&neighbours.distance <= MAX_DISTANCE_FROM_ROAD){

         // found valid information
         pNew_Info->line_id = neighbours.line.line_id;
         pNew_Info->square_id = neighbours.line.square;
         pNew_Info->time_stamp  = roadmap_square_version(pNew_Info->square_id);

      }else{
         // We have the alert's tile, yet still we could not locate it satisfactorily on
         // a street. Tag this alert so it will not be checked in the future.
         if(alert_location_info){
            alert_location_info->square_id = ALERTER_SQUARE_ID_INVALID;
         }

         return FALSE;
      }
   }

   if(alert_location_info){ // update the information of the alert, for future calls.
      alert_location_info->line_id = pNew_Info->line_id;
      alert_location_info->square_id = pNew_Info->square_id;
      alert_location_info->time_stamp = pNew_Info->time_stamp;
   }
   return TRUE;
}


/*
 * searches for relevant alerts in the given param provider - D.F.
 */
static BOOL is_alert_in_range_provider(roadmap_alert_provider* provider,
      const RoadMapGpsPosition *gps_position, const PluginLine* line,
      int * pAlert_index, int * pDistance, const char* cur_street_name){
   int i;
   int steering;
   RoadMapPosition pos;
   int azymuth;
   int delta;
   int square_current;
   int count = 0;
   const char *street_name;
   roadmap_alerter_location_info location_info;
   roadmap_alerter_location_info * pAlert_location_info = NULL;
   RoadMapPosition gps_pos;
   gps_pos.latitude = gps_position->latitude;
   gps_pos.longitude = gps_position->longitude;
   square_current = roadmap_square_active ();
   count =  (* (provider->count)) ();

   // no alerts for this provider
   if (count == 0) {
      return FALSE;
   }

   for (i=0; i<count; i++) {

      roadmap_square_set_current (square_current);
      // if the alert is not alertable, continue. (dummy speed cams, etc.)
      if (!(* (provider->is_alertable))(i)) {
         continue;
      }

      (* (provider->get_position)) (i, &pos, &steering);

      // check that the alert is within alert distance
      * pDistance = roadmap_math_distance(&pos, &gps_pos);

      if (*pDistance > (*(provider->get_distance))(i))
         continue;

      // Let the provider handle the Alert.
      if ((* (provider->handle_event))((* (provider->get_id))(i)))
         continue;

      // get this alerts cached location info
      pAlert_location_info = (*(provider->get_location_info))(i);

      // retrieve the current location info for this alert ( square id, line id... )
      if(!get_alert_location_info(&pos,&location_info,pAlert_location_info))
         continue; // could not find relevant location.

      // check if the alert is on the navigation route
      if (!alert_is_on_route (&location_info, steering)) {

         roadmap_square_set_current (square_current);
         if ((* (provider->check_same_street))(i)){

            // check that the alert is in the direction of driving
            delta = azymuth_delta(gps_position->steering, steering);
            if (delta > AZYMUTH_DELTA || delta < (0-AZYMUTH_DELTA)) {
               continue;
            }

            // check that we didnt pass the alert
            azymuth = roadmap_math_azymuth (&gps_pos, &pos);
            delta = azymuth_delta(azymuth, steering);
            if (delta > 90|| delta < (-90))
               continue;

            // get the street name of the alert
            get_street_from_line(location_info.square_id,location_info.line_id, &street_name);

            // if same street found an alert infront of us
            if(strcmp(street_name,cur_street_name))
               continue; // not the same street.

         }
      }

      // if we got until here, then we found a valid alert
      * pAlert_index = i;
      roadmap_square_set_current (square_current);
      return TRUE;

   }

   roadmap_square_set_current (square_current);
   return FALSE;
}

//checks whether an alert is within range
static int is_alert_in_range(const RoadMapGpsPosition *gps_position, const PluginLine *line){
   int i;
   int j;
   unsigned int speed;
   int distance; // will hold the distance of the alert
   RoadMapPosition gps_pos;
   int square;
   int squares[9];
   int count_squares;
   const char * street_name; // the current street name.
   char current_street_name[512];
   RoadMapPosition context_save_pos;
   int context_save_zoom;
   int alert_index; // will hold the index in the provider of the alert
   int priorities [] = {ALERTER_PRIORITY_HIGH,ALERTER_PRIORITY_MEDIUM,ALERTER_PRIORITY_LOW};
   int num_priorities = sizeof(priorities)/sizeof(int);
   int alert_providor_ind;
   int square_ind;
   BOOL found_alert = FALSE;
   gps_pos.latitude = gps_position->latitude;
   gps_pos.longitude = gps_position->longitude;

   roadmap_math_get_context(&context_save_pos, &context_save_zoom);
   roadmap_math_set_context((RoadMapPosition *)&gps_pos, 20);

   count_squares = roadmap_square_find_neighbours (&gps_pos, 0, squares);
   // get the street infromation of the current position.
   get_street_from_line(line->square,line->line_id, &street_name);
   strncpy_safe (current_street_name, street_name, sizeof (current_street_name));
   // go over priorities one by one from highest to lowest.
   for (j=0; (j< num_priorities); j++){

      if ( found_alert )   // Removed from the "for" condition due to some problem in android gcc AGA
         break;

		// loop all the providers for an alert
		for (i = 0 ; ((i < RoadMapAlertProviders.count)&&(!found_alert)); i++){

		   // if provider doesn't have a high enough priority skip him, we'll get to him later.
	      if(RoadMapAlertProviders.provider[i]->get_priority()!=priorities[j])
	  	       continue;

			/*
		    * If not enough distance has been passed from last check, provider can order to ignore
		    * if alert is active, always perform check, so distance will be updated
		    */
	  	   if(!alert_should_be_visible){
		       if(!(* (RoadMapAlertProviders.provider[i]->distance_check))(gps_pos))
		          continue;
		   }

			if ((* (RoadMapAlertProviders.provider[i]->is_square_dependent))()){
			/*
			* This provider has different alerts for each square, so we need to loop over all
			* these squares and check. (Speed cams in tiles for example, as opposed to RealTimeAlerts).
			*/
	         for (square = 0; ((square < count_squares)&&(!found_alert)); square++) {
	            roadmap_square_set_current(squares[square]);
	            if(is_alert_in_range_provider(RoadMapAlertProviders.provider[i],
	                     gps_position, line, &alert_index,&distance, current_street_name))
	            {
	               square_ind = square;
	               alert_providor_ind = i;
	               found_alert = TRUE;
	            }
	         }

	      } else {
	           if(is_alert_in_range_provider(RoadMapAlertProviders.provider[i],
	                   gps_position, line, &alert_index,&distance,current_street_name))
	           {
	             	square_ind = square;
	             	alert_providor_ind = i;
	             	found_alert = TRUE;
	           }
	      }
		}
   }

   if(found_alert){
      speed = (* (RoadMapAlertProviders.provider[alert_providor_ind]->get_speed))(alert_index);
      // if driving speed is over the speed limit - it's an ALERT, otherwise it's a WARNING
      if ((unsigned int)roadmap_math_to_speed_unit(gps_position->speed) < speed) {
         the_active_alert.alert_type = WARN;
         the_active_alert.distance_to_alert 	= distance;
         the_active_alert.active_alert_id 	=  (* (RoadMapAlertProviders.provider[alert_providor_ind]->get_id))(alert_index);
         the_active_alert.alert_provider = alert_providor_ind;
         the_active_alert.square = squares[square_ind];
         roadmap_math_set_context(&context_save_pos, context_save_zoom);
         return TRUE;
      }
      else{
         the_active_alert.alert_type = ALERT;
         the_active_alert.distance_to_alert 	= distance;
         the_active_alert.active_alert_id 	= (* (RoadMapAlertProviders.provider[alert_providor_ind]->get_id))(alert_index);
         the_active_alert.alert_provider = alert_providor_ind;
         the_active_alert.square = squares[square_ind];
         roadmap_math_set_context(&context_save_pos, context_save_zoom);
         return TRUE;
      }
   }

   roadmap_math_set_context(&context_save_pos, context_save_zoom);
   return FALSE;
}


// Checks if there are alerts to be displayed
void roadmap_alerter_check(const RoadMapGpsPosition *gps_position, const PluginLine *line){

	if( (!line) || (line->line_id == -1) )
		return;

   //check that alerts are enabled
   if (!config_alerts_enabled()) {
      return;
   }

   // check if there is a alert in range
   if (is_alert_in_range(gps_position, line) > 0) {
      alert_should_be_visible = TRUE;
   } else {
      alert_should_be_visible = FALSE;
   }

}


// generate the audio alert
void roadmap_alerter_audio(){
   RoadMapSoundList sound_list;

   if (config_audio_alerts_enabled()) {
      sound_list = (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_sound)) (roadmap_alerter_get_active_alert_id());
      roadmap_sound_play_list (sound_list);
   }
}


static void delete_callback(int exit_code, void *context){
   BOOL success;

   if (exit_code != dec_yes)
      return;
   success = (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->cancel))(the_active_alert.active_alert_id);
   alert_active = FALSE;
   the_active_alert.active_alert_id = -1;
   alert_should_be_visible = FALSE;
   if (success)
      roadmap_messagebox_timeout("Thank you!!!", "Your request was sent to the server",5);
   ssd_dialog_hide("Alert_Dlg", dec_close);


}


static int hide(SsdWidget widget, const char *new_value, void *context){
   hide_alert_dialog();
   return 1;
}
static int report_irrelevant(SsdWidget widget, const char *new_value, void *context){
   char message[200];
   PluginLine line;
   int direction;
   const char *str;
   RoadMapGpsPosition 	*CurrentGpsPoint;

   if (the_active_alert.active_alert_id == -1)
      return 1;

   CurrentGpsPoint = malloc(sizeof(*CurrentGpsPoint));
   if (roadmap_navigate_get_current
         (CurrentGpsPoint, &line, &direction) == -1) {
      roadmap_messagebox ("Error", "Can't find current street.");
      return 0;
   }

   roadmap_trip_set_gps_position ("AlertSelection", "Selection", NULL, CurrentGpsPoint);
   str = (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_string)) (the_active_alert.active_alert_id);
   if (str != NULL){
      const char *alertStr = roadmap_lang_get(str);
      sprintf(message,"%s\n%s",roadmap_lang_get("Please confirm that the following alert is not relevant:"), alertStr);

      ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)NULL);
   }
   return 1;
}

static void set_softkey(void){

	ssd_widget_set_left_softkey_callback(dialog->parent, report_irrelevant);
	ssd_widget_set_left_softkey_text(dialog->parent, roadmap_lang_get("Not there"));
	ssd_widget_set_right_softkey_text(dialog->parent, roadmap_lang_get("Hide"));
	ssd_widget_set_right_softkey_callback(dialog->parent, hide);
}



/////////////////////////////////////////////////////////////////////
static void hide_alert_dialog(void){
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "Alert_Dlg")))
      ssd_dialog_hide("Alert_Dlg", dec_close);
}


/////////////////////////////////////////////////////////////////////
#ifdef TOUCH_SCREEN
static int alert_dialog_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "Irrelevant")){
      hide_alert_dialog();
      report_irrelevant(NULL, NULL, NULL);
   }
   else if (!strcmp(widget->name, "Hide")){
      hide_alert_dialog();
   }
   return 1;
}
#endif

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
   SsdWidget space;
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   return space;
}


/////////////////////////////////////////////////////////////////////
void show_alert_dialog(){
   SsdWidget box;
   SsdWidget bitmap, text;
   char TextStr[200];
   int 	alertId;
   BOOL is_cancelable;
   const char * iconName;


   alertId = roadmap_alerter_get_active_alert_id();
   roadmap_square_set_current(the_active_alert.square);

   sprintf(TextStr, "%s",roadmap_lang_get((* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_string)) (alertId) ));

   dialog =  ssd_popup_new("Alert_Dlg", TextStr, NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,NULL, SSD_PERSISTENT|SSD_ROUNDED_BLACK | SSD_HEADER_BLACK);

   box = ssd_container_new ("box", NULL, SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE);
   ssd_widget_set_color(box, NULL, NULL);
   ssd_widget_add (box, space(1));
   sprintf(TextStr,"%s: %d %s", roadmap_lang_get("In"), the_active_alert.distance_to_alert, roadmap_lang_get(roadmap_math_distance_unit()));

   text = ssd_text_new ("Distance", TextStr, 14,0);
   ssd_widget_set_color(text, "#ffffff", "#ffffff");
   ssd_widget_add (box, text);
   ssd_widget_add (dialog, box);

   if (the_active_alert.alert_type == ALERT){
      iconName =  (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_alert_icon)) (alertId);
   }
   else {
      iconName =  (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_warning_icon)) (alertId);
   }
   bitmap = ssd_bitmap_new("alert_Icon",iconName, SSD_ALIGN_RIGHT);
   ssd_widget_add (box, bitmap);

   is_cancelable = (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->is_cancelable)) (alertId);
#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog,
         ssd_button_label ("Hide", roadmap_lang_get ("Hide"),
            SSD_WS_TABSTOP|SSD_ALIGN_CENTER, alert_dialog_buttons_callback));

   if (is_cancelable){
      ssd_widget_add (dialog,
            ssd_button_label ("Irrelevant", roadmap_lang_get ("Not there"),
               SSD_WS_TABSTOP|SSD_ALIGN_CENTER, alert_dialog_buttons_callback));
   }

#else
   if (is_cancelable)
      set_softkey();
#endif
   ssd_dialog_activate ("Alert_Dlg", NULL);

}


void update_alert(){
   SsdWidget text, bitmap;
   char TextStr[200];
   const char * iconName;
   const char *alert_text;
   int alertId = roadmap_alerter_get_active_alert_id();
   roadmap_square_set_current(the_active_alert.square);
   text = ssd_widget_get(dialog, "Alert Title");
   alert_text = (*(RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_string)) (alertId) ;
   if (!alert_text)
      return;
   sprintf(TextStr, "%s",roadmap_lang_get(alert_text));
   ssd_text_set_text(text, TextStr);

   text = ssd_widget_get(dialog, "Distance");
   sprintf(TextStr,"%s: %d %s", roadmap_lang_get("In"), the_active_alert.distance_to_alert, roadmap_lang_get(roadmap_math_distance_unit()));
   ssd_text_set_text(text, TextStr);

   bitmap = ssd_widget_get(dialog, "alert_Icon");
   if (the_active_alert.alert_type == ALERT){
      iconName =  (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_alert_icon)) (alertId);
   }
   else {
      iconName =  (* (RoadMapAlertProviders.provider[the_active_alert.alert_provider]->get_warning_icon)) (alertId);
   }
   ssd_bitmap_update(bitmap, iconName);
}

static RoadMapCallback AlerterTimerCallback = NULL;

static void kill_timer (void) {

   if (AlerterTimerCallback) {
      roadmap_main_remove_periodic (AlerterTimerCallback);
      AlerterTimerCallback = NULL;
   }
}

static void update_button(void){
   char button_txt[20];
   SsdWidget button = ssd_widget_get(dialog, "Hide");
   if (g_seconds != -1)
      sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Hide"), g_seconds);
   else
      sprintf(button_txt, "%s", roadmap_lang_get ("Hide"));
#ifdef TOUCH_SCREEN
   if (button)
      ssd_button_change_text(button,button_txt );
#else
   ssd_widget_set_right_softkey_text(dialog->parent, button_txt);
   ssd_dialog_refresh_current_softkeys();
#endif
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
}

void hide_alert_timeout(void){
   g_seconds --;
   if (g_seconds > 0){
      update_button();
      return;
   }

   hide_alert_dialog();
   alert_active = FALSE;
   the_active_alert.active_alert_id = -1;
   kill_timer();
}

// Draw the warning on the screen
void roadmap_alerter_display(void){
   if (alert_should_be_visible) {
      if (the_active_alert.active_alert_id == -1){
         return;
      }
      if ((!alert_active) || (prev_alert.active_alert_id != the_active_alert.active_alert_id)){
         if (alert_active)
            hide_alert_dialog();
         kill_timer();
         prev_alert.active_alert_id = the_active_alert.active_alert_id;
         show_alert_dialog();
         if (the_active_alert.alert_type == ALERT)
            roadmap_alerter_audio();
         alert_active = TRUE;
      }
      else{
         update_alert();
      }
   } else {
      if (alert_active && !alert_should_be_visible) {
         if (AlerterTimerCallback == NULL){
            SsdWidget text = ssd_widget_get(dialog, "Distance");
            ssd_text_set_text(text, " ");
            g_seconds = 5;
            AlerterTimerCallback = hide_alert_timeout;
            roadmap_main_set_periodic (1000, AlerterTimerCallback);
         }
      }
   }
}



