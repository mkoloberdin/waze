/* roadmap_ticker.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_lang.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_skin.h"
#include "roadmap_plugin.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_bar.h"
#include "roadmap_sound.h"
#include "roadmap_pointer.h"
#include "roadmap_ticker.h"
#include "roadmap_device.h"
#include "editor/editor_screen.h"
#include "editor/editor_points.h"
#include "Realtime/Realtime.h"
#include "roadmap_display.h"
#include "roadmap_device_events.h"

static RoadMapSize gMiddleImageSize = {-1, -1};
static RoadMapPen RoadMapTickerPen;
static BOOL gTickerOn = FALSE;
static BOOL gTickerHide = FALSE;
static BOOL gTickerSupressHide = FALSE;
static int gLastUpdateEvent = default_event;

static BOOL gInitialized;
static RoadMapGuiRect OpenIconRct;
//static RoadMapGuiRect CloseIconRct;

static RoadMapConfigDescriptor ShowTickerCfg =
                        ROADMAP_CONFIG_ITEM("User", "Show points ticker");

static int gTickerTopBarOffset = 0;

#define TICKER_TOP_BAR_DIVIDER_OFFSET 25
#define TICKER_TOP_BAR_TEXT_OFFSET 25
////////////////////////////////////////////////////////////////////////////////
static void show_close_icon(){
}

////////////////////////////////////////////////////////////////////////////////
int ticker_cfg_on (void) {
   return (roadmap_config_match(&ShowTickerCfg, "yes"));
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_display() {

   char text[256];
   char points[20];
   char rank[20];
   int text_width, ascent, descent;
   RoadMapGuiPoint position;
   RoadMapGuiPoint text_position;
   RoadMapImage image;
   int start,end, start_x;
	int iMyTotalPoints;
	int iMyRanking;
   int allign;
   int width;
   int text_offset_y = 25;
	int point_start_x = 190;
	int rank_start_x = 270;
   int new_pnts_start_x = 50;

   const char * point_text = NULL;

   if ( roadmap_screen_is_hd_screen() )
   {
		text_offset_y = 22;
		point_start_x = 200;
		rank_start_x = 280;
		new_pnts_start_x = 65;
   }

   width = roadmap_canvas_width();


    if (!gInitialized) return ;

    if (!ticker_cfg_on() && !gTickerSupressHide){
       gTickerOn = FALSE;
       return;
    }


    if (gTickerHide && (!roadmap_message_format (text, sizeof(text), "%X"))){
		gTickerOn = FALSE;
	 	return;
	}

    if (!roadmap_message_format (text, sizeof(text), "%X|%*")) {
    	if (gTickerSupressHide){
    		roadmap_message_set('*', "%d", 0);
    		roadmap_message_format (text, sizeof(text), "%*");
    	}
    	else{
    		show_close_icon();
    		gTickerOn = FALSE;
        	return;
    	}
    }

	gTickerOn = TRUE;
   roadmap_canvas_get_text_extents (text, 14, &text_width, &ascent, &descent, NULL);


   roadmap_canvas_select_pen (RoadMapTickerPen);

   end = roadmap_canvas_width();
   start = 1;
   start_x = 1;

   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, TICKER_MIDDLE_IMAGE);
   if ( image )
   {
	   while (start < end){
			position.x = start;
			position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset();
			roadmap_canvas_draw_image ( image, &position, 0, IMAGE_NORMAL);
			start += roadmap_canvas_image_width( image );
		}
   }
	position.x = 0;

	if (ssd_widget_rtl(NULL)){
	   allign = ROADMAP_CANVAS_RIGHT;
	   text_position.x = roadmap_canvas_width() -4;
	}
	else{
	   allign = ROADMAP_CANVAS_LEFT;
	   text_position.x = 4;
	}
   text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + 2;
	roadmap_canvas_draw_string_size (&text_position,
	                                 allign |ROADMAP_CANVAS_TOP,
	                                 14,roadmap_lang_get("Your Points (updated once a day)"));

   text_position.x = 4;
	text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + 2 +TICKER_TOP_BAR_TEXT_OFFSET;

	switch( gLastUpdateEvent ) {
    case default_event:
        point_text = roadmap_lang_get("New points");
        break;
    case report_event:
  	    point_text = roadmap_lang_get("Road report");
        break;
    case comment_event:
        point_text = roadmap_lang_get("Event comment");
        break;
    case confirm_event:
        point_text= roadmap_lang_get("Prompt response");
        break;
    case road_munching_event :
    	 point_text= roadmap_lang_get("Road munching");
    	break;
    case user_contribution_event :
    	point_text= roadmap_lang_get("Traffic detection");
    	break;
    case bonus_points :
      point_text= roadmap_lang_get("Bonus points");
      break;
	}


	roadmap_canvas_draw_string_size (&text_position,
	                                    ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
	                                   14,point_text);


	image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, TICKER_DIVIDER);

	if ( image )
	{
	   position.y = roadmap_bar_top_height() + TICKER_TOP_BAR_DIVIDER_OFFSET;
	   position.x = width /3 +4 + 30;
	   roadmap_canvas_draw_image ( image, &position, 0, IMAGE_NORMAL);

		position.y = roadmap_bar_top_height() + TICKER_TOP_BAR_DIVIDER_OFFSET;
	    position.x = 2*width/3+4 +15;
	    roadmap_canvas_draw_image ( image, &position, 0, IMAGE_NORMAL);
	}

	text_position.x =  (width/3) + (width/3)/4 + 30;
	text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + 2 + TICKER_TOP_BAR_TEXT_OFFSET;
  	roadmap_canvas_draw_string_size (&text_position,
       	    	                     ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
           	    	                 14,roadmap_lang_get("Total"));

   text_position.x = (width/3) + (width/3)/4+ 30;
	text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + text_offset_y +TICKER_TOP_BAR_TEXT_OFFSET;
	iMyTotalPoints = editor_points_get_total_points();
	if (iMyTotalPoints != -1){
		if (iMyTotalPoints < 10000){
			sprintf(points,"%d", iMyTotalPoints);
		}
		else{
			sprintf(points,"%dK", (int)iMyTotalPoints/1000);
		}
	}
	else{
		strcpy(points,"");
	}
  	roadmap_canvas_draw_string_size (&text_position,
       	    	                     ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
           	    	                 14,points);

	text_position.x = width - (width/3) + (width/3)/4+ 15;
	text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + 2 +TICKER_TOP_BAR_TEXT_OFFSET;
	iMyRanking = RealTime_GetMyRanking();
	if (iMyRanking == -1)
		strcpy(rank,"");
	else
		sprintf(rank, "%d", iMyRanking);
  	roadmap_canvas_draw_string_size (&text_position,
       	    	                     ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
           	    	                 14,roadmap_lang_get("Rank"));

   text_position.x = width - (width/3) + (width/3)/4+ 12;
   text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + text_offset_y +TICKER_TOP_BAR_TEXT_OFFSET;
  	roadmap_canvas_draw_string_size (&text_position,
       	    	                     ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
           	    	                 14,rank);

  	if (strcmp(text,"0")){
  	   text_position.x = new_pnts_start_x - start_x + 30;
  	   text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + text_offset_y +TICKER_TOP_BAR_TEXT_OFFSET;
  	   roadmap_canvas_draw_string_size (&text_position,
                     ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
                     14,"+");


  	   text_position.x = new_pnts_start_x - start_x + 40;
  	   text_position.y = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + text_offset_y +TICKER_TOP_BAR_TEXT_OFFSET;
  	   roadmap_canvas_draw_string_size (&text_position,
  	            ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
                     14,text);
   }
}

void roadmap_ticker_supress_hide(void){
   gTickerSupressHide = FALSE;
   if(!roadmap_screen_refresh())
      roadmap_screen_redraw();
   roadmap_main_remove_periodic (roadmap_ticker_supress_hide);
}

static int roadmap_ticker_short_click(RoadMapGuiPoint *point) {
   static RoadMapSoundList list;

   if (!gInitialized) return 0 ;

   if (!gTickerOn)
      return 0;

	if (!list) {
		list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
		roadmap_sound_list_add (list, "click");
		roadmap_res_get (RES_SOUND, 0, "click");
	}

    if (gTickerOn)
      if ((point->y >= (OpenIconRct.miny)) &&
        (point->y <= (OpenIconRct.maxy))) {

			roadmap_sound_play_list (list);
			roadmap_ticker_hide();
            return 1;
        }

   return 0;

}
////////////////////////////////////////////////////////////////////////////////
static int roadmap_ticker_key_pressed (RoadMapGuiPoint *point) {

   if (!gInitialized) return 0 ;

   if (!gTickerOn)
      return 0;

   if (gTickerOn)
      if (gTickerOn)
         if ((point->y >= (OpenIconRct.miny)) &&
           (point->y <= (OpenIconRct.maxy))) {
		   roadmap_pointer_cancel_dragging();
        	return 1;
        }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_initialize(void){
	gInitialized = FALSE;
	RoadMapImage image;
	if ( roadmap_screen_is_hd_screen() )
	{
		gTickerTopBarOffset = -5;
	}

	roadmap_config_declare_enumeration ("user", &ShowTickerCfg, NULL, "yes", "no", NULL);

   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, TICKER_MIDDLE_IMAGE);
   if ( image == NULL){
		roadmap_log (ROADMAP_ERROR, "roadmap_ticker - cannot load %s image ", TICKER_MIDDLE_IMAGE);
		return;
    }

   gMiddleImageSize.width = roadmap_canvas_image_width( image );
   gMiddleImageSize.height = roadmap_canvas_image_height( image );


	OpenIconRct.miny = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset();
	OpenIconRct.maxy = roadmap_bar_top_height() + roadmap_ticker_top_bar_offset() + gMiddleImageSize.height;

	RoadMapTickerPen = roadmap_canvas_create_pen ("Ticker_pen");
   roadmap_canvas_set_foreground (TICKER_FOREGROUND);
   show_close_icon();
   roadmap_pointer_register_pressed
         (roadmap_ticker_key_pressed, POINTER_HIGH);
   roadmap_pointer_register_short_click(roadmap_ticker_short_click, POINTER_HIGH);

   gInitialized = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
int roadmap_ticker_height(){
    if (!gTickerOn || ( gMiddleImageSize.height < 0 ) )
    	 return 0;
    else
    	return gMiddleImageSize.height + roadmap_ticker_top_bar_offset();
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_hide(void){
	gTickerHide = TRUE;
	if (!roadmap_screen_refresh())
		roadmap_screen_redraw();

}

////////////////////////////////////////////////////////////////////////////////
int roadmap_ticker_state(void){
   if (gTickerOn)
      return 1;
   else
      return 0;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_ticker_show(void){
   char text[256];
   gTickerHide = FALSE;

   if (! roadmap_message_format (text, sizeof(text), "%*|%X")) {
      roadmap_message_set('*', "%d", 0);
      roadmap_ticker_set_last_event(default_event);
   }

   if (gTickerSupressHide)
      roadmap_main_remove_periodic (roadmap_ticker_supress_hide);

   roadmap_main_set_periodic (15000, roadmap_ticker_supress_hide);

   gTickerSupressHide = TRUE;
   if (!roadmap_screen_refresh())
     roadmap_screen_redraw();
}

void roadmap_ticker_set_last_event(int event){
	gLastUpdateEvent = event;
}


int roadmap_ticker_top_bar_offset(void)
{
	return gTickerTopBarOffset;

}

void roadmap_ticker_set_suppress_hide(BOOL myBool){
	gTickerSupressHide = myBool;
}
