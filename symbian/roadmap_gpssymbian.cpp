/* roadmap_gpssymbian.cpp - Symbian GPS interface implementation for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *   See roadmap_gps.h
 */

extern "C"{
#include "roadmap_gpssymbian.h"
}

#include "locationprocessor.h"

static RoadMapGps NativeGPS = NULL;

void roadmap_gpssymbian_open()
{
  if ( NativeGPS == NULL )
  {
    CLocationProcessor* pLocationProcessor = CLocationProcessor::NewL();  //TODO TRAPD
    NativeGPS = (RoadMapGps)pLocationProcessor;
  }
}

void roadmap_gpssymbian_shutdown()
{
  CLocationProcessor* pLocationProcessor = (CLocationProcessor*)NativeGPS;
  delete pLocationProcessor;
  NativeGPS = NULL;
 }

void roadmap_gpssymbian_subscribe_to_navigation (RoadMapGpsdNavigation navigation)
{
  CLocationProcessor* pLocationProcessor = (CLocationProcessor*)NativeGPS;
  pLocationProcessor->SetNavigationCallback(navigation);
  pLocationProcessor->ConnectToDeviceL();  //TODO TRAPD
}

void roadmap_gpssymbian_subscribe_to_satellites (RoadMapGpsdSatellite satellite)
{
  
}

void roadmap_gpssymbian_subscribe_to_dilution   (RoadMapGpsdDilution dilution)
{
  
}

int roadmap_gpssymbian_decode (void *user_context,
                                void *decoder_context, char *sentence, int length)
{
  return 0;
}

