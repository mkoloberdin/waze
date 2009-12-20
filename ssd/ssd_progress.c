/* ssd_progress.c
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

#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"


#include "ssd_widget.h"
#include "ssd_button.h"


typedef struct tag_bitmap_info
{
   int            percentage;

}  progress_info, *progress_info_ptr;

static void draw (SsdWidget this, RoadMapGuiRect *rect, int flags)
{
   static RoadMapImage   left;
   static RoadMapImage   middle;
   static RoadMapImage   right;
   static RoadMapImage   filled;
   static RoadMapImage   left_fill;
   static RoadMapImage   right_fill;
   static RoadMapImage   wazzy;
   RoadMapGuiPoint point;
   int   width_left, width_right, width_middle;
   int width_left_fill = 0, width_right_fill = 0;
   int   num_images;
   int   num_filled;
   int   i;
   progress_info_ptr pi = (progress_info_ptr)this->data;

   if (!left)
         left =   roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_left");

   if (!right)
         right =  roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_right");

   if (!middle)
         middle =    roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_middle");

   if (!filled)
      filled =    roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_filled");

   if (!ssd_widget_rtl(NULL) && !left_fill)
      left_fill =    roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_left_fill");

   if (ssd_widget_rtl(NULL) && !right_fill)
         right_fill =    roadmap_res_get(
                                       RES_BITMAP,
                                       RES_SKIN,
                                       "progress_right_fill");


   if (!wazzy)
      wazzy =    roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN,
                                    "progress_wazzy");

   if (!left || !right || !middle || !filled)
      return;

   width_right = roadmap_canvas_image_width(right);
   width_left = roadmap_canvas_image_width(left);
   width_middle = roadmap_canvas_image_width(middle);

   if (left_fill)
   width_left_fill = roadmap_canvas_image_width(left_fill);

   if (right_fill)
      width_right_fill = roadmap_canvas_image_width(right_fill);

   if( flags & SSD_GET_SIZE)
   {
      rect->maxy = rect->miny + roadmap_canvas_image_height(middle) + 5;
      return;
   }


   num_images = (rect->maxx - rect->minx - width_right - width_left-40)/width_middle;

   point.x = rect->minx + 30;
   point.y = rect->miny;
   roadmap_canvas_draw_image (left, &point, 0, IMAGE_NORMAL);

   num_images = (rect->maxx - rect->minx - width_right - width_left -60)/width_middle;
   for (i = 0; i < num_images; i++){
      point.x = rect-> minx + 30 + width_right + i * width_middle;
      point.y = rect->miny;
      roadmap_canvas_draw_image (middle, &point, 0, IMAGE_NORMAL);
   }

   point.x = rect->maxx - width_right - 30;
   point.y = rect->miny;
   roadmap_canvas_draw_image (right, &point, 0, IMAGE_NORMAL);

   num_filled = (int)((rect->maxx - rect->minx-62)*pi->percentage/100) - width_left_fill ;
   if ((ssd_widget_rtl(NULL))){
      if (right_fill){
         point.x = rect-> maxx -width_right_fill - 30;
         roadmap_canvas_draw_image (right_fill, &point, 0, IMAGE_NORMAL);
      }
   }
   else{
      if (left_fill){
         point.x = rect-> minx + 30 ;
         roadmap_canvas_draw_image (left_fill, &point, 0, IMAGE_NORMAL);
      }
   }

   for (i = 0; i < num_filled; i++){
      if ((ssd_widget_rtl(NULL)))
         point.x = rect-> maxx - 30 -width_right_fill- i ;
      else
         point.x = rect-> minx +width_left_fill+ 30 +  i ;
      point.y = rect->miny;
      roadmap_canvas_draw_image (filled, &point, 0, IMAGE_NORMAL);
   }

   if (pi->percentage == 0){
      point.x = rect->minx + 30;
      point.y = rect->miny;

   }

   if (wazzy){
      point.x -= roadmap_canvas_image_width(wazzy)/2;
      point.y -= roadmap_canvas_image_height(wazzy)/2;
      roadmap_canvas_draw_image(wazzy, &point, 0, IMAGE_NORMAL);
   }


}

void ssd_progress_set_value (SsdWidget widget, int percentage){
   progress_info_ptr pi = (progress_info_ptr)widget->data;
   if (percentage > 100) percentage = 100;
   if (percentage < 0) percentage = 0;
   pi->percentage = percentage;
}

SsdWidget ssd_progress_new (const char *name,
                            int percentage,
                            int flags)
{
   SsdWidget         w  = ssd_widget_new(name, NULL, flags);
   progress_info_ptr   pi = (progress_info_ptr)malloc(sizeof(progress_info));

   w->data        = pi;
   w->_typeid     = "Progress";
   w->draw        = draw;
   w->flags       = flags;

   return w;
}
