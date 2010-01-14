/* ssd_bitmap.c - Icon widget
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
#include <string.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"

#include "ssd_dialog.h"
#include "ssd_icon_defs.h"
#include "ssd_icon.h"

#define  MAX_ICONS_COUNT   (10)

static   int   s_width  = -1;
static   int   s_height = -1;

BOOL ssd_icon_wimage_is_valid( ssd_icon_wimage* this)
{
   return   this
               &&
            this->left     && this->middle      && this->right
               && 
            this->left[0]  && this->middle[0]   && this->right[0];
}

// Wide images:
typedef struct tag_wimage_info
{
   RoadMapImage   left;
   RoadMapImage   middle;
   RoadMapImage   right;
   
}     wimage_info;
BOOL  wimage_info_load( wimage_info*      this,
                        ssd_icon_wimage*  filenames);


typedef struct tag_wimageset_info
{
   wimage_info normal;
   wimage_info in_focus;

}     wimageset_info;
BOOL  wimageset_info_load( wimageset_info*      this, 
                           ssd_icon_wimage_set* set);

typedef struct tag_imageset_info
{
   RoadMapImage   normal;
   RoadMapImage   in_focus;

}     imageset_info;
BOOL  imageset_info_load(  imageset_info*       this,
                           ssd_icon_image_set*  set);

typedef struct tag_icon_ctx
{
   union
   {
      wimageset_info wimages[MAX_ICONS_COUNT];
       imageset_info  images[MAX_ICONS_COUNT];
   };
   
   int   image_count;
   BOOL  wide_image;
   int   requested_width;
   int   width;
   int   height;   
   int   state;  
   BOOL  loaded;
   CB_OnWidgetKeyPressed
         on_unhandled_key_pressed;


}     icon_ctx, *icon_ctx_ptr;
void  icon_ctx_init( icon_ctx_ptr this)
{ memset( this, 0, sizeof(icon_ctx));}

BOOL wimage_info_load(  wimage_info*       this,
                        ssd_icon_wimage*   filenames)
{
   if( !ssd_icon_wimage_is_valid( filenames))
   {
      assert(0);
      return FALSE;
   }

   LOAD_WIDE_ICON(left)
   LOAD_WIDE_ICON(middle)
   LOAD_WIDE_ICON(right)
   
   return TRUE;
}

BOOL  wimageset_info_load( wimageset_info*      this, 
                           ssd_icon_wimage_set* set)
{
   if( !wimage_info_load( &(this->normal), set->normal))
      return FALSE;

   if( ssd_icon_wimage_is_valid( set->in_focus))
   {
      if( !wimage_info_load( &(this->in_focus), set->in_focus))
         return FALSE;
   }
   else
      memcpy( &(this->in_focus), &(this->normal), sizeof(wimage_info));

   return TRUE;
}

RoadMapImage load_image( const char* filenames)
{
   RoadMapImage image = NULL;

   if( !filenames || !filenames[0])
   {
      assert(0);
      return NULL;
   }

   // Else
   image = roadmap_res_get( RES_BITMAP, RES_SKIN|RES_LOCK, filenames);
   if( !image)
   {
      assert(0);
      return NULL;
   }
   
   if( -1 == s_width)
      s_width = roadmap_canvas_image_width( image);
   else
   {
      if( s_width != roadmap_canvas_image_width( image))
      {
         assert(0);
         return NULL;
      }
   }
   if( -1 == s_height)
      s_height = roadmap_canvas_image_height( image);
   else
   {
      if( s_height != roadmap_canvas_image_height( image))
      {
         assert(0);
         return NULL;
      }
   }

   return image;
}

BOOL  imageset_info_load(  imageset_info*       this,
                           ssd_icon_image_set*  set)
{
   if( !set->normal || !set->normal[0])
   {
      assert(0);
      return FALSE;
   }
   
   this->normal = load_image( set->normal);
   if( !this->normal)
   {
      assert(0);
      return FALSE;
   }

   if( set->in_focus && set->in_focus[0])
   {
      this->in_focus = load_image( set->in_focus);
      if( !this->in_focus)
      {
         assert(0);
         return FALSE;
      }
   }
   else
      this->in_focus = this->normal;

   return TRUE;
}

static void draw_wimage(icon_ctx_ptr      ctx,
                        wimage_info*      ii,
                        RoadMapGuiPoint*  point)
{
   int            offset               = point->x;
   int            left_bitmap_width    = roadmap_canvas_image_width(ii->left);
   int            middle_bitmap_width  = roadmap_canvas_image_width(ii->middle);
   int            right_bitmap_width   = roadmap_canvas_image_width(ii->right);
   int            middle_paint_width;
   int            i;

   // Minimum image size is (left + right):
   assert( (left_bitmap_width+right_bitmap_width) <= ctx->requested_width);
   
   // If middle-bitmap is wider then right-bitmap then during the 
   // painting the middle-bitmap can be painted to the right of
   // the right bitmap:
   assert( middle_bitmap_width < right_bitmap_width);
   
   // Paint left
   roadmap_canvas_draw_image( ii->left, point, 0, IMAGE_NORMAL);
   point->x += left_bitmap_width;
   
   // Paint middle
   middle_paint_width = ctx->requested_width - (left_bitmap_width+right_bitmap_width);
   for( i=0; i<middle_paint_width; i+=middle_bitmap_width)
   {
      roadmap_canvas_draw_image( ii->middle, point, 0, IMAGE_NORMAL);
      point->x += middle_bitmap_width;
   }
   
   // Paint right
   point->x = offset + ctx->requested_width - right_bitmap_width;
   roadmap_canvas_draw_image( ii->right, point, 0, IMAGE_NORMAL);      
}

static void draw( SsdWidget this, RoadMapGuiRect *rect, int flags)
{
   icon_ctx_ptr      ctx = (icon_ctx_ptr)this->data;
   RoadMapGuiPoint   point;
   
   assert( ctx->loaded);
   
   if( ctx->width)
      rect->maxx = rect->minx + ctx->width -1;
   else
      rect->maxx = rect->minx;
      
   if( ctx->height)
      rect->maxy = rect->miny + ctx->height-1;
   
   if( SSD_GET_SIZE & flags)
      return;
  
   point.x = rect->minx;
   point.y = rect->miny;

   if( !ctx->wide_image)
   {
      RoadMapImage image;
      
      assert( 0 == ctx->requested_width);
      
      if( this->in_focus)
         image = ctx->images[ctx->state].in_focus;
      else
         image = ctx->images[ctx->state].normal;
      
      roadmap_canvas_draw_image( image, &point, 0, IMAGE_NORMAL);
   }
   else
   {
      wimage_info* ii;
      
      assert( 0 != ctx->requested_width);
      
      if( this->in_focus)
         ii = &(ctx->wimages[ctx->state].in_focus);
      else
         ii = &(ctx->wimages[ctx->state].normal);
   
      draw_wimage( ctx, ii, &point);
   }
}

static BOOL on_key_pressed( SsdWidget this, const char* utf8char, uint32_t flags)
{ 
   icon_ctx_ptr ctx = (icon_ctx_ptr)this->data;

   if( KEY_IS_ENTER && this->callback)
   {
      this->callback( this, "short_click");
      return TRUE;
   }
   
   if( ctx->on_unhandled_key_pressed)
      return ctx->on_unhandled_key_pressed( this, utf8char, flags);

   return FALSE;
}

static int on_pointer_down( SsdWidget this, const RoadMapGuiPoint* point)
{
   BOOL done_something = FALSE;
   
   if( this->tab_stop)
   {
      done_something = TRUE;
      
      if( !this->in_focus)
         ssd_dialog_set_focus( this);
   }
      
   if( this->short_click && this->short_click( this, point))
      done_something = TRUE;
   
   return done_something;
}

SsdWidget ssd_icon_create( const char* name,
                           BOOL        wide_image,
                           int         flags)
{
   icon_ctx_ptr   ctx= (icon_ctx_ptr)malloc(sizeof(icon_ctx));
   SsdWidget      w  = ssd_widget_new( name, on_key_pressed, flags);
   
   icon_ctx_init( ctx);
   ctx->wide_image= wide_image;

   w->_typeid     = "Icon";
   w->flags       = flags;
   w->draw        = draw;
   w->pointer_down= on_pointer_down;
   w->data        = ctx;
   
   return w;
}

// Simple image(s)
void ssd_icon_set_images(  SsdWidget            this,
                           ssd_icon_image_set   images[],// One image per state
                           int                  count)   // Images count
{
   int            i;
   icon_ctx_ptr   ctx = (icon_ctx_ptr)this->data;
   
   if( ctx->wide_image || ctx->loaded ||
       (count < 0) || (MAX_ICONS_COUNT < count))
   {
      assert(0);
      return;
   }
   
   s_width  = -1;
   s_height = -1;
   
   for( i=0; i<count; i++)
   {
      imageset_info* ii = ctx->images + i;
      if( !imageset_info_load( ii, (images + i)))
      {
         assert(0);
         return;
      }
      
      ctx->image_count++;
   }

   ctx->width  = s_width;
   ctx->height = s_height;
   ctx->loaded = TRUE;
}                           

// Wide image(s)
void ssd_icon_set_wimages( SsdWidget            this,
                           ssd_icon_wimage_set  images[],   // One image per state
                           int                  count)      // Images count
{
   int            i;
   icon_ctx_ptr   ctx = (icon_ctx_ptr)this->data;
   
   if( !ctx->wide_image || ctx->loaded ||
       (count < 0) || (MAX_ICONS_COUNT < count))
   {
      assert(0);
      return;
   }
   
   s_width  = -1;
   s_height = -1;
   
   for( i=0; i<count; i++)
   {
      wimageset_info* ii = ctx->wimages + i;
      if( !wimageset_info_load( ii, (images + i)))
      {
         assert(0);
         return;
      }
      
      ctx->image_count++;
   }

   ctx->height = s_height;
   ctx->loaded = TRUE;
}                           

// Operation on 'wide-image'
void ssd_icon_set_width( SsdWidget this, int width)
{
   icon_ctx_ptr ctx = (icon_ctx_ptr)this->data;
   assert( ctx->wide_image);
   ctx->requested_width = width;
   ctx->width           = width;
}

// Operation on all image types:
int ssd_icon_get_state( SsdWidget this)
{
   icon_ctx_ptr ctx = (icon_ctx_ptr)this->data;
   return ctx->state;
}

int ssd_icon_set_state( SsdWidget this, int state)
{
   icon_ctx_ptr   ctx      = (icon_ctx_ptr)this->data;
   int            old_state= ctx->state;
   
   ctx->state = state;
   return old_state;
}

void ssd_icon_set_unhandled_key_press( SsdWidget   this,
                                       CB_OnWidgetKeyPressed
                                                   on_unhandled_key_pressed)
{
   icon_ctx_ptr ctx = (icon_ctx_ptr)this->data;
   ctx->on_unhandled_key_pressed = on_unhandled_key_pressed;
}   
