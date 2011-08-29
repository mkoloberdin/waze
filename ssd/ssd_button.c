/* ssd_button.c - Bitmap button widget
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
 *   See ssd_button.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_sound.h"
#include "roadmap_screen.h"
#include "roadmap_main.h"

#include "roadmap_keyboard.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"

/* Buttons states */
#define BUTTON_STATE_NORMAL   0
#define BUTTON_STATE_SELECTED 1
#define BUTTON_STATE_DISABLED 2
#define MAX_STATES            3

#define BUTTON_CLICK_OFFSETS_DEFAULT	{0, -15, 0, 15 }

#define SSD_BUTTON_BMP_NAME_MAXLEN		64

struct ssd_button_data {
   int state;
   char bitmap_names[MAX_STATES][SSD_BUTTON_BMP_NAME_MAXLEN];
   RoadMapImage bitmap_images[MAX_STATES];	/* These should be initialized only in case of cutom no-cache images */
};


void get_state (SsdWidget widget, int *state, RoadMapImage *image, int *image_state) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   int i;

   if ((widget->in_focus ))
      *state = IMAGE_SELECTED;
   else
      *state = data->state;

   if (data->state == BUTTON_STATE_DISABLED)
      *state = BUTTON_STATE_DISABLED;

   for (i=*state; i>=0; i--)
   {
      if ( data->bitmap_images[i] )
      {
    	  *image = data->bitmap_images[i];
         break;
      }
      else if ( ( (*image) = roadmap_res_get ( RES_BITMAP, RES_SKIN, data->bitmap_names[i] ) ) )
      {
    	  break;
      }
   }
   *image_state = i;
}

void ssd_button_set_selected (SsdWidget widget) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   data->state = BUTTON_STATE_SELECTED;
}
static void set_bitmap_name( struct ssd_button_data *data, int state, const char* name )
{
	if ( strlen( name ) <= SSD_BUTTON_BMP_NAME_MAXLEN )
	{
	   strcpy( data->bitmap_names[state], name );
	}
	else
	{
	   roadmap_log( ROADMAP_ERROR, "Failed setting bitmap name %s. Cannot set bitmap names larger than %d. ",
								   name, SSD_BUTTON_BMP_NAME_MAXLEN );
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

static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage image = NULL;
   RoadMapGuiPoint point;
   int image_state;
   int state;

   point.x = rect->minx;
   point.y = rect->miny;

   get_state(widget, &state, &image, &image_state);

   if ((flags & SSD_GET_SIZE))
   {
   	  if ( data->bitmap_images[state] != NULL )
     		image = data->bitmap_images[state];
   	  else
   		    image = roadmap_res_get ( RES_BITMAP, RES_SKIN, data->bitmap_names[state] );

      if ( !image )
    	    image = data->bitmap_images[0];

      if ( !image )
    	    image = roadmap_res_get ( RES_BITMAP, RES_SKIN, data->bitmap_names[0] );

      if (!image )
      {
         widget->size.height = widget->size.width = 0;
         return;
      }

      rect->maxx = rect->minx + roadmap_canvas_image_width(image);
      rect->maxy = rect->miny + roadmap_canvas_image_height(image);

   	return;
   }


   if (!image)
   {
      roadmap_log (ROADMAP_ERROR, "SSD - Can't get image for button widget: %s",
      widget->name);
      return;
   }

   switch (state)
   {
	   case BUTTON_STATE_NORMAL:
			roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
			break;
      case BUTTON_STATE_DISABLED:
         roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
         break;
	   case BUTTON_STATE_SELECTED:
			if (image_state == state)
			{
				roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
			}
			else
			{
				roadmap_canvas_draw_image (image, &point, 0, IMAGE_SELECTED);
			}
      break;
   }
}


static SsdWidget delayed_widget;

static void button_callback (void) {

	struct ssd_button_data *data = (struct ssd_button_data *) delayed_widget->data;

   roadmap_main_remove_periodic (button_callback);
	delayed_widget->in_focus = FALSE;
   data->state = BUTTON_STATE_NORMAL;
   if (delayed_widget->callback) {
      (*delayed_widget->callback) (delayed_widget, SSD_BUTTON_SHORT_CLICK);
   }

	roadmap_screen_redraw ();
}

static int ssd_button_short_click (SsdWidget widget,
                                   const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   static RoadMapSoundList list;

   if (data->state == BUTTON_STATE_DISABLED)
      return 1;

   widget->force_click = FALSE;

#ifdef PLAY_CLICK
	if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click");
      roadmap_res_get (RES_SOUND, 0, "click");
   }

   roadmap_sound_play_list (list);
#endif //IPHONE

#ifdef TOUCH_SCREEN
	if (widget->callback == NULL){
		data->state = BUTTON_STATE_NORMAL;
		roadmap_screen_redraw ();
		return 0;
	}
	else{
		delayed_widget = widget;
		widget->in_focus = TRUE;
		data->state = BUTTON_STATE_SELECTED;
  	    roadmap_main_set_periodic (100, button_callback);
  	    return 1;
	}
#else
   if (widget->callback) {
      (*widget->callback) ( widget, SSD_BUTTON_SHORT_CLICK );
      return 1;
   }

	data->state = BUTTON_STATE_NORMAL;
	roadmap_screen_redraw ();
#endif

   return 0;
}


