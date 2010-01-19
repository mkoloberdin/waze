/* roadmap_border.h - Handle Drawing of borders
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

#ifndef ROADMAP_BORDER_H_
#define ROADMAP_BORDER_H_

typedef enum border_images
{
   border_image_top,
   border_image_top_right,
   border_image_top_left,

   border_image_bottom,
   border_image_bottom_right,
   border_image_bottom_left,

   border_image_left,
   border_image_right,

   border_white_top,
   border_white_top_right,
   border_white_top_left,

   border_white_bottom,
   border_white_bottom_right,
   border_white_bottom_left,

   border_white_left,
   border_white_right,

   border_black_top,
   border_black_top_right,
   border_black_top_left,

   border_black_bottom,
   border_black_bottom_right,
   border_black_bottom_left,

   border_black_left,
   border_black_right,

   border_heading_black_left,
   border_heading_black_middle,
   border_heading_black_right,

   border_pointer_menu,
   border_pointer_comment,

   border_image_bottom_no_frame,
   border_image_bottom_no_frame2,
   border_black_bottom_no_frame,

   border_img__count,
   border_img__invalid

}  border_images;

#define STYLE_NORMAL  border_image_top
#define STYLE_WHITE   border_white_top
#define STYLE_BLACK   border_black_top

#define POINTER_NONE 	 -1
#define POINTER_POSITION 0
#define POINTER_MENU  	 border_pointer_menu
#define POINTER_COMMNET  border_pointer_comment

#define HEADER_NONE		 -1
#define HEADER_BLACK	    border_heading_black_left

typedef struct broder_image{
	RoadMapImage image;
	int height;
	int width;
}border_image;

int roadmap_display_border(int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, const char* background, RoadMapPosition *position);
int get_heading_height(int header_type);

BOOL roadmap_border_initialize();
void roadmap_border_shutdown();


#endif /*ROADMAP_BORDER_H_*/
