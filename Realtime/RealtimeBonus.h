/* RealtimeBonus.h
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


#ifndef __REALTIMEBONUS_H__
#define __REALTIMEBONUS_H__
#include "../roadmap_alerter.h"
#define MAX_ADD_ONS 250
#define MAX_GUID_ID 30

#define BONUS_TYPE_POINTS     0
#define BONUS_TYPE_TREASURE   1
//////////////////////////////////////////////////////////////////////////////////////////////////
// bonus
typedef struct
{
    int  iID;     //   bonus ID (within the server)
    int  iType;   //   bonus Type
    int  iToken;  //   bonus Token
    int  iRadius; //   Radius to collect 
    int  iNumPoints; // Number of bonus points
    RoadMapPosition position;
    char *pIconName; // Name of the icon
    char sGUIID[MAX_GUID_ID+1]; // GUI ID 
    BOOL collected;
    roadmap_alerter_location_info location_info;
} RTBonus;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Runtime Bonus Table
typedef struct
{
   RTBonus *bonus[MAX_ADD_ONS];
   int iCount;
} RTBonusTable;

void RealtimeBonus_Init (void);
void RealtimeBonus_Term (void);

BOOL RealtimeBonus_Add (RTBonus *pbonus);
BOOL RealtimeBonus_Delete (int iId);

int RealtimeBonus_CollectedPointsConfirmed(int iID, int iType, int iPoints, BOOL bHasGift, BOOL bIsBigPrize, const char *gift);
void RealtimeBonus_Record_Init (RTBonus *pbonus);
int   RealtimeBonus_Count (void);
unsigned int RealtimeBonus_Get_Speed (int index);
int   RealtimeBonus_Check_Same_Street(int record);
int   RealtimeBonus_HandleEvent(int iId);
int   RealtimeBonus_Is_Alertable(int index);
int   RealtimeBonus_Get_Id (int index);
int   RealtimeBonus_Get_Type (int iId);
int   RealtimeBonus_Get_Token (int iId);
void  RealtimeBonus_Get_Position(int record, RoadMapPosition *position, int *steering);
int   RealtimeBonus_Get_Distance (int index);
int   RealtimeBonus_Get_NumberOfPoints (int iId);
const char *RealtimeBonus_Get_Icon_Name (int iId);
BOOL RealtimeBonus_is_square_dependent(void);
int RealtimeBonus_get_priority(void);
roadmap_alerter_location_info *  RealtimeBonus_get_location_info(int index);
BOOL RealtimeBonus_distance_check(RoadMapPosition gps_pos);

#endif /* __REALTIMEBONUS_H__ */