static int ssd_button_long_click (SsdWidget widget,
                                  const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   static RoadMapSoundList list;

   widget->force_click = FALSE;

#ifndef IPHONE
  if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click_long");
      roadmap_res_get (RES_SOUND, 0, "click_long");
   }

   roadmap_sound_play_list (list);
#endif //IPHONE

   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_LONG_CLICK);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 1;
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage bmp = NULL;
	int max_width = 0;
	int max_height = 0;
	int is_different = 0;
   int i;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) widget->value = strdup(value);
   else widget->value = "";

   if (widget->flags & SSD_VAR_SIZE) {
   	widget->size.height = -1;
   	widget->size.width  = -1;
      return 0;
   }

	for (i=0; i<MAX_STATES; i++)
	{
      if (data->bitmap_images[i])
    	  bmp = data->bitmap_images[i];
      if ( !bmp )
    	  bmp = roadmap_res_get ( RES_BITMAP, RES_SKIN, data->bitmap_names[i] );

      if (!bmp) continue;

      if ( !max_width || !max_height )
      {
         max_width  = roadmap_canvas_image_width(bmp);
         max_height = roadmap_canvas_image_height(bmp);
      }
      else {
         int val = roadmap_canvas_image_width(bmp);
         if (max_width != val) {
            is_different = 1;
            if (val > max_width) max_width = val;
         }

         val = roadmap_canvas_image_height(bmp);
         if (max_height != val) {
            is_different = 1;
            if (val > max_height) max_height = val;
         }
      }
   }

	widget->size.height = max_height;
  	widget->size.width  = max_width;

   return 0;
}

static BOOL ssd_button_on_key_pressed (SsdWidget button, const char* utf8char, uint32_t flags)
{
   struct ssd_button_data *data = (struct ssd_button_data *) button->data;
   if (data->state == BUTTON_STATE_DISABLED)
      return FALSE;

   if( KEY_IS_ENTER )
   {
      button->callback(button, SSD_BUTTON_SHORT_CLICK);

      return TRUE;
   }

   return FALSE;
}


SsdWidget ssd_button_new (const char *name, const char *value,
                          const char **bitmap_names, int num_bitmaps,
                          int flags, SsdCallback callback) {

   int i;
   SsdWidget w;
   SsdClickOffsets btn_offsets = BUTTON_CLICK_OFFSETS_DEFAULT;
//   RoadMapImage image;
   struct ssd_button_data *data =
      (struct ssd_button_data *)calloc (1, sizeof(*data));


   w = ssd_widget_new (name, ssd_button_on_key_pressed, flags);

   w->_typeid = "Button";

   w->draw  = draw;
   w->release = release;
   w->flags = flags;
   w->data = data;

   data->state  = BUTTON_STATE_NORMAL;
   // TODO :: Load the bitmaps here
   for (i=0; i<num_bitmaps; i++)
   {
	   set_bitmap_name( data, i, bitmap_names[i] );

	   data->bitmap_images[i] = NULL;
   }


   w->callback = callback;

   set_value (w, value);

   ssd_widget_set_pointer_force_click( w );
   ssd_widget_set_click_offsets( w, &btn_offsets );

   w->short_click  = ssd_button_short_click;
   w->long_click   = ssd_button_long_click;
   w->set_value    = set_value;

   return w;
}


