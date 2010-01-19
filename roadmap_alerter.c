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
	int 			   alert_providor;
	int            square;
} active_alert_st;

static int alert_active;
static active_alert_st  the_active_alert, prev_alert;
static BOOL alert_should_be_visible;
static int g_seconds = 0;
static roadmap_alert_providors RoadMapAlertProvidors;

static void hide_alert_dialog();

#define AZYMUTH_DELTA  45

#define MAX_DISTANCE_FROM_ROAD	50


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

void roadmap_alerter_register(roadmap_alert_providor *providor){
	  RoadMapAlertProvidors.providor[RoadMapAlertProvidors.count] = providor;
	  RoadMapAlertProvidors.count++;
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

	RoadMapAlertProvidors.count = 0;

	alert_should_be_visible = FALSE;
	alert_active = FALSE;
	the_active_alert.active_alert_id = -1;
	the_active_alert.alert_providor = 	-1;

	roadmap_alerter_register(&RoadmapAlertProvidor);


}
//return the ID of the active alert
int roadmap_alerter_get_active_alert_id(){
	return the_active_alert.active_alert_id;
}

static void get_street_from_line (int square, int line, const char **street_name, const char **city_name) {

	RoadMapStreetProperties properties;
	roadmap_square_set_current(square);
	roadmap_street_get_properties (line, &properties);
	*street_name = roadmap_street_get_street_name (&properties);
	*city_name = roadmap_street_get_city_name (&properties);
}


//return the street name given a GPS position
static int get_street(const RoadMapPosition *position,
							 const char **street_name,
							 const char **city_name,
							 int max_distance){

	int count = 0;
	int layers[128];
	int layers_count;
	RoadMapNeighbour neighbours;

	layers_count =  roadmap_layer_all_roads (layers, 128);

   if (layers_count > 0) {
      RoadMapPosition context_save_pos;
      int context_save_zoom;
      roadmap_math_get_context(&context_save_pos, &context_save_zoom);
	   roadmap_math_set_context((RoadMapPosition *)position, 20);
		count = roadmap_street_get_closest
				(position, 0, layers, layers_count, 1, &neighbours, 1);
      roadmap_math_set_context(&context_save_pos, context_save_zoom);
		if (count > 0 && neighbours.distance <= max_distance) {
			get_street_from_line (neighbours.line.square, neighbours.line.line_id, street_name, city_name);
			return 0;
		}
		else{
			return -1;
		}
	}

	return -1;
}

// checks if a positions is on the same street as a segment
static int check_same_street(const PluginLine *line, const RoadMapPosition *point_position){

	const char *street_name;
	const char *city_name;
	char			current_street_name[512];
	char			current_city_name[512];
   char        current_street_name2[512];
   char        current_city_name2[512];
	int point_res;
	int square_current = roadmap_square_active ();

	get_street_from_line (line->square, line->line_id, &street_name, &city_name);
	strncpy_safe (current_street_name, street_name, sizeof (current_street_name));
	strncpy_safe (current_city_name, city_name, sizeof (current_city_name));

	point_res = get_street(point_position, &street_name, &city_name, MAX_DISTANCE_FROM_ROAD);
   strncpy_safe (current_street_name2, street_name, sizeof (current_street_name2));
   strncpy_safe (current_city_name2, city_name, sizeof (current_city_name2));

	roadmap_square_set_current (square_current);

	if (point_res == -1)
		return FALSE;

	if (strcmp (current_street_name, current_street_name2) == 0 &&
		 strcmp (current_city_name, current_city_name2) == 0)
		return TRUE;
	else
		return FALSE;

}

static int azymuth_delta (int az1, int az2) {

	int delta;

	delta = az1 - az2;

	while (delta > 180)	 delta -= 360;
	while (delta < -180) delta += 360;

	return delta;
}

