/* roadmap_db_range.h - the format of the address range table used by RoadMap.
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
 *   The RoadMap address ranges are described by the following tables:
 *
 *   range.bystreet   for each street, the ranges that belong to the street.
 *   range.bycity     the list of ranges by street, brocken for each city.
 *   range.addr       for each range, the street address numbers.
 *   range.noaddr     reference to the street for the lines with no address.
 *   range.byzip      for each range, the ZIP code.
 *   range.place      for each place, the city it belongs to.
 *   range.bysquare   for each square, a street search accelerator.
 *
 *   The ZIP codes are stored in a separate table to make the database more
 *   compact: the ZIP value is a 8 bit value that references the zip table.
 */

#ifndef _ROADMAP_DB_RANGE__H_
#define _ROADMAP_DB_RANGE__H_

#include "roadmap_types.h"

#define RANGE_FLAG_MASK   		(3 << 14)
#define RANGE_FLAG_COMPACT		(0 << 14) /* left+right 4 byte-size numbers */
#define RANGE_FLAG_LEFT_ONLY	(1 << 14) /* 2 short values, left address only */
#define RANGE_FLAG_RIGHT_ONLY	(2 << 14) /* 2 short values, right address only */
#define RANGE_FLAG_BOTH			(3 << 14) /* 2 short values, left address, right to follow */

#define RANGE_FLAG_STREET_ONLY	0x8000
#define RANGE_ADDR_MAX_VALUE	0xffffff

typedef struct {   /* table range.addr */

	unsigned short street;
   unsigned short fradd;
   unsigned short toadd;

} RoadMapRange;


#endif // _ROADMAP_DB_RANGE__H_

