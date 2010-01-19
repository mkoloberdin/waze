/* ssd_widget.c - small screen devices Widgets (designed for touchscreens)
 * (requires agg)
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
 *   See ssd_widget.h
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_lang.h"
#include "ssd_widget.h"
#include "roadmap_bar.h"
#include "roadmap_pointer.h"
#include "roadmap_softkeys.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32

#define SSD_WIDGET_SEP 2
#define MAX_WIDGETS_PER_ROW 100
/*
 * This threshold defines the maximum movement (in inches!) defined as
 * abs( press.x - release.x) + abs( press.y - release.y )
 * Pay attention that this threshold has to be adjusted according to the device ppi
 * in order to match the physical movement
 * For example threshold 60 on device of 180 ppi (G1) corresponds to actual movement of 1/3 inch
 *
 */
#define FORCE_CLICK_MOVEMENT_THRESHOLD  (60)
#define ABS_POINTS_DISTANCE( a, b ) ( abs( (a).x - (b).x ) + abs( (a).y - (b).y ) )

static RoadMapGuiPoint PressedPointerPoint = {-1, -1};

// Get child by ID (name)
// Can return child child...
SsdWidget ssd_widget_get (SsdWidget child, const char *name) {

   if (!name) return child;

   while (child != NULL) {
      if (0 == strcmp (child->name, name)) {
         return child;
      }

      if (child->children != NULL) {
         SsdWidget w = ssd_widget_get (child->children, name);
         if (w) return w;
      }

      child = child->next;
   }

   return NULL;
}


static void calc_pack_size (SsdWidget w_cur, RoadMapGuiRect *rect,
                            SsdSize *size) {

   int         width       = rect->maxx - rect->minx + 1;
   int         height      = rect->maxy - rect->miny + 1;
   int         cur_width   = 0;
   int         max_height  = 0;
   int         max_width   = 0;
   int         index_in_row= 0;
   SsdWidget   w_last_drawn= NULL;
   SsdWidget   w_prev      = NULL;

   while( w_cur)
   {
      SsdSize max_size;
      SsdSize size = {0, 0};

      // If hiding widget - hide also all children:
      if (w_cur->flags & SSD_WIDGET_HIDE) {
         w_cur = w_cur->next;
         continue;
      }

      if (0 == index_in_row) {
         width  = rect->maxx - rect->minx + 1;

         // Padd with space?
         if (w_cur->flags & SSD_WIDGET_SPACE) {
            rect->miny  += 2;
            width       -= 2;
            cur_width   += 2;
         }
      }

      if (!(w_cur->flags & SSD_START_NEW_ROW)) {
         max_size.width = width;
         max_size.height= rect->maxy - rect->miny + 1;
         ssd_widget_get_size (w_cur, &size, &max_size);
      }

      if ((index_in_row == MAX_WIDGETS_PER_ROW) ||
         ((index_in_row > 0) &&
                     ((cur_width + size.width) > width)) ||
                     (w_cur->flags & SSD_START_NEW_ROW)) {

         if (cur_width > max_width) max_width = cur_width;

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }

         index_in_row = 0;
         cur_width = 0;
         w_last_drawn = w_cur;
         w_prev = NULL;

         rect->miny += max_height;
         max_height = 0;

         if (w_cur->flags & SSD_WIDGET_SPACE) {
            rect->miny += 2;
            width -= 2;
            cur_width += 2;
         }

         max_size.width = width;
         max_size.height = rect->maxy - rect->miny + 1;
         ssd_widget_get_size (w_cur, &size, &max_size);
      }

      index_in_row++;
      cur_width += size.width;
      if (w_prev && w_prev->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (size.height > max_height) max_height = size.height;

      if (w_cur->flags & SSD_END_ROW) {
         if (cur_width > max_width) max_width = cur_width;

         index_in_row = 0;
         cur_width = 0;
         rect->miny += max_height;
         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }
         max_height = 0;
         w_last_drawn = w_cur;
         w_prev = NULL;
      }

      w_prev = w_cur;
      w_cur = w_cur->next;
   }

   if (index_in_row) {
      if (cur_width > max_width) max_width = cur_width;

      index_in_row = 0;
      cur_width = 0;
      rect->miny += max_height;
      if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

         rect->miny += SSD_WIDGET_SEP;
      }
   }

   size->width = max_width;
   size->height = height - (rect->maxy - rect->miny);
}


