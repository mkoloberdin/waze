/* RealtimeTrafficInfo.c - Manage real time traffic info
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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


#ifndef	__REALTIME_ROAD_INFO_H__
#define	__REALTIME_TRAFFIC_INFO_H__

#include "../roadmap_plugin.h"

#define RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT 		500
#define RT_TRAFFIC_INFO_ADDRESS_MAXSIZE 	               200
#define RT_TRAFFIC_INFO_MAX_DESCRIPTION		            250
#define RT_TRAFFIC_INFO_MAX_NODES 			                50
#define RT_TRAFFIC_INFO_TILE_FETCH_LIST_MAXSIZE      	16
#define RT_TRAFFIC_INFO_MAX_GEOM 			               200

#define ALERT_ID_OFFSET 100000

#ifdef J2ME
#define RT_TRAFFIC_INFO_MAX_LINES 300
#else
#define RT_TRAFFIC_INFO_MAX_LINES 3000
#endif

#define LIGHT_TRAFFIC	    0
#define MODERATE_TRAFFIC    1
#define HEAVY_TRAFFIC       2
#define STAND_STILL_TRAFFIC 3

//////////////////////////////////////////////////////////////////////////////
//

typedef struct _RTTrafficInfo RTTrafficInfo;

typedef struct
{
	int iSquare;
	int iVersion;
	int iLine;
	int cfcc;
	RoadMapPosition positionFrom;
	RoadMapPosition positionTo;
	RoadMapArea boundingBox;
	int iDirection;
	int iFirstShape;
	int iLastShape;
	int iType;
	int iSpeed;
	int iTrafficInfoId;
	RTTrafficInfo *pTrafficInfo;
	int isInstrumented;
} RTTrafficInfoLines;

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Road Info
struct _RTTrafficInfo
{
    int iID; 			//	Alert ID (within the server)
    int fSpeedMpS; 	// Alowed speed to alert (meters per second)
    int iSpeed; 	// Alowed speed to alert (client speed units)
    int iType; 		//  Alert Type
    int iUserContribution;	// User contibution to creating this traffic alert
    char sStreet [RT_TRAFFIC_INFO_ADDRESS_MAXSIZE+1]; // The Street name
    char sCity		[RT_TRAFFIC_INFO_ADDRESS_MAXSIZE+1]; // The City name
    char sStart	[RT_TRAFFIC_INFO_ADDRESS_MAXSIZE+1]; // The Start name
    char sEnd 	[RT_TRAFFIC_INFO_ADDRESS_MAXSIZE+1]; // The End name

	int iNumGeometryPoints;
	RoadMapPosition geometry[RT_TRAFFIC_INFO_MAX_GEOM];
	RoadMapArea boundingBox;
	
	char sDescription[RT_TRAFFIC_INFO_MAX_DESCRIPTION+1];
};



//////////////////////////////////////////////////////////////////////////////////////////////////
//	Realtime Traffic info table
typedef struct
{
    RTTrafficInfo *pTrafficInfo[RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT];
    int iCount;
} RTTrafficInfos;

typedef struct{
	RTTrafficInfoLines *pRTTrafficInfoLines[RT_TRAFFIC_INFO_MAX_LINES];
	int iCount;
}RTTrafficLines;



void    RTTrafficInfo_InitRecord(RTTrafficInfo *pTrafficInfo);
void    RTTrafficInfo_Init(void);
void    RTTrafficInfo_Term(void);
void    RTTrafficInfo_ClearAll(void);
void    RTTrafficInfo_Reset(void);
BOOL RTTrafficInfo_IsEmpty(void );
BOOL  RTTrafficInfo_Exists  (int    iInfoID);
int      RTTrafficInfo_Count(void);
RTTrafficInfo *RTTrafficInfo_RecordByID(int iInfoID);
BOOL RTTrafficInfo_Add(RTTrafficInfo *pTrafficInfo);
BOOL RTTrafficInfo_Remove(int iID);
int RTTrafficInfo_GetNumLines(void);
RTTrafficInfoLines *RTTrafficInfo_GetLine(int Record);
int RTTrafficInfo_Get_Line(int line, int square,  int against_dir);
RTTrafficInfo *RTTrafficInfo_Get(int index);
int RTTrafficInfo_GetAlertForLine(int iLineid, int iSquareId);
int RTTrafficInfo_Get_Avg_Cross_Time (int line, int square, int against_dir);
int RTTrafficInfo_Get_Avg_Speed(int line, int square, int against_dir);
BOOL RTTrafficInfo_UpdateGeometry(RTTrafficInfo *pTrafficInfo);
BOOL RTTrafficInfo_AddSegments( int iTrafficInfoID, int iSquare, int iVersion, int nLines, int iLines[] );
void RTTrafficInfo_RecalculateSegments();
#endif  //__REALTIME_TRAFFIC_INFO_H__
