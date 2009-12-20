/* roadmap_db_square.h - the format of the square table used by RoadMap.
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
 *   The RoadMap square divide the complete territory into a grid of smaller
 *   rectangular areas. The goal is 91) to speed up RoadMap by localizing
 *   the data to be accessed (most tables are sorted by squares) and (2) to
 *   make the database more compact by using point locations relative to
 *   the center of the square.
 *
 *   The squares are described by the following tables:
 *
 *   square.global     Some global information regarding the territory.
 *   square.data       The position of each square.
 *
 *   The square are built so that a relative location fits into a signed
 *   16 bits value.
 */

#ifndef _ROADMAP_DB_SQUARE__H_
#define _ROADMAP_DB_SQUARE__H_

#include "roadmap_types.h"

typedef struct {

	int square_id;
	int scale;
   unsigned int timestamp;
} RoadMapSquare;

typedef struct {

   RoadMapArea edges;
   int num_scales;
   int count_squares;  /* Necessary because empty squares have been removed. */
   int step_longitude;
   int step_latitude;

} RoadMapGrid;

typedef struct {

   unsigned int timestamp;

} RoadMapGlobal;

typedef struct {

   int first_longitude;
   int first_latitude;

   int count_longitude;
   int count_latitude;

	int scale_factor;
} RoadMapScale;

#define GZM_ENTRY_LEN	8		
typedef struct {
	
	char		name[GZM_ENTRY_LEN];
	int		offset;
	int		compressed_size;
	int		raw_size;
} RoadMapSquareIndex;

#endif // _ROADMAP_DB_SQUARE__H_