static void ssd_widget_draw_one (SsdWidget w, int x, int y, int height) {

   RoadMapGuiRect rect;
   SsdSize size;

   ssd_widget_get_size (w, &size, NULL);

   if (w->flags & SSD_ALIGN_VCENTER) {
      y += (height - size.height) / 2;
   }

   w->position.x = x;
   w->position.y = y;


   if (size.width && size.height) {
      rect.minx = x;
      rect.miny = y;
      rect.maxx = x + size.width - 1;
      rect.maxy = y + size.height - 1;

#if 0
      if (!w->parent) printf("****** start draw ******\n");
      printf("draw - %s:%s x=%d-%d y=%d-%d\n", w->_typeid, w->name, rect.minx, rect.maxx, rect.miny, rect.maxy);
#endif

      w->draw(w, &rect, 0);

      if (w->children) ssd_widget_draw (w->children, &rect, w->flags);
   }
}


static int ssd_widget_draw_row (SsdWidget *w, int count,
                                int width, int height,
                                int x, int y) {
   int row_height = 0;
   int cur_x;
   int total_width;
   int space;
   int vcenter = 0;
   int bottom = 0;
   int i;
   int rtl = ssd_widget_rtl(w[0]->parent);
   SsdWidget prev_widget;
   int orig_width = width;
   int orig_x = x;
   if (y > roadmap_canvas_height()){
      return 0;
   }
   if (w[0]->flags & SSD_ALIGN_LTR) rtl = 0;

   if (rtl) cur_x = x;
   else cur_x = x + width;

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (size.height > row_height) row_height = size.height;
      if (w[i]->flags & SSD_ALIGN_VCENTER) vcenter = 1;
      if (w[i]->flags & SSD_ALIGN_BOTTOM) bottom = 1;
   }

   if (bottom) {
      y += (height - row_height);

   } else if (vcenter) {
      y += (height - row_height) / 2;
   }

   for (i=count-1; i>=0; i--) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (w[i]->flags & SSD_ALIGN_RIGHT) {
         cur_x += w[i]->offset_x;
         if (rtl) {
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x += SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
            cur_x += size.width;
         } else {
            cur_x -= size.width;
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x -= SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         }
         //width -= size.width;
         if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
            //width -= SSD_WIDGET_SEP;
         }
         count--;
         if (i != count) {
            memmove (&w[i], &w[i+1], sizeof(w[0]) * count - i);
         }
      }
   }

   if (rtl) cur_x = x + width;
   else cur_x = x;

   prev_widget = NULL;

   while (count > 0) {
      SsdSize size;

      if (w[0]->flags & SSD_ALIGN_CENTER) {
         break;
      }

      ssd_widget_get_size (w[0], &size, NULL);

      if (rtl) {
         cur_x -= size.width;
         cur_x -= w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x -= SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
      } else {
         cur_x += w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x += SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
         cur_x += size.width;
      }

      width -= size.width;
      if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
         width -= SSD_WIDGET_SEP;
      }
      prev_widget = *w;
      w++;
      count--;
   }

   if (count == 0) return row_height;

   /* align center */

   total_width = 0;
   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      total_width += size.width;
   }

   space = (width - total_width) / (count + 1);

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);
      // beginning of patch to take care of title which wasn't aligned to center - dan
	  if (!strcmp(w[i]->name,"title_text")){
   		total_width=0;
	  	total_width += size.width;
	  	cur_x = orig_x;
	  	if (rtl)
	  		cur_x +=orig_width;
	  	space = (orig_width - total_width) / (count + 1);
      }
      // end of patch
      if (rtl) {
         cur_x -= space;
         cur_x -= size.width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         cur_x += size.width;
      }
   }

   return row_height;
}


static int ssd_widget_draw_grid_row (SsdWidget *w, int count,
                                     int width,
                                     int avg_width,
                                     int height,
                                     int x, int y) {
   int cur_x;
   int space;
   int i;
   int rtl = ssd_widget_rtl (w[0]->parent);

   if (w[0]->flags & SSD_ALIGN_LTR) rtl = 0;

   /* align center */
   space = (width - avg_width*count) / (count + 1);

   if (rtl) cur_x = x + width;
   else cur_x = x;

   for (i=0; i<count; i++) {

      if (rtl) {
         cur_x -= space;
         cur_x -= avg_width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
         cur_x += avg_width;
      }
   }

   return height;
}


