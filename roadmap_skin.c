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
static BOOL  ToggleToNightMode = TRUE;

static void toggle_skin_timer(void);

static const char *get_map_schema(void){
   return (const char *) roadmap_config_get(&RoadMapConfigMapScheme);
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


void roadmap_skin_set_subskin (const char *sub_skin) {
   const char *base_path = roadmap_path_preferred ("skin");
   char path[512]; //Avi R - was 256 - not enough for iPhone
   char *skin_path;
   char *subskin_path;
   CurrentSubSkin = sub_skin;

   skin_path = roadmap_path_join (base_path, CurrentSkin);
   subskin_path = roadmap_path_join (skin_path, CurrentSubSkin);
   
   if (!strcmp(CurrentSubSkin,"day")){
      char *subskin_path2;
      subskin_path2 = roadmap_path_join (subskin_path, get_map_schema());
      snprintf (path, sizeof(path), "%s,%s,%s", subskin_path2, skin_path, roadmap_path_downloads());
      roadmap_path_free (subskin_path2);
   }
   else{
      snprintf (path, sizeof(path), "%s,%s,%s", subskin_path, skin_path, roadmap_path_downloads());
   }
   roadmap_path_set ("skin", path);

   roadmap_path_free (subskin_path);
   roadmap_path_free (skin_path);

   roadmap_config_reload ("schema");
   notify_listeners ();

   roadmap_screen_redraw ();
   
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
      roadmap_skin_set_subskin ("night");
   else
      roadmap_skin_set_subskin ("day");
   roadmap_skin_auto_night_mode_kill_timer();
}

static int auto_night_mode_cfg_on (void) {
   return (roadmap_config_match(&RoadMapConfigAutoNightMode, "yes"));
}

static void roadmap_skin_gps_listener
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *position){
   time_t rawtime_sunset, rawtime_sunrise, rawtime_temp;
   static int has_run = 0;
   time_t now ;
   int timer_t;
   struct tm realtime, *tmp_tm;
   //struct tm *realtime2;
   
   //time_t now = time(NULL);

#ifdef _WIN32
   struct tm curtime_gmt;
#endif

   if ((gps_time + (3600 * 24)) < time(NULL))
      now = time(NULL);
   else
      now = gps_time;

#ifdef _WIN32
   curtime_gmt = *(gmtime(&now));
   now  = mktime(&curtime_gmt);
#endif
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
//   realtime2 = localtime (&rawtime_sunset);
  
   //printf ("sunset:  %d:%d\n", realtime2->tm_hour, realtime2->tm_min);

   if (realtime.tm_hour > 12){
      rawtime_temp = rawtime_sunrise;
      rawtime_sunrise = rawtime_sunset;
      rawtime_sunset = rawtime_temp;
   }
   
   //printf ("sunrise: %d:%d\n", realtime->tm_hour, realtime->tm_min);
   //printf ("sunset %s\n\n", asctime (realtime2) );

   //sprintf(msg, "sunset:  %d:%d %d-%d\n sunrise: %d:%d %d-%d\n", realtime2->tm_hour, realtime2->tm_min, realtime2->tm_mday, realtime2->tm_mon+1,realtime->tm_hour, realtime->tm_min, realtime->tm_mday, realtime->tm_mon+1);
   //printf("%s\n", msg);

   if (rawtime_sunset > rawtime_sunrise){
      roadmap_skin_set_subskin ("night");
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
   
   roadmap_config_declare
      ("user", &RoadMapConfigMapScheme, "2", NULL);
   
   map_scheme = get_map_schema();
   if (map_scheme[0] != 0){
      roadmap_skin_set_subskin ("day");
   }
   roadmap_skin_auto_night_mode();
}

void roadmap_skin_set_scheme(const char *new_scheme){
   roadmap_config_set(&RoadMapConfigMapScheme,new_scheme );
   roadmap_skin_set_subskin (CurrentSubSkin);
}

const int roadmap_skin_get_scheme(void){
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

