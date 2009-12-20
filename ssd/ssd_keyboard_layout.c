/* ssd_keyboard_layout_mgr.c
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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

#include <string.h>

#include "roadmap_canvas.h"
#include "roadmap_res.h"
#include "roadmap_utf8.h"
#include "roadmap_sound.h"
#include "roadmap_main.h"
#include "roadmap_pointer.h"
#include "ssd_dialog.h"

#include "ssd_keyboard_layout.h"

//#define  PROFILING
#ifdef   PROFILING
   #pragma message("    PROFILING KEYBOARD LAYOUT")
#endif   // PROFILING

#define MAX_BUTTONS_COUNT        (200)
#define MAX_DISTANCE_FOR_CLICK    70   // In pixels


static BOOL          initialized       = FALSE;
//static const char*   background_color  = "#dddddd";
static const char*   background_color  = "#222222";
//static const char*   font_color        = "#ffffff";
static const char*   font_color        = "#dddddd";
//static const char*   selected_font_clr = "#b0d504";
static const char*   selected_font_clr = "#eeeeee";
static const char*   buttons_color     = "#5555ff";
static const char*   black             = "#111111";
static button_info*  the_pressed_button= NULL;
static RoadMapPen    background_pen;
static RoadMapPen    font_pen;
static RoadMapPen    selected_font_pen;
static RoadMapPen    buttons_pen;
static RoadMapPen    black_pen;

static BOOL          gButtonHandled    = FALSE; // Indicates whether the button already handled
// Keyboard-key image info
typedef struct tag_image_info
{
   const char* name;
   void*       image;
   int         width;
   int         height;

}  image_info;

// Typical name of keyboard-key image: 'kb_key_20x30.png'
#define DECLARE_IMAGE(_width_,_height_)   \
   {"kb_key_" #_width_ "x" #_height_, NULL, _width_, _height_}

static image_info key_images[] =
{
   DECLARE_IMAGE(19,23),
   DECLARE_IMAGE(22,41),
   DECLARE_IMAGE(25,30),
   DECLARE_IMAGE(26,44),
   DECLARE_IMAGE(26,47),
   DECLARE_IMAGE(28,30),
   DECLARE_IMAGE(30,23),
   DECLARE_IMAGE(30,50),
   DECLARE_IMAGE(37,41),
   DECLARE_IMAGE(39,26),
   DECLARE_IMAGE(39,30),
   DECLARE_IMAGE(40,30),
   DECLARE_IMAGE(41,23),
   DECLARE_IMAGE(42,44),
   DECLARE_IMAGE(43,47),
   DECLARE_IMAGE(44,31),
   DECLARE_IMAGE(44,36),
   DECLARE_IMAGE(48,50),
   DECLARE_IMAGE(52,34),
   DECLARE_IMAGE(56,30),
   DECLARE_IMAGE(58,44),
   DECLARE_IMAGE(59,30),
   DECLARE_IMAGE(59,40),
   DECLARE_IMAGE(62,30),
   DECLARE_IMAGE(63,26),
   DECLARE_IMAGE(63,30),
   DECLARE_IMAGE(64,23),
   DECLARE_IMAGE(65,50),
   DECLARE_IMAGE(83,41),
   DECLARE_IMAGE(85,34),
   DECLARE_IMAGE(86,26),
   DECLARE_IMAGE(86,30),
   DECLARE_IMAGE(89,44),
   DECLARE_IMAGE(94,47),
   DECLARE_IMAGE(96,30),
   DECLARE_IMAGE(97,31),
   DECLARE_IMAGE(97,36),
   DECLARE_IMAGE(101,50),
   DECLARE_IMAGE(116,34),
   DECLARE_IMAGE(130,40),
   DECLARE_IMAGE(134,26),
   DECLARE_IMAGE(134,30),
   DECLARE_IMAGE(150,31),
   DECLARE_IMAGE(150,36),
   DECLARE_IMAGE(180,34),
   DECLARE_IMAGE(200,40)

};

// Instance context
typedef struct tag_kb_ctx
{
   buttons_layout_info**      layouts;
   int                        layouts_count;
   int                        current_layout;
   CB_OnKeyboardButtonPressed cbOnKey;
   CB_OnKeyboardCommand       cbOnCommand;
   CB_OnDrawBegin             cbOnDrawBegin;
   CB_OnDrawButton            cbOnDrawButton;
   CB_OnDrawEnd               cbOnDrawEnd;
   CB_OnApproveValueType      cbOnApproveValueType;
   void*                      context;

}  kbl_ctx;

typedef struct tag_image_draw_info
{
   void* image;
   int   x;
   int   y;

}  image_draw_info;


typedef struct tag_button_data
{
   char**            utf8chars;
   int               count;
   void*             image;
   void*             image_highlighted;
   int               image_width;
   int               image_height;
   image_draw_info   button_background;

}     button_data;
void  button_data_init( button_data* this)
{ memset( this, 0, sizeof(button_data));}

void  button_info_reset( button_info* this)
{
   if( NULL == this->item_data)
   {
      this->item_data = malloc( sizeof(button_data));
      button_data_init( this->item_data);
   }
}

void  buttons_layout_info_reset( buttons_layout_info* this)
{
   int i;

   for( i=0; i<this->buttons_count; i++)
      button_info_reset( this->buttons + i);
}

static void init()
{
   if( initialized)
      return;

   background_pen = roadmap_canvas_create_pen( "keyboard_layout_pen_background");
   roadmap_canvas_set_foreground( background_color);

   buttons_pen = roadmap_canvas_create_pen( "keyboard_layout_pen_buttons");
   roadmap_canvas_set_foreground( buttons_color);

   black_pen = roadmap_canvas_create_pen( "keyboard_layout_pen_black");
   roadmap_canvas_set_foreground( black);
   roadmap_canvas_set_thickness (3);

   font_pen = roadmap_canvas_create_pen( "keyboard_layout_pen_font");
   roadmap_canvas_set_foreground( font_color);
   roadmap_canvas_set_thickness (2);

   selected_font_pen = roadmap_canvas_create_pen( "keyboard_layout_selected_pen_font");
   roadmap_canvas_set_foreground( selected_font_clr);
   roadmap_canvas_set_thickness (4);

   initialized = TRUE;
}

static const char* get_button_value( const button_info* button, int value_index)
{
   static char s_special_keys[2];

   button_data* data = button->item_data;

   if( BUTTON_FLAG_BITMAP & button->flags)
   {
      if( !strcmp( button->name, "Backspace"))
      {
         s_special_keys[0] = BACKSPACE;
         s_special_keys[1] = '\0';

         return s_special_keys;
      }

      assert(0);
      return NULL;
   }

   if( (BUTTON_FLAG_COMMAND & button->flags) || !data)
   {
      assert(0);
      return NULL;
   }

   if( !data->utf8chars && button->values && button->values[0])
      data->utf8chars = utf8_to_char_array( button->values, &data->count);

   if( !data->count)
   {
      assert(0);
      return NULL;
   }

   if( value_index < data->count)
      return data->utf8chars[ value_index];

   return data->utf8chars[ data->count - 1];
}


static void draw_button( button_info* button, int value_index)
{
   RoadMapGuiPoint   tl ;
   RoadMapGuiPoint   tr ;
   RoadMapGuiPoint   br ;
   RoadMapGuiPoint   bl ;

   int               height;
   int               width;
   button_data*      data  = button->item_data;
   RoadMapGuiPoint   point;

   tl.x =  button->rect_corner_infos[top_left     ].cached_x;
   tl.y =  button->rect_corner_infos[top_left     ].cached_y;
   tr.x =  button->rect_corner_infos[bottom_right ].cached_x;
   tr.y =  button->rect_corner_infos[top_left     ].cached_y;
   br.x =  button->rect_corner_infos[bottom_right ].cached_x;
   br.y =  button->rect_corner_infos[bottom_right ].cached_y;
   bl.x =  button->rect_corner_infos[top_left     ].cached_x;
   bl.y =  button->rect_corner_infos[bottom_right ].cached_y;

   height = bl.y - tl.y;
   width =  tr.x - tl.x;
   point.x = data->button_background.x;
   point.y = data->button_background.y;

   roadmap_canvas_draw_image( data->button_background.image, &point, 0, IMAGE_NORMAL);

   if( BUTTON_FLAG_SELECTED & button->flags)
   {
      point.y  -= height;
      tl.y     -= height;
      br.y     -= height;
      roadmap_canvas_draw_image( data->button_background.image, &point, 0, IMAGE_NORMAL);
   }

   if( BUTTON_FLAG_BITMAP & button->flags)
   {
      RoadMapGuiPoint image_pos = tl;
      void*             image    = NULL;

      if( BUTTON_FLAG_SELECTED & button->flags)
         image = data->image_highlighted;
      else
         image = data->image;

      if( !image)
      {
         char  image_name[112];

         if( BUTTON_FLAG_SELECTED & button->flags)
            sprintf( image_name, "%s_highlighted", button->values);
         else
            sprintf( image_name, "%s", button->values);

         image = roadmap_res_get( RES_BITMAP, RES_SKIN|RES_LOCK, image_name);

         assert( image);

         data->image_width = roadmap_canvas_image_width ( image);
         data->image_height= roadmap_canvas_image_height( image);

         if( BUTTON_FLAG_SELECTED & button->flags)
            data->image_highlighted = image;
         else
            data->image = image;
      }

      image_pos.x += ((width - data->image_width ) / 2);
      image_pos.y += ((height- data->image_height) / 2);

      roadmap_canvas_draw_image( image, &image_pos, 0, IMAGE_NORMAL);
   }
   else
   {
      int               font_size= (height / 2) + 3;
      const char*       label    = NULL;
      RoadMapGuiPoint   text_pos = tl;
      int               text_width;
      int               text_height;
      int               text_ascent;
      int               text_descent;

#ifdef HI_RES_SCREEN
      font_size = (height / 3) + 3;
#endif
      if( BUTTON_FLAG_VALUE_IS_LABEL & button->flags)
         label = button->values;
      else if( BUTTON_FLAG_NAME_IS_LABEL & button->flags)
         label = button->name;
      else
         label = get_button_value( button, value_index);

      if( BUTTON_FLAG_SELECTED & button->flags)
      {
         roadmap_canvas_select_pen( selected_font_pen);
         font_size += 8;
      }
      else
         roadmap_canvas_select_pen( font_pen);

      roadmap_canvas_get_text_extents( label, font_size, &text_width, &text_ascent, &text_descent, NULL);
      text_height = 5 + (text_ascent + text_descent);

      text_pos.x += ((width - text_width ) / 2);
      text_pos.y += ((height- text_height) / 2);

      roadmap_canvas_draw_string_size( &text_pos,
                                       ROADMAP_CANVAS_TOPLEFT,
                                       font_size,
                                       label);
   }
}

// Fix button point-1 (X1,Y1), so button width/height will be the same like similiar buttons:
static void fix_button_dimentions(  buttons_layout_info* layout,
                                    button_info*         button,
                                    int*                 Widths,
                                    int*                 Heights)
{
   int   i;
   int   width = button->rect_corner_infos[1].cached_x - button->rect_corner_infos[0].cached_x;
   int   height= button->rect_corner_infos[1].cached_y - button->rect_corner_infos[0].cached_y;

   for( i=0; i<layout->buttons_count; i++)
   {
      int   w_diff_;
      int   h_diff_;
      int   w_diff;
      int   h_diff;
      BOOL  size_fixed  = FALSE;
      BOOL  keep_looking = FALSE;

      if( !Widths[i] || !Heights[i])
      {
         assert(!Widths[i]);
         assert(!Heights[i]);
#ifndef PROFILING
         roadmap_log( ROADMAP_DEBUG, "\tBUTTON - (%d, %d)", width, height);
#endif   // PROFILING

         Widths [i] = width;
         Heights[i] = height;
         break;
      }

      if( (Widths[i] == width) && (Heights[i] == height))
         break;

      w_diff_= Widths [i] - width;
      h_diff_= Heights[i] - height;
      w_diff = abs( w_diff_);
      h_diff = abs( h_diff_);

      // Width is 4 pixel away from other width(s)?
      if( w_diff)
      {
         if( w_diff < 4)
         {
            button->rect_corner_infos[1].cached_x += w_diff_;
            width += w_diff_;
#ifndef PROFILING
            roadmap_log(ROADMAP_DEBUG,
                        "ssd_keyboard_layout_mgr::fix_button_dimentions() - Fixed width for '%s'",
                        button->name);
#endif   // PROFILING
            size_fixed = TRUE;
         }
         else
            keep_looking = TRUE;
      }

      // Height is 4 pixel away from other height(s)?
      if( h_diff)
      {
         if( h_diff < 4)
         {
            button->rect_corner_infos[1].cached_y += h_diff_;
            height += h_diff_;

#ifndef PROFILING
            roadmap_log(ROADMAP_DEBUG,
                        "ssd_keyboard_layout_mgr::fix_button_dimentions() - Fixed height for '%s'",
                        button->name);
#endif   // PROFILING

            size_fixed = TRUE;
         }
         else
            keep_looking = TRUE;
      }

#ifndef PROFILING
      if( size_fixed)
         roadmap_log( ROADMAP_DEBUG, "\tBUTTON NEW SIZE - (%d, %d)", width, height);
#endif   // PROFILING

      if( !keep_looking)
         break;
   }
}

static void calc_layout( buttons_layout_info* layout, RoadMapGuiRect *rect, BOOL get_size_only)
{
   static int s_widths [MAX_BUTTONS_COUNT];
   static int s_heights[MAX_BUTTONS_COUNT];

   int i;
   int j;
   int rect_width = rect->maxx - rect->minx;
   int rect_height= rect->maxy - rect->miny;

   memset( s_widths, 0, sizeof(s_widths));
   memset( s_heights,0, sizeof(s_heights));

   // Cache was created for the following positions and sizes:
   layout->cache_ref.x     = rect->minx;
   layout->cache_ref.y     = rect->miny;
   layout->cache_ref.width = rect_width;
   layout->cache_ref.height= rect_height;

   // In practice, the following positions and sizes where used:
   layout->area_used.x     = rect->minx;
   layout->area_used.y     = rect->miny;
   layout->area_used.width = rect_width;
   layout->area_used.height= (int)(((double)rect_width / layout->image_relation)+0.5);

   // Verify height is not overflowing:
   if(layout->area_used.height > rect_height)
      layout->area_used.height = rect_height;

   if( get_size_only)
      return;

#ifndef PROFILING
   roadmap_log(ROADMAP_DEBUG,
               "KEYBORAD BUTTON SIZE - (WIDTH ,HEIGHT)");
#endif   // PROFILING

   for( i=0; i<layout->buttons_count; i++)
   {
      button_info*      button         = layout->buttons + i;
      image_draw_info*  bbdi           = &(((button_data*)button->item_data)->button_background);
      int               images_count   = sizeof(key_images)/sizeof(image_info);
      int               cur_w_distance = -1;
      int               cur_h_distance = -1;
      int               button_width;
      int               button_height;
      int               selected_image = -1;

      // Calculate buttons locations:
      for( j=0; j<rect_corners_count; j++)
      {
         rect_corner_info* corner= button->rect_corner_infos + j;
         double            rel_x = corner->relative_x;
         double            rel_y = corner->relative_y;

         corner->cached_x = layout->area_used.x + (int)(((double)layout->area_used.width * rel_x) + 0.5);
         corner->cached_y = layout->area_used.y + (int)(((double)layout->area_used.height* rel_y) + 0.5);
      }

      // Fix button point-1 (X1,Y1), so button width/height will be the same like similiar buttons:
      fix_button_dimentions( layout, button, s_widths, s_heights);

      button_width   = button->rect_corner_infos[1].cached_x - button->rect_corner_infos[0].cached_x;
      button_height  = button->rect_corner_infos[1].cached_y - button->rect_corner_infos[0].cached_y;

      // Select image for button:
      for( j=0; j<images_count; j++)
      {
         image_info* ii       = key_images + j;

         int   w_distance_    = ii->width - button_width;
         int   h_distance_    = ii->height- button_height;
         int   w_distance     = abs( w_distance_);
         int   h_distance     = abs( h_distance_);

         if( (-1 == cur_w_distance) || ((w_distance+h_distance) < (cur_w_distance+cur_h_distance)))
         {
            cur_w_distance = w_distance;
            cur_h_distance = h_distance;
            bbdi->image    = ii->image;
            bbdi->x        = button->rect_corner_infos[0].cached_x - (w_distance_/2);
            bbdi->y        = button->rect_corner_infos[0].cached_y - (h_distance_/2);
            selected_image = j;
         }
      }

      // Verify selected image was loaded:
      assert( -1 != selected_image);
      if( NULL == bbdi->image)
      {
         image_info* ii = key_images + selected_image;

         assert( NULL == ii->image);

         ii->image = roadmap_res_get(  RES_BITMAP,
                                       RES_SKIN|RES_LOCK,
                                       ii->name);

         if( NULL == ii->image)
         {
            roadmap_log( ROADMAP_ERROR, "ssd_keyboard_layout_mgr::calc_layout() - Image bitmap file missing (%s)", ii->name);
            assert(0);
         }

         bbdi->image = ii->image;
      }
#ifndef PROFILING
      // LOG: If selected image does not fit very good:
      if( (0 < cur_w_distance) || (0 < cur_h_distance))
      {
         roadmap_log(ROADMAP_DEBUG,
                     "ssd_keyboard_layout_mgr::calc_layout() - Button '%s' (w:%d, h:%d), image size differences( %d, %d)",
                     button->name,
                     button_width,
                     button_height,
                     cur_w_distance,
                     cur_h_distance);
      }
#endif   // PROFILING
   }
}
extern long roadmap_main_time_msec();

static void draw( SsdWidget this, RoadMapGuiRect *rect, int flags)
{
   int                  i;
   kbl_ctx*             ctx = (kbl_ctx*)this->context;
   buttons_layout_info* layout;

   // WINCE Profiling DEBUG:
   //    calc_layout()  -  ~10 ms
   //    draw()         -  ~80 ms
   //
   // Notes:
   //    calc_layout() happens only first time round for each layout.

   	if ( the_pressed_button && !roadmap_pointer_is_down()  )
	{
	   the_pressed_button->flags &= ~BUTTON_FLAG_SELECTED;
	   the_pressed_button = NULL;
	}

#ifdef PROFILING
   DWORD dw0, dw1, dw2;
   BOOL done_calc = FALSE;
   dw0 = GetTickCount();
#endif   // PROFILING
   if( (rect->maxx < 1) || (rect->maxy < 1))
      return;

   ctx->cbOnDrawBegin( ctx->context, ctx->current_layout);

   // Do this after above callback
   layout = ctx->layouts[ ctx->current_layout];

   // Do we have a valid cache?
   if(/*ayout->cache_ref.x       !=  rect->minx)               ||
      (layout->cache_ref.y       !=  rect->miny)               */
      (layout->cache_ref.width   != (rect->maxx - rect->minx)) ||
      (layout->cache_ref.height  != (rect->maxy - rect->miny)))
   {
#ifdef PROFILING
      done_calc = TRUE;
#endif   // PROFILING
      calc_layout( layout, rect, (SSD_GET_SIZE & flags));

#ifdef PROFILING
      dw1 = GetTickCount();
#endif   // PROFILING
   }

   rect->maxy = rect->miny + layout->area_used.height;

   if( SSD_GET_SIZE & flags)
   {
      ctx->cbOnDrawEnd( ctx->context, ctx->current_layout);

#ifdef PROFILING
      if( done_calc)
         roadmap_log(ROADMAP_DEBUG,
                     "PROFILING: GetSize time: %d ms", (dw1-dw0));
#endif   // PROFILING

      return;
   }

   roadmap_canvas_select_pen( background_pen);
   roadmap_canvas_erase_area( rect);

   for( i=0; i<layout->buttons_count; i++)
   {
      button_info* button = layout->buttons + i;

      ctx->cbOnDrawButton( ctx->context, ctx->current_layout, i);

      if( BUTTON_FLAG_VISIBLE & button->flags)
         draw_button( button, layout->value_index);
   }

   ctx->cbOnDrawEnd( ctx->context, ctx->current_layout);

