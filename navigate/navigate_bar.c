/* navigate_bar.c - implement navigation bar
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
 * SYNOPSYS:
 *
 *   See navigate_bar.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_math.h"
#include "roadmap_lang.h"
#include "roadmap_bar.h"
#include "roadmap_message.h"
#include "roadmap_utf8.h"
#include "roadmap_pointer.h"
#include "roadmap_navigate.h"
#include "roadmap_speedometer.h"
#include "roadmap_skin.h"
#include "ssd/ssd_widget.h"

#include "navigate_main.h"
#include "navigate_bar.h"
#include "roadmap_config.h"


static RoadMapConfigDescriptor RMConfigNavBarDestDistanceFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Destination Distance Font");
static RoadMapConfigDescriptor RMConfigNavBarDestTimeFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Destination Time Font");
static RoadMapConfigDescriptor RMConfigNavBarStreetNormalFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Street Normal Font");
static RoadMapConfigDescriptor RMConfigNavBarStreetSmalllFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Street Small Font");

typedef struct {
   RoadMapGuiPoint instruction_pos;
   RoadMapGuiPoint exit_pos;
   RoadMapGuiPoint distance_value_pos;
   int		       street_line1_pos;
   int		       street_line2_pos;
   int             street_start;
   int             street_width;

   RoadMapGuiPoint distance_to_destination_pos;

   RoadMapGuiPoint time_to_destination_pos;

} NavigateBarPanel;



#define  NAV_BAR_PIXELS( val ) \
           ( roadmap_screen_is_hd_screen() ? ( ( (val << 7)  + (val << 6) ) >> 7 ) : val )       // 1.5 * val

#define  NAV_BAR_TEXT_COLOR                   ("#000000")
#define  NAV_BAR_TEXT_COLOR_GREEN             ("#d7ff00")

#define NAV_BAR_TIME_TO_DESTINATION_COLOR_DAY     ("#ffffff")
#define NAV_BAR_DIST_TO_DESTINATION_COLOR_DAY     ("#ffffff")

#define NAV_BAR_TIME_TO_DESTINATION_COLOR_NIGHT     ("#d7ff00")
#define NAV_BAR_DIST_TO_DESTINATION_COLOR_NIGHT     ("#d7ff00")

static NavigateBarPanel NavigateBarDefaultPanels[] = {

   {{0, 0}, {0, 0}, {0, 0}, 0,-1, 0, 0, {0, 0}, {0,0} },

};

static NavigateBarPanel *NavigatePanel = NULL;
static RoadMapScreenSubscriber navigate_prev_after_refresh = NULL;


static const char NAVIGATE_DIR_IMG[][40] = {
   "nav_turn_left",
   "nav_turn_right",
   "nav_keep_left",
   "nav_keep_right",
   "nav_continue",
   "nav_roundabout_e",
   "nav_roundabout_e",
   "nav_roundabout_l",
   "nav_roundabout_l",
   "nav_roundabout_s",
   "nav_roundabout_s",
   "nav_roundabout_r",
   "nav_roundabout_r",
   "nav_roundabout_u",
   "nav_roundabout_u",
   "nav_approaching",
   "nav_exit_left",
   "nav_exit_right"
};

static const char NAVIGATE_UK_DIR_IMG[][40] = {
   "nav_turn_left",
   "nav_turn_right",
   "nav_keep_left",
   "nav_keep_right",
   "nav_continue",
   "nav_roundabout_UK_e",
   "nav_roundabout_UK_e",
   "nav_roundabout_UK_l",
   "nav_roundabout_UK_l",
   "nav_roundabout_UK_s",
   "nav_roundabout_UK_s",
   "nav_roundabout_UK_r",
   "nav_roundabout_UK_r",
   "nav_roundabout_UK_u",
   "nav_roundabout_UK_u",
   "nav_approaching",
   "nav_exit_left",
   "nav_exit_right"

};
static RoadMapImage NavigateBarStretchedAddressImage;
static RoadMapImage NavigateBarAddressImage;
static RoadMapImage NavigateBarEtaImage;
static RoadMapImage NavigateBarDirectionImage;
static RoadMapImage NavigateBarDirectionTallImage;
static RoadMapImage NavigateBarArrow;

static int NavigateBarInitialized = 0;
static RoadMapGuiPoint NavigateBarLocation;

static enum NavigateInstr NavigateBarCurrentInstr = LAST_DIRECTION;
static char NavigateBarCurrentStreet[256] = {0};
static int  NavigateBarCurrentDistance = -1;
static int  NavigateBarCurrentExit = -1;
static int  NavigateBarEnabled = 0;
static int  gOffset;
static int  height;

static BOOL drag_start_on_bar;
static enum NavigateInstr NavigateBarNextInstr = LAST_DIRECTION;
static int  NavigateBarNextDistance = -1;
static int  NavigateBarNextExit = -1;

static void navigate_bar_draw_ETA (void);
static void navigate_bar_draw_time_to_destination () ;
static void navigate_bar_draw_distance_to_destination () ;
static void navigate_bar_draw_street (const char *street);
static void navigate_bar_draw_distance (int distance, int offset);
static void navigate_bar_draw_instruction (enum NavigateInstr instr, int offset);
static void navigate_bar_draw_exit (int exit, int offset);
static void navigate_bar_draw_additional_str (int offset);

BOOL navigate_bar_is_hidden(void){
   return (NavigateBarEnabled == 0);
}


static BOOL show_ETA_box(){
#if (defined(__SYMBIAN32__)  && !defined (TOUCH_SCREEN))
  if (is_screen_wide())
    return FALSE;
  else
    return TRUE;
#elif  (defined (_WIN32))
  if (roadmap_screen_is_hd_screen())
    return TRUE;
  else if (is_screen_wide())
    return FALSE;
  else
    return TRUE;
#else
  return TRUE;
#endif
}

static void LoadBarImages(){

   NavigateBarAddressImage = (RoadMapImage) roadmap_res_get
      (RES_BITMAP, RES_SKIN|RES_NOCACHE, "nav_bar_address");

   NavigateBarEtaImage = (RoadMapImage) roadmap_res_get
      (RES_BITMAP, RES_SKIN|RES_NOCACHE, "nav_bar_eta");

   NavigateBarDirectionImage = (RoadMapImage) roadmap_res_get
       (RES_BITMAP, RES_SKIN|RES_NOCACHE, "nav_bar_direction_small");

   NavigateBarDirectionTallImage = (RoadMapImage) roadmap_res_get
   (RES_BITMAP, RES_SKIN|RES_NOCACHE, "nav_bar_direction_tall");

#ifdef TOUCH_SCREEN
   NavigateBarArrow = (RoadMapImage) roadmap_res_get
       (RES_BITMAP, RES_SKIN|RES_NOCACHE, "nav_bar_arrow");
#endif

}

static int get_AddressBarHeight(void){
   return roadmap_canvas_image_height(NavigateBarAddressImage);
}

static int get_AddressBarWidth(void){
   return roadmap_canvas_image_width(NavigateBarAddressImage);
}

static int get_EtaBoxHeight(void){
   return roadmap_canvas_image_height(NavigateBarEtaImage);
}

static int get_EtaBoxWidth(void){
   return roadmap_canvas_image_width(NavigateBarEtaImage);
}

static int get_DirectionsBoxHeight(void){
   return roadmap_canvas_image_height(NavigateBarDirectionImage);
}

static int get_DirectionsBoxWidth(void){
   return roadmap_canvas_image_width(NavigateBarDirectionImage);
}

static int get_TallDirectionsBoxHeight(void){
   return roadmap_canvas_image_height(NavigateBarDirectionTallImage);
}

static int get_NavBarHeight(void){
   return (get_AddressBarHeight() + get_DirectionsBoxHeight());
}

RoadMapImage CreateBgImage(){
   RoadMapGuiPoint BarLocation;
   RoadMapImage image;
   int image_width;
   int i;
   int num_images;
   int width = roadmap_canvas_width ();
   int new_image_height;


   new_image_height = get_AddressBarHeight();

   image = roadmap_canvas_new_image (width, new_image_height);

   image_width = get_AddressBarWidth();

   num_images = width / image_width +1 ;

   BarLocation.y = 0;
   for (i = 0; i < num_images; i++){
       BarLocation.x = i * image_width;
       roadmap_canvas_copy_image
          (image, &BarLocation, NULL, NavigateBarAddressImage ,CANVAS_COPY_NORMAL);
    }

   return image;
}

static int navigate_bar_align_text (char *text, char **line1, char **line2,
                                    int size) {

   int width, ascent, descent;

   roadmap_canvas_get_text_extents
      (text, size, &width, &ascent, &descent, NULL);

   if (width >= 2 * NavigatePanel->street_width) return -1;

   if (width < NavigatePanel->street_width) {

      *line1 = text;
      return 1;

   } else {

      /* Check if we can place the text in two lines */

      char *text_line = text;
      char *text_end = text_line + strlen(text_line);
      char *p1 = text_line + (strlen(text_line) / 2);
      char *p2 = p1;

      while (p1 > text_line) {
         if (*p1 == ' ') {
            break;
         }
         p1 -= 1;
      }
      while (p2 < text_end) {
         if (*p2 == ' ') {
            break;
         }
         p2 += 1;
      }
      if (text_end - p1 > p2 - text_line) {
         p1 = p2;
      }
      if (p1 > text_line) {

         char saved = *p1;
         *p1 = 0;

         roadmap_canvas_get_text_extents
            (text_line, size, &width, &ascent, &descent, NULL);

         if (width < NavigatePanel->street_width) {

            roadmap_canvas_get_text_extents
               (text_line, size, &width, &ascent, &descent, NULL);

            if (width < NavigatePanel->street_width) {

               *line1 = text_line;
               *line2 = p1 + 1;
               return 2;
            }
         }

         *p1 = saved;
      }
   }

   return -1;
}

