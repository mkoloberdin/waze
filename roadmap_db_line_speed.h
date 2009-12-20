/* roadmap_db_line_speed.h - the format of a line's speed data
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   RoadMap's lines speed data is described by the following table:
 *
 */

#ifndef _ROADMAP_DB_LINE_SPEED__H_
#define _ROADMAP_DB_LINE_SPEED__H_

#define INVALID_SPEED 0xFFFF
#define SPEED_EOL 0x40
//#define SPPED_ 0x80

typedef struct {
   unsigned short from_speed_ref;
   unsigned short to_speed_ref;
} RoadMapLineSpeed;

typedef struct {

   unsigned char from_avg_speed;
   unsigned char to_avg_speed;
} RoadMapLineSpeedAvg;

typedef struct {

   unsigned char speed;
   unsigned char time_slot;
} RoadMapLineSpeedRef;

#endif // _ROADMAP_DB_LINE_SPEED__H_