static int alert_is_on_route (const RoadMapPosition *point_position, int steering) {

	int count = 0;
	int layers[128];
	int layers_count;
	RoadMapNeighbour neighbours;
	int rc = 0;
	int from;
	int to;
	RoadMapPosition from_position;
	RoadMapPosition to_position;
	int road_azymuth;
	int delta;
	int square_current = roadmap_square_active ();

	layers_count =  roadmap_layer_all_roads (layers, 128);
	if (layers_count > 0) {
		count = roadmap_street_get_closest
				(point_position, 0, layers, layers_count, 1, &neighbours, 1);
		if (count > 0) {
			roadmap_square_set_current (neighbours.line.square);
			roadmap_line_points (neighbours.line.line_id, &from, &to);
			roadmap_point_position (from, &from_position);
			roadmap_point_position (to, &to_position);
			road_azymuth = roadmap_math_azymuth (&from_position, &to_position);
			delta = azymuth_delta (steering, road_azymuth);
			if (delta >= -90 && delta < 90)
				rc = navigate_is_line_on_route (neighbours.line.square, neighbours.line.line_id, from, to);
			else
				rc = navigate_is_line_on_route (neighbours.line.square, neighbours.line.line_id, to, from);
		}
	}

	roadmap_square_set_current (square_current);
	return rc;
}


//checks whether an alert is within range
static int is_alert_in_range(const RoadMapGpsPosition *gps_position, const PluginLine *line){

	int count;
	int i;
	int distance ;
	int steering;
	RoadMapPosition pos;
	int azymuth;
	int delta;
	int j;
	unsigned int speed;
	RoadMapPosition gps_pos;
	int square;
	int squares[9];
	int count_squares;
   RoadMapPosition context_save_pos;
   int context_save_zoom;

	gps_pos.latitude = gps_position->latitude;
	gps_pos.longitude = gps_position->longitude;

	roadmap_math_get_context(&context_save_pos, &context_save_zoom);
   roadmap_math_set_context((RoadMapPosition *)&gps_pos, 20);

   count_squares = roadmap_square_find_neighbours (&gps_pos, 0, squares);



   for (square = 0; square < count_squares; square++) {

		roadmap_square_set_current(squares[square]);

		// loop alll prvidor for an alert
		for (j = 0 ; j < RoadMapAlertProvidors.count; j++){

			count =  (* (RoadMapAlertProvidors.providor[j]->count)) ();


			// no alerts for this providor
			if (count == 0) {
				continue;
			}

			for (i=0; i<count; i++) {

				// if the alert is not alertable, continue. (dummy speed cams, etc.)
				if (!(* (RoadMapAlertProvidors.providor[j]->is_alertable))(i)) {
					continue;
				}

				(* (RoadMapAlertProvidors.providor[j]->get_position)) (i, &pos, &steering);

				// check that the alert is within alert distance
				distance = roadmap_math_distance(&pos, &gps_pos);

				if (distance > (*(RoadMapAlertProvidors.providor[j]->get_distance))(i)) {
					continue;
				}

				// check if the alert is on the navigation route
				if (!alert_is_on_route (&pos, steering)) {

				   if ((* (RoadMapAlertProvidors.providor[j]->check_same_street))(i)){

				      // check that the alert is in the direction of driving
				      delta = azymuth_delta(gps_position->steering, steering);
				      if (delta > AZYMUTH_DELTA || delta < (0-AZYMUTH_DELTA)) {
				         continue;
				      }

				      // check that we didnt pass the alert
				      azymuth = roadmap_math_azymuth (&gps_pos, &pos);
				      delta = azymuth_delta(azymuth, steering);
				      if (delta > 90|| delta < (-90)){
				         continue;

				      }

				      // check that the alert is on the same street
				      if (!check_same_street(line, &pos))  {
				         continue;
				      }
					}
				}



				// Let the provoider handle the Alert.
				if ((* (RoadMapAlertProvidors.providor[j]->handle_event))((* (RoadMapAlertProvidors.providor[j]->get_id))(i))){
				   continue;
				}

            speed = (* (RoadMapAlertProvidors.providor[j]->get_speed))(i);
				// check that the driving speed is over the allowed speed for that alert
				if ((unsigned int)roadmap_math_to_speed_unit(gps_position->speed) < speed) {
					the_active_alert.alert_type = WARN;
					the_active_alert.distance_to_alert 	= distance;
					the_active_alert.active_alert_id 	=  (* (RoadMapAlertProvidors.providor[j]->get_id))(i);
					the_active_alert.alert_providor = j;
					the_active_alert.square = squares[square];
				   roadmap_math_set_context(&context_save_pos, context_save_zoom);
					return TRUE;
				}
				else{
					the_active_alert.alert_type = ALERT;
					the_active_alert.alert_providor = j;
					the_active_alert.distance_to_alert 	= distance;
					the_active_alert.active_alert_id 	= (* (RoadMapAlertProvidors.providor[j]->get_id))(i);
				   roadmap_math_set_context(&context_save_pos, context_save_zoom);
	            the_active_alert.square = squares[square];
					return TRUE;
				}
			}
		}
	}

   roadmap_math_set_context(&context_save_pos, context_save_zoom);
	return FALSE;
}