static void ssd_widget_draw_grid (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget widgets[MAX_WIDGETS_PER_ROW];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   SsdSize max_size;
   int cur_y = rect->miny;
   int max_height = 0;
   int avg_width = 0;
   int count = 0;
   int width_per_row;
   int cur_width = 0;
   int rows;
   int num_widgets;
   int space;
   int i;

   max_size.width = width;
   max_size.height = height;

   while (w != NULL) {
      SsdSize size;
      ssd_widget_get_size (w, &size, &max_size);

      if (size.height > max_height) max_height = size.height;
      avg_width += size.width;

      widgets[count] = w;
      count++;

      if (count == sizeof(widgets) / sizeof(SsdWidget)) {
         roadmap_log (ROADMAP_FATAL, "Too many widgets in grid!");
      }

      w = w->next;
   }

   max_height += SSD_WIDGET_SEP;
   avg_width = avg_width / count + 1;

   rows = height / max_height;

   while ((rows > 1) && ((count * avg_width / rows) < (width * 3 / 5))) rows--;

   if ((rows == 1) && (count > 2)) rows++;

   width_per_row = count * avg_width / rows;
   num_widgets = 0;

   space = (height - max_height*rows) / (rows + 1);

   for (i=0; i < count; i++) {
      SsdSize size;
      ssd_widget_get_size (widgets[i], &size, NULL);

      cur_width += avg_width;

      if (size.width > avg_width*1.5) {
         cur_width += avg_width;
      }

      num_widgets++;

      if ((cur_width >= width_per_row) || (i == (count-1))) {
         cur_y += space;

          ssd_widget_draw_grid_row
               (&widgets[i-num_widgets+1], num_widgets,
                width, avg_width, max_height, rect->minx, cur_y);
         cur_y += max_height;
         cur_width = 0;
         num_widgets = 0;
      }
   }
}


static void ssd_widget_draw_pack (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget row[MAX_WIDGETS_PER_ROW];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   int minx   = rect->minx;
   int cur_y  = rect->miny;
   int cur_width = 0;
   int count = 0;
   SsdWidget w_last_drawn = NULL;
   SsdWidget w_prev = NULL;

   while (w != NULL) {

      SsdSize size =   {0,0};
      SsdSize max_size;

      if (w->flags & SSD_WIDGET_HIDE) {
         w = w->next;
         continue;
      }

      if (!count) {
         width  = rect->maxx - rect->minx + 1;
         height = rect->maxy - cur_y + 1;
         minx   = rect->minx;

         if (w->flags & SSD_WIDGET_SPACE) {
            width  -= 4;
            height -= 2;
            cur_y  += 2;
            minx   += 2;
         }
      }

      if (!(w->flags & SSD_START_NEW_ROW)) {
         max_size.width = width - cur_width;
         max_size.height = height;
         ssd_widget_get_size (w, &size, &max_size);
      }

      if ((count == MAX_WIDGETS_PER_ROW) ||
         ((count > 0) &&
                     (((cur_width + size.width) > width) ||
                     (w->flags & SSD_START_NEW_ROW)))) {

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         w_last_drawn = w_prev;
         w_prev = NULL;

         width  = rect->maxx - rect->minx + 1;
         height = rect->maxy - cur_y + 1;
         minx   = rect->minx;

         if (w->flags & SSD_WIDGET_SPACE) {
            width  -= 4;
            height -= 2;
            cur_y  += 2;
            minx   += 2;
         }

         max_size.width = width;
         max_size.height = height;
         ssd_widget_get_size (w, &size, &max_size);
      }

      row[count++] = w;

      cur_width += size.width;
      if (w_prev && w_prev->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (w->flags & SSD_END_ROW) {

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         w_last_drawn = w;
         w_prev = NULL;
      }

      w_prev = w;
      w = w->next;
   }

   if (count) {
      if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

         cur_y += SSD_WIDGET_SEP;
      }
      ssd_widget_draw_row (row, count, width, height, minx, cur_y);
   }
}


void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect,
                      int parenlt_flags) {

   if (parenlt_flags & SSD_ALIGN_GRID) ssd_widget_draw_grid (w, rect);
   else ssd_widget_draw_pack (w, rect);
}

static BOOL ssd_widget_default_on_key_pressed( SsdWidget w, const char* utf8char, uint32_t flags)
{ return FALSE;}

SsdWidget ssd_widget_new (const char *name,
                          CB_OnWidgetKeyPressed pfn_on_key_pressed,
                          int flags) {

   static int tab_order_sequence = 0;

   SsdWidget w;

   w = (SsdWidget) calloc (1, sizeof (*w));

   roadmap_check_allocated(w);

   w->name           = strdup(name);
   w->size.height    = SSD_MIN_SIZE;
   w->size.width     = SSD_MIN_SIZE;
   w->in_focus       = FALSE;
   w->focus_highlight = TRUE;
   w->default_widget = (SSD_WS_DEFWIDGET  & flags)? TRUE: FALSE;
   w->tab_stop       = (SSD_WS_TABSTOP & flags)? TRUE: FALSE;
   w->prev_tabstop   = NULL;
   w->next_tabstop   = NULL;
   w->tabstop_left   = NULL;
   w->tabstop_right  = NULL;
   w->tabstop_above  = NULL;
   w->tabstop_below  = NULL;
   w->parent_dialog  = NULL;
   w->get_input_type = NULL;
   w->pointer_down   = NULL;
   w->pointer_up     = NULL;
   w->release = NULL;
   w->tab_sequence   = tab_order_sequence++;
   w->force_click  = FALSE;

   memset( &w->click_offsets, 0, sizeof( SsdClickOffsets ) );

   if( pfn_on_key_pressed)
      w->key_pressed = pfn_on_key_pressed;
   else
      w->key_pressed = ssd_widget_default_on_key_pressed;

   w->cached_size.height = w->cached_size.width = -1;

   return w;
}


SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point, BOOL use_offsets ) {

   while (widget != NULL) {

	   if ( ssd_widget_contains_point( widget, point, use_offsets ) ) {

         return widget;
      }

      widget = widget->next;
   }

   return NULL;
}
/* Finds the clickable widgets (short and long click) at the deeper level,
 * for which the current position belongs to
 */
BOOL ssd_widget_find_clickable_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point, SsdWidget* widget_short_click, SsdWidget* widget_long_click )
{
   while ( widget != NULL ) {
      SsdSize size;
      ssd_widget_get_size ( widget, &size, NULL );

      if ((widget->position.x <= point->x) &&
          ((widget->position.x + size.width) >= point->x) &&
          (widget->position.y <= point->y) &&
          ((widget->position.y + size.height) >= point->y)) {

         if ( widget->short_click )
        	 *widget_short_click = widget;
         if ( widget->long_click )
        	 *widget_long_click = widget;

         if ( ssd_widget_find_clickable_by_pos( widget->children, point, widget_short_click, widget_long_click ) )
        	 return TRUE;
      }
      widget = widget->next;
   }

   return FALSE;
}



/* Checks if the point relies in the vicinity of the widget defined by the given frame offsets */
BOOL ssd_widget_check_point_location ( SsdWidget widget,
                                  const RoadMapGuiPoint *point, int frame_offset_x, int frame_ofset_y ) {

      SsdSize size;
      BOOL res = FALSE;

      if ( widget )
      {
		  ssd_widget_get_size ( widget, &size, NULL );

		  if ( ( ( widget->position.x - frame_offset_x ) <= point->x) &&
			   ( ( widget->position.x + size.width + frame_offset_x ) >= point->x) &&
			   ( ( widget->position.y - frame_ofset_y ) <= point->y) &&
			   ( ( widget->position.y + size.height + frame_ofset_y ) >= point->y) )
		  {

			 res = TRUE;
		  }
      }
   return res;
}


void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback) {
   widget->callback = callback;
}


int ssd_widget_rtl (SsdWidget parent) {

   if (parent && (parent->flags & SSD_ALIGN_LTR)) return 0;
   else return roadmap_lang_rtl ();
}


int ssd_widget_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point) {

	SsdWidget widget_next = widget;

   if (!widget) return 0;

   /* Find clickable widget and remember.
    * Drag end will check if the release event was at the vicinity of this widget
    */
   PressedPointerPoint = *point;


   /* Check all the overlapping ( in terms of click area ) widgets
   * First widget, that it or its children handle the event - wins
   */
   while ( widget_next != NULL )
   {
	   if ( ssd_widget_contains_point( widget_next, point, TRUE ) )
	   {
		   if ( ( widget_next->pointer_down && widget_next->pointer_down( widget_next, point ) ) ||
				   ( widget_next->children && ssd_widget_pointer_down( widget_next->children, point ) ) )
		   {
			  return 1;
		   }
	   }
	   widget_next = widget_next->next;
   }
   return 0;
}

int ssd_widget_pointer_up(SsdWidget widget, const RoadMapGuiPoint *point) {

  SsdWidget widget_next = widget;

   if( !widget)
      return 0;

   /* Check all the overlapping ( in terms of click area ) widgets
   * First widget, that it or its children handle the event - wins
   */
   while ( widget_next != NULL )
   {
	   if ( ssd_widget_contains_point( widget_next, point, TRUE ) )
	   {
		   if ( ( widget_next->pointer_up && widget_next->pointer_up( widget_next, point ) ) ||
				   ( widget_next->children && ssd_widget_pointer_up( widget_next->children, point ) ) )
		   {
			  return 1;
		   }
	   }
	   widget_next = widget_next->next;
   }
   return 0;
}


int ssd_widget_short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   SsdWidget widget_next = widget;

   if (!widget) {
      return 0;
   }

   /* Check all the overlapping ( in terms of click area ) widgets
   * First widget, that it or its children handle the event - wins
   */
   while ( widget_next != NULL )
   {
	   if ( ssd_widget_contains_point( widget_next, point, TRUE ) )
	   {
		   if ( ( widget_next->short_click && widget_next->short_click( widget_next, point ) ) ||
				   ( widget_next->children && ssd_widget_short_click( widget_next->children, point ) ) )
		   {
			  return 1;
		   }
	   }
	   widget_next = widget_next->next;
   }

   return 0;
}

