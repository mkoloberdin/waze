/* ssd_text.h - Static text widget
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
 */

#ifndef __SSD_WIDGET_CONTAINER_H_
#define __SSD_WIDGET_CONTAINER_H_
  
#include "ssd_widget.h"

void ssd_container_get_visible_dimentions(
                     SsdWidget         this, 
                     RoadMapGuiPoint*  position,
                     SsdSize*          size);

void ssd_container_get_zero_offset(
                     SsdWidget         this, 
                     int*              zero_offset_x,
                     int*              zero_offset_y);

SsdWidget ssd_container_new (const char *name, const char *title,
                             int width, int height, int flags);
#endif // __SSD_WIDGET_CONTAINER_H_
