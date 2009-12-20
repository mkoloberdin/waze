/* roadmap_gpsj2me.c - a module to interact with j2me location API.
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 *
 * DESCRIPTION:
 *
 *   This module implements the interface with the java syscall.
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gps_manager.h>

#include "roadmap.h"
#include "roadmap_gpsj2me.h"


static RoadMapGpsdNavigation RoadmapGpsJ2meNavigationListener = NULL;
static RoadMapGpsdSatellite  RoadmapGpsd2SatelliteListener = NULL;
static RoadMapGpsdDilution   RoadmapGpsd2DilutionListener = NULL;

#if 0
RoadMapSocket roadmap_gpsd2_connect (const char *name) {

   RoadMapSocket socket = roadmap_net_connect ("tcp", name, 2947);

   if (ROADMAP_NET_IS_VALID(socket)) {

      /* Start watching what happens. */

      static const char request[] = "w+\n";

      if (roadmap_net_send
            (socket, request, sizeof(request)-1, 1) != sizeof(request)-1) {

         roadmap_log (ROADMAP_WARNING, "Lost gpsd server session");
         roadmap_net_close (socket);

         return ROADMAP_INVALID_SOCKET;
      }
   }

   return socket;
}
#endif

void roadmap_gpsj2me_subscribe_to_navigation (RoadMapGpsdNavigation navigation) {

   RoadmapGpsJ2meNavigationListener = navigation;
}


/*
void roadmap_gpsd2_subscribe_to_satellites (RoadMapGpsdSatellite satellite) {

   RoadmapGpsd2SatelliteListener = satellite;
}


void roadmap_gpsd2_subscribe_to_dilution (RoadMapGpsdDilution dilution) {

   RoadmapGpsd2DilutionListener = dilution;
}
*/

int roadmap_gpsj2me_decode (void *user_context,
                            void *decoder_context, char *data, int length) {

   struct GpsData *d = (struct GpsData *)data;
   int altitude;

   /* default value (invalid value): */

   altitude  = ROADMAP_NO_VALID_DATA;

   //roadmap_log (ROADMAP_DEBUG, "GPS data: %d, %d, %d\n", d->status, d->speed, d->azymuth);
   RoadmapGpsJ2meNavigationListener
         ((char)d->status, d->time, d->latitude, d->longitude, altitude, d->speed, d->azymuth);

   return length;
}

