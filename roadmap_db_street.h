/* roadmap_db_street.h - the format of the street table used by RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   The RoadMap streets are described by the following table:
 *
 *   street         the identification (type, name, etc..) of each street.
 */

#ifndef _ROADMAP_DB_STREET__H_
#define _ROADMAP_DB_STREET__H_

#include "roadmap_types.h"

typedef struct {

   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString fetype;
   RoadMapString fedirs;
#ifndef J2MEMAP
   RoadMapString t2s;
#endif

} RoadMapStreet;

typedef struct {
	RoadMapString  city;
	unsigned short first_street;
} RoadMapCity;

#endif // _ROADMAP_DB_STREET__H_

