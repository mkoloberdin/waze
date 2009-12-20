/* ssd_bitmap.h - Bitmap widget
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

#ifndef __SSD_WIDGET_ICON_H__
#define __SSD_WIDGET_ICON_H__
  
#include "ssd_widget.h"


SsdWidget   ssd_icon_create(  const char* name,
                              BOOL        wide_image,
                              int         flags);




// Single 'wide-image' image file names:
typedef struct tag_ssd_icon_wimage
{
   const char* left;
   const char* middle;
   const char* right;
   
}  ssd_icon_wimage;

typedef struct tag_ssd_icon_wimage_set
{
   ssd_icon_wimage*  normal;
   ssd_icon_wimage*  in_focus;   // [OPTIONAL]  Can be NULL
   
}  ssd_icon_wimage_set;

typedef struct tag_ssd_icon_image_set
{
   const char*       normal;
   const char*       in_focus;   // [OPTIONAL]  Can be NULL
   
}  ssd_icon_image_set;

// Set simple image(s)
void        ssd_icon_set_images(
               SsdWidget            this,
               ssd_icon_image_set   images[],   // One image per state
               int                  count);     // Images count

// Set wide image(s)
void        ssd_icon_set_wimages(
               SsdWidget            this,
               ssd_icon_wimage_set  images[],   // One image per state
               int                  count);     // Images count

// Operation on 'wide-image'
void        ssd_icon_set_width(  SsdWidget   this,
                                 int         width);

// Operation on all image types:
int         ssd_icon_get_state(  SsdWidget   this);
int         ssd_icon_set_state(  SsdWidget   this,
                                 int         state);

void        ssd_icon_set_unhandled_key_press(
                                 SsdWidget   this,
                                 CB_OnWidgetKeyPressed
                                             on_unhandled_key_pressed);

#endif // __SSD_WIDGET_ICON_H__

