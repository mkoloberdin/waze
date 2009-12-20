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

#ifndef __SSD_WIDGET_BITMAP_H__
#define __SSD_WIDGET_BITMAP_H__

#include "../roadmap_canvas.h"
#include "ssd_widget.h"

SsdWidget ssd_bitmap_new(  const char *name,
                           const char *bitmap,
                           int         flags);

void ssd_bitmap_update(SsdWidget widget, const char *bitmap);

SsdWidget ssd_bitmap_image_new(  const char *name,
                                 RoadMapImage image,
                                 int         flags );

void ssd_bitmap_image_update( SsdWidget widget, RoadMapImage image );

void ssd_bitmap_splash(const char *bitmap, int seconds);
const char *ssd_bitmap_get_name(SsdWidget widget);
#endif // __SSD_WIDGET_BITMAP_H__

