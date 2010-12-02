/* RealtimeExternalPoiNotifier.h - Manage External POIs Notofications
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

#ifndef REALTIMEEXTERNALPOINOTIFIER_H_
#define REALTIMEEXTERNALPOINOTIFIER_H_

void RealtimeExternalPoiNotifier_NotifyOnPopUp(int iID);
void RealtimeExternalPoiNotifier_NotifyOnNavigate(int iID);

void RealtimeExternalPoiNotifier_DisplayedList_clear(void);
void RealtimeExternalPoiNotifier_DisplayedList_Init(void);
void RealtimeExternalPoiNotifier_DisplayedList_add_ID(int ID);
BOOL RealtimeExternalPoiNotifier_DisplayedList_IsEmpty();
int  RealtimeExternalPoiNotifier_DisplayedList_Count();
int  RealtimeExternalPoi_DisplayedList_get_ID(int index);

#endif /* REALTIMEEXTERNALPOINOTIFIER_H_ */