int ssd_button_change_icon( SsdWidget widget, const char **bitmap_names, int num_bitmaps )
{
    RoadMapImage bitmap_images[MAX_STATES];
    int i, res;

    assert( num_bitmaps <= MAX_STATES);

    for ( i=0; i<num_bitmaps; i++ )
	{
		   bitmap_images[i] = NULL;
	}

	res = ssd_button_change_images( widget, bitmap_images, bitmap_names, num_bitmaps );

	return res;
}

int ssd_button_change_images( SsdWidget widget, RoadMapImage* bitmap_images,
											const char **bitmap_names, int num_bitmaps )
{
	int i;
	RoadMapImage bmp;
	struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
	for (i=0; i<num_bitmaps; i++)
	{
		set_bitmap_name( data, i, bitmap_names[i] );
		data->bitmap_images[i] = bitmap_images[i];
	}

	bmp = data->bitmap_images[0];
    if ( !bmp )
    {
    	bmp = roadmap_res_get ( RES_BITMAP, RES_SKIN, bitmap_names[0] );
    }

    if ( !bmp )
    {
      widget->size.height = widget->size.width = 0;
      return -1;
    }

    widget->size.height = roadmap_canvas_image_height( bmp );
    widget->size.width  = roadmap_canvas_image_width( bmp );

    return 0;
}



void ssd_button_change_text(SsdWidget this, const char * newText){
	SsdWidget text;
	text = ssd_widget_get(this, "label");
	if (text == NULL)
		return;
	ssd_text_set_text(text, newText);
}


const char *ssd_button_get_name(SsdWidget widget){
	struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
	return data->bitmap_names[0];
}

SsdWidget ssd_button_label (const char *name, const char *label,
                            int flags, SsdCallback callback) {

   const char *button_icon[]   = {"button_up", "button_down", "button_disabled"};
   int y_offset = ADJ_SCALE( -2 );

   SsdWidget text;
   SsdWidget button = ssd_button_new (name, "", button_icon, 3,
                                      flags, callback);

   #if defined (_WIN32) && !defined (OPENGL)
      text = ssd_text_new ("label", label, 12, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
   #else
      text = ssd_text_new ("label", label, 14, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER) ;
   #endif

   ssd_widget_set_offset( text, 0, y_offset );
   ssd_widget_set_color(text, "#ffffff", "#ffffff");
   ssd_widget_add (button,text);

   return button;
}


SsdWidget ssd_button_label_custom (const char *name, const char *label,
                            int flags, SsdCallback callback, const char **button_icon, int num_bitmaps, const char *txt_color, const char *txt_focus_color, int txt_size) {

   SsdWidget text;
   SsdWidget button = ssd_button_new (name, "", button_icon, num_bitmaps,
                                      flags, callback);
   int y_offset = ADJ_SCALE( -2 );

   text = ssd_text_new ("label", label, txt_size, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
   ssd_widget_set_color(text, txt_color, txt_focus_color);
   ssd_widget_set_offset( text, 0, y_offset );
   ssd_widget_add (button,text);

   return button;
}

void ssd_button_disable(SsdWidget widget){
   struct ssd_button_data *data ;
   SsdWidget text;

   if (!widget)
      return;
   data = (struct ssd_button_data *) widget->data;
   if (data)
      data->state = BUTTON_STATE_DISABLED;

   text = ssd_widget_get(widget, "label");
   if (text)
      ssd_widget_set_color(text, "#c0c0c0", "#c0c0c0");
}

void ssd_button_enable(SsdWidget widget){
   struct ssd_button_data *data ;
   SsdWidget text;

   if (!widget)
      return;
   data = (struct ssd_button_data *) widget->data;
   if (data)
      data->state = BUTTON_STATE_NORMAL;
   text = ssd_widget_get(widget, "label");
   if (text)
      ssd_widget_set_color(text, "#ffffff", "#ffffff");


}
