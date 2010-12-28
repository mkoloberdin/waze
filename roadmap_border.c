/* roadmap_border.c - Handle Drawing of borders
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
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
#include "roadmap_border.h"

#include "roadmap_display.h"
#include "roadmap_device.h"

static border_image s_images[border_img__count];

typedef enum border_cache_types
{
   border_top,
   border_bottom,
   border_sides
}  border_cache_types;


#define __ROADMAP_BORDER_CACHE__


#ifdef __ROADMAP_BORDER_CACHE__
#define MAX_CACHE 20
typedef struct cahced_resource {
   RoadMapImage image;
   int side;
   int style;
   int header;
   int width;
   int num_items;
   unsigned int age;
   //unsigned int cache_hit_count; Deprecated
} RoadMapCacheBorder;

typedef struct cahce_table {
   RoadMapCacheBorder resource[MAX_CACHE];
   int count;
} RoadMapBorderCacheTable;

static RoadMapBorderCacheTable border_cached_table;
#endif //__ROADMAP_BORDER_CACHE__





static const char* get_img_filename( border_images image_id)
{
   switch( image_id)
   {
   	  case border_heading_black_left:   return "Header_Black_left";
   	  case border_heading_black_right:  return "Header_Black_right";
   	  case border_heading_black_middle: return "Header_Black_Mid";
   	  case border_white_top:   			return "border_white_top";
   	  case border_white_top_right:   	return "border_white_top_right";
   	  case border_white_top_left:   		return "border_white_top_left";
   	  case border_white_bottom:   		return "border_white_bottom";
   	  case border_white_bottom_right:   return "border_white_bottom_right";
   	  case border_white_bottom_left:   	return "border_white_bottom_left";
   	  case border_white_left:   			return "border_white_left";
   	  case border_white_right:   			return "border_white_right";
   	  case border_black_top:   			return "border_black_top";
   	  case border_black_top_right:   	return "border_black_top_right";
   	  case border_black_top_left:   		return "border_black_top_left";
   	  case border_black_bottom:   		return "border_black_bottom";
   	  case border_black_bottom_right:   return "border_black_bottom_right";
   	  case border_black_bottom_left:   	return "border_black_bottom_left";
   	  case border_black_left:   			return "border_black_left";
   	  case border_black_right:   			return "border_black_right";
   	  case border_black_bottom_no_frame:return "border_black_bottom_no_frame";

   	  case border_pointer_comment:		return "PointerComment";
   	  case border_pointer_menu:			return "PointerMenu";


   	  default:
   		  break;
   }

   return NULL;
}

void roadmap_border_shutdown()
{
	int i;
	for( i=0; i<border_img__count; i++)
    {
	   if( s_images[i].image )
	   {
 		   roadmap_canvas_free_image( s_images[i].image );
	   }
    }
#ifdef __ROADMAP_BORDER_CACHE__
	for ( i=0; i < border_cached_table.count; ++i )
	{
		roadmap_canvas_free_image( border_cached_table.resource[i].image );
	}
#endif 	// __ROADMAP_BORDER_CACHE__
}

BOOL roadmap_border_initialize()
{
   int   i;

   for( i=0; i<border_img__count; i++)
   {
      const char* image_filename = get_img_filename(i);

      assert(NULL != image_filename);

      s_images[i].image = (RoadMapImage)roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN|RES_NOCACHE,
                                    image_filename);

      if( !s_images[i].image)
      {
         roadmap_log(ROADMAP_ERROR,
                     "load_border_images::load_border_images() - Failed to load image file '%s.png'",
                     image_filename);
         return FALSE;
      }

      s_images[i].height = roadmap_canvas_image_height(s_images[i].image);
      s_images[i].width = roadmap_canvas_image_width(s_images[i].image);

   }

#ifdef __ROADMAP_BORDER_CACHE__
   border_cached_table.count = 0;
#endif // __ROADMAP_BORDER_CACHE__

   return TRUE;
}

static RoadMapImage create_sides_image (int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, int num_items) {
   RoadMapImage image;
   int image_width = abs(bottom->x  - top->x);
   RoadMapGuiPoint point;
   int i;

   image = roadmap_canvas_new_image (image_width, s_images[border_white_left+style].height * num_items);

   for (i = 0; i<num_items; i++){
      point.x = image_width - s_images[border_white_right+style].width ;
      point.y = i* s_images[border_white_right+style].height;
      roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_right+style].image, CANVAS_COPY_NORMAL);


      point.x = 0;
      point.y = i* s_images[border_white_left+style].height;
      roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_left+style].image, CANVAS_COPY_NORMAL);
   }


   return image;
}

static RoadMapImage create_top_image (int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top) {

   RoadMapImage image;
   int screen_width, screen_height, image_width;
   RoadMapGuiPoint right_point, left_point;
   RoadMapGuiPoint sign_bottom, sign_top, point;
   int i,num_items;
   image_width = abs(bottom->x  - top->x);


   screen_width = roadmap_canvas_width();
   screen_height = roadmap_canvas_height();

   if (top == NULL)
   {
      sign_top.x = 1;
      sign_top.y = 0;
   }
   else{
      sign_top.x = top->x ;
      sign_top.y = top->y ;
   }

   if (bottom == NULL)
   {
      sign_bottom.x = screen_width-1;
      sign_bottom.y = 0;
   }
   else{
      sign_bottom.x = bottom->x;
      sign_bottom.y = bottom->y;
   }


   image = roadmap_canvas_new_image (image_width, s_images[border_white_top+style].height);

#ifdef TOUCH_SCREEN
      left_point.x = sign_top.x;
#else
      left_point.x = sign_top.x-1;
#endif
      right_point.x = image_width - s_images[border_white_right+style].width;
      left_point.y = 0;

      point.x = 0;
      point.y = 0;
      roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_top_left+style].image, CANVAS_COPY_NORMAL);


       point.x = right_point.x;
       point.y = 0;
       roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_top_right+style].image, CANVAS_COPY_NORMAL);

      num_items = (image_width - s_images[border_white_right+style].width - s_images[border_white_left+style].width)/s_images[border_white_top+style].width;
      for (i = 0; i<num_items; i++){
         point.x = s_images[border_white_top_left+style].width + i * s_images[border_white_top+style].width ;
         point.y = 0;
         roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_top+style].image, CANVAS_COPY_NORMAL);
      }

   return image;
}

static RoadMapImage create_top_header_image (int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top) {

   RoadMapImage image;
   int screen_width, screen_height, image_width;
   RoadMapGuiPoint right_point, left_point;
   RoadMapGuiPoint sign_bottom, sign_top, point;
   int i,num_items;
   image_width = abs(bottom->x  - top->x);


   screen_width = roadmap_canvas_width();
   screen_height = roadmap_canvas_height();

   if (top == NULL)
   {
      sign_top.x = 1;
      sign_top.y = 0;
   }
   else{
      sign_top.x = top->x ;
      sign_top.y = top->y ;
   }

   if (bottom == NULL)
   {
      sign_bottom.x = screen_width-1;
      sign_bottom.y = 0;
   }
   else{
      sign_bottom.x = bottom->x;
      sign_bottom.y = bottom->y;
   }


   image = roadmap_canvas_new_image (image_width, s_images[header].height);

#ifdef TOUCH_SCREEN
      left_point.x = sign_top.x;
#else
      left_point.x = sign_top.x-1;
#endif
      right_point.x = image_width - s_images[header+2].width;
      left_point.y = 0;

      point.x = 0;
      point.y = 0;
      roadmap_canvas_copy_image (image, &point, NULL, s_images[header].image, CANVAS_COPY_NORMAL);


       point.x = right_point.x;
       point.y = 0;
       roadmap_canvas_copy_image (image, &point, NULL, s_images[header+2].image, CANVAS_COPY_NORMAL);
      num_items = (image_width - s_images[header].width - s_images[header+2].width)/s_images[header+1].width;
      for (i = 0; i<num_items; i++){
         point.x = s_images[header].width + i * s_images[header+1].width ;
         point.y = 0;
         roadmap_canvas_copy_image (image, &point, NULL, s_images[header+1].image, CANVAS_COPY_NORMAL);
      }

   return image;
}

static RoadMapImage create_bottom_image (int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top) {

   RoadMapImage image;
   int screen_width, screen_height, image_width;
   RoadMapGuiPoint right_point, left_point;
   RoadMapGuiPoint sign_bottom, sign_top, point;
   int i,num_items;
   image_width = abs(bottom->x  - top->x);


   screen_width = roadmap_canvas_width();
   screen_height = roadmap_canvas_height();

   if (top == NULL)
   {
      sign_top.x = 1;
      sign_top.y = 0;
   }
   else{
      sign_top.x = top->x ;
      sign_top.y = top->y ;
   }

   if (bottom == NULL)
   {
      sign_bottom.x = screen_width-1;
      sign_bottom.y = 0;
   }
   else{
      sign_bottom.x = bottom->x;
      sign_bottom.y = bottom->y;
   }


   image = roadmap_canvas_new_image (image_width, s_images[border_white_top+style].height);

#ifdef TOUCH_SCREEN
      left_point.x = sign_top.x;
#else
      left_point.x = sign_top.x-1;
#endif
      right_point.x = image_width - s_images[border_white_right+style].width;
      left_point.y = 0;

      point.x = 0;
      point.y = 0;
      roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_bottom_left+style].image, CANVAS_COPY_NORMAL);


       point.x = right_point.x;
       point.y = 0;
       roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_bottom_right+style].image, CANVAS_COPY_NORMAL);


      num_items = (image_width - s_images[border_white_right+style].width - s_images[border_white_left+style].width)/s_images[border_white_top+style].width;
      for (i = 0; i<num_items; i++){
         point.x = s_images[border_white_top_left+style].width + i * s_images[border_white_bottom+style].width ;
         point.y = 0;
         roadmap_canvas_copy_image (image, &point, NULL, s_images[border_white_bottom+style].image, CANVAS_COPY_NORMAL);
      }

   return image;
}


static RoadMapImage create_image (int side, int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top,int num_items){
   switch (side){
      case border_top:
         if (header == HEADER_NONE)
            return create_top_image(style, header, pointer_type, bottom, top);
         else
            return create_top_header_image(style, header, pointer_type, bottom, top);
      case border_bottom:
         return create_bottom_image(style, header, pointer_type, bottom, top);

      case border_sides:
         return create_sides_image(style, header, pointer_type, bottom, top, num_items);

      default:
         return NULL;
   }
}

static RoadMapImage get_image (int side, int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, int num_items){
   int i;
   int screen_width, screen_height, image_width;
   RoadMapGuiPoint sign_bottom, sign_top;
   int index;
   int lru_index = 0;
   // unsigned int min_cache_hit_count = UINT_MAX; Deprecated
   unsigned int least_recent_age = UINT_MAX;
   static unsigned int counter  = 0 ;  // counter for how many times function was accessed - A timeline needed for the cache age calculations
   counter++;

   screen_width = roadmap_canvas_width();
   screen_height = roadmap_canvas_height();

   if (top == NULL)
   {
       sign_top.x = 1;
       sign_top.y = 0;
   }
   else{
       sign_top.x = top->x ;
       sign_top.y = 0 ;
   }

   if (bottom == NULL)
   {
       sign_bottom.x = screen_width-1;
       sign_bottom.y = 0;
   }
   else{
       sign_bottom.x = bottom->x ;
       sign_bottom.y = 0 ;
   }

   image_width = sign_bottom.x  - sign_top.x;

   /*
    * To avoid chance of overflow, though HIGHLY unlikely.
    */
   if(counter == UINT_MAX){
	   counter = 1;
	   for (i = 0; i < border_cached_table.count; i++){
		   border_cached_table.resource[i].age = 1;
	   }
   }