void navigate_bar_resize(void){

   RoadMapImage RoundaboutImage;
   NavigatePanel = NavigateBarDefaultPanels;

#ifndef OPENGL
   if ( NavigateBarStretchedAddressImage )  // Free the previous image
       roadmap_canvas_free_image( NavigateBarStretchedAddressImage );

   NavigateBarStretchedAddressImage = CreateBgImage();

#endif

   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = height - get_NavBarHeight() - roadmap_bar_bottom_height();

#ifdef TOUCH_SCREEN
   NavigatePanel->street_start = roadmap_canvas_width()  / 10;
   NavigatePanel->street_width = roadmap_canvas_width() - NavigatePanel->street_start - 5;
#else
   NavigatePanel->street_start = 5;
   NavigatePanel->street_width = roadmap_canvas_width() - NavigatePanel->street_start - 5;
#endif

   NavigatePanel->street_line1_pos = height - get_AddressBarHeight()/2 -  roadmap_bar_bottom_height() ;

   NavigatePanel->instruction_pos.x = 0;
   NavigatePanel->instruction_pos.y = height - get_AddressBarHeight() - get_DirectionsBoxHeight() - roadmap_bar_bottom_height() +2;

   RoundaboutImage = (RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN,"nav_roundabout_e");
   NavigatePanel->exit_pos.x = NavigatePanel->instruction_pos.x + roadmap_canvas_image_width(RoundaboutImage)/2 ;
   NavigatePanel->exit_pos.y = NavigatePanel->instruction_pos.y + roadmap_canvas_image_height(RoundaboutImage)/2 ;

   NavigatePanel->distance_value_pos.x = get_DirectionsBoxWidth() - 3;
   NavigatePanel->distance_value_pos.y = height - get_AddressBarHeight() -  roadmap_bar_bottom_height() - NAV_BAR_PIXELS( 5 );

   NavigatePanel->time_to_destination_pos.x = roadmap_canvas_width() - get_EtaBoxWidth() + 3;
   NavigatePanel->time_to_destination_pos.y = height - get_AddressBarHeight() -  roadmap_bar_bottom_height() - NAV_BAR_PIXELS( 3 );

   NavigatePanel->distance_to_destination_pos.x = roadmap_canvas_width() - get_EtaBoxWidth()*366/1000;
   NavigatePanel->distance_to_destination_pos.y = height - get_AddressBarHeight() -  roadmap_bar_bottom_height() - NAV_BAR_PIXELS( 3 );

#ifdef IPHONE
   NavigatePanel->time_to_destination_pos.y += 2;
   NavigatePanel->distance_to_destination_pos.y += 2;
#endif
}