#ifdef PROFILING
   if( done_calc)
   {
      dw2 = GetTickCount();
      roadmap_log(ROADMAP_DEBUG,
                  "PROFILING: Draw time: %d ms;   Calc time: %d ms",
                  (dw2-dw0), (dw1-dw0));
   }
   else
   {
      dw1 = GetTickCount();
      roadmap_log(ROADMAP_DEBUG,
                  "PROFILING: Draw time: %d ms", (dw1-dw0));
   }
#endif   // PROFILING
}

static BOOL on_key_pressed( SsdWidget widget, const char* utf8char, uint32_t flags)
{
   return FALSE;
}


static int on_button_down( kbl_ctx*                ctx,
                           buttons_layout_info*    layout,
                           const button_info*      button,
                           const RoadMapGuiPoint*  point)
{

   if(!(BUTTON_FLAG_ENABLED & button->flags) ||
      !(BUTTON_FLAG_VISIBLE & button->flags))
      return 0;

   if( BUTTON_FLAG_COMMAND & button->flags)
      ctx->cbOnCommand( ctx->context, button->name);
   else
   {
      int         flags = 0;
      const char* value = get_button_value( button, layout->value_index);

      if( !value)
         return 0;

      assert(*value);

      if( !value[1])
         flags |= KEYBOARD_ASCII;

      if ( roadmap_pointer_long_click_expired() )
        flags |= KEYBOARD_LONG_PRESS;

      ctx->cbOnKey( ctx->context, value, flags);
   }

   return 1;
}

