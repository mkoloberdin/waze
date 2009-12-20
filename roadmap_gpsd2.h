/* roadmap_gpsd2.h - a module to interact with gpsd using its library.
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
 *
 * DESCRIPTION:
 *
 *   This module hides the gpsd library API (version 2).
 */

#ifndef INCLUDE__ROADMAP_GPSD2__H
#define INCLUDE__ROADMAP_GPSD2__H

#include "roadmap_net.h"
#include "roadmap_gps.h"
#include "roadmap_input.h"


void roadmap_gpsd2_subscribe_to_navigation (RoadMapGpsdNavigation navigation);

void roadmap_gpsd2_subscribe_to_satellites (RoadMapGpsdSatellite satellite);

void roadmap_gpsd2_subscribe_to_dilution   (RoadMapGpsdDilution dilution);


RoadMapSocket roadmap_gpsd2_connect (const char *name);

int roadmap_gpsd2_decode (void *user_context,
                          void *decoder_context, char *sentence, int length);

#endif // INCLUDE__ROADMAP_GPSD2__H