static void navigate_bar_after_refresh (void) {

	int new_height;


   if (NavigateBarEnabled) {

	  new_height = roadmap_canvas_height();
	  if (new_height != height){
	    height = roadmap_canvas_height();
   		navigate_bar_resize();
   		roadmap_screen_refresh();
      }

	   roadmap_speedometer_set_offset(gOffset);
	   navigate_bar_draw ();
      navigate_bar_draw_street(NavigateBarCurrentStreet);
      if (NavigateBarNextInstr == LAST_DIRECTION){
         navigate_bar_draw_distance(NavigateBarCurrentDistance, 0);
         navigate_bar_draw_instruction(NavigateBarCurrentInstr, 0);
         navigate_bar_draw_exit(NavigateBarCurrentExit, 0);

      }
      else{
         int offset = -(get_TallDirectionsBoxHeight()- get_DirectionsBoxHeight());
         navigate_bar_draw_distance(NavigateBarCurrentDistance, 0);
         navigate_bar_draw_instruction(NavigateBarCurrentInstr, 0);
         navigate_bar_draw_exit(NavigateBarCurrentExit, 0);
         navigate_bar_draw_additional_str(offset-5);
         navigate_bar_draw_instruction(NavigateBarNextInstr, offset);
         navigate_bar_draw_exit(NavigateBarNextExit, offset);


      }
      if (show_ETA_box()){
        navigate_bar_draw_ETA();
        navigate_bar_draw_distance_to_destination();
        navigate_bar_draw_time_to_destination();
      }
   }
   else{
      roadmap_speedometer_set_offset(0);
   }

   if (navigate_prev_after_refresh) {
      (*navigate_prev_after_refresh) ();
   }
}