void on_timer__unselect_button(void)
{
   roadmap_main_remove_periodic( on_timer__unselect_button);
   ssd_dialog_draw();
}

static int pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{
   int                  i;
   kbl_ctx*             ctx   = (kbl_ctx*)this->context;
   buttons_layout_info* layout= ctx->layouts[ ctx->current_layout];
   int                  center_x, center_y;
   int                  measure_min = -1, measure_current;

   if ( this->flags & SSD_WIDGET_HIDE )
	   return 0;

   // WINCE Profiling DEBUG: 30..100 ms
   the_pressed_button = NULL;

   gButtonHandled    = FALSE;

   for( i=0; i<layout->buttons_count; i++)
   {
      button_info* button = layout->buttons + i;
      if(!(BUTTON_FLAG_ENABLED & button->flags) ||
         !(BUTTON_FLAG_VISIBLE & button->flags))
        continue;

      // Find the matching according the L1 criteria
      // TODO:: Precalculate the centers in the layaout cration  *** AGA ***
      center_x = ( button->rect_corner_infos[top_left].cached_x + button->rect_corner_infos[bottom_right].cached_x ) >> 1;
      center_y = ( button->rect_corner_infos[top_left].cached_y + button->rect_corner_infos[bottom_right].cached_y ) >> 1;

      measure_current =  abs( center_x - point->x ) + abs( center_y - point->y );

      if ( ( measure_min < 0 ) || ( measure_min > measure_current ) )
      {
       measure_min = measure_current;
         the_pressed_button = button;
      }
   }

   if ( the_pressed_button && ( measure_min < MAX_DISTANCE_FOR_CLICK ) )
   {
      the_pressed_button->flags |= BUTTON_FLAG_SELECTED;
      return 1;
   }
   else
   {
      return 0;
   }
}

