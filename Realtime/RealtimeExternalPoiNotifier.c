/* RealtimeExternalPoiNotifier.c - Manage External POIs Notofications
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "RealtimeExternalPoi.h"
#include "Realtime.h"

typedef struct {
   int entitiesID[RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT];
   int iCount;
} RTExternalPoiDisplayedList;

RTExternalPoiDisplayedList   gExternalPoisDisplayedList;



/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoiNotifier_DisplayedList_Count() {
   return gExternalPoisDisplayedList.iCount;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_DisplayedList_get_ID(int index) {
   return gExternalPoisDisplayedList.entitiesID[index];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoiNotifier_DisplayedList_IsEmpty() {
   return (gExternalPoisDisplayedList.iCount == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiNotifier_DisplayedList_clear() {
   memset(&gExternalPoisDisplayedList, 0, sizeof(gExternalPoisDisplayedList));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiNotifier_DisplayedList_Init(){
   RealtimeExternalPoi_DisplayList_clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiNotifier_DisplayedList_add_ID(int ID) {
   if (gExternalPoisDisplayedList.iCount < RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT)
      gExternalPoisDisplayedList.entitiesID[gExternalPoisDisplayedList.iCount++] = ID;
   else{
      // Table is Full. Send List to the server.
      roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoiNotifier_DisplayedList_add_ID  Display list is full ");
      SendMessage_ExternalPoiDisplayed(NULL);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiNotifier_NotifyOnPopUp(int iID){
   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoiNotifier NotifyOnPopUp - external POI DB id =%d",iID);
   Realtime_ExternalPoiNotifyOnPopUp(iID);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiNotifier_NotifyOnNavigate(int iID){
   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoiNotifier NotifyOnNavigate - external POI DB =id =%d",iID);
   Realtime_ExternalPoiNotifyOnNavigate(iID);
}