static int navigate_bar_short_click(RoadMapGuiPoint *point) {

   int min, max;
   if (NavigateBarInitialized != 1) return 0;

   if (!NavigateBarEnabled) return 0;

   min = roadmap_canvas_height() - get_AddressBarHeight() - get_EtaBoxHeight() - roadmap_bar_bottom_height();
   max = min + get_AddressBarHeight() + get_EtaBoxHeight() - 5;

   if ( ((point->y >= min) && (point->y <= max)) ||
         ( (point->x <= get_DirectionsBoxWidth()) && (point->y >= min + get_EtaBoxHeight() - get_DirectionsBoxHeight()) && (point->y <= max)   )) {
       navigate_menu();
       return 1;
   }

   return 0;

}

static int navigate_bar_drag_start(RoadMapGuiPoint *point) {

   int min, max;
   drag_start_on_bar = FALSE;
   if (NavigateBarInitialized != 1) return 0;

   if (!NavigateBarEnabled) return 0;
   min = roadmap_canvas_height() - get_AddressBarHeight() - roadmap_bar_bottom_height();
   max = min + get_AddressBarHeight();

   if ( (point->y >= min) && (point->y <= max) ) {
       drag_start_on_bar = TRUE;
       return 1;
   }

   return 0;

}

static int navigate_bar_drag_end(RoadMapGuiPoint *point) {

   if (drag_start_on_bar){
      drag_start_on_bar = FALSE;
      navigate_menu();
      return 1;
   }

   return 0;

}
void navigate_bar_initialize (void) {

   if ( roadmap_screen_get_background_run() )
   {
	   roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
	   return;
   }

   if (NavigateBarInitialized) return;

#ifndef IPHONE
   roadmap_config_declare
      ("preferences", &RMConfigNavBarDestDistanceFont, "14", NULL);
   roadmap_config_declare
      ("preferences", &RMConfigNavBarDestTimeFont, "14", NULL);
   roadmap_config_declare
      ("preferences", &RMConfigNavBarStreetNormalFont, "20", NULL);
   roadmap_config_declare
   ("preferences", &RMConfigNavBarStreetSmalllFont, "17", NULL);
#else
	roadmap_config_declare
	("preferences", &RMConfigNavBarDestDistanceFont, "13", NULL);
	roadmap_config_declare
	("preferences", &RMConfigNavBarDestTimeFont, "13", NULL);
	roadmap_config_declare
	("preferences", &RMConfigNavBarStreetNormalFont, "18", NULL);
   roadmap_config_declare
   ("preferences", &RMConfigNavBarStreetSmalllFont, "15", NULL);
#endif //IPHONE

   height = roadmap_canvas_height();

   LoadBarImages();

   navigate_bar_resize();

   NavigateBarInitialized = 1;
   navigate_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (navigate_bar_after_refresh);
#ifdef TOUCH_SCREEN
   roadmap_pointer_register_short_click(navigate_bar_short_click, POINTER_HIGH);

   roadmap_pointer_register_drag_start   (navigate_bar_drag_start, POINTER_HIGHEST);

   roadmap_pointer_register_drag_end(navigate_bar_drag_end, POINTER_HIGHEST);

#endif
}

