/* roadmap_polygon.h - the format of the polygon table used by RoadMap.
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
 * The RoadMap polygons are described by the following tables:
 *
 * polygon.bysquare the list of polygons located in each square.
 * polygon.head     for each polygon, the category and list of lines.
 * polygon.points   the list of points defining the polygons border.
 */

#ifndef _ROADMAP_DB_POLYGON__H_
#define _ROADMAP_DB_POLYGON__H_

#include "roadmap_types.h"

typedef struct {  /* table polygon.head */

   unsigned short first;
   unsigned short count;

   RoadMapString name;
   char  cfcc;
   unsigned char filler;

   /* TBD: replace these with a RoadMapArea (not compatible!). */
   unsigned short   north;
   unsigned short   west;
   unsigned short   east;
   unsigned short   south;

} RoadMapPolygon;

typedef struct {  /* table polygon.lines */

   unsigned short point;

} RoadMapPolygonPoint;

#endif // _ROADMAP_DB_POLYGON__H_

