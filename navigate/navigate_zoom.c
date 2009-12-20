/* navigate_zoom.c - implement auto zoom
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 *   See navigate_zoom.h
 */

#include <stdlib.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_layer.h"

#include "navigate_main.h"
#include "navigate_zoom.h"

static int NavigateZoomScale;

void navigate_zoom_update (int distance,
                           int distance_to_prev,
                           int distance_to_next) {

   /* We might still be close to the previous junction. Let's make
    * sure that we don't zoom out too fast
    */

   if ((distance_to_prev < distance) && (distance > 200) && (distance_to_prev < 200)) {

      distance = (distance_to_prev * (200 - distance_to_prev) + distance * distance_to_prev) / 200;

   }

   if ((distance_to_next <= 150) && (distance < distance_to_next)) {
      distance = distance_to_next;
   }

   if (distance < 100) distance = 100;
   
#if 0
   if (distance > 500) {
      units = roadmap_screen_height();
   } else {
      units = roadmap_screen_height() / 3;
   }
#endif

#if 0
   if (distance <= 500) {
      distance *= 3;
      if (distance > 500) 
      	distance = 500;
   }
#endif

   if (distance <= 250) {
      NavigateZoomScale = 750;
   } else if (distance <= 500) {
   	NavigateZoomScale = distance*3;
	} else if (distance <= 1000) {
   	NavigateZoomScale = ((distance - 500) * 2000 + (1000 - distance) * 1500) / 500;
	} else {
	   if (distance > 5000)
	      NavigateZoomScale = 10000;
	   else
	      NavigateZoomScale = distance * 2;
	}
}

int navigate_zoom_get_scale (void) {
   return NavigateZoomScale;
}