void navigate_bar_set_instruction (enum NavigateInstr instr){
  NavigateBarCurrentInstr = instr;
}

void navigate_bar_set_next_instruction (enum NavigateInstr instr){
  NavigateBarNextInstr = instr;
}

const char *navigate_image(int inst){

   if (navigate_main_drive_on_left())
      return NAVIGATE_UK_DIR_IMG[(int) inst];
   else
      return NAVIGATE_DIR_IMG[(int) inst];
}

static void navigate_bar_draw_instruction (enum NavigateInstr instr, int offset) {


   RoadMapGuiPoint pos = NavigatePanel->instruction_pos;
   RoadMapImage direction_image;

   if (NavigateBarInitialized != 1) return;
   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }

   pos.y += offset;
   if (instr == LAST_DIRECTION)
      return;

   direction_image =(RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN, navigate_image((int) instr) );

   if ( direction_image )
   {
     roadmap_canvas_draw_image ( direction_image, &pos, 0, IMAGE_NORMAL);
   }

}

void navigate_bar_set_exit (int exit) {
  NavigateBarCurrentExit = exit;
}

void navigate_bar_set_next_exit (int exit) {
  NavigateBarNextExit = exit;
}
static void navigate_bar_draw_exit (int exit, int offset) {

	char text[3];
	RoadMapGuiPoint pos = NavigatePanel->exit_pos;
	if (exit < 0 || exit > 9) return;

   if (NavigateBarInitialized != 1) return;

	if ( roadmap_screen_get_background_run() )
   {
	   roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
	   return;
   }

	sprintf (text, "%d", exit);
	pos.y += offset;
	roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR_GREEN);

  	roadmap_canvas_draw_string_size(&pos, ROADMAP_CANVAS_CENTERMIDDLE, 18, text);

}


void navigate_bar_set_distance (int distance) {
  NavigateBarCurrentDistance = distance;
}

void navigate_bar_set_next_distance (int distance) {
  NavigateBarNextDistance = distance;
}

static void navigate_bar_draw_distance (int distance, int offset) {

   char str[100];
   char unit_str[20];
   int text_width ;
   int text_ascent;
   int text_descent;
   int  distance_far;
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   int font_size = 22;
   int font_size_units = 12;
   #ifdef IPHONE
   	font_size = 20;
   #endif

   if (NavigateBarInitialized != 1) return;
   if ( roadmap_screen_get_background_run() )
   {
	   roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
	   return;
   }

   if (distance < 0)
      return;

   if (NavigateBarCurrentInstr == LAST_DIRECTION)
      return;

   distance_far =
      roadmap_math_to_trip_distance(distance);

   if (distance_far > 0) {

      int tenths = roadmap_math_to_trip_distance_tenths(distance);
      if (distance_far < 10)
      	snprintf (str, sizeof(str), "%d.%d", distance_far, tenths % 10);
      else
      	snprintf (str, sizeof(str), "%d", distance_far);
      snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
   } else {
      if (!roadmap_math_is_metric()){
         int tenths = roadmap_math_to_trip_distance_tenths(distance);
         if (tenths > 0){
            snprintf (str, sizeof(str), "0.%d", tenths % 10);
            snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
         }
         else{
            snprintf (str, sizeof(str), "%d", (roadmap_math_distance_to_current(distance)/25)*25);
            snprintf (unit_str, sizeof(unit_str), "%s",
                     roadmap_lang_get(roadmap_math_distance_unit()));
         }
      }
      else{
         snprintf (str, sizeof(str), "%d", (roadmap_math_distance_to_current(distance)/10)*10);
         snprintf (unit_str, sizeof(unit_str), "%s",
                  roadmap_lang_get(roadmap_math_distance_unit()));
      }
   }

	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR_GREEN);

    roadmap_canvas_get_text_extents
              (unit_str, font_size_units, &text_width, &text_ascent, &text_descent, NULL);
  	if (ssd_widget_rtl(NULL)){

      position = NavigatePanel->distance_value_pos;
      position.y += offset;
      position.x = (position.x +text_width)/2;
      roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMMIDDLE, font_size, str);

      position.x = NAV_BAR_PIXELS( 5 );
      roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size_units, unit_str);
  	}
  	else{
     position = NavigatePanel->distance_value_pos;
     position.y += offset;
  	  position.x = (position.x -text_width)/2;
  	  roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMMIDDLE, font_size, str);

  	  position = NavigatePanel->distance_value_pos;
     position.y += offset;
  	  position.x -= NAV_BAR_PIXELS( 5 );
      roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMRIGHT, font_size_units, unit_str);

  	}
}

