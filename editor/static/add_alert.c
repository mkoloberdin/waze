/* add_alert.c - adding an alert
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
 *   See add_alert.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_dialog.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_gps.h"
#include "roadmap_locator.h"
#include "roadmap_county.h"
#include "roadmap_math.h"
#include "roadmap_adjust.h"
#include "roadmap_plugin.h"
#include "roadmap_preferences.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"
#include "roadmap_config.h"
#include "roadmap_layer.h"
#include "roadmap_alert.h"
#include "roadmap_start.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"


#include "add_alert.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_street.h"
#include "../db/editor_marker.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "../export/editor_report.h"

#include "ssd/ssd_menu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_segmented_control.h"
#include "ssd/ssd_separator.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAlerts.h"

#ifdef _WIN32
	#define NEW_LINE "\r\n"
#else
	#define NEW_LINE "\n"
#endif

static void alert_get_city_street(PluginLine *line, const char **city_name, const char **street_name);

#define SPEED_CAM_NEW_ICON "rm_new_speed_cam"
#define RED_LIGHT_NEW_ICON "rm_new_red_light_cam"

#define ALERT_NAME        "Name"
#define ALERT_DESCRIPTION "Description"
#define ALERT_CATEGORY     "Category"

#define ALERT_NAME_PREFIX "AlertName"
#define ALERT_TYPE_PREFIX "AlertType"
#define ALERT_DESCRIPTION_PREFIX "AlertDescription"
#define ALERT_CATEGORY_PREFIX "AlertCategory"

#define NUMBER        "Number"
#define STREET_PREFIX "Street"
#define CITY_PREFIX   "City"

static int g_type;
static int g_direction;

static int extract_field(const char *note, const char *field_name,
						 char *field, int size) {

	const char *lang_field_name = roadmap_lang_get (field_name);

	/* Save space for string termination */
	size--;

	while (*note) {
		if (!strncmp (field_name, note, strlen(field_name)) ||
			!strncmp (lang_field_name, note, strlen(lang_field_name))) {

			const char *end;

			while (*note && (*note != ':'))	note++;
			if (!*note)	break;
			note++;

			while (*note && (*note == ' '))	note++;

			end = note;

			while (*end && (*end != NEW_LINE[0])) end++;

			if (note == end) {
				field[0] = '\0';
				return 0;
			}

			if ((end - note) < size) size = end - note;
			strncpy(field, note, size);
			field[size] = '\0';

			return 0;
		}

		while (*note && (*note != NEW_LINE[strlen(NEW_LINE)-1])) note++;
		if (*note) note++;
	}

	return -1;
}

static int add_alert_export(int marker,
						  const char **name,
						  const char **description,
						  const char  *keys[ED_MARKER_MAX_ATTRS],
						  char        *values[ED_MARKER_MAX_ATTRS],
						  int         *count) {

	char field[255];
	const char *note = editor_marker_note (marker);
	*count = 0;
	*description = NULL;

	if (extract_field (note, STREET_PREFIX, field, sizeof(field)) != -1) {
		keys[*count] = "street";
		values[*count] = strdup(field);
		(*count)++;
	}

	if (extract_field (note, CITY_PREFIX, field, sizeof(field)) != -1) {
		keys[*count] = "city";
		values[*count] = strdup(field);
		(*count)++;
	}

	if (extract_field (note, ALERT_CATEGORY_PREFIX, field, sizeof(field)) != -1) {
		keys[*count] = "category";
		values[*count] = strdup(field);
		(*count)++;
	}


	if (extract_field (note, NUMBER, field, sizeof(field)) != -1) {
		if (atoi(field) <= 0) {
			roadmap_log (ROADMAP_ERROR, "Add Point of Interest - POI Street address is invalid.");
			return -1;
		} else {
			keys[*count] = "number";
			values[*count] = strdup(field);
			(*count)++;
		}
	}

	if (extract_field (note, ALERT_NAME_PREFIX, field, sizeof(field)) != -1) {
		*name = strdup(field);
	} else {
		*name ="";
	}

	if (extract_field (note, ALERT_DESCRIPTION_PREFIX, field, sizeof(field)) != -1) {
		*description = strdup(field);
	} else {
		*description = "";

	}


	return 0;
}