static int pointer_up( SsdWidget this, const RoadMapGuiPoint *point)
{
   static
   RoadMapSoundList     list  = NULL;
   kbl_ctx*             ctx   = (kbl_ctx*)this->context;
   buttons_layout_info* layout= ctx->layouts[ ctx->current_layout];
   int event_handled = 0;

   if ( this->flags & SSD_WIDGET_HIDE )
	   return 0;

   if ( gButtonHandled )
      return 1;

    gButtonHandled    = TRUE;

	if ( roadmap_keyboard_typing_locked( TRUE ) )
	{
		event_handled = 0;
	}
	else
	{
	   if( !the_pressed_button || !on_button_down( ctx, layout, the_pressed_button, point))
	   {
		   event_handled = 0;
	   }
	   else
	   {
		   event_handled = 1;
#ifndef ANDROID
		   if (!list) {
			  list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
			  roadmap_sound_list_add (list, "click");
			  roadmap_res_get (RES_SOUND, 0, "click");
		   }
		   roadmap_sound_play_list (list);
#endif
	   }
	}

	if (the_pressed_button){
	   the_pressed_button->flags &= ~BUTTON_FLAG_SELECTED;
	   the_pressed_button = NULL;
	}

   return event_handled;
}

static int on_long_click( SsdWidget this, const RoadMapGuiPoint *point)
{
   return pointer_up( this, point );
}