static void navigate_bar_draw_additional_str (int offset) {
   RoadMapGuiPoint position = {0, 0};
   int font_size = 14;

   if (NavigateBarInitialized != 1) return;
   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }

   position = NavigatePanel->distance_value_pos;
   position.y += offset;
   position.x = position.x/2;
   roadmap_canvas_create_pen ("nav_bar_pen");
   roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR_GREEN);
   roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMMIDDLE, font_size, roadmap_lang_get("and then"));

}


void navigate_bar_set_street (const char *street){
  if (NavigateBarInitialized != 1) return;

  if (street == NULL)
   return;

  strncpy_safe (NavigateBarCurrentStreet, street, sizeof(NavigateBarCurrentStreet));

}

static void navigate_bar_draw_street (const char *street) {

//#define NORMAL_SIZE 20
//#define SMALL_SIZE  16

   int width, ascent, descent;
   int size;
   char *line1 = NULL;
   char *line2 = NULL;
   char *text = NULL;
   int num_lines;
   int i;
   int font_size_normal = roadmap_config_get_integer( &RMConfigNavBarStreetNormalFont );
   int font_size_small = roadmap_config_get_integer( &RMConfigNavBarStreetSmalllFont );
   RoadMapPen nav_bar_pen;

   //street = "בלה יציאה: רח' ההלכה";
   if (NavigateBarInitialized != 1) return;
   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }

   if (street == NULL)
   	return;


   text = strdup(street);

   size = font_size_normal;

   num_lines = navigate_bar_align_text (text, &line1, &line2, size);

   if ((num_lines < 0)  || ((num_lines>1) && (NavigatePanel->street_line2_pos == -1))){
      /* Try again with a smaller font size */
	   text = strdup(street);
      size = font_size_small;
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }


   if ( roadmap_screen_is_hd_screen() && (num_lines > 1) )
   {
   	   size = ( 7 * font_size_normal ) / 10;
   }

   if ((num_lines < 0) || ((num_lines > 1) && (NavigatePanel->street_line2_pos == -1)))
      text = strdup(street);
   /* Cut some text until it fits */
   while ((num_lines < 0) || ((num_lines > 1) && (NavigatePanel->street_line2_pos == -1))) {

      char *end = text + strlen(text) - 1;
      while ((end > text) && (*end != ' ')) end--;
      if (end == text) {

         roadmap_log (ROADMAP_ERROR, "Can't align street in nav bar: %s",
                      street);

         free(text);
         return;
      }
      	*end = '\0';
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }


   for (i=0; i < num_lines; i++) {

      char *line;
      RoadMapGuiPoint position;

      position.x = NavigatePanel->street_start;
      position.y = NavigatePanel->street_line1_pos;

		if ((num_lines == 1) && (NavigatePanel->street_line2_pos != -1)){
			position.y = (NavigatePanel->street_line1_pos + NavigatePanel->street_line2_pos) / 2;
		}

      if (i ==0 ) {
         line = line1;
      } else {
         line = line2;
         position.y = NavigatePanel->street_line2_pos;
      }

      	roadmap_canvas_get_text_extents
         	(line, size, &width, &ascent, &descent, NULL);


      	if (ssd_widget_rtl(NULL))
      	   position.x = roadmap_canvas_width() - width - NavigatePanel->street_start;
      	else
      	   position.x = 5;

   		nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	   	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR);

  	   	roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_CENTERLEFT, size, line);
   }
   free(text);
}