void ssd_widget_set_backgroundfocus( SsdWidget w, BOOL set)
{
   if(w->tab_stop)
      w->background_focus = set;
   else
      w->background_focus = FALSE;
}

static void ssd_widget_sort_children (SsdWidget widget) {
   SsdWidget first = widget;
   SsdWidget prev = NULL;
   SsdWidget bottom = NULL;

   if (!widget) return;

   /* No support for first widget as ORDER_LAST */
   /* Comment by AGA. THere is no assignment for this flag
    *  assert (! (widget->flags & SSD_ORDER_LAST));
    */

   assert( widget != widget->next);

   /* Comment by AGA. THere is no assignment for this flag
   while (widget) {


      if (widget->flags & SSD_ORDER_LAST) {
         SsdWidget tmp = widget->next;

         if (prev) prev->next = widget->next;
         widget->next = bottom;
         bottom = widget;
         widget = tmp;

      } else {

         prev = widget;
         widget = widget->next;
      }
   }
   */

   if (bottom) prev->next = bottom;

   while (first) {
      ssd_widget_sort_children (first->children);
      first = first->next;
   }
}


int ssd_widget_long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   SsdWidget widget_next = widget;

   if (!widget) return 0;

   /* Check all the overlapping ( in terms of click area ) widgets
   * First widget, that it or its children handle the event - wins
   */
   while ( widget_next != NULL )
   {
	   if ( ssd_widget_contains_point( widget_next, point, TRUE ) )
	   {
		   if ( ( widget_next->long_click && widget_next->long_click( widget_next, point ) ) ||
				   ( widget_next->children && ssd_widget_long_click( widget_next->children, point ) ) )
		   {
			  return 1;
		   }
	   }
	   widget_next = widget_next->next;
   }
   return 0;
}

const char *ssd_widget_get_value (const SsdWidget widget, const char *name) {
   SsdWidget w = widget;

   if (name) w = ssd_widget_get (w, name);
   if (!w) return "";

   if (w->get_value) return w->get_value (w);

   return w->value;
}


const void *ssd_widget_get_data (const SsdWidget widget, const char *name) {
   SsdWidget w = ssd_widget_get (widget, name);
   if (!w) return NULL;

   if (w->get_data) return w->get_data (w);
   else return NULL;
}


int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_value) return -1;

   return w->set_value(w, value);
}


int ssd_widget_set_data (const SsdWidget widget, const char *name,
                          const void *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_data) return -1;

   return w->set_data(w, value);
}


void ssd_widget_add (SsdWidget parent, SsdWidget child) {

   SsdWidget last = parent->children;

   child->parent = parent;

   if (!last) {
      parent->children = child;
      return;
   }

   while (last->next) last=last->next;
   last->next = child;

   /* TODO add some dirty flag and only resort when needed */
   ssd_widget_sort_children(parent->children);
}

static BOOL focus_belong_to_widget( SsdWidget w)
{
   SsdWidget p = w;

   while( p)
   {
      if( p->in_focus || focus_belong_to_widget( p->children))
         return TRUE;

      p = p->next;
   }

   return FALSE;
}

extern void ssd_dialog_invalidate_tab_order ();

SsdWidget ssd_widget_remove(SsdWidget parent, SsdWidget child)
{
   SsdWidget cur_child     = parent->children;
   SsdWidget child_behind  = NULL;

   assert(parent);
   assert(child);

   while( cur_child)
   {
      if( cur_child == child)
      {
         if( child_behind)
            child_behind->next= child->next;
         else
            parent->children  = child->next;

         child->next   = NULL;
         child->parent = NULL;

         ssd_dialog_invalidate_tab_order();

         return child;
      }

      child_behind= cur_child;
      cur_child   = cur_child->next;
   }

   return NULL;
}

SsdWidget ssd_widget_replace(SsdWidget parent, SsdWidget old_child, SsdWidget new_child)
{
   SsdWidget cur_child     = parent->children;
   SsdWidget child_behind  = NULL;

   assert(parent);
   assert(old_child);
   assert(new_child);

   while( cur_child)
   {
      if( cur_child == old_child)
      {
         if( child_behind)
            child_behind->next= new_child;
         else
            parent->children  = new_child;

         new_child->next   = old_child->next;
         new_child->parent = old_child->parent;
         old_child->next   = NULL;
         old_child->parent = NULL;

         ssd_dialog_invalidate_tab_order();

         return old_child;
      }

      child_behind= cur_child;
      cur_child   = cur_child->next;
   }

   return NULL;
}

void ssd_widget_set_size (SsdWidget widget, int width, int height) {

   widget->size.width  = width;
   widget->size.height = height;
}


void ssd_widget_set_offset (SsdWidget widget, int x, int y) {

   widget->offset_x = x;
   widget->offset_y = y;
}


void ssd_widget_set_context (SsdWidget widget, void *context) {

   widget->context = context;
}