// Checks if there are alerts to be displayed
void roadmap_alerter_check(const RoadMapGpsPosition *gps_position, const PluginLine *line){

	//check that alerts are enabled
	if (!config_alerts_enabled()) {
		return;
	}

	// check minimum speed
	if (roadmap_math_to_speed_unit(gps_position->speed) < config_alerts_min_speed()) {
		// If there is any active alert turn it off.
		the_active_alert.active_alert_id = -1;
		alert_should_be_visible = FALSE;
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
		sound_list = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_sound)) (roadmap_alerter_get_active_alert_id());
		roadmap_sound_play_list (sound_list);
	}
}


static void delete_callback(int exit_code, void *context){
    BOOL success;

    if (exit_code != dec_yes)
         return;
   	success = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->cancel))(the_active_alert.active_alert_id);
      alert_active = FALSE;
      the_active_alert.active_alert_id = -1;
      alert_should_be_visible = FALSE;
   	if (success)
       	roadmap_messagebox_timeout("Thank you!!!", "Your request was sent to the server",5);
   	ssd_dialog_hide("Alert_Dlg", dec_close);


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
    str = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_string)) (the_active_alert.active_alert_id);
	if (str != NULL){
		const char *alertStr = roadmap_lang_get(str);
    	sprintf(message,"%s\n%s",roadmap_lang_get("Please confirm that the following alert is not relevant:"), alertStr);

    	ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)NULL);
	}
    return 1;
}

void set_softkey(void){

	ssd_widget_set_right_softkey_callback(dialog->parent, report_irrelevant);
	ssd_widget_set_right_softkey_text(dialog->parent, roadmap_lang_get("Not there"));
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

	sprintf(TextStr, "%s",roadmap_lang_get((* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_string)) (alertId) ));

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
			iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_alert_icon)) (alertId);
	}
	else {
			iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_warning_icon)) (alertId);
	}
	bitmap = ssd_bitmap_new("alert_Icon",iconName, SSD_ALIGN_RIGHT);
	ssd_widget_add (box, bitmap);

	is_cancelable = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->is_cancelable)) (alertId);
#ifdef TOUCH_SCREEN
	if (is_cancelable){
		ssd_widget_add (dialog,
    					ssd_button_label ("Irrelevant", roadmap_lang_get ("Not there"),
                     	SSD_WS_TABSTOP|SSD_ALIGN_CENTER, alert_dialog_buttons_callback));
	}

	ssd_widget_add (dialog,
               ssd_button_label ("Hide", roadmap_lang_get ("Hide"),
                     SSD_WS_TABSTOP|SSD_ALIGN_CENTER, alert_dialog_buttons_callback));

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
	alert_text = (*(RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_string)) (alertId) ;
	if (!alert_text)
	   return;
	sprintf(TextStr, "%s",roadmap_lang_get(alert_text));
	ssd_text_set_text(text, TextStr);

	text = ssd_widget_get(dialog, "Distance");
	sprintf(TextStr,"%s: %d %s", roadmap_lang_get("In"), the_active_alert.distance_to_alert, roadmap_lang_get(roadmap_math_distance_unit()));
	ssd_text_set_text(text, TextStr);

	bitmap = ssd_widget_get(dialog, "alert_Icon");
	if (the_active_alert.alert_type == ALERT){
			iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_alert_icon)) (alertId);
	}
	else {
			iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_warning_icon)) (alertId);
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
   ssd_button_change_text(button,button_txt );
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



