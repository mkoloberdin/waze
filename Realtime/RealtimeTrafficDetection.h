/* RealtimeTrafficDetection.h - Manage real time traffic detection
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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
 */


#ifndef REALTIMETRAFFICDETECTION_H_
#define REALTIMETRAFFICDETECTION_H_

#include "RealtimeAlerts.h"
//////////////////////////////////////////////////////////////////////////////////////////////////
// Traffic Detection Res
typedef struct
{
   int  iPoints;
   char title[RT_ALERT_RES_TITLE_MAX_SIZE];
   char msg[RT_ALERT_RES_TEXT_MAX_SIZE];
} RealtimeTrafficDetection_Res;

#define TRAFFIC_VALUE_NO        0
#define TRAFFIC_VALUE_YES       1
#define TRAFFIC_VALUE_NO_ANSWER 2


void  RealtimeTrafficDetection_Init(void);
BOOL  RealtimeTrafficDetection_OnRes (RealtimeTrafficDetection_Res *res);

#endif /* REALTIMETRAFFICDETECTION_H_ */
