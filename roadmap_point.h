/* roadmap_point.h - Manage the points that define tiger lines.
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
 */

#ifndef _ROADMAP_POINT__H_
#define _ROADMAP_POINT__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_square.h"
#include "roadmap_db_point.h"

typedef struct RoadMapPointContext_t {

   char *type;

   RoadMapPoint *Point;
   int           PointCount;

   int *PointID;
   int  PointIDCount;

} RoadMapPointContext;

extern RoadMapPointContext *RoadMapPointActive;

#ifndef J2ME
extern int point_cache_square;
extern int point_cache_scale_factor;
extern int point_cache_square_position_x;
extern int point_cache_square_position_y;
#endif

#if defined(FORCE_INLINE) || defined(DECLARE_ROADMAP_POINT)
#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

INLINE_DEC void roadmap_point_position
                     (int point, RoadMapPosition *position) {

   RoadMapPoint *Point;

   int point_id = point & POINT_REAL_MASK;

#ifdef J2ME
   Point = RoadMapPointActive->Point + point_id;
   position->longitude = Point->longitude;
   position->latitude  = Point->latitude;

#else   
   int point_square = roadmap_square_active (); //(point >> 16) & 0xffff;

#ifdef DEBUG
   if (point_id < 0 || point_id >= RoadMapPointActive->PointCount) {
    		roadmap_log (ROADMAP_FATAL, "invalid point index %d", point_id);      
   }
#endif

   if  (point_cache_square != point_square) {

      RoadMapPosition square_position;
      point_cache_square = point_square;
      roadmap_square_min (point_square, &square_position);
      point_cache_square_position_x = square_position.longitude;
      point_cache_square_position_y = square_position.latitude;
      point_cache_scale_factor = roadmap_square_current_scale_factor ();
   }

   Point = RoadMapPointActive->Point + point_id;
   position->longitude = point_cache_square_position_x + Point->longitude * point_cache_scale_factor;
   position->latitude  = point_cache_square_position_y  + Point->latitude * point_cache_scale_factor;
#endif
}
#endif // inline

int  roadmap_point_in_square (int square, int *first, int *last);
void roadmap_point_position  (int point, RoadMapPosition *position);
int roadmap_point_db_id (int point);
int roadmap_point_count (void);
int roadmap_point_square (int point);

extern roadmap_db_handler RoadMapPointHandler;

#endif // _ROADMAP_POINT__H_