int navigate_bar_get_height( void ) {
   int height = 0;
   if ( NavigateBarEnabled )
   {
      height = get_NavBarHeight();
   }
   return height;
}

void navigate_bar_set_mode (int mode) {
   if (NavigateBarEnabled == mode) return;
   NavigateBarEnabled = mode;
}

void navigate_bar_draw (void){

#ifdef OPENGL
   RoadMapGuiPoint AddressBottomRightPoint;
#endif
   RoadMapGuiPoint BarLocation;
   int arrow_offset = 0;

   if (NavigateBarInitialized != 1) return;

   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }


   BarLocation.y = roadmap_canvas_height() - get_AddressBarHeight() - roadmap_bar_bottom_height();
   BarLocation.x = 0;
#ifndef OPENGL
   roadmap_canvas_draw_image ( NavigateBarStretchedAddressImage, &BarLocation, 0,  IMAGE_NORMAL );
#else
   AddressBottomRightPoint.x = roadmap_canvas_width();
   AddressBottomRightPoint.y = BarLocation.y + get_AddressBarHeight();
   roadmap_canvas_draw_image_scaled( NavigateBarAddressImage, &BarLocation, &AddressBottomRightPoint, 0, IMAGE_NORMAL );
#endif


   if (NavigateBarCurrentInstr != LAST_DIRECTION){
         // Direction Box
         if (NavigateBarNextInstr == LAST_DIRECTION){
               BarLocation.y = NavigateBarLocation.y  + NAV_BAR_PIXELS( 2 );
               BarLocation.x = 0;
               roadmap_canvas_draw_image ( NavigateBarDirectionImage, &BarLocation, 0,  IMAGE_NORMAL );
         }
         else{
               BarLocation.y = NavigateBarLocation.y - (get_TallDirectionsBoxHeight() - get_DirectionsBoxHeight()) + NAV_BAR_PIXELS( 2 );
               BarLocation.x = 0;
               roadmap_canvas_draw_image ( NavigateBarDirectionTallImage, &BarLocation, 0,  IMAGE_NORMAL );
         }
   }

   //ETA box
   if (show_ETA_box()){
      BarLocation.y =  roadmap_canvas_height() - get_AddressBarHeight() - roadmap_bar_bottom_height() - get_EtaBoxHeight() + NAV_BAR_PIXELS( 2 );
      BarLocation.x = roadmap_canvas_width()-get_EtaBoxWidth();
      roadmap_canvas_draw_image ( NavigateBarEtaImage, &BarLocation, 0,  IMAGE_NORMAL );
   }

#ifdef TOUCH_SCREEN
   // Arrow
   arrow_offset = roadmap_canvas_width()/20 + roadmap_canvas_image_width(NavigateBarArrow)/2;

   BarLocation.y = roadmap_canvas_height() - roadmap_bar_bottom_height() -
                     (get_AddressBarHeight() -2 + roadmap_canvas_image_height(NavigateBarArrow))/2;
   BarLocation.x = roadmap_canvas_width() - arrow_offset;
   roadmap_canvas_draw_image ( NavigateBarArrow, &BarLocation, 0,  IMAGE_NORMAL );
#endif

   if (show_ETA_box())
      gOffset =  get_AddressBarHeight()+get_EtaBoxHeight() -2;
   else
      gOffset =  get_AddressBarHeight() - 2;

}


static void navigate_bar_draw_time_to_destination () {
   char text[256],text2[256], text3[256];
   char * pch;
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   static int text_width = -1;
   int text_ascent;
   int text_descent;
   int font_size = roadmap_config_get_integer( &RMConfigNavBarDestTimeFont );

   if (NavigateBarInitialized != 1) return;

   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }

   if (!navigate_main_ETA_enabled())
      return;

   if (!roadmap_message_format (text, sizeof(text), "%A|%T"))
  		return;

  	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");


   if (roadmap_skin_state() == 1)
      roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR_NIGHT);
   else
      roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR_DAY);
   pch = strtok (text," ");
   sprintf(text2, "%s", pch);

   pch = strtok (NULL," ");
   sprintf(text3, "%s",pch);


   position = NavigatePanel->time_to_destination_pos;
   position.x += 10;
   if (ssd_widget_rtl(NULL)){
     if (text_width == -1){
        roadmap_canvas_get_text_extents
              (text3, font_size-10, &text_width, &text_ascent, &text_descent, NULL);
        text_width += 3;
      }
     position.x += text_width+7;
   }
   roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size, text2);

   position = NavigatePanel->time_to_destination_pos;
   position.x += 10;
   if (!ssd_widget_rtl(NULL)){
       roadmap_canvas_get_text_extents
             (text2, font_size, &text_width, &text_ascent, &text_descent, NULL);
       text_width += NAV_BAR_PIXELS( 5 );
       position.x += text_width;
    }
    roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size-10, text3);

}

