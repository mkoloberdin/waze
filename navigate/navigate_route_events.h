/* navigate_route_events.h - Events on route
 *
 * LICENSE:
 *
 *   Copyright 2007 Avi Ben-Shoshan
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
 */

#ifndef NAVIGATE_ROUTE_EVENTS_H_
#define NAVIGATE_ROUTE_EVENTS_H_

#define MAX_ALERTS_ON_ROUTE 120
#define MAX_ADD_ON_NAME 	 128
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
   int iAlertId;
   int iAltRouteID;
   int iType;
   int iSubType;
   int iSeverity;
   int iStart;
   int iEnd;
   int iPrecentage;
   RoadMapPosition positionStart;
   RoadMapPosition positionEnd;
   char sAddOnName[MAX_ADD_ON_NAME];
} event_on_route_info;

///////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
   event_on_route_info *events_on_route[MAX_ALERTS_ON_ROUTE];
   int iCount;
} event_on_routes_table;

BOOL events_on_route_feature_enabled (void);
void events_on_route_clear(void);
void events_on_route_init(void);
int events_on_route_count(void);
int events_on_route_count_alternative(int iAltRouteID);
void events_on_route_clear_record(event_on_route_info *event);
event_on_route_info * evenys_on_route_at_index(int index);
event_on_route_info * events_on_route_at_index_alternative(int index, int iAltRouteID) ;
void events_on_route_add(event_on_route_info *event);
#endif /* NAVIGATE_ROUTE_EVENTS_H_ */