#ifdef __ROADMAP_BORDER_CACHE__


   for (i = 0; i < border_cached_table.count; i++){
	   RoadMapCacheBorder* res = &border_cached_table.resource[i];
       if ( ( res->style == style ) &&
            ( res->header == header ) &&
            ( res->side == side ) &&
            ( res->num_items == num_items ) &&
            ( res->width == image_width ) )
       {
    	   // res->cache_hit_count++; Deprecated
    	   res->age = counter;
    	   return res->image;
       }
       else
       {
    	   /* Deprecated
    	   if ( min_cache_hit_count > res->cache_hit_count )
    	   {
    		   min_cache_hit_count = res->cache_hit_count;
    		   lru_index = i;
    	   }
    	   */

    	   if ( least_recent_age > res->age )
		   {
    		   least_recent_age = res->age;
			   lru_index = i;
		   }

       }

   }

   if ( border_cached_table.count == MAX_CACHE ) /* Cache is full - replace the lru element */
   {
	   index = lru_index;
	   roadmap_log( ROADMAP_DEBUG, "Border cache no hit. Index %d age: %d  year : %d, **OLD**  style : %d ,  Side : %d , Header : %d. Num_items : %d, Image_width : %d  **NEW** style %d, new side : %d Header : %d, Num_items : %d, Image_width : %d \n",
	  			       index, least_recent_age, counter,
	  			       border_cached_table.resource[index].style,
	  			       border_cached_table.resource[index].side,
	  			       border_cached_table.resource[index].header,
	  			       border_cached_table.resource[index].num_items,
	  			       border_cached_table.resource[index].width, image_width);

	   roadmap_canvas_free_image( border_cached_table.resource[index].image );

   }
   else
   {
	   index = border_cached_table.count;
	   border_cached_table.count++;
   }


   border_cached_table.resource[index].style = style;
   border_cached_table.resource[index].header = header;
   border_cached_table.resource[index].width = image_width;
   border_cached_table.resource[index].side = side;
   border_cached_table.resource[index].num_items = num_items;
   border_cached_table.resource[index].image = create_image( side, style, header, pointer_type, bottom, top, num_items );
   border_cached_table.resource[index].age = counter;
   // border_cached_table.resource[index].cache_hit_count = 1; Deprecated
   return border_cached_table.resource[index].image;
