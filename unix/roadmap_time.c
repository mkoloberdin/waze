/* roadmap_time.c - Manage time information & display.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_time.h
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_time.h"


char *roadmap_time_get_hours_minutes (time_t gmt) {
    
    static char image[16];
    
    struct tm *tm;
    
    tm = localtime (&gmt);
    snprintf (image, sizeof(image), "%2d:%02d", tm->tm_hour, tm->tm_min);

    return image;
}

static unsigned long tv_to_msec(struct timeval *tv)
{
    return (tv->tv_sec & 0xffff) * 1000 + tv->tv_usec/1000;
}

uint32_t roadmap_time_get_millis(void) {
   struct timeval tv;

   gettimeofday(&tv, NULL);
   return tv_to_msec(&tv);

}