int ssd_keyboard_get_layout( SsdWidget kb)
{
   kbl_ctx* ctx = (kbl_ctx*)kb->context;

   return ctx->current_layout;
}

void ssd_keyboard_set_layout( SsdWidget   kb,
                              int         layout)
{
   kbl_ctx* ctx = (kbl_ctx*)kb->context;

   if( (0 <= layout) && (layout < ctx->layouts_count))
      ctx->current_layout = layout;
#ifdef _DEBUG
   else
      assert(0);
#endif // _DEBUG
}

void ssd_keyboard_increment_value_index( SsdWidget kb)
{
   kbl_ctx*             ctx         = (kbl_ctx*)kb->context;
   buttons_layout_info* layout      = ctx->layouts[ ctx->current_layout];
   int                  cur_index   = layout->value_index;

   do
   {
      layout->value_index++;

      if( cur_index == layout->value_index)
      {
         assert(0);
         return;
      }
      if( layout->max_values_count <= layout->value_index)
         layout->value_index = 0;

   }  while( !ctx->cbOnApproveValueType( ctx->context, layout->value_index));
}

int ssd_keyboard_get_value_index( SsdWidget kb)
{
   kbl_ctx*             ctx   = (kbl_ctx*)kb->context;
   buttons_layout_info* layout= ctx->layouts[ ctx->current_layout];

   return layout->value_index;
}

