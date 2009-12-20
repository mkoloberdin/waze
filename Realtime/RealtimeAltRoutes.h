/* RealtimeAltRoutes.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *
 */

#ifndef REALTIMEALTROUTES_H_
#define REALTIMEALTROUTES_H_

#define MAX_ALT_ROUTES        10
#define MAX_ROUTE_NAME        256
#define MAX_DESTINATION_NAME  256
#define MAX_SOURCE_NAME       256
#define MAX_ROUTES            3

#include "navigate/navigate_route_trans.h"
//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
   int iTripId;
   char sTripName[MAX_ROUTE_NAME];
   char sDestinationName[MAX_DESTINATION_NAME];
   char sSrcName[MAX_SOURCE_NAME];
   RoadMapPosition srcPosition;
   RoadMapPosition destPosition;
   int iNumRoutes;
   NavigateRouteResult pRouteResults[MAX_ROUTES];
} AltRouteTrip;

int RealtimeAltRoutes_Count ();
void RealtimeAltRoutes_Init_Record (AltRouteTrip *route);
AltRouteTrip *RealtimeAltRoutes_Get_Record (int index);
BOOL RealtimeAltRoutes_Add_Route (AltRouteTrip *route);
BOOL RealtimeAltRoutes_Route_Request(int iTripId, const RoadMapPosition *from_pos, const RoadMapPosition *to_pos);
NavigateRouteResult *RealtimeAltRoutes_Get_Route_Result(int index);
int RealtimeAltRoutes_Get_Num_Routes();
#endif /* REALTIMEALTROUTES_H_ */