void ssd_widget_get_size (SsdWidget w, SsdSize *size, const SsdSize *max) {

   SsdSize pack_size = {0, 0};

   RoadMapGuiRect max_size = {0, 0, 0, 0};
   int total_height_below = 0;

   *size = w->size;

   if ((w->size.height >= 0) && (w->size.width >= 0)) {
      return;
   }

   if (!max && (w->cached_size.width < 0)) {
       static SsdSize canvas_size;

       canvas_size.width   = roadmap_canvas_width();
#ifdef TOUCH_SCREEN
 	   canvas_size.height  = roadmap_canvas_height() ;
#else
       canvas_size.height  = roadmap_canvas_height() - roadmap_bar_bottom_height();
#endif
       max = &canvas_size;
   }
   else{
   	if (!max)
   		max = &w->cached_size;
   }



   if ((w->cached_size.width >= 0) && (w->cached_size.height >= 0)) {
      *size = w->cached_size;
      return;
   }
   /* Comment by AGA. THere is no assignment for this flag
   if (size->height == SSD_MAX_SIZE) {
      /* Check if other siblings exists and should be placed below this one *
      SsdWidget below_w = w->next;


      while (below_w) {

         if (below_w->flags & SSD_ORDER_LAST) {
            SsdSize s;
            ssd_widget_get_size (below_w, &s, max);

            total_height_below += s.height;
         }
         below_w = below_w->next;
      }

   }
   */

   if (w->flags & SSD_DIALOG_FLOAT) {
      if ((size->width == SSD_MAX_SIZE) && ((max->width >= roadmap_canvas_width()) || (max->width >= roadmap_canvas_height()))){
         if (roadmap_canvas_width() > roadmap_canvas_height())
            size->width = roadmap_canvas_height();
         else
            size->width = roadmap_canvas_width();
      }else
         if (size->width == SSD_MAX_SIZE) size->width = max->width -10;
      if (size->height== SSD_MAX_SIZE) size->height= max->height - total_height_below;

   } else {

      if (size->width == SSD_MAX_SIZE) size->width = max->width;
      if (size->height== SSD_MAX_SIZE) size->height= max->height - total_height_below;
   }

   if ((size->height >= 0) && (size->width >= 0)) {
      w->cached_size = *size;
      return;
   }

   if (size->width >= 0)  {
      max_size.maxx = size->width - 1;
   } else {
      if (!max){
                static SsdSize canvas_size;
                canvas_size.width = roadmap_canvas_width();
#ifdef TOUCH_SCREEN
				canvas_size.height = roadmap_canvas_height();
#else
                canvas_size.height = roadmap_canvas_height() - roadmap_bar_bottom_height();
#endif
                max = &canvas_size;
      }
      max_size.maxx = max->width - 1;
   }

   if (size->height >= 0) {
      max_size.maxy = size->height - 1;
   } else {
      max_size.maxy = max->height - 1;
   }

   if (!(w->flags & SSD_VAR_SIZE) && w->children) {
      RoadMapGuiRect container_rect = max_size;
      int container_width;
      int container_height;

      w->draw (w, &max_size, SSD_GET_SIZE);

      container_width  = max_size.minx - container_rect.minx +
                         container_rect.maxx - max_size.maxx;
      container_height = max_size.miny - container_rect.miny +
                         container_rect.maxy - max_size.maxy;

      calc_pack_size (w->children, &max_size, &pack_size);

      pack_size.width  += container_width;
      pack_size.height += container_height;

   } else {
      w->draw (w, &max_size, SSD_GET_SIZE);
      pack_size.width  = max_size.maxx - max_size.minx + 1;
      pack_size.height = max_size.maxy - max_size.miny + 1;
   }

   if (size->height< 0) size->height = pack_size.height;
   if (size->width < 0) size->width  = pack_size.width;

   w->cached_size = *size;
}


void ssd_widget_set_color (SsdWidget w, const char *fg_color,
                           const char *bg_color) {
   w->fg_color = fg_color;
   w->bg_color = bg_color;
}


void ssd_widget_container_size (SsdWidget dialog, SsdSize *size) {

   SsdSize max_size;

   /* Calculate size recurisvely */
   if (dialog->parent) {
      ssd_widget_container_size (dialog->parent, size);
      max_size.width = size->width;
      max_size.height = size->height;

   } else {
      max_size.width = roadmap_canvas_width ();
#ifdef TOUCH_SCREEN
      max_size.height = roadmap_canvas_height ();
#else
      max_size.height = roadmap_canvas_height () - roadmap_bar_bottom_height();
#endif
   }

   ssd_widget_get_size (dialog, size, &max_size);

   if (dialog->draw) {
      RoadMapGuiRect rect;
      rect.minx = 0;
      rect.miny = 0;
      rect.maxx = size->width - 1;
      rect.maxy = size->height - 1;

      dialog->draw (dialog, &rect, SSD_GET_SIZE|SSD_GET_CONTAINER_SIZE);

      size->width = rect.maxx - rect.minx + 1;
      size->height = rect.maxy - rect.miny + 1;
   }
}