static int add_alert_verify(int marker,
						  unsigned char *flags,
						  const char **note) {

	return 0;

}

static int remove_alert_export(int marker,
						  const char **name,
						  const char **description,
						  const char  *keys[ED_MARKER_MAX_ATTRS],
						  char        *values[ED_MARKER_MAX_ATTRS],
						  int         *count) {

	char field[255];
	const char *note = editor_marker_note (marker);
	*count = 0;

	*description = "";

	if (extract_field (note, ALERT_CATEGORY_PREFIX, field, sizeof(field)) != -1) {
		keys[*count] = "category";
		values[*count] = strdup(field);
		(*count)++;
	}

	return 0;
}

static int remove_alert_verify(int marker,
						  unsigned char *flags,
						  const char **note) {

	return 0;

}

static int RemoveAlertMarkerType;
static int AddAlertMarkerType;

static EditorMarkerType AddAlertMarker = {
	"Add Alert",
	add_alert_export,
	add_alert_verify
};

static EditorMarkerType RemoveAlertMarker = {
	"Remove Alert",
	remove_alert_export,
	remove_alert_verify
};

void remove_alert(RoadMapPosition * FixedPosition, int direction, const char *category){
	char note[500];
	int fips;


	note[0] = 0;

	if (roadmap_county_by_position (FixedPosition, &fips, 1) < 1) {
		roadmap_messagebox ("Error", "Can't locate county");
		return;
	}

	if (category && *category) {

		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (ALERT_CATEGORY_PREFIX), (char *)category,
				 NEW_LINE);
	}

	if (editor_db_activate (fips) == -1) {

		editor_db_create (fips);

		if (editor_db_activate (fips) == -1) {

			roadmap_messagebox ("Error", "Cannot remove speed cam");
			return;
		}
	}

	if (editor_marker_add (FixedPosition->longitude,
						   FixedPosition->latitude,
						   direction,
						   time(NULL),
						   RemoveAlertMarkerType,
						   ED_MARKER_UPLOAD, note, NULL) == -1) {

		roadmap_messagebox ("Error", "Can't save marker.");
	} else {
		editor_report_markers ();
	}

}


void add_alert (RoadMapPosition * FixedPosition, int direction,  const char * name, const char * description, const char *category, const char *number,
              const char *city, const char *street, const char *icon) {

	char note[500];
	int fips;


	note[0] = 0;

	if (roadmap_county_by_position (FixedPosition, &fips, 1) < 1) {
		roadmap_messagebox ("Error", "Can't locate county");
		return;
	}

	if (editor_db_activate (fips) == -1) {

		editor_db_create (fips);

		if (editor_db_activate (fips) == -1) {

			roadmap_messagebox ("Error", "Cannot add camera");
			return;
		}
	}

	if (street && *street) {
		snprintf(note, sizeof(note), "%s: %s%s",
				 roadmap_lang_get (STREET_PREFIX), (char *)street,
				 NEW_LINE);
	}

	if (city && *city) {
		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (CITY_PREFIX), (char *)city,
				 NEW_LINE);
	}

	if (name && *name) {

		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (ALERT_NAME_PREFIX), (char *)name,
				 NEW_LINE);
	}

	if (description && *description) {

		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (ALERT_DESCRIPTION_PREFIX), (char *)description,
				 NEW_LINE);
	}

	if (category && *category) {

		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (ALERT_CATEGORY_PREFIX), (char *)category,
				 NEW_LINE);
	}


	if (number && *number) {

		snprintf(note + strlen(note), sizeof(note) - strlen(note),
				 "%s: %s%s", roadmap_lang_get (NUMBER), number,
				 NEW_LINE);
	}


	if (editor_marker_add (FixedPosition->longitude,
						   FixedPosition->latitude,
						   direction,
						   time(NULL),
						   AddAlertMarkerType,
						   ED_MARKER_UPLOAD, note, icon) == -1) {

		roadmap_messagebox ("Error", "Can't save marker.");
	} else {
		editor_report_markers ();
	}
}