static void navigate_bar_draw_ETA (void) {
   char text[256],text2[256], text3[256];
   char * pch;
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   static int text_width = -1;
   int text_ascent;
   int text_descent;
   int eta_offset = NAV_BAR_PIXELS( 4 );

   int font_size = roadmap_config_get_integer( &RMConfigNavBarDestTimeFont );

   if (NavigateBarInitialized != 1) return;

   if ( roadmap_screen_get_background_run() )
   {
      roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
      return;
   }

   if (!navigate_main_ETA_enabled())
      return;

   if (!roadmap_message_format (text, sizeof(text), "%A|%@"))
      return;

   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

   if (roadmap_skin_state() == 1)
      roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR_NIGHT);
   else
      roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR_DAY);
   pch = strtok (text," ");
   sprintf(text2, "%s", pch);

   pch = strtok (NULL," ");
   sprintf(text3, "%s",pch);

   position = NavigatePanel->time_to_destination_pos;
   position.x += eta_offset;
   roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size-6, text2);

//   position = NavigatePanel->time_to_destination_pos;
   if (text_width == -1){
     roadmap_canvas_get_text_extents
           (text2, font_size-6, &text_width, &text_ascent, &text_descent, NULL);
     text_width += NAV_BAR_PIXELS( 5 );
   }

   position.x += (text_width);
   roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size, text3);
}

static void navigate_bar_draw_distance_to_destination () {

   char text[256];
   char distance[100];
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   char * pch;
   int width, ascent, descent;
   int font_size = roadmap_config_get_integer( &RMConfigNavBarDestDistanceFont );
   int distance_offset = NAV_BAR_PIXELS( 6 );

   if (NavigateBarInitialized != 1) return;
   if ( roadmap_screen_get_background_run() )
   {
	   roadmap_log( ROADMAP_INFO, "Calling draw functions in background is not permitted!");
	   return;
   }

   if (NavigatePanel->distance_to_destination_pos.x == -1) return;

	if (!roadmap_message_format (distance, sizeof(distance), "%D (%W)|%D"))
		return;

	pch = strtok (distance," ");
   sprintf(text, "%s", pch);
	roadmap_canvas_get_text_extents
         (text, font_size, &width, &ascent, &descent, NULL);



   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

   if (roadmap_skin_state() == 1)
      roadmap_canvas_set_foreground(NAV_BAR_DIST_TO_DESTINATION_COLOR_NIGHT);
   else
      roadmap_canvas_set_foreground(NAV_BAR_DIST_TO_DESTINATION_COLOR_DAY);

  	if (ssd_widget_rtl(NULL)){
  	   position = NavigatePanel->distance_to_destination_pos;
  	   position.x = roadmap_canvas_width() - distance_offset;
       roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMRIGHT, font_size, text);

  	   pch = strtok (NULL," ");
  	   sprintf(text, "%s",pch);

	   position = NavigatePanel->distance_to_destination_pos;
	   position.x = position.x - NAV_BAR_PIXELS( 3 );
	   roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size-10, text);

  	}
  	else{
      position = NavigatePanel->distance_to_destination_pos;
      roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMLEFT, font_size, text);

      pch = strtok (NULL," ");
      sprintf(text, "%s",pch);

      position = NavigatePanel->distance_to_destination_pos;
      position.x =roadmap_canvas_width() - distance_offset;
      roadmap_canvas_draw_string_size(&position, ROADMAP_CANVAS_BOTTOMRIGHT, font_size-10, text);

  	}

}