void *ssd_widget_get_context (SsdWidget w) {

   return w->context;
}


void ssd_widget_reset_cache (SsdWidget w) {

   SsdWidget child = w->children;

   w->cached_size.width = w->cached_size.height = -1;

   while (child != NULL) {

      ssd_widget_reset_cache (child);
      child = child->next;
   }
}

void ssd_widget_reset_position (SsdWidget w) {

   SsdWidget child = w->children;

   w->position.x = -1;
   w->position.y = -1;

   while (child != NULL) {

      ssd_widget_reset_position (child);
      child = child->next;
   }
}
void ssd_widget_hide (SsdWidget w) {
   w->flags |= SSD_WIDGET_HIDE;
}


void ssd_widget_show (SsdWidget w) {
   w->flags &= ~SSD_WIDGET_HIDE;
}


void ssd_widget_set_flags (SsdWidget widget, int flags) {

   widget->flags = flags;
   widget->default_widget = (SSD_WS_DEFWIDGET  & flags)? TRUE: FALSE;
   widget->tab_stop       = (SSD_WS_TABSTOP & flags)? TRUE: FALSE;
}

BOOL ssd_widget_on_key_pressed( SsdWidget w, const char* utf8char, uint32_t flags)
{
   SsdWidget child;

   if(w->key_pressed && w->key_pressed( w, utf8char, flags))
      return TRUE;

   child =  w->children;
   while( child)
   {
      if( ssd_widget_on_key_pressed( child, utf8char, flags))
         return TRUE;

      child = child->next;
   }

   return FALSE;
}


int ssd_widget_set_right_softkey_text(SsdWidget widget, const char *value) {

   widget->right_softkey = value;

   switch (roadmap_softkeys_orientation ()) {

         case SOFT_KEYS_ON_BOTTOM:
               ssd_widget_set_value (widget, "right_title_text", "");
               if (value != NULL && *value)
                  return ssd_widget_set_value (widget, "right_softkey_text", value);
              break;
       case SOFT_KEYS_ON_RIGHT:
              if (widget->left_softkey != NULL)
                 ssd_widget_set_value (widget, "right_softkey_text", widget->left_softkey);
              if (value != NULL && *value)
                 return ssd_widget_set_value (widget, "right_title_text", value);
              break;
      default:
              return -1;
    }

    return 0;

}

int ssd_widget_set_left_softkey_text(SsdWidget widget, const char *value) {




   widget->left_softkey = value;

    switch (roadmap_softkeys_orientation ()) {

         case SOFT_KEYS_ON_BOTTOM:
               if (widget->right_softkey != NULL)
                  ssd_widget_set_value (widget, "right_softkey_text", widget->right_softkey);
               if (value != NULL && *value)
                  return ssd_widget_set_value (widget, "left_softkey_text", value);
              break;
       case SOFT_KEYS_ON_RIGHT:
              ssd_widget_set_value (widget, "left_softkey_text", "");
                 if (value != NULL && *value)
                    return ssd_widget_set_value (widget, "right_softkey_text", value);
              break;
      default:
              return -1;
     }

    return 0;
}

void ssd_widget_set_left_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback) {

   widget->left_softkey_callback = callback;
}

void ssd_widget_set_right_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback) {
   widget->right_softkey_callback = callback;
}


BOOL ssd_widget_contains_point(  SsdWidget widget, const RoadMapGuiPoint *point, BOOL use_offsets )
{
   SsdSize size;
   BOOL res;
   ssd_widget_get_size( widget, &size, NULL );

   res =
	   ( ( point->x >= ( widget->position.x + widget->click_offsets.left * use_offsets ) ) &&
		 ( point->y >= ( widget->position.y + widget->click_offsets.top * use_offsets  ) ) &&
		 ( point->x <= ( widget->position.x  + size.width -1 + widget->click_offsets.right * use_offsets ) ) &&
		 ( point->y <= ( widget->position.y + size.height -1 + widget->click_offsets.bottom * use_offsets ) )
	   );
   return res;
}

int ssd_widget_pointer_down_force_click (SsdWidget widget,
                                    const RoadMapGuiPoint *point)
{
   widget->force_click = TRUE;
   // TODO :: Check return value
   return 1;
}