#endif

}

int roadmap_display_border(int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, const char* background, RoadMapPosition *position){
	//static const char *fill_bg = "#f3f3f5";// "#e4f1f9";
	RoadMapPen fill_pen;
	int screen_width, screen_height, sign_width, sign_height;
	RoadMapGuiPoint right_point, left_point, start_sides_point, point, new_point;
	int i, num_items;
	int count, start_pos_x, top_height;
	RoadMapGuiPoint sign_bottom, sign_top;
	RoadMapImage image;

	RoadMapGuiPoint fill_points[4];


	screen_width = roadmap_canvas_width();
	screen_height = roadmap_canvas_height();

	if (top == NULL)
	{
		sign_top.x = 1;
		sign_top.y = roadmap_bar_top_height();
	}
	else{
		sign_top.x = top->x ;
		sign_top.y = top->y ;
	}

	if (bottom == NULL)
	{
		sign_bottom.x = screen_width-1;
		sign_bottom.y = screen_height - roadmap_bar_bottom_height();
	}
	else{
		sign_bottom.x = bottom->x ;
		sign_bottom.y = bottom->y ;
	}

	sign_width = sign_bottom.x  - sign_top.x;
	sign_height = sign_bottom.y - sign_top.y;

	if (header != HEADER_NONE){
	    image = get_image(border_top, style, header, pointer_type, bottom, top, 0);
    	roadmap_canvas_draw_image (image, &sign_top, 0, IMAGE_NORMAL);

    	left_point.x = sign_top.x;
		right_point.x = left_point.x + sign_width - s_images[header+2].width ;
		start_sides_point.y = sign_top.y + s_images[header].height  ;
		top_height = s_images[header].height+7;
#ifndef __ROADMAP_BORDER_CACHE__
    	// Release after drawing
		roadmap_canvas_free_image( image );
#endif
	}
	else{
		 RoadMapImage image;
		left_point.x = sign_top.x;
		right_point.x = sign_bottom.x - s_images[border_white_right+style].width;
		start_sides_point.y = sign_top.y + s_images[border_white_top_right+style].height;
		left_point.y = sign_top.y ;
		image =  get_image(border_top, style, header, pointer_type, bottom, top, 0);
  		roadmap_canvas_draw_image (image, &left_point, 0, IMAGE_NORMAL);
#ifndef __ROADMAP_BORDER_CACHE__
  		// Release after drawing
		roadmap_canvas_free_image( image );
#endif
  		right_point.y = sign_top.y;
  		if (style == STYLE_BLACK)
  		   top_height = s_images[border_white_top+style].height+7;
  		else
  		 top_height = s_images[border_white_top+style].height;
	}

	num_items = ( sign_height - start_sides_point.y +sign_top.y - s_images[border_white_bottom+style].height ) / s_images[border_white_right+style].height;
	if (num_items < 0 )
	   return;
	image  = get_image(border_sides, style, header, pointer_type, bottom, top, 1);

	for (i = 0; i < num_items; i++){
	   point.x = left_point.x;
      point.y = start_sides_point.y + i*s_images[border_white_right+style].height;
      roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);

	}

	point.y += roadmap_canvas_image_height(image)-1;
