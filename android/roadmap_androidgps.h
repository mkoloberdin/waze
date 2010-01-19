 /* roadmap_androidgps.h - Android GPS interface
 *
 * LICENSE:
 *
  *   Copyright 2008 Alex Agranovich
 *
 *   This file is part of RoadMap.
 *
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
 */

#ifndef INCLUDE__ROADMAP_ANDROIDGPS__H
#define INCLUDE__ROADMAP_ANDROIDGPS__H

#include "roadmap_gps.h"

typedef enum {
	_andr_pos_none = 0,
	_andr_pos_cell,
	_andr_pos_gps
} AndroidPositionMode;
extern RoadMapGpsdNavigation GpsdNavigationCallback;
extern RoadMapGpsdSatellite GpsdSatelliteCallback;
extern RoadMapGpsdDilution GpsdDilutionCallback;

void roadmap_gpsandroid_subscribe_to_navigation (RoadMapGpsdNavigation navigation);

void roadmap_gpsandroid_subscribe_to_satellites (RoadMapGpsdSatellite satellite);

void roadmap_gpsandroid_subscribe_to_dilution   (RoadMapGpsdDilution dilution);

void roadmap_gpsandroid_set_position( AndroidPositionMode mode, char status, int gps_time, int latitude, int longitude,
					int altitude, int speed, int steering );

#endif // INCLUDE__ROADMAP_ANDROIDGPS__H
