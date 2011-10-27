/* roadmap_androidgps.c - Android GPS functionality
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

#include "roadmap_androidgps.h"
#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include <string.h>

RoadMapGpsdNavigation GpsdNavigationCallback = NULL;
RoadMapGpsdSatellite GpsdSatelliteCallback = NULL;
RoadMapGpsdDilution GpsdDilutionCallback = NULL;


#define 	RM_ANDROID_POS_MODE_VALIDATION_PERIOD   100 		// Milliseconds
void roadmap_gpsandroid_subscribe_to_navigation (RoadMapGpsdNavigation navigation)
{
	GpsdNavigationCallback = navigation;
}

void roadmap_gpsandroid_subscribe_to_satellites (RoadMapGpsdSatellite satellite)
{
	GpsdSatelliteCallback = satellite;
}

void roadmap_gpsandroid_subscribe_to_dilution   (RoadMapGpsdDilution dilution)
{
	GpsdDilutionCallback = dilution;
}

void roadmap_gpsandroid_set_position( AndroidPositionMode mode, char status, int gps_time, int latitude, int longitude,
					int altitude, int speed, int steering, int accuracy )
{
	if ( mode == _andr_pos_gps )
	{
		if ( GpsdNavigationCallback )
		{
			(*GpsdNavigationCallback)( status, gps_time, latitude,
											longitude, altitude, speed, steering, accuracy );
		}
		else
		{
			roadmap_log( ROADMAP_WARNING, "The navigation callback is not initialized" );
		}

	}
	if ( mode == _andr_pos_cell )
	{
		roadmap_gps_coarse_fix( latitude, longitude, accuracy, gps_time );
	}
}