#ifndef __ROADMAP_BORDER_CACHE__
	// Release after drawing
	roadmap_canvas_free_image( image );
#endif

	if (pointer_type == POINTER_NONE){
	   RoadMapImage image;
	   left_point.y = point.y +  s_images[border_white_left+style].height ;
	   image =  get_image(border_bottom, style, header, pointer_type, bottom, top, 0);
	   roadmap_canvas_draw_image (image, &left_point, 0, IMAGE_NORMAL);
#ifndef __ROADMAP_BORDER_CACHE__
	   // Release after drawing
		roadmap_canvas_free_image( image );
#endif
	}
	else{
	   left_point.y = point.y +  s_images[border_white_left+style].height ;
	   roadmap_canvas_draw_image (s_images[border_white_bottom_left+style].image, &left_point, 0, IMAGE_NORMAL);


	    right_point.y = point.y + s_images[border_white_right+style].height;
	    roadmap_canvas_draw_image (s_images[border_white_bottom_right+style].image, &right_point, 0, IMAGE_NORMAL);

		if (pointer_type == POINTER_MENU)
			num_items = 1;
		else if (pointer_type == POINTER_COMMENT)
			num_items = ((right_point.x - left_point.x)/s_images[border_white_bottom+style].width)/4 -2;
		else if (pointer_type == POINTER_POSITION)
			num_items = (right_point.x - left_point.x- 100)/s_images[border_white_bottom+style].width;
		else
			num_items = ((right_point.x - left_point.x)/s_images[border_white_bottom+style].width)/2 -2;
		for (i = 0; i<num_items; i++){
			point.x = left_point.x + s_images[border_white_bottom_left+style].width + i * s_images[border_white_bottom+style].width ;
			point.y = left_point.y;
			roadmap_canvas_draw_image (s_images[border_white_bottom+style].image, &point, 0, IMAGE_NORMAL);
		}


		if (pointer_type == POINTER_POSITION){
			int visible = roadmap_math_point_is_visible (position);
			if (visible){
				int count = 3;
				RoadMapGuiPoint points[3];
				RoadMapPen pointer_pen;
				roadmap_math_coordinate (position, points);
				roadmap_math_rotate_project_coordinate ( points );
				if (points[0].y > (point.y +10) ){
				points[1].x = point.x;
				points[1].y = point.y+s_images[border_black_bottom_no_frame].height-3;
				points[2].x = point.x+42;
				points[2].y = point.y+s_images[border_black_bottom_no_frame].height-3;

    			pointer_pen = roadmap_canvas_create_pen ("fill_pop_up_pen");

    			roadmap_canvas_set_foreground("#000000");
    		   roadmap_canvas_set_opacity(181);
    		   count = 3;
				roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);

//				count = 3;
//				roadmap_canvas_set_foreground("#9e9e9e");
//				roadmap_canvas_set_thickness(2);
//				roadmap_canvas_draw_multiple_polygons (1, &count, points, 0, 0);

    			//point.x += 1;
    			for (i=0; i<40; i++){

    				point.x = point.x + s_images[border_black_bottom_no_frame].width ;
					point.y = point.y;
					roadmap_canvas_draw_image (s_images[border_black_bottom_no_frame].image, &point, 0, IMAGE_NORMAL);
    			}
				}

   			}
   			start_pos_x = point.x+1;
		}
		else{
			roadmap_canvas_draw_image (s_images[pointer_type].image, &point, 0, IMAGE_NORMAL);
			start_pos_x = point.x + s_images[pointer_type].width;
		}



		num_items = (right_point.x - start_pos_x )/s_images[border_white_bottom+style].width ;
		for (i = 0; i<num_items; i++){
			new_point.x = start_pos_x + i * s_images[border_white_bottom+style].width ;
			new_point.y = point.y;
			roadmap_canvas_draw_image (s_images[border_white_bottom+style].image, &new_point, 0, IMAGE_NORMAL);
		}
	}

	//Fill the
	if ((style == STYLE_NORMAL) || (style == STYLE_BLACK)){
	   if ((pointer_type == POINTER_POSITION) || (pointer_type == POINTER_COMMENT))
	      point.y -= 1;
		fill_points[0].x =right_point.x ;
		fill_points[0].y =point.y +1;
		fill_points[1].x =right_point.x ;
		fill_points[1].y = top->y + top_height -7;
		fill_points[2].x = left_point.x + s_images[border_white_left+style].width ;
		fill_points[2].y = top->y + top_height - 7;
		fill_points[3].x =left_point.x + s_images[border_white_left+style].width ;
		fill_points[3].y = point.y +1;
		count = 4;
	}
	else{
		fill_points[0].x =right_point.x+2 ;
		fill_points[0].y =point.y +2;
		fill_points[1].x =right_point.x +2;
		fill_points[1].y = top->y + top_height -2;
		fill_points[2].x = left_point.x + s_images[border_white_left+style].width -2;
		fill_points[2].y = top->y + top_height - 2;
		fill_points[3].x =left_point.x + s_images[border_white_left+style].width -2;
		fill_points[3].y = point.y +2;
		count = 4;
	}
  	fill_pen = roadmap_canvas_create_pen ("fill_pop_up_pen");
   roadmap_canvas_set_foreground(background);

   if (style == STYLE_BLACK)
      roadmap_canvas_set_opacity(181);
	roadmap_canvas_draw_multiple_polygons (1, &count, fill_points, 1,0 );

	return sign_width;

}

int get_heading_height(int header_type){
	return s_images[header_type].height;
}