void ssd_keyboard_set_value_index( SsdWidget kb, int index)
{
		kbl_ctx*             ctx   = (kbl_ctx*)kb->context;
	   buttons_layout_info* layout= ctx->layouts[ ctx->current_layout];

	   if( (index < 0) || (layout->max_values_count <= index))
	   {
		  assert(0);
		  return;
	   }
	   if( ctx->cbOnApproveValueType( ctx->context, index))
		  layout->value_index = index;
}

BOOL ssd_keyboard_update_button_data(
               SsdWidget      kb,
               const char*    button_name,
               const char*    new_values,
               unsigned long  new_flags)
{
   kbl_ctx*             ctx   = (kbl_ctx*)kb->context;
   buttons_layout_info* layout= ctx->layouts[ ctx->current_layout];
   button_info*         button= NULL;
   button_data*         data  = NULL;
   int                  i;

   for( i=0; i<layout->buttons_count; i++)
      if( 0 == strcmp( layout->buttons[i].name, button_name))
      {
         button = layout->buttons + i;
         break;
      }

   if( !button)
      return FALSE;

   button->values = new_values;
   button->flags  = new_flags;

   data = button->item_data;
   // Free previous value(s)
   if( data->utf8chars && data->count)
   {
      utf8_free_char_array( data->utf8chars, data->count);

      data->utf8chars= NULL;
      data->count    = 0;
   }
   // Next time when a value will be requested, the allocation of the
   // utf-8 char array will happen automatically.

   return TRUE;
}