void add_alert_initialize(void) {
	AddAlertMarkerType = editor_marker_reg_type (&AddAlertMarker);
	RemoveAlertMarkerType = editor_marker_reg_type (&RemoveAlertMarker);
}



static void add_speed_cam(int direction){


    const char *street = NULL;
    const char *city = NULL;
    PluginLine line;
    RoadMapGpsPosition *CurrentGpsPoint;
    RoadMapPosition    CurrentFixedPosition;
    int layers[128];
    int layers_count;
    RoadMapNeighbour neighbours[5];
    int count;
    int steering;
    int cfcc;
    char *speed;

    CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");

    CurrentFixedPosition.longitude = CurrentGpsPoint->longitude;
    CurrentFixedPosition.latitude = CurrentGpsPoint->latitude;

    layers_count = roadmap_layer_all_roads(layers, 128);
    count = roadmap_street_get_closest(&CurrentFixedPosition, 0, layers, layers_count,
            1, &neighbours[0], 1);

    if (count == -1) {
        return;
    }
    line = neighbours[0].line;

    cfcc = roadmap_line_cfcc (line.line_id);
	switch (cfcc) {
	case ROADMAP_ROAD_FREEWAY:
	case ROADMAP_ROAD_PRIMARY:
		speed = "90";
		break;
	case ROADMAP_ROAD_SECONDARY:
		speed = "70";
		break;
	case ROADMAP_ROAD_MAIN:
		speed = "60";
		break;
	default:
		speed = "50";
	}

	steering = CurrentGpsPoint->steering;
	roadmap_square_set_current(line.square);
	alert_get_city_street(&line, &city, &street);

	if (direction == OPPOSITE_DIRECTION){
        steering = CurrentGpsPoint->steering + 180;
        while (steering > 360)
            steering -= 360;
	}

	add_alert(&CurrentFixedPosition,steering, "", speed, "Speed Cam", "", city, street, SPEED_CAM_NEW_ICON);
	roadmap_trip_restore_focus();
	ssd_dialog_hide_all(dec_ok);
}

static void add_red_light_cam(int direction){


    const char *street = NULL;
    const char *city = NULL;
    PluginLine line;
    RoadMapGpsPosition *CurrentGpsPoint;
    RoadMapPosition    CurrentFixedPosition;
    int layers[128];
    int layers_count;
    RoadMapNeighbour neighbours[5];
    int count;
    int steering;

    CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");

    CurrentFixedPosition.longitude = CurrentGpsPoint->longitude;
    CurrentFixedPosition.latitude = CurrentGpsPoint->latitude;

    layers_count = roadmap_layer_all_roads(layers, 128);
    count = roadmap_street_get_closest(&CurrentFixedPosition, 0, layers, layers_count,
            1, &neighbours[0], 1);

    if (count == -1) {
        return;
    }
    line = neighbours[0].line;

    steering = CurrentGpsPoint->steering;
    roadmap_square_set_current(line.square);
    alert_get_city_street(&line, &city, &street);

    if (direction == OPPOSITE_DIRECTION){
        steering = CurrentGpsPoint->steering + 180;
        while (steering > 360)
            steering -= 360;
    }

    add_alert(&CurrentFixedPosition,steering, "", "0", "Red light cam", "", city, street, RED_LIGHT_NEW_ICON);
    roadmap_trip_restore_focus();
    ssd_dialog_hide_all(dec_ok);
}

