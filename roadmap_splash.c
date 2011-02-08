/* roadmap_splash.c
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
#include "roadmap_canvas.h"
#include "roadmap_res.h"
#include "roadmap_res_download.h"
#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_splash.h"
#include "Realtime/Realtime.h"
#include "websvc_trans/web_date_format.h"

static RoadMapConfigDescriptor RoadMapConfigSplashFeatureEnabled = ROADMAP_CONFIG_ITEM("Splash", "Feature Enabled");
static RoadMapConfigDescriptor RoadMapConfigSplashUpdateTime = ROADMAP_CONFIG_ITEM("Splash", "Update time");
static RoadMapConfigDescriptor RoadMapConfigLastCheckTime = ROADMAP_CONFIG_ITEM("Splash", "Last check time");

static BOOL initialized = FALSE;
static RoadMapCallback SplashNextLoginCb = NULL;
static void download_wide_splash(void);

#define START_DOWNLOAD_DELAY  30000
#define SPLASH_CHECK_INTERVAL 6 * 3600


typedef struct {
   const char   *name;
   int           min_screen_width;
   int           height;
   BOOL          is_wide;
} SplashFiles;

static SplashFiles RoadMapSplashFiles[] = {
#ifdef IPHONE
   {"welcome_768_1004", 700, -1,FALSE},
   {"welcome_640_960", 500, -1,FALSE},
   {"welcome_320_480", 200, -1,FALSE},
   {"welcome_wide_480_320", 200,-1, TRUE},
#else
   {"welcome_480_816", 480, -1,FALSE},
   {"welcome_360_640", 360, -1,FALSE},
   {"welcome_320_480", 320, 480,FALSE},
   {"welcome_320_455", 320, 455,FALSE},
   {"welcome_240_320", 240, -1,FALSE},
   {"welcome_wide_854_442", 800,-1, TRUE},
   {"welcome_wide_640_360", 640,-1, TRUE},
   {"welcome_wide_480_320", 480,320, TRUE},
   {"welcome_wide_480_295", 480,295, TRUE},
   {"welcome_wide_320_240", 320,-1, TRUE},
#endif
};

//////////////////////////////////////////////////////////////////
static void roadmap_splash_init_params (void) {

   roadmap_config_declare ("session", &RoadMapConfigSplashUpdateTime, "", NULL);

   roadmap_config_declare ("session", &RoadMapConfigLastCheckTime, "-1", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigSplashFeatureEnabled, NULL, "no",
                  "yes", NULL);

   initialized = TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_splash_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigSplashFeatureEnabled), "yes"))
      return TRUE;
   return FALSE;

}
//////////////////////////////////////////////////////////////////
static const char *roadmap_splash_get_update_time (void) {
   if (!initialized)
      roadmap_splash_init_params ();

   return roadmap_config_get (&RoadMapConfigSplashUpdateTime);
}

//////////////////////////////////////////////////////////////////
void roadmap_splash_set_update_time (const char *update_time) {

   if (!initialized)
      roadmap_splash_init_params ();

   roadmap_config_set (&RoadMapConfigSplashUpdateTime, update_time);
}

//////////////////////////////////////////////////////////////////
static void roadmap_splash_set_check_time(void){
   int time_now = (int)time (NULL);
   roadmap_config_set_integer (&RoadMapConfigLastCheckTime, time_now);
}

//////////////////////////////////////////////////////////////////
void roadmap_splash_reset_check_time(void){
   roadmap_config_set_integer (&RoadMapConfigLastCheckTime, -1);
}

//////////////////////////////////////////////////////////////////
int roadmap_splash_get_last_check_time(void){
 return roadmap_config_get_integer(&RoadMapConfigLastCheckTime);
}

//////////////////////////////////////////////////////////////////
static BOOL should_check_for_new_file(){
   time_t now;
   int last_check_time = roadmap_splash_get_last_check_time();

   if (last_check_time == -1)
      return TRUE;

   now = time(NULL);

   if ((now - last_check_time) > SPLASH_CHECK_INTERVAL)
      return TRUE;
   else
      return FALSE;
}

//////////////////////////////////////////////////////////////////
static const char *roadmap_splash_get_splash_name(BOOL wide){
   unsigned int i;
   const char *splash_file = NULL;

   int width = roadmap_canvas_width();
   int height = roadmap_canvas_height();

   if ((height < width) && !wide){
      width = height;
      height = roadmap_canvas_width();
   }
   else if ((width < height) && wide){
      width = height;
      height = roadmap_canvas_width();
   }

   for (i=0; i<sizeof(RoadMapSplashFiles)/sizeof(RoadMapSplashFiles[0]); i++) {

      if ((width >= RoadMapSplashFiles[i].min_screen_width) && (wide == RoadMapSplashFiles[i].is_wide)) {
         if ((RoadMapSplashFiles[i].height != -1) ){
            if (RoadMapSplashFiles[i].height == height){
               splash_file = RoadMapSplashFiles[i].name;
               break;
            }
         }
         else{
            splash_file = RoadMapSplashFiles[i].name;
            break;
         }
      }
   }

   if (!splash_file) {
      static char file_name[50];
      static char file_name_wide[50];
      roadmap_log
         (ROADMAP_ERROR, "Can't find splash file for screen width: %d height: %d (wide =%d)",
          width, height, wide);
      if (wide){
         sprintf(file_name_wide, "welcome_wide_%d_%d",width, height);
         return &file_name_wide[0];
      }else{
         sprintf(file_name,"welcome_%d_%d",width, height);
         return &file_name[0];
      }

   }

   return splash_file;
}



//////////////////////////////////////////////////////////////////
static void on_splash_downloaded (const char* res_name, int success, void *context, char *last_modified) {
   if (success){
       if (last_modified && *last_modified)
          roadmap_splash_set_update_time(last_modified);
#ifndef IPHONE
       download_wide_splash();
#endif
       roadmap_splash_set_check_time();
   }
   else{
      roadmap_splash_set_update_time("");
   }
}

//////////////////////////////////////////////////////////////////
static void on_wide_splash_downloaded (const char* res_name, int success, void *context, char *last_modified) {
   if (success){
       if (last_modified && *last_modified)
          roadmap_splash_set_update_time(last_modified);
   }
   else{
         roadmap_splash_set_update_time("");
   }
}

//////////////////////////////////////////////////////////////////
static void download_splash(void){
   time_t update_time;
   const char *file_name = roadmap_splash_get_splash_name(FALSE);
   const char* last_save_time = roadmap_splash_get_update_time();

   if (!file_name)
      return;

   if (last_save_time[0] == 0) {
      update_time = 0;
   }
   else {
      update_time = WDF_TimeFromModifiedSince(last_save_time);
   }
   roadmap_res_download (RES_DOWNLOAD_COUNTRY_SPECIFIC_IMAGES, file_name, "welcome", "", TRUE, update_time,
                  on_splash_downloaded, NULL);

}

//////////////////////////////////////////////////////////////////
static void download_wide_splash(void){
   time_t update_time  = 0;
   const char *file_name = roadmap_splash_get_splash_name(TRUE);

   roadmap_res_download (RES_DOWNLOAD_COUNTRY_SPECIFIC_IMAGES, file_name, "welcome_wide", "", TRUE, update_time,
                  on_wide_splash_downloaded, NULL);
}

//////////////////////////////////////////////////////////////////
static void roadmap_splash_delayed_start_download(void){
   download_splash();
   roadmap_main_remove_periodic(roadmap_splash_delayed_start_download);
}

//////////////////////////////////////////////////////////////////
void roadmap_splash_login_cb(void){
   if (should_check_for_new_file())
      roadmap_main_set_periodic(START_DOWNLOAD_DELAY,roadmap_splash_delayed_start_download);

   Realtime_NotifySplashUpdateTime(roadmap_splash_get_update_time());

   if (SplashNextLoginCb) {
      SplashNextLoginCb ();
      SplashNextLoginCb = NULL;
   }
}

//////////////////////////////////////////////////////////////////
void roadmap_splash_download_init(void){
   if (!initialized)
      roadmap_splash_init_params();

   if (roadmap_splash_feature_enabled())
      SplashNextLoginCb = Realtime_NotifyOnLogin (roadmap_splash_login_cb);
   else
      roadmap_log (ROADMAP_DEBUG, "Splash download disabled");
}

//////////////////////////////////////////////////////////////////
void roadmap_splash_display (void) {
#if !defined(ANDROID) && !defined(IPHONE)
   int height, width;
   RoadMapImage image;
   RoadMapGuiPoint pos;

   height = roadmap_canvas_height ();
   width = roadmap_canvas_width ();


   if (height > width)
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE, "welcome");
   else
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN|RES_NOCACHE, "welcome_wide");

   if( !image)
      return;

   pos.x = (width - roadmap_canvas_image_width(image)) / 2;
   pos.y = (height - roadmap_canvas_image_height(image)) / 2;
   roadmap_canvas_draw_image (image, &pos, 0, IMAGE_NORMAL);
   roadmap_canvas_free_image (image);
   roadmap_canvas_refresh ();
#endif
}