void ssd_keyboard_layout_reset( SsdWidget kb)
{
   kbl_ctx*ctx = kb->context;
   int i,j;

   for( i=0; i<ctx->layouts_count; i++)
   {
      buttons_layout_info* layout = ctx->layouts[i];

      for( j=0; j<layout->buttons_count; j++)
      {
         button_info* btn = layout->buttons + j;

         btn->flags &= ~BUTTON_FLAG_SELECTED;
      }
   }
}

void* ssd_keyboard_get_context( SsdWidget kb)
{
   kbl_ctx*ctx = kb->context;
   return  ctx->context;
}

SsdWidget ssd_keyboard_layout_create(
                        // SSD usage:
                        const char*                name,
                        int                        flags,
                        // Layout(s) info:
                        buttons_layout_info*       layouts[],
                        int                        layouts_count,
                        // This module:
                        CB_OnKeyboardButtonPressed cbOnKey,
                        CB_OnKeyboardCommand       cbOnCommand,
                        CB_OnDrawBegin             cbOnDrawBegin,
                        CB_OnDrawButton            cbOnDrawButton,
                        CB_OnDrawEnd               cbOnDrawEnd,
                        CB_OnApproveValueType      cbOnApproveValueType,
                        void*                      context)
{
   int         i;
   SsdWidget   w  = ssd_widget_new( name, on_key_pressed, flags);
   kbl_ctx*    ctx= malloc( sizeof(kbl_ctx));

   assert(name);
   assert(layouts);
   assert(layouts_count);
   assert(cbOnKey);
   assert(cbOnCommand);

   init();

   for( i=0; i<layouts_count; i++)
   {
      assert( layouts[i]->buttons_count < MAX_BUTTONS_COUNT);
      buttons_layout_info_reset( layouts[i]);
   }

   ctx->layouts               = layouts;
   ctx->layouts_count         = layouts_count;
   ctx->current_layout        = 0;
   ctx->cbOnKey               = cbOnKey;
   ctx->cbOnCommand           = cbOnCommand;
   ctx->context               = context;
   ctx->cbOnDrawBegin         = cbOnDrawBegin;
   ctx->cbOnDrawButton        = cbOnDrawButton;
   ctx->cbOnDrawEnd           = cbOnDrawEnd;
   ctx->cbOnApproveValueType  = cbOnApproveValueType;

   w->_typeid           = "keyboard_layout";
   w->flags             = flags;
   w->draw              = draw;
   w->pointer_down      = pointer_down;
   w->pointer_up        = pointer_up;
   w->long_click     = on_long_click;
   w->fg_color          = background_color;
   w->bg_color          = font_color;
   w->context           = ctx;
   w->size.width        = SSD_MAX_SIZE;
   w->size.height       = SSD_MAX_SIZE;

   return w;
}