static void add_dummy_cam(int direction){


    const char *street = NULL;
    const char *city = NULL;
    PluginLine line;
    RoadMapGpsPosition *CurrentGpsPoint;
    RoadMapPosition    CurrentFixedPosition;
    int layers[128];
    int layers_count;
    RoadMapNeighbour neighbours[5];
    int count;
    int steering;

    CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");

    CurrentFixedPosition.longitude = CurrentGpsPoint->longitude;
    CurrentFixedPosition.latitude = CurrentGpsPoint->latitude;

    layers_count = roadmap_layer_all_roads(layers, 128);
    count = roadmap_street_get_closest(&CurrentFixedPosition, 0, layers, layers_count,
            1, &neighbours[0], 1);

    if (count == -1) {
        return;
    }
    line = neighbours[0].line;

    steering = CurrentGpsPoint->steering;
    roadmap_square_set_current(line.square);
    alert_get_city_street(&line, &city, &street);

    if (direction == OPPOSITE_DIRECTION){
        steering = CurrentGpsPoint->steering + 180;
        while (steering > 360)
            steering -= 360;
    }

    add_alert(&CurrentFixedPosition,steering, "", "0", "Dummy Cam", "", city, street, SPEED_CAM_NEW_ICON);
    roadmap_trip_restore_focus();
    ssd_dialog_hide_all(dec_ok);
}


void add_speed_cam_my_direction(void){
	add_speed_cam(MY_DIRECTION);
}

void add_red_light_cam_my_direction(void){
   add_red_light_cam(MY_DIRECTION);
}

void add_red_light_cam_opposite_direction(void){
   add_red_light_cam(OPPOSITE_DIRECTION);
}
void add_dummy_cam_my_direction(void){
   add_dummy_cam(MY_DIRECTION);
}

void add_dummy_cam_opposite_direction(void){
   add_dummy_cam(OPPOSITE_DIRECTION);
}
void add_speed_cam_opposite_direction(void){
	add_speed_cam(OPPOSITE_DIRECTION);
}

static void add_speed_cam_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    add_speed_cam_my_direction();
}

static void add_red_light_cam_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    add_red_light_cam_my_direction();
}

static int send_button_callback (SsdWidget widget, const char *new_value){
   if (g_type == -1)
      return 0;

   if (g_type == ALERT_CATEGORY_SPEED_CAM){
      if (g_direction == MY_DIRECTION)
         add_speed_cam_my_direction();
      else
         add_speed_cam_opposite_direction();
   }
   else if (g_type == ALERT_CATEGORY_RED_LIGHT_CAM){
      if (g_direction == MY_DIRECTION)
         add_red_light_cam_my_direction();
      else
         add_red_light_cam_opposite_direction();
   }
   else {
      if (g_direction == MY_DIRECTION)
         add_dummy_cam_my_direction();
      else
         add_dummy_cam_opposite_direction();
   }
   ssd_dialog_hide_all(dec_close);
   return 1;
}

static const char * get_type_label(int type){
   switch (type){
      case 2:
         return roadmap_lang_get("Speed");
      case 3:
         return roadmap_lang_get("Fake");
      case 4:
         return roadmap_lang_get("Red light");
      default:
         return "";
   }

}

static const char * get_type_title(int type){
   switch (type){
      case 2:
         return roadmap_lang_get("Speed cam");
      case 3:
         return roadmap_lang_get("Fake");
      case 4:
         return roadmap_lang_get("Red light cam");
      default:
         return "";
   }

}

static int on_segmented_control_selected (SsdWidget widget, const char *new_value, void *context){
   g_type = atoi(new_value);
   ssd_dialog_change_text("title_text", get_type_title(g_type));
   return 1;
}

static int on_lanes_segmented_control_selected (SsdWidget widget, const char *new_value, void *context){
   g_direction = atoi(new_value);
   return 1;
}

