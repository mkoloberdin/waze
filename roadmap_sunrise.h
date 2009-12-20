/* roadmap_sunrise.h - Calculate sunrise/sunset time
 *
 * LICENSE:
 *
 *   Copyright 2003 Eric Domazlicky
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
 * DESCRIPTION:
 *
 *   roadmap_sunrise() computes the time for sunrise at the specified location.
 *
 *   roadmap_sunset() computes the time for sunset at the specified location.
 *
 *   position: the longitude/latitude of the position considered.
 *
 *   The function returns -1 on failure or the GMT time on success.
 *
 * LIMITATIONS:
 *
 *   These functions won't work for latitudes at 63 and above or -63 and
 *   below.
 */

#ifndef INCLUDE__ROADMAP_SUNRISE__H
#define INCLUDE__ROADMAP_SUNRISE__H

#include <time.h>

#include "roadmap.h"
#include "roadmap_gps.h"

time_t roadmap_sunrise (const RoadMapGpsPosition *position, time_t now);
time_t roadmap_sunset  (const RoadMapGpsPosition *position, time_t now);

#endif // INCLUDE__ROADMAP_SUNRISE_H

