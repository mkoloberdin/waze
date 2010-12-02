/* RealtimeExternalPoi.h - Manage External POIs
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

#ifndef REALTIMEEXTERNALPOI_H_
#define REALTIMEEXTERNALPOI_H_

#define MAX_URL_LENGTH  128
#define MAX_ICON_LENGHT 128


#define  RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT    500
#define  RT_MAXIMUM_EXTERNAL_POI_TYPES                100

#define EXTERNAL_POI_SIZE_SMALL  1
#define EXTERNAL_POI_SIZE_MEDIUM 2
#define EXTERNAL_POI_SIZE_LARGE  3


typedef struct
{
    int  iID;        //  POI Type ID
    int  iExternalPoiServiceID; // Service ID
    int  iExternalPoiProviderID; // Provider ID
    char cBigIcon[MAX_ICON_LENGHT];
    char cSmallIcon[MAX_ICON_LENGHT];
    char cOnClickUrl[MAX_URL_LENGTH];
    int  iSize;
    int  iMaxDisplayZoomBigIcon;
    int  iMaxDisplayZoomSmallIcon;
    int  iPromotionType;
    int  iIsNavigable;
} RTExternalPoiType;

typedef struct
{
    int  iID;        //  POI ID
    int  iTypeID;
    int  iServerID;  //  Entity ID (within the server)
    int  iProviderID;
    int  iServiceID;
    RTExternalPoiType *ExternalPoiType;   // POI Type
    int  iLongitude; //  Entity location:  Longtitue
    int  iLatitude; //   Entity location:  Latitude
    int  iCreationTime;
    int  iUpdateTime;
    int  iPromotionType;
    int  iIsPromotioned;
    char cResourceUrlParams[MAX_URL_LENGTH];
    BOOL isDiplayed;
} RTExternalPoi;


void RealtimeExternalPoi_Init(void);
void RealtimeExternalPoi_Term(void);
void RealtimeExternalPoi_Reset(void);

void RealtimeExternalPoi_ExternalPoiType_Init(RTExternalPoiType *pEntity);
void RealtimeExternalPoi_ExternalPoi_Init(RTExternalPoi *pEntity);
BOOL RealtimeExternalPoi_ExternalPoiType_Add(RTExternalPoiType *pEntity);

void RealtimeExternalPoi_ExternalPoi_Init(RTExternalPoi *pEntity);
int  RealtimeExternalPoi_ExternalPoiType_Count(void);
BOOL RealtimeExternalPoi_ExternalPoiType_IsEmpty(void);
BOOL RealtimeExternalPoi_ExternalPoiType_Add(RTExternalPoiType *pEntity);

int  RealtimeExternalPoi_ExternalPoi_Count(void);
BOOL RealtimeExternalPoi_ExternalPoi_IsEmpty(void);
BOOL RealtimeExternalPoi_ExternalPoi_Add(RTExternalPoi *pEntity);
BOOL RealtimeExternalPoi_ExternalPoi_Remove(int iID);
RTExternalPoi * RealtimeExternalPoi_ExternalPoi_GetById(int iID);
int RealtimeExternalPoi_ExternalPoi_GetIndexById(int iID);

void RealtimeExternalPoi_DisplayList(void);
int  RealtimeExternalPoi_DisplayList_Count();
BOOL RealtimeExternalPoi_DisplayList_IsEmpty();
void RealtimeExternalPoi_DisplayList_clear();
void RealtimeExternalPoi_DisplayList_add_ID(int ID);
int  RealtimeExternalPoi_DisplayList__get_ID(int index);

const char *RealtimeExternalPoi_GetUrl(void);

#endif /* REALTIMEEXTERNALPOI_H_ */