#ifndef IPHONE_NATIVE
void add_cam_dlg(void){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   SsdWidget button;
   SsdWidget container;
   SsdWidget text;
   SsdWidget segmented_control;
   char *icon[3];
   const char *tab_labels[3];
   const char *tab_values[3];
   const char *tab_icons[3];
   SsdSegmentedControlCallback callbacks[3];
   g_type = -1;
   const RoadMapGpsPosition *pos;

   pos = RTAlerts_alerts_location(TRUE);
   if (pos == NULL) {
         return;
   }


   dialog = ssd_dialog_new ("AddCamDlg",
                            roadmap_lang_get("Speed cam"),
                            NULL,
                            SSD_CONTAINER_TITLE);

   group = ssd_container_new ("Report.Con", NULL,
                                 SSD_MAX_SIZE,SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   ssd_dialog_add_vspace(group,5,0);


    container = ssd_container_new ("Alert.SubType.Container", NULL,
                                SSD_MIN_SIZE,SSD_MIN_SIZE,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_CENTER);

    tab_labels[0] = roadmap_lang_get("Speed");
    tab_values[0] = "2";
    tab_icons[0] = "speedcam";
    callbacks[0] = on_segmented_control_selected;

    tab_labels[1] = roadmap_lang_get("Red light");
    tab_values[1] = "4";
    tab_icons[1] = "redlightcam";
    callbacks[1] = on_segmented_control_selected;

    tab_labels[2] = roadmap_lang_get("Fake");
    tab_values[2] = "3";
    tab_icons[2] = "dummy_cam";
    callbacks[2] = on_segmented_control_selected;

    segmented_control =  ssd_segmented_icon_control_new ("CamOptions", 3, tab_labels, (const void **)tab_values, tab_icons, SSD_END_ROW|SSD_ALIGN_CENTER, callbacks, NULL, -1);
    ssd_widget_add(container, segmented_control);
    ssd_widget_add(group, container);

    ssd_dialog_add_vspace(group,10,0);
    ssd_widget_add(group, ssd_separator_new("sep", SSD_END_ROW));

    box = ssd_container_new ("Lanes group", NULL,
                   SSD_MAX_SIZE, ssd_container_get_row_height(),
                   SSD_WIDGET_SPACE|SSD_END_ROW);
     ssd_widget_set_color(box, NULL, NULL);

     tab_labels[0] = roadmap_lang_get("My lane");
     tab_labels[1] = roadmap_lang_get("Other lane");
     tab_values[0] = roadmap_lang_get("1");
     tab_values[1] = roadmap_lang_get("2");
     callbacks[0] = on_lanes_segmented_control_selected;
     callbacks[1] = on_lanes_segmented_control_selected;
     segmented_control =  ssd_segmented_control_new ("Lanes", 2, tab_labels, (const void **)tab_values,SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER, callbacks, NULL, 0);

     g_direction = MY_DIRECTION;
     ssd_widget_add(box, segmented_control);
     ssd_widget_add(group, box);
     ssd_widget_add(group, ssd_separator_new("sep", SSD_END_ROW));

     ssd_dialog_add_vspace(group,20,0);
     text = ssd_text_new("AddCamText1", roadmap_lang_get("Wazers are notified of speed cams only when approaching at an excessive speed."), 14, SSD_ALIGN_CENTER|SSD_END_ROW);
     ssd_text_set_color(text, "#929292");
     ssd_widget_add(group, text);

     ssd_dialog_add_vspace(group,10,SSD_END_ROW);
     text = ssd_text_new("AddCamText1", roadmap_lang_get("Note: New speed cams need to be validated by community map editors. You can do it, too: www.waze.com"), 14, SSD_ALIGN_CENTER);
     ssd_text_set_color(text, "#929292");
     ssd_widget_add(group, text);

     icon[0] = "send_button";
     icon[1] = "send_button_s";
     icon[2] = NULL;
     button = ssd_button_label_custom( "Send", roadmap_lang_get ("Send"),SSD_WS_TABSTOP|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ALIGN_BOTTOM, send_button_callback,
         (const char **)icon, 2, "#FFFFFF", "#FFFFFF",18 );
    ssd_widget_add (group, button);

    ssd_widget_add(dialog, group);

    ssd_dialog_activate("AddCamDlg", NULL);

}
#endif //IPHONE_NATIVE

/**
 * Starts the Direction menu
 * @param actions - list of actions
 * @return void
 */
void  add_speed_cam_alert(){
   RoadMapGpsPosition *CurrentGpsPoint;


   if (!roadmap_gps_have_reception()) {
      	roadmap_messagebox("Error", "No GPS connection. Make sure you are outdoors. Currently showing approximate location");
#ifdef IPHONE
	   roadmap_main_show_root(1);
#endif //IPHONE
      	return;
   }

   CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");
   if ((CurrentGpsPoint == NULL) /*|| (CurrentGpsPoint->latitude <= 0) || (CurrentGpsPoint->longitude <= 0)*/) {
 		roadmap_messagebox ("Error", "Can't find current street.");
#ifdef IPHONE
	   roadmap_main_show_root(1);
#endif //IPHONE
 		return;
   }

#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE

   ssd_confirm_dialog ("Speed cam", "Add speed camera on your lane?", TRUE, add_speed_cam_callback , NULL);

}

void add_red_light_cam_alert(void){

    RoadMapGpsPosition *CurrentGpsPoint;

    if (!roadmap_gps_have_reception()) {
           roadmap_messagebox("Error", "No GPS connection. Make sure you are outdoors. Currently showing approximate location");
#ifdef IPHONE
       roadmap_main_show_root(1);
#endif //IPHONE
           return;
    }

    CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");
    if ((CurrentGpsPoint == NULL) /*|| (CurrentGpsPoint->latitude <= 0) || (CurrentGpsPoint->longitude <= 0)*/) {
       roadmap_messagebox ("Error", "Can't find current street.");
#ifdef IPHONE
       roadmap_main_show_root(1);
#endif //IPHONE
       return;
    }

#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE

    ssd_confirm_dialog ("Red light cam", "Add red light camera on your lane?", TRUE, add_red_light_cam_callback , NULL);
}

static void alert_get_city_street(PluginLine *line, const char **city_name, const char **street_name){

	if (line->plugin_id == EditorPluginID) {
#if 0
		EditorStreetProperties properties;

		if (editor_db_activate (line->fips) == -1) return ;

		editor_street_get_properties (line->line_id, &properties);

		*street_name = editor_street_get_street_name (&properties);

		*city_name = editor_street_get_street_city
					 (&properties, ED_STREET_LEFT_SIDE);
#endif

	} else {
		RoadMapStreetProperties properties;

		if (roadmap_locator_activate (line->fips) < 0) {
			*city_name = "";
			*street_name = "";
			return ;
		}

		roadmap_street_get_properties (line->line_id, &properties);

		*street_name = roadmap_street_get_street_name (&properties);

		*city_name = roadmap_street_get_street_city
					 (&properties, ROADMAP_STREET_LEFT_SIDE);

	}
}

void request_speed_cam_delete(){
    RoadMapGpsPosition *CurrentGpsPoint;
    RoadMapPosition    CurrentFixedPosition;
    int steering;

    CurrentGpsPoint = (RoadMapGpsPosition *)roadmap_trip_get_gps_position("AlertSelection");

    CurrentFixedPosition.longitude = CurrentGpsPoint->longitude;
    CurrentFixedPosition.latitude = CurrentGpsPoint->latitude;

	steering = CurrentGpsPoint->steering;

	remove_alert(&CurrentFixedPosition,steering,"Speed Cam");
	roadmap_trip_restore_focus();
	ssd_dialog_hide_all(dec_ok);

}
