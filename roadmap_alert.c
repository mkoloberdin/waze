/* roadmap_alert.c - Manage the alert points in DB.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_db_alert.h"
#include "roadmap_layer.h"
#include "roadmap_config.h"
#include "roadmap_sound.h"
#include "editor/static/add_alert.h"


#include "roadmap_screen.h"
#include "roadmap_alert.h"
#include "roadmap_alerter.h"

#define ALERT_AUDIO_SPEED_CAM		"ApproachSpeedCam"
#define ALERT_AUDIO_RED_LIGHT    "ApproachRedLightCam"
#define ALERT_ICON_SPEED_CAM 		"speed_cam_alert"
#define WARN_ICON_SPEED_CAM 		"speed_cam_warn"
#define ALERT_ICON_RED_LIGHT     "redlightcam_alert"
#define MINIMUM_DISTANCE_TO_CHECK 80

static void *roadmap_alert_map (const roadmap_db_data_file *file);
static void roadmap_alert_activate (void *context);
static void roadmap_alert_unmap (void *context);
static int roadmap_alert_is_cancelable(int alertId);
static BOOL roadmap_alert_can_send_thumbs_up(int alertId);
static int  roadmap_alert_thumbs_up(int alertId);
static int roadmap_alert_cancel(int alertId);
static int roadmap_alert_check_same_street(int record);
static int roadmap_alert_handle_alert(int alertId);
static BOOL roadmap_alert_is_square_dependent(void);
static BOOL  roadmap_alert_distance_check(RoadMapPosition);
static roadmap_alerter_location_info * roadmap_alert_get_location_info(int alertId){
	return NULL; // information (line_id, square_id) is not cached in these types of alerts
}


static RoadMapConfigDescriptor AlertDistanceCfg =
			ROADMAP_CONFIG_ITEM("Alerts", "Alert Distance");


static char *RoadMapAlertType = "RoadMapAlertContext";
typedef struct {
   char *type;
   RoadMapAlert *Alert;
   int           AlertCount;
} RoadMapAlertContext;

static RoadMapAlertContext *RoadMapAlertActive = NULL;

roadmap_db_handler RoadMapAlertHandler = {
   "alert",
   roadmap_alert_map,
   roadmap_alert_activate,
   roadmap_alert_unmap
};

roadmap_alert_provider RoadmapAlertProvider = {
   "alertDb",
   roadmap_alert_count,
   roadmap_alert_get_id,
   roadmap_alert_get_position,
   roadmap_alert_get_speed,
   roadmap_alert_get_map_icon,
   roadmap_alert_get_alert_icon,
   roadmap_alert_get_warning_icon,
   roadmap_alert_get_distance,
   roadmap_alert_get_alert_sound,
   roadmap_alert_alertable,
   roadmap_alert_get_string,
   roadmap_alert_is_cancelable,
   roadmap_alert_cancel,
   roadmap_alert_check_same_street,
   roadmap_alert_handle_alert,
   roadmap_alert_is_square_dependent,
   roadmap_alert_get_location_info,
   roadmap_alert_distance_check,
   roadmap_alert_get_priority,
   NULL,
   roadmap_alert_can_send_thumbs_up,
   roadmap_alert_thumbs_up,
   NULL,
   NULL,
   NULL,
   roadmap_alert_start_handling,
   roadmap_alert_stop_handling
};

RoadMapAlert * roadmap_alert_get_alert(int record){
   return RoadMapAlertActive->Alert + record;

}

static BOOL roadmap_alert_is_square_dependent(void){
	return TRUE;
}

BOOL roadmap_alert_distance_check(RoadMapPosition gps_pos){
	static RoadMapPosition last_checked_position = {0,0};
	int distance;
	if(!last_checked_position.latitude){ // first time
		last_checked_position = gps_pos;
		return TRUE;
	}

	distance = roadmap_math_distance(&gps_pos, &last_checked_position);
	if (distance < MINIMUM_DISTANCE_TO_CHECK)
		return FALSE;
	else{
		last_checked_position = gps_pos;
		return TRUE;
	}
}

RoadMapAlert *roadmap_alert_get_alert_by_id( int id)
{
   int i;
   RoadMapAlert *alert;

   if (RoadMapAlertActive == NULL)
   	return NULL;

   // Find alert:
   for( i=0; i< RoadMapAlertActive->AlertCount; i++){
      alert = roadmap_alert_get_alert(i);
      if( alert->id == id)
         return alert;
   }
   return NULL;
}

int roadmap_alert_is_cancelable(int alertId){
   return TRUE;
}

static BOOL roadmap_alert_can_send_thumbs_up(int alertId){
   return FALSE;
}

static int roadmap_alert_thumbs_up(int alertId){
   return TRUE;
}

int roadmap_alert_cancel(int alertId){
   RoadMapAlert *pAlert;

   request_speed_cam_delete();
   pAlert = roadmap_alert_get_alert_by_id( alertId);
   if (pAlert)
      pAlert->category = 0;

   return TRUE;
}

static int roadmap_alert_check_same_street(int record){
   return TRUE;
}

static int roadmap_alert_handle_alert(int alertId){
   return FALSE;
}

static void *roadmap_alert_map (const roadmap_db_data_file *file) {

   RoadMapAlertContext *context;

   context = malloc(sizeof(RoadMapAlertContext));
   roadmap_check_allocated(context);

   context->type = RoadMapAlertType;

	if (!roadmap_db_get_data (file,
									  model__tile_alert_data,
									  sizeof(RoadMapAlert),
									  (void **)&(context->Alert),
									  &(context->AlertCount))) {

      roadmap_log (ROADMAP_FATAL, "invalid alert/data structure");
   }

   	//Distance to alert
	roadmap_config_declare
	("preferences", &AlertDistanceCfg, "400", NULL);

   return context;
}

static void roadmap_alert_activate (void *context) {

   RoadMapAlertContext *alert_context = (RoadMapAlertContext *) context;

   if (alert_context != NULL) {

      if (alert_context->type != RoadMapAlertType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate alert (bad type)");
      }
   }

   RoadMapAlertActive = alert_context;
}

static void roadmap_alert_unmap (void *context) {

   RoadMapAlertContext *alert_context = (RoadMapAlertContext *) context;

   if (alert_context->type != RoadMapAlertType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate alert (bad type)");
   }
   if (RoadMapAlertActive == alert_context) {
      RoadMapAlertActive = NULL;
   }
   free(alert_context);
}


int roadmap_alert_count (void) {

   if (RoadMapAlertActive == NULL) {
      return 0;
   }

   return RoadMapAlertActive->AlertCount;
}





void roadmap_alert_get_position(int alert, RoadMapPosition *position, int *steering) {

   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   position->longitude = alert_st->pos.longitude;
   position->latitude = alert_st->pos.latitude;

   *steering = alert_st->steering;
}


unsigned int roadmap_alert_get_speed(int alert){
   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);
   return (int) alert_st->speed;
}

int roadmap_alert_get_category(int alert) {

   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   return (int) alert_st->category;
}


int roadmap_alert_get_id(int alert){
   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   return (int) alert_st->id;
}

// check if an alert should be generated for a specific category
int roadmap_alert_alertable(int record){
	int alert_category = roadmap_alert_get_category(record);

	switch (alert_category) {
	   case ALERT_CATEGORY_SPEED_CAM:
	      return 1;

      case ALERT_CATEGORY_RED_LIGHT_CAM:
         return 1;

      default:
         return 0;
	}
}

const char *  roadmap_alert_get_string(int id){
   RoadMapAlert *alert_st  = roadmap_alert_get_alert_by_id(id);
   if (alert_st == NULL)
      return NULL;
   switch (alert_st->category) {
      case ALERT_CATEGORY_SPEED_CAM:
         return "Speed trap ahead" ;

      case ALERT_CATEGORY_RED_LIGHT_CAM:
         return "Red light cam ahead";

      default:
         return  NULL;
   }

}

const char * roadmap_alert_get_map_icon(int id){

	RoadMapAlert *alert_st  = roadmap_alert_get_alert_by_id(id);
   if (alert_st == NULL)
      return NULL;

	switch (alert_st->category) {

	   case ALERT_CATEGORY_SPEED_CAM:
	      return "rm_speed_cam" ;

	   case ALERT_CATEGORY_DUMMY_SPEED_CAM:
	      return "rm_dummy_speed_cam";

	   case ALERT_CATEGORY_RED_LIGHT_CAM:
	      return "rm_red_light_cam";

	   default:
	      return  NULL;
	}
}

int roadmap_alert_get_priority(void){
	return ALERTER_PRIORITY_MEDIUM;
}

int roadmap_alert_get_distance(int record){
	return roadmap_config_get_integer(&AlertDistanceCfg);
}

const char *roadmap_alert_get_alert_icon(int Id){
   RoadMapAlert *alert_st  = roadmap_alert_get_alert_by_id(Id);
   if (alert_st == NULL)
      return NULL;

   switch (alert_st->category) {

      case ALERT_CATEGORY_SPEED_CAM:
         return ALERT_ICON_SPEED_CAM;

      case ALERT_CATEGORY_RED_LIGHT_CAM:
         return ALERT_ICON_RED_LIGHT;

      default:
         return  NULL;
   }

}

const char *roadmap_alert_get_warning_icon(int Id){
   RoadMapAlert *alert_st  = roadmap_alert_get_alert_by_id(Id);
   if (alert_st == NULL)
      return NULL;

   switch (alert_st->category) {

   case ALERT_CATEGORY_SPEED_CAM:
      return WARN_ICON_SPEED_CAM;

   case ALERT_CATEGORY_RED_LIGHT_CAM:
      return ALERT_ICON_RED_LIGHT;

   default:
      return  NULL;
   }
}

RoadMapSoundList roadmap_alert_get_alert_sound (int Id) {

   RoadMapSoundList sound_list;
   RoadMapAlert *alert_st = roadmap_alert_get_alert_by_id (Id);
   if (alert_st == NULL)
      return NULL;
   sound_list = roadmap_sound_list_create (0);
   switch (alert_st->category) {
      case ALERT_CATEGORY_SPEED_CAM:
         roadmap_sound_list_add (sound_list, ALERT_AUDIO_SPEED_CAM);
         break;

      case ALERT_CATEGORY_RED_LIGHT_CAM:
         roadmap_sound_list_add (sound_list, ALERT_AUDIO_RED_LIGHT);
         break;

      default:
         break;
   }
   return sound_list;
}

int roadmap_alert_start_handling(int id){
#ifdef OPENGL
   RoadMapGuiPoint offset = {0,0};
   RoadMapAlert *pAlert = roadmap_alert_get_alert_by_id (id);
   if (pAlert == NULL)
      return FALSE;
   roadmap_screen_start_glow(&pAlert->pos, 120, &offset);
   return TRUE;
#endif
   return FALSE;
}

int roadmap_alert_stop_handling(int alert){
#ifdef OPENGL
   roadmap_screen_stop_glow();
   return TRUE;
#endif
   return FALSE;
}
