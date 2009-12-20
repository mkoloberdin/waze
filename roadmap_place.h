/* roadmap_place.h - Manage placename points.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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

#ifndef _ROADMAP_PLACE__H_
#define _ROADMAP_PLACW__H_

#include "roadmap_dbread.h"

int  roadmap_place_in_square (int square, int cfcc, int *first, int *last);

void roadmap_place_point   (int place, RoadMapPosition *position);

int  roadmap_place_count (void);

extern roadmap_db_handler RoadMapPlaceHandler;

#endif // _ROADMAP_PLACE__H_
