/* roadmap_types.h - general type definitions used in RoadMap.
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
 */

#ifndef INCLUDED__ROADMAP_TYPES__H
#define INCLUDED__ROADMAP_TYPES__H

#if !defined(_WIN32) || defined(__SYMBIAN32__)
#ifdef __cplusplus
   #define EXTERN_C extern "C"
#else
   #define EXTERN_C
#endif
#endif   // ~EXTERN_C

#define ROADMAP_INVALID_STRING ((unsigned short) -1)

typedef unsigned short RoadMapZip;
typedef unsigned short RoadMapString;

typedef unsigned char  LineRouteFlag;
typedef unsigned char  LineRouteMax;
typedef unsigned short LineRouteTime;

typedef struct {
   int longitude;
   int latitude;
} RoadMapPosition;

typedef struct {
   int first;
   int count;
} RoadMapSortedList;

typedef struct {
   int east;
   int north;
   int west;
   int south;
} RoadMapArea;

typedef void (*RoadMapShapeItr) (int shape, RoadMapPosition *position);

/* The cfcc category codes: */

/* Category: road. */

#define ROADMAP_ROAD_FIRST       1

#define ROADMAP_ROAD_FREEWAY     1
#define ROADMAP_ROAD_PRIMARY     2
#define ROADMAP_ROAD_SECONDARY   3
#define ROADMAP_ROAD_RAMP        4
#define ROADMAP_ROAD_MAIN        5
#define ROADMAP_ROAD_EXIT        6
#define ROADMAP_ROAD_STREET      7
#define ROADMAP_ROAD_PEDESTRIAN  8
#define ROADMAP_ROAD_4X4         9
#define ROADMAP_ROAD_TRAIL      10 
#define ROADMAP_ROAD_WALKWAY    11

#define ROADMAP_ROAD_LAST       11


/* Category: area. */

#define ROADMAP_AREA_FIRST      12

#define ROADMAP_AREA_PARC       12
#define ROADMAP_AREA_HOSPITAL   13
#define ROADMAP_AREA_AIRPORT    14
#define ROADMAP_AREA_STATION    15
#define ROADMAP_AREA_CITY       16

#define ROADMAP_AREA_LAST       16


/* Category: water. */

#define ROADMAP_WATER_FIRST     17

#define ROADMAP_WATER_SHORELINE 17
#define ROADMAP_WATER_RIVER     18
#define ROADMAP_WATER_LAKE      19
#define ROADMAP_WATER_SEA       20

#define ROADMAP_WATER_LAST      20

#define ROADMAP_CATEGORY_RANGE  20

/* flags for fake (on tile border) points */

#define POINT_FAKE_FLAG				0x8000
#define POINT_REAL_MASK				0x7FFF

enum {
	ROADMAP_DIRECTION_EAST,
	ROADMAP_DIRECTION_NORTH,
	ROADMAP_DIRECTION_WEST,
	ROADMAP_DIRECTION_SOUTH,
	ROADMAP_DIRECTION_COUNT
};

#endif // INCLUDED__ROADMAP_TYPES__H

