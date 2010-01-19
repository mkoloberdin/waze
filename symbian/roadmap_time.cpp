/* roadmap_time.cpp - Manage time information & display.
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
 *   See roadmap_time.h
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <e32base.h>

extern "C"{
#include "roadmap.h"
#include "roadmap_time.h"
}

char *roadmap_time_get_hours_minutes (time_t /*gmt*/) 
{
  //TODO note that the original implementation can work here as well
  static TBuf8<32> buf;
  _LIT8(KTimeFormatTxt,"%2d:%02d");
  
  //  We don't care for gmt here
  TTime homeTime;
  homeTime.HomeTime();
  TDateTime dateTime = homeTime.DateTime();
  buf.Format(KTimeFormatTxt, dateTime.Hour(), dateTime.Minute()); 

  return (char*)(buf.Ptr());
}

time_t roadmap_time(time_t * /*_timer*/)
{
  //TODO note that time() implementation can work here as well using STDLIB
  TTime utcTime;
  utcTime.UniversalTime();
  TDateTime baseTime;
  baseTime.Set(1970,EJanuary,1,0,0,0,0);
  
  TTimeIntervalSeconds intervalSeconds;
  if ( utcTime.SecondsFrom(baseTime, intervalSeconds) != KErrNone )
  {
    return 0;
  }
  return intervalSeconds.Int();
} 

uint32_t roadmap_time_get_millis(void) {
  TTime utcTime;
  utcTime.UniversalTime();
  TDateTime baseTime;
  baseTime.Set(1970,EJanuary,1,0,0,0,0);

  TTimeIntervalMicroSeconds tmicro = utcTime.MicroSecondsFrom(baseTime);
  
  return (unsigned long) (tmicro.Int64() / 1000);

}
