/* roadmap_db_alert.h - the format of the alert points table used by RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi
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
 *   shape.bysquare   for each square, the list of shape lines.
 *   shape.byline     for each line, the list of shape points.
 *   shape.data       the position of each shape point.
 */

#ifndef _ROADMAP_DB_ALERT_H_
#define _ROADMAP_DB_ALERT_H_

#include "roadmap_types.h"

typedef struct {  /* table alert.data */
   RoadMapPosition pos;
   unsigned short steering;
   unsigned char category;
   unsigned char speed;
   unsigned short id;
   short filler; // keeps the structure aligned
} RoadMapAlert;

#endif // _ROADMAP_DB_ALERT_H_

