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

typedef struct {
   int longitude;
   int latitude;
} RoadMapPointJ2ME;

typedef struct {
   unsigned short longitude;
   unsigned short latitude;
} RoadMapPointC;

#if defined(J2MEMAP)
	typedef RoadMapPointJ2ME RoadMapPoint; 
#else
	typedef RoadMapPointC RoadMapPoint; 
#endif

#endif // _ROADMAP_DB_POINT__H_

