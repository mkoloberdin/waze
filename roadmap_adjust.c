/* roadmap_adjust.c - Convert a GPS position into a map position.
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
 *   See roadmap_adjust.h
 *
 * BUGS:
 *
 *   At this time this module only perfoms a fixed adjustment.
 */

#include "roadmap_config.h"

#include "roadmap_adjust.h"


static RoadMapConfigDescriptor RoadMapConfigGPSOffsetLat =
                     ROADMAP_CONFIG_ITEM("Map", "GPS map offset latitude");

static RoadMapConfigDescriptor RoadMapConfigGPSOffsetLong =
                     ROADMAP_CONFIG_ITEM("Map", "GPS map offset longitude");


static RoadMapGpsPosition RoadMapAdjustLatestGps;
static RoadMapPosition    RoadMapAdjustLatestMap;


int  roadmap_adjust_offset_latitude (void) {
   return roadmap_config_get_integer (&RoadMapConfigGPSOffsetLat);
}

int  roadmap_adjust_offset_longitude (void) {
   return roadmap_config_get_integer (&RoadMapConfigGPSOffsetLong);
}
      

static void roadmap_adjust_convert (const RoadMapGpsPosition *gps) {

   RoadMapAdjustLatestMap.longitude =
      gps->longitude + roadmap_adjust_offset_longitude();

   RoadMapAdjustLatestMap.latitude =
      gps->latitude + roadmap_adjust_offset_latitude();

   RoadMapAdjustLatestGps = *gps;
}


void roadmap_adjust_position
         (const RoadMapGpsPosition *gps, RoadMapPosition *map) {

   /* Because the conversion might be expensive, and requested
    * several times for the same GPS position (from different
    * modules), we cache the latest result.
    */
   if ((gps->latitude != RoadMapAdjustLatestGps.latitude) ||
       (gps->longitude != RoadMapAdjustLatestGps.longitude)) {

       roadmap_adjust_convert (gps);
   }
   *map = RoadMapAdjustLatestMap;
}


void roadmap_adjust_initialize (void) {

   roadmap_config_declare
      ("preferences", &RoadMapConfigGPSOffsetLat, "0", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGPSOffsetLong, "0", NULL);
}

