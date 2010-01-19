/* ssd_bitmap.c - Bitmap widget
 *
 * LICENSE:
 *
 *   Copyright 2006 PazO
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
 */

#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_screen.h"

#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_bitmap.h"
#include "ssd_dialog.h"

#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_res.h"

#define SSD_BITMAP_NAME_MAXLEN		64
typedef struct tag_bitmap_info
{
   char    bitmap_name[SSD_BITMAP_NAME_MAXLEN];
   RoadMapImage   bitmap;
   int            width;
   int            height;

}  bitmap_info, *bitmap_info_ptr;

void bitmap_info_init( bitmap_info_ptr this)
{
   this->bitmap      = NULL;
   this->width       = -1;
   this->height      = -1;
}


static void draw (SsdWidget this, RoadMapGuiRect *rect, int flags)
{
   bitmap_info_ptr bi = (bitmap_info_ptr)this->data;
   RoadMapGuiPoint point;
   RoadMapImage image2draw;

   point.x = rect->minx;
   point.y = rect->miny;

   if ( bi->bitmap )
   {
	   image2draw = bi->bitmap;
   }
   else
   {
	   image2draw = (RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN, bi->bitmap_name );
   }

   if( -1 == bi->width )
   {
      bi->width = roadmap_canvas_image_width ( image2draw );
      bi->height= roadmap_canvas_image_height( image2draw );
   }

   if ( ( flags & SSD_GET_SIZE ) )
   {
      return;
   }

   if ( image2draw )
   {
	   roadmap_canvas_draw_image( image2draw, &point, 0, IMAGE_NORMAL);
   }
   else
   {
	   roadmap_log( ROADMAP_ERROR, "Cannot draw bitmap image. Widget: %s, Bitmap: %s", this->name, bi->bitmap_name );
   }

}


static void set_bitmap_name( bitmap_info_ptr bi, const char* name )
{
	if ( strlen(name) <= SSD_BITMAP_NAME_MAXLEN )
	{
	   strcpy( bi->bitmap_name, name );
	}
	else
	{
	   roadmap_log( ROADMAP_ERROR, "Failed setting bitmap name %s. Cannot set bitmap names larger than %d. ",
								   name, SSD_BITMAP_NAME_MAXLEN );
	}
}

static void release( SsdWidget widget )
{
	if ( widget->data )
	{
		free( widget->data );
		widget->data = NULL;
	}
}

static int set_value( SsdWidget widget, const char *value )
{
   bitmap_info_ptr   bi = (bitmap_info_ptr) malloc(sizeof(bitmap_info) );
   RoadMapImage image;
   bitmap_info_init( bi);

   if ( widget->data )
	   free( widget->data );

   set_bitmap_name( bi, value );

   bi->bitmap     = NULL;
   widget->data        = bi;

   image = (RoadMapImage) roadmap_res_get( RES_BITMAP, RES_SKIN, bi->bitmap_name );
   if ( image != NULL )
   {
	  widget->size.height = roadmap_canvas_image_height( image );
	  widget->size.width  = roadmap_canvas_image_width( image );
   }
   return 1;
}


// Bitmap from file
SsdWidget ssd_bitmap_new(  const char *name,
                           const char *bitmap,
                           int         flags)
{
   SsdWidget         w  = ssd_widget_new(name, NULL, flags);


   w->_typeid     = "Bitmap";
   w->draw        = draw;
   w->release 	  = release;
   w->flags       = flags;
   w->set_value   = set_value;

   set_value( w, bitmap );

   return w;
}


// Bitmap from image
SsdWidget ssd_bitmap_image_new(  const char *name,
                                 RoadMapImage image,
                                 int         flags)
{
   bitmap_info_ptr   bi = (bitmap_info_ptr)malloc(sizeof(bitmap_info));
   SsdWidget         w  = ssd_widget_new(name, NULL, flags);

   bitmap_info_init( bi);

   set_bitmap_name( bi, name );

   bi->bitmap     = image;
   w->_typeid     = "Bitmap";
   w->draw        = draw;
   w->flags       = flags;
   w->data        = bi;
   w->set_value   = set_value;
   w->release 	  = release;
   return w;
}

void ssd_bitmap_image_update(SsdWidget widget, RoadMapImage image )
{
   bitmap_info_ptr   bi =  (bitmap_info_ptr) widget->data;
   bitmap_info_init( bi );

   set_bitmap_name( bi, "" );
   bi->bitmap = image;
}
void ssd_bitmap_update(SsdWidget widget, const char *bitmap){
   bitmap_info_ptr   bi = (bitmap_info_ptr)widget->data;

   set_bitmap_name( bi, bitmap );
   bi->bitmap     = NULL;
}
static void close_splash (void) {

	roadmap_main_remove_periodic (close_splash);
	ssd_dialog_hide ("splash_image", dec_ok);
	roadmap_screen_redraw ();
}

void ssd_bitmap_splash(const char *bitmap, int seconds){

   SsdWidget dialog;

   dialog = ssd_dialog_new ("splash_image", "", NULL,
         SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

   ssd_widget_set_color (dialog, "#000000", "#ff0000000");

   ssd_widget_add(dialog,
   				  ssd_bitmap_new("splash_image", bitmap, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER));
   ssd_dialog_activate ("splash_image", NULL);

   roadmap_main_set_periodic (seconds * 1000, close_splash);
}

const char *ssd_bitmap_get_name(SsdWidget widget){
	bitmap_info_ptr   bi = (bitmap_info_ptr)widget->data;
	return bi->bitmap_name;
}
