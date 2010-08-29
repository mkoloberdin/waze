/* roadmap_message_ticker.c
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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
#include "roadmap_message_ticker.h"
#include "roadmap_device.h"
#include "roadmap_display.h"
#include "roadmap_device_events.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_widget.h"

static BOOL           gInitialized;
static RoadMapSize    gMiddleImageSize = {-1, -1};
static RoadMapGuiRect OpenIconRct;
static RoadMapPen     RoadMapTickerPen;
static BOOL           gTickerOn = FALSE;

char *g_title = NULL;
char *g_icon = NULL;
char *g_text = NULL;

////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_display() {

   RoadMapGuiPoint position;
   RoadMapGuiPoint sign_bottom, sign_top;
   RoadMapImage image;
   RoadMapImage icon = NULL;
   RoadMapImage x_image = NULL;
   int start,end, start_x, width;
   SsdWidget text;
   RoadMapGuiRect rect;
   int separator_position = 22;
   float factor = 1.0;


   width = roadmap_canvas_width();


    if (!gInitialized) return ;

    if (!gTickerOn) return;



   end = roadmap_canvas_width();
   start = 1;
   start_x = 1;
#ifdef OPENGL
   if (roadmap_screen_is_hd_screen())
      factor = 1.5;
   image = roadmap_res_get( RES_BITMAP, RES_SKIN, "TickerBackground" );
   if (image){
      sign_top.x = 0;
      sign_top.y =roadmap_bar_top_height();
      position.x = roadmap_canvas_image_width(image)/2;
      position.y =roadmap_canvas_image_height(image) / 2;
      sign_bottom.x = roadmap_canvas_width();
      sign_bottom.y = roadmap_bar_top_height() + roadmap_canvas_image_height(image)*factor ;
      roadmap_canvas_draw_image_stretch( image, &sign_top, &sign_bottom, &position, 0, IMAGE_NORMAL );
   }
   else{
      roadmap_log (ROADMAP_ERROR, "roadmap_message_ticker - cannot load TickerBackground image ");
   }
#else
   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "ticker_middle");
   if ( image )
   {
      while (start < end){
         position.x = start;
         position.y = roadmap_bar_top_height() ;
         roadmap_canvas_draw_image ( image, &position, 0, IMAGE_NORMAL);
         start += roadmap_canvas_image_width( image );
      }
   }
#endif

   position.x = 0;
   roadmap_canvas_select_pen (RoadMapTickerPen);
   position.x = roadmap_canvas_width()/2;
   position.y = roadmap_bar_top_height() +  separator_position -4;
   roadmap_canvas_draw_string_size (&position,
                                    ROADMAP_CANVAS_BOTTOMMIDDLE,
                                    16,g_title);

   if (g_icon){
      icon = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, g_icon);
      if ( icon ) {
         position.x = 10;
         position.y = roadmap_bar_top_height()+ gMiddleImageSize.height*factor/2 ;
         roadmap_canvas_draw_image ( icon, &position, 0, IMAGE_NORMAL);
      }
   }

#ifdef TOUCH_SCREEN
   x_image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "x_close");
   if ( image ) {
       position.x = roadmap_canvas_width() - roadmap_canvas_image_width(x_image) - 5;
       position.y = roadmap_bar_top_height() + gMiddleImageSize.height*factor -roadmap_canvas_image_height(x_image) -5;
       roadmap_canvas_draw_image ( x_image, &position, 0, IMAGE_NORMAL);
   }
#endif
   if (g_text){
      rect.minx = roadmap_canvas_image_width(image) + 15;
      rect.maxx = roadmap_canvas_width()-roadmap_canvas_image_width(x_image)-5;
      rect.miny = roadmap_bar_top_height() + separator_position + 5;
      rect.maxy = roadmap_bar_top_height() + gMiddleImageSize.height*factor -5;
      text = ssd_text_new("MessageTickerTxt",g_text, 14, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
      text->draw(text, &rect, 0);
   }

}

void roadmap_message_ticker_hide(void){
   gTickerOn = FALSE;
   roadmap_main_remove_periodic(roadmap_message_ticker_hide);
}


int roadmap_message_ticker_height(void){
   if (!gTickerOn)
      return 0;
   else
      return gMiddleImageSize.height;
}

void roadmap_message_ticker_show(const char *title, const char* text, const char* icon, int timer){
   gTickerOn = TRUE;

   if (g_title)
      free(g_title);
   g_title = strdup(title);


   if (g_icon)
      free(g_icon);
   g_icon = strdup(icon);


   if (g_text)
      free(g_text);
   g_text = strdup(text);

   if (timer > 0)
      roadmap_main_set_periodic(timer*1000, roadmap_message_ticker_hide);

}

////////////////////////////////////////////////////////////////////////////////
static int roadmap_message_ticker_key_pressed (RoadMapGuiPoint *point) {

   if (!gInitialized) return 0 ;

   if (!gTickerOn)
      return 0;

   if ((point->y >= (OpenIconRct.miny)) &&
       (point->y <= (OpenIconRct.maxy))) {
         roadmap_pointer_cancel_dragging();
       return 1;
   }

   return 0;
}

static int roadmap_message_ticker_short_click(RoadMapGuiPoint *point) {

   if (!gInitialized) return 0 ;

   if (!gTickerOn)
      return 0;

   if ((point->y >= (OpenIconRct.miny)) &&
       (point->y <= (OpenIconRct.maxy))) {
          gTickerOn = FALSE;
          return 1;
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
void roadmap_message_ticker_initialize(void){
   RoadMapImage image;
   gInitialized = FALSE;

#ifdef OPENGL
   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "TickerBackground");
   if ( image == NULL){
      roadmap_log (ROADMAP_ERROR, "roadmap_message_ticker - cannot load TickerBackground image ");
      return;
    }
#else
   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "ticker_middle");
   if ( image == NULL){
      roadmap_log (ROADMAP_ERROR, "roadmap_message_ticker - cannot load %s image ", "ticker_middle");
      return;
    }
#endif
   gMiddleImageSize.width = roadmap_canvas_image_width( image );
   gMiddleImageSize.height = roadmap_canvas_image_height( image );


   OpenIconRct.miny = roadmap_bar_top_height()+2;
   OpenIconRct.maxy = roadmap_bar_top_height() + 2 + gMiddleImageSize.height;

   RoadMapTickerPen = roadmap_canvas_create_pen ("Ticker_pen");
   roadmap_canvas_set_foreground ("#343434");

   roadmap_pointer_register_pressed
         (roadmap_message_ticker_key_pressed, POINTER_HIGH);
   roadmap_pointer_register_short_click(roadmap_message_ticker_short_click, POINTER_HIGH);

   gInitialized = TRUE;
}