int ssd_widget_pointer_up_force_click (SsdWidget widget,
                                    const RoadMapGuiPoint *point) {

   int ret_val = 0;
   int movement = ABS_POINTS_DISTANCE( PressedPointerPoint, *point );
   if ( widget->force_click &&
		ssd_widget_contains_point( widget, point, TRUE ) &&
		(  movement < FORCE_CLICK_MOVEMENT_THRESHOLD ) )
   {
	   if ( widget->long_click && roadmap_pointer_long_click_expired() )
	   {
		   ret_val = widget->long_click( widget, point );
	   }
	   else
	   {
		   if ( widget->short_click )
			   ret_val = widget->short_click( widget, point );
	   }
	   widget->force_click = FALSE;
   }
   // TODO :: Check return value
   return 1;
}

/*
 * Updates the click offsets for the parent if the child click area
 * is out of the borders of the parent widget
 */
void ssd_widget_update_click_offsets( SsdWidget parent, SsdWidget child )
{
   SsdSize size_parent, size_child;
   if ( parent == NULL || child == NULL )
	   return;

   ssd_widget_get_size( child, &size_child, NULL );
   ssd_widget_get_size( parent, &size_parent, NULL );
   // Left offset
   if ( parent->position.x > ( child->position.x + child->click_offsets.left ) )
   {
		   parent->click_offsets.left = child->position.x + child->click_offsets.left - parent->position.x;
   }
   // Top offset
   if ( parent->position.y > ( child->position.y + child->click_offsets.top ) )
   {
		   parent->click_offsets.top = child->position.y + child->click_offsets.top - parent->position.y;
   }
   // Right
   if ( ( parent->position.x + size_parent.width ) < ( child->position.x + size_child.width +
										   child->click_offsets.right ) )
   {
		   parent->click_offsets.right = child->position.x + size_child.width +
		   child->click_offsets.top - parent->position.x - size_parent.width;
   }
   // Bottom
   if ( ( parent->position.y + size_parent.height ) < ( child->position.y + size_child.height +
										   child->click_offsets.bottom ) )
   {
		   parent->click_offsets.bottom = child->position.y + size_child.height +
						   child->click_offsets.bottom - parent->position.y - size_parent.height;
   }
}

/*******************************
 * This is workaround because of absence of flags.
 * After the flags problem will be removed this should be
 * placed in the new() of each relevant widget
 *  *** AGA ***
 */
void ssd_widget_set_pointer_force_click( SsdWidget widget )
{
	widget->pointer_down =  ssd_widget_pointer_down_force_click;
	widget->pointer_up =  ssd_widget_pointer_up_force_click;
}

void ssd_widget_set_click_offsets( SsdWidget widget, const SsdClickOffsets* offsets )
{
	widget->click_offsets = *offsets;
}

void ssd_widget_set_click_offsets_ext( SsdWidget widget, int left, int top, int right, int bottom )
{
   widget->click_offsets.left = left;
   widget->click_offsets.top = top;
   widget->click_offsets.right = right;
   widget->click_offsets.bottom = bottom;
}

void ssd_widget_set_focus_highlight( SsdWidget widget, BOOL is_highlight )
{
	widget->focus_highlight = is_highlight;
}

/*******************************************************
 * Deallocates the widget node only
 */
static void ssd_widget_free_node( SsdWidget widget )
{
	/*
	 * Release the widget itself
	 */
   if ( widget->release )
   {
	   widget->release( widget );
   }

   free( (char*) widget->name );
   free( widget );
}


/*****************************
 * Deallocates the widget tree
 * Frees brothers and children
 *
 */
static void ssd_widget_free_all( SsdWidget widget )
{
	SsdWidget next, cursor = widget;

	if ( widget == NULL || 	( widget->flags & SSD_PERSISTENT ) )/* if persistent - nothing to do */
		return;

   /* Pass through the brothers if necessary */
   while ( cursor != NULL )
   {
	   next = cursor->next;

	   if ( cursor->children )
	   {
		   ssd_widget_free_all( cursor->children );
	   }

	   /*
	    * Release the widget itself
	    */
	   ssd_widget_free_node( cursor );

	   cursor = next;
   }
}


/*****************************
 * Deallocates the widget recursively. Updates the parent references
 */
void ssd_widget_free( SsdWidget widget, BOOL force, BOOL update_parent )
{

	/* if persistent - nothing to do */
	if ( !force && ( widget->flags & SSD_PERSISTENT ) )
		return;

	/* Update the references for the parent and brothers */
	if ( update_parent && widget->parent )
	{
		SsdWidget parent = widget->parent;
		SsdWidget next;
		if ( parent->children == widget )
		{
			parent->children = widget->next;
		}
		else
		{
			for ( next = parent->children; next; next = next->next )
			{
				if ( next->next == widget )
				{
					next->next = widget->next;
					break;
				}
			}
		}
	}
	/* Update the references */
	ssd_widget_free_all( widget->children );

   /*
	* Release the widget itself
	*/
   ssd_widget_free_node( widget );


}

