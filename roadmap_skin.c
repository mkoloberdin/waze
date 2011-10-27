/* roadmap_skin.c - manage skins
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 * DESCRIPTION:
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_messagebox.h"
#include "roadmap_main.h"
#include "roadmap_path.h"
#include "roadmap_config.h"
#include "roadmap_screen.h"
#include "roadmap_plugin.h"
#include "roadmap_gps.h"
#include "roadmap_navigate.h"
#include "roadmap_skin.h"
#include "roadmap_sunrise.h"
#include "roadmap_trip.h"
#include "roadmap_config.h"

#define MAX_LISTENERS 16
static RoadMapCallback RoadMapSkinListeners[MAX_LISTENERS] = {NULL};
static const char *CurrentSkin = "default";
static const char *CurrentSubSkin = "day";

RoadMapConfigDescriptor RoadMapConfigAutoNightMode =
                        ROADMAP_CONFIG_ITEM("Display", "Auto night mode");

static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Auto night mode", "Feature enabled");

RoadMapConfigDescriptor RoadMapConfigMapScheme =
                        ROADMAP_CONFIG_ITEM("Display", "Map scheme");

RoadMapConfigDescriptor RoadMapConfigMapSubSkin =
                        ROADMAP_CONFIG_ITEM("Display", "Map sub_skin");

static BOOL  ToggleToNightMode = TRUE;

static void toggle_skin_timer(void);
static int auto_night_mode_cfg_on (void);


static const char *get_map_schema(void){
   return (const char *) roadmap_config_get(&RoadMapConfigMapScheme);
}

static const char *get_map_sub_skin(void){
   return (const char *) roadmap_config_get(&RoadMapConfigMapSubSkin);
}


static void notify_listeners (void) {
   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {

      if (RoadMapSkinListeners[i] == NULL) break;

      (RoadMapSkinListeners[i]) ();
   }

}


void roadmap_skin_register (RoadMapCallback listener) {

   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {
      if (RoadMapSkinListeners[i] == NULL) {
         RoadMapSkinListeners[i] = listener;
         break;
      }
   }
}


static void roadmap_skin_set_subskin_int (const char *sub_skin, BOOL save) {
   const char *base_path = roadmap_path_preferred ("skin");
   char path[1024];
   char *skin_path = NULL;
   char *subskin_path;
   const char *cursor;
   char *subskin_path2 = NULL;
   CurrentSubSkin = sub_skin;

   skin_path = roadmap_path_join (base_path, CurrentSkin);
   subskin_path = roadmap_path_join (skin_path, CurrentSubSkin);
//   offset = strlen(path);

   if (!strcmp(CurrentSubSkin,"day")){
      subskin_path2 = roadmap_path_join (subskin_path, get_map_schema());

      snprintf (path, sizeof(path), "%s", subskin_path2);
      roadmap_path_free (subskin_path2);
   }
   else{
      snprintf (path, sizeof(path), "%s", subskin_path);
   }


   for ( cursor = roadmap_path_first ("skin");
                  cursor != NULL;
			  cursor = roadmap_path_next ("skin", cursor))
   {
	 if ( !((strstr(cursor,"day") || strstr(cursor,"night")))){
	   strcat(path, ",");
		strcat(path, cursor);

	 }
   }

   roadmap_path_set ("skin", path);

   roadmap_path_free (subskin_path);
   roadmap_path_free (skin_path);
   
   if (save && !auto_night_mode_cfg_on()) {
      roadmap_config_set(&RoadMapConfigMapSubSkin,sub_skin );
   }

   if (save && !auto_night_mode_cfg_on()) {
      roadmap_config_set(&RoadMapConfigMapSubSkin,sub_skin );
   }

   roadmap_config_reload ("schema");
   notify_listeners ();

   roadmap_screen_redraw ();

}

void roadmap_skin_set_subskin (const char *sub_skin) {
   roadmap_skin_set_subskin_int (sub_skin, TRUE);
}

void roadmap_skin_toggle (void) {
   if (!strcmp (CurrentSubSkin, "day")) {
      roadmap_skin_set_subskin ("night");
   } else {

      roadmap_skin_set_subskin ("day");
   }
}


int roadmap_skin_state_screen_touched(void){

  if (roadmap_screen_touched_state() == -1)
      return -1;

   if (!strcmp (CurrentSubSkin, "day")) {
      return 0;
   } else {
      return 1;
   }
}


int roadmap_skin_state(void){

   if (!strcmp (CurrentSubSkin, "day")) {
      return 0;
   } else {
      return 1;
   }
}


void roadmap_skin_auto_night_mode_kill_timer(void){
   roadmap_main_remove_periodic(toggle_skin_timer);
}

static void toggle_skin_timer(void){
   if (ToggleToNightMode)
      roadmap_skin_set_subskin_int ("night", FALSE);
   else
      roadmap_skin_set_subskin_int ("day", FALSE);
   roadmap_skin_auto_night_mode_kill_timer();
}

static int auto_night_mode_cfg_on (void) {
   return (roadmap_config_match(&RoadMapConfigAutoNightMode, "yes"));
}

static void roadmap_skin_gps_listener
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *position){
   time_t rawtime_sunset, rawtime_sunrise;
   static int has_run = 0;
   time_t now ;
   int timer_t;
   struct tm realtime, *tmp_tm;
   struct tm *realtime2;
   static int num_points = 0;

   //time_t now = time(NULL);

   if (num_points < 3){
	   num_points++;
	   return;
   }

   now = gps_time;


   //realtime = localtime (&now);
//   printf ("gpstime %s", asctime (realtime) );

   roadmap_gps_unregister_listener(roadmap_skin_gps_listener);

   if (has_run)
      return;


   has_run = TRUE;


   rawtime_sunrise = roadmap_sunrise (position, now);
   tmp_tm = localtime (&rawtime_sunrise);
   realtime = *tmp_tm;
//   printf ("sunrise: %d:%d\n", tmp_tm->tm_hour, tmp_tm->tm_min);

   rawtime_sunset = roadmap_sunset (position, now);
   realtime2 = localtime (&rawtime_sunset);

   //printf ("sunset:  %d:%d\n", realtime2->tm_hour, realtime2->tm_min);


   //printf ("sunrise: %d:%d\n", realtime->tm_hour, realtime->tm_min);
   //printf ("sunset %s\n\n", asctime (realtime2) );

   //sprintf(msg, "sunset:  %d:%d %d-%d\n sunrise: %d:%d %d-%d\n", realtime2->tm_hour, realtime2->tm_min, realtime2->tm_mday, realtime2->tm_mon+1,realtime->tm_hour, realtime->tm_min, realtime->tm_mday, realtime->tm_mon+1);
   //printf("%s\n", msg);

   if (rawtime_sunset > rawtime_sunrise){
      roadmap_skin_set_subskin_int ("night", FALSE);
      timer_t = rawtime_sunrise - now;
      if ((timer_t > 0) && (timer_t < 1800))
         roadmap_main_set_periodic (timer_t*1000, toggle_skin_timer);
   }
   else{
      timer_t = rawtime_sunset - now;
      if ((timer_t > 0) && (timer_t < 1800)){
         ToggleToNightMode = TRUE;
         roadmap_main_set_periodic (timer_t*1000, toggle_skin_timer);
      }
   }

}

void roadmap_skin_init(void){
   const char *map_scheme;
   const char *map_sub_skin;

   roadmap_config_declare
      ("user", &RoadMapConfigMapScheme, "", NULL);
   
   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigMapSubSkin, NULL, "day", "night", NULL);

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigMapSubSkin, NULL, "day", "night", NULL);

   map_scheme = get_map_schema();
   map_sub_skin = get_map_sub_skin();
   
   roadmap_skin_auto_night_mode();
   
   if (!auto_night_mode_cfg_on() && !strcmp(map_sub_skin, "night")){
      roadmap_skin_set_subskin_int (map_sub_skin, FALSE);
   } else if (map_scheme[0] != 0){
      roadmap_skin_set_subskin_int ("day", FALSE);
   }
}

void roadmap_skin_set_scheme(const char *new_scheme){
   roadmap_config_set(&RoadMapConfigMapScheme,new_scheme );
   roadmap_skin_set_subskin_int (CurrentSubSkin, FALSE);
}

int roadmap_skin_get_scheme(void){
   return roadmap_config_get_integer(&RoadMapConfigMapScheme);
}

BOOL roadmap_skin_auto_night_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes"))
      return TRUE;
   return FALSE;

}

void roadmap_skin_auto_night_mode(void){

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "no",
                  "yes", NULL);

   if (!roadmap_skin_auto_night_feature_enabled())
      return;

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigAutoNightMode, NULL, "yes", "no", NULL);

   if (!auto_night_mode_cfg_on())
      return;

   roadmap_gps_register_listener (roadmap_skin_gps_listener);

}

