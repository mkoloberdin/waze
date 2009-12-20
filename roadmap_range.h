/* roadmap_range.h - RoadMap street address operations and attributes.
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
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

#ifndef _ROADMAP_RANGE__H_
#define _ROADMAP_RANGE__H_

#include "roadmap_types.h"

typedef struct {

   int fradd;
   int toadd;

} RoadMapStreetRange;


int roadmap_range_get_street
       (int range);

void roadmap_range_get_address
		(int range,
		 RoadMapStreetRange *left,
		 RoadMapStreetRange *right);



extern roadmap_db_handler RoadMapRangeHandler;

#endif // _ROADMAP_RANGE__H_
