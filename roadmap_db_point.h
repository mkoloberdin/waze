/* roadmap_db_point.h - the format of the point table used by RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   The RoadMap points are described by the following tables:
 *
 *   point.bysquare   for each square, the list of points in that square.
 *   point.data       the point position, relative to the center of the square.
 */

#ifndef _ROADMAP_DB_POINT__H_
#define _ROADMAP_DB_POINT__H_

#include "roadmap_types.h"

typedef struct {  /* table point.data */

   /* The position here is relative to the upper-left corner
    * of the square the point belongs to.
    */
#if defined(J2MEMAP)
   int longitude;
   int latitude;
#else
   unsigned short longitude;
   unsigned short latitude;
#endif

} RoadMapPoint;

#endif // _ROADMAP_DB_POINT__H_

