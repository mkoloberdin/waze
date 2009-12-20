/* editor_bar.c
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
#include "../roadmap.h"
#include "../roadmap_res.h"
#include "../roadmap_bar.h"
#include "../roadmap_sound.h"
#include "../roadmap_screen.h"
#include "../roadmap_canvas.h"
#include "../roadmap_math.h"
#include "../roadmap_pointer.h"
#include "../roadmap_config.h"
#include "../roadmap_lang.h"
#include "../ssd/ssd_widget.h"
#include "../navigate/navigate_main.h"
#include "track/editor_track_main.h"

#define EDITOR_BAR_BG "editor_bar_bg"
#define EDITOR_BAR_FOREGROUND  ("#ffffff")


static RoadMapImage gBgImage;
static RoadMapImage gBgFullImage;
static int  gBgImageWidth, gBgImageHeight;
static BOOL gShowBar;
static RoadMapPen gEditorBarPen;
static BOOL gInitialized = FALSE;
static RoadMapScreenSubscriber gEditor_bar_prev_after_refresh = NULL;
static RoadMapGuiRect RecIconRct;
static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Editor Bar", "Feature Enabled");
static int gLength = 0;
static int gTempLength = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL editor_bar_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes"))
      return TRUE;
   return FALSE; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int editor_bar_pressed (RoadMapGuiPoint *point) {
   static RoadMapImage RecordDownImage = NULL;  
   RoadMapGuiPoint position;
   
   if (!gShowBar)
      return 0;
   
   if (RecordDownImage == NULL){
      RecordDownImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "StopButtonDown");
        if (RecordDownImage == NULL){
           roadmap_log (ROADMAP_ERROR, "editor_bar - cannot load %s image ", "StopButtonDown");
        }
   }
   
   if ((point->x >= (RecIconRct.minx -10)) &&
        (point->x <= (RecIconRct.maxx +10)) &&
        (point->y >= (RecIconRct.miny -10)) &&
        (point->y <= (RecIconRct.maxy + 10))) {
      
      position.x = 20;
      position.y = roadmap_canvas_height() - roadmap_bar_bottom_height() - gBgImageHeight + 5 ;
      if (RecordDownImage)
         roadmap_canvas_draw_image (RecordDownImage, &position,0, IMAGE_NORMAL);
      roadmap_canvas_refresh ();
      return 1;
      
   }
   
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int editor_bar_short_click(RoadMapGuiPoint *point) {
   static RoadMapSoundList list;

   if (!gShowBar)
      return 0;

   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click");
      roadmap_res_get (RES_SOUND, 0, "click");
   }

   if ((point->x >= (RecIconRct.minx -10)) &&
        (point->x <= (RecIconRct.maxx +10)) &&
        (point->y >= (RecIconRct.miny -10)) &&
        (point->y <= (RecIconRct.maxy + 10))) {
       
      roadmap_sound_play_list (list);
      editor_track_toggle_new_roads();
      return 1;
       
    }

   return 0;

}



//////////////////////////////////////////////////////////////////////////////////////////////////
static void editor_bar_after_refresh (void) {
   RoadMapGuiPoint position;
   int allign;
   static RoadMapImage RecordUpImage = NULL;
   char msg[120];
   int distance_far;
   char str[100];
   char unit_str[20];
   int length;
   
   if (!gInitialized)
      return;
   
   if (navigate_main_state() == 0){
      if (gEditor_bar_prev_after_refresh) {
            (*gEditor_bar_prev_after_refresh) ();
      }
      return;
   }
   
   if (!gShowBar){
      if (gEditor_bar_prev_after_refresh) {
            (*gEditor_bar_prev_after_refresh) ();
      }
      return;
   }

   if (editor_ignore_new_roads())
   {
      if (gEditor_bar_prev_after_refresh) {
           (*gEditor_bar_prev_after_refresh) ();
      }
      return;
   }
   
   position.x = 0;
   position.y = roadmap_canvas_height() - roadmap_bar_bottom_height() - gBgImageHeight ;
   roadmap_canvas_draw_image (gBgFullImage, &position, 0, IMAGE_NORMAL);
   
   
#ifdef TOUCH_SCREEN   
   position.x = 20;
   position.y = roadmap_canvas_height() - roadmap_bar_bottom_height() - gBgImageHeight + 5 ;
   if (RecordUpImage == NULL){
      RecordUpImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "StopButtonUp");
        if (RecordUpImage == NULL){
           roadmap_log (ROADMAP_ERROR, "editor_bar - cannot load %s image ", "StopButtonUp");
        }
   }
   
   
   if (RecordUpImage){
      roadmap_canvas_draw_image (RecordUpImage, &position, 0, IMAGE_NORMAL);
      RecIconRct.minx = position.x;
      RecIconRct.miny = position.y;
      RecIconRct.maxx = RecIconRct.minx + roadmap_canvas_image_width(RecordUpImage); 
      RecIconRct.maxy = RecIconRct.miny + roadmap_canvas_image_height(RecordUpImage); 
   }
#endif //TOUCH_SCREEN   
   if (ssd_widget_rtl(NULL)){
      allign = ROADMAP_CANVAS_RIGHT;
      position.x = roadmap_canvas_width() -10;
   }
   else{
      allign = ROADMAP_CANVAS_LEFT;
      position.x = 60;
   }
   
   length = gLength + gTempLength;
   distance_far =
      roadmap_math_to_trip_distance(length);
   str[0] = 0;
   unit_str[0] = 0;
   if (distance_far > 0) {

      int tenths = roadmap_math_to_trip_distance_tenths(length);
      if (distance_far < 50)
         snprintf (str, sizeof(str), "%d.%d", distance_far, tenths % 10);
      else
         snprintf (str, sizeof(str), "%d", distance_far);
         snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
   } else {
      if (!roadmap_math_is_metric()){
         int tenths = roadmap_math_to_trip_distance_tenths(length);
         snprintf (str, sizeof(str), "0.%d", tenths % 10);
         snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
      }
      else{
         snprintf (str, sizeof(str), "%d", roadmap_math_distance_to_current(length));
         snprintf (unit_str, sizeof(unit_str), "%s",
                  roadmap_lang_get(roadmap_math_distance_unit()));
      }
   }
   
   sprintf(msg, "%s %s %s",roadmap_lang_get("Recording new roads:"), str, unit_str );
   roadmap_canvas_select_pen (gEditorBarPen);
   position.y = roadmap_canvas_height() - roadmap_bar_bottom_height() - gBgImageHeight +   gBgImageHeight/4;
   roadmap_canvas_draw_string_size (&position,
                                    allign |ROADMAP_CANVAS_TOP,
                                    16,msg);
   
   
   if (gEditor_bar_prev_after_refresh) {
      (*gEditor_bar_prev_after_refresh) ();
   }
}

/////////////////////////////////////////////////////////
void editor_bar_show(void){
   if (!gInitialized)
      return;
   gShowBar = TRUE;
}


/////////////////////////////////////////////////////////
void editor_bar_hide(void){
   if (!gInitialized)
      return;
   gShowBar = FALSE;
}

void editor_bar_set_length(int length){
   gLength += length;
}


void editor_bar_set_temp_length(int length){
   gTempLength = length;
}

/////////////////////////////////////////////////////////
static RoadMapImage createBGImage (RoadMapImage BarBgImage) {

   RoadMapImage image;
   int width = roadmap_canvas_width ();
   int height = roadmap_canvas_height();
   int num_images;
   int image_width;
   int i;

   if (height > width){
      width = width*2;
   }
   
   image = roadmap_canvas_new_image (width,
               roadmap_canvas_image_height(BarBgImage));
   image_width = roadmap_canvas_image_width(BarBgImage);
   
   num_images = width / image_width ;
   
   for (i = 0; i < num_images; i++){
      RoadMapGuiPoint BarLocation;
      BarLocation.y = 0;
      BarLocation.x = i * image_width;
         roadmap_canvas_copy_image
         (image, &BarLocation, NULL, BarBgImage ,IMAGE_NORMAL);
   }

   return image;
}

/////////////////////////////////////////////////////////
void editor_bar_initialize(void){
   
   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "no",
                  "yes", NULL);
   
   if (!editor_bar_feature_enabled())
      return;
   
   gBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, EDITOR_BAR_BG);
   if (gBgImage == NULL){
      roadmap_log (ROADMAP_ERROR, "editor_bar - cannot load %s image ", EDITOR_BAR_BG);
      return;
   }
   
   gBgFullImage = createBGImage(gBgImage);
   
   gBgImageWidth= roadmap_canvas_image_width(gBgImage);
   gBgImageHeight = roadmap_canvas_image_height(gBgImage);
   
   gEditorBarPen = roadmap_canvas_create_pen ("editor_bar_pen");
   roadmap_canvas_set_foreground (EDITOR_BAR_FOREGROUND);
   
   
   gEditor_bar_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (editor_bar_after_refresh);

#ifdef TOUCH_SCREEN   
   roadmap_pointer_register_pressed
         (editor_bar_pressed, POINTER_HIGH);
   roadmap_pointer_register_short_click(editor_bar_short_click, POINTER_HIGH);
   roadmap_pointer_register_long_click(editor_bar_short_click, POINTER_HIGH);
#endif //TOUCH_SCREEN
   gInitialized = TRUE;

}
