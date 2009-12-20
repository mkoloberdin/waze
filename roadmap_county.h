/* roadmap_county.c - retrieve the county to which a specific place belongs.
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

#ifndef _ROADMAP_COUNTY__H_
#define _ROADMAP_COUNTY__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_county_by_position
        (const RoadMapPosition *position, int *fips, int count);

int  roadmap_county_by_city (RoadMapString city, RoadMapString state);

int  roadmap_county_by_state(RoadMapString state, int *fips, int count);

const char *roadmap_county_get_name (int fips);
const char *roadmap_county_get_state (int fips);

int  roadmap_county_count (void);

const RoadMapArea *roadmap_county_get_edges (int fips);

extern roadmap_db_handler RoadMapCountyHandler;

#endif // _ROADMAP_COUNTY__H_

