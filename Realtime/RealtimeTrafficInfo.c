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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap_main.h"
#include "../roadmap_factory.h"
#include "../roadmap_config.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_types.h"
#include "../roadmap_lang.h"
#include "../roadmap_trip.h"
#include "../roadmap_line.h"
#include "../roadmap_res.h"
#include "../roadmap_layer.h"
#include "../roadmap_square.h"
#include "../roadmap_locator.h"
#include "../roadmap_line_route.h"
#include "../roadmap_street.h"
#include "../roadmap_math.h"
#include "../roadmap_navigate.h"
#include "../editor/editor_points.h"
#include "../roadmap_ticker.h"

#include "roadmap_tile.h"
#include "roadmap_tile_manager.h"
#include "roadmap_tile_status.h"
#include "roadmap_hash.h"
#include "Realtime.h"
#include "RealtimeNet.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeAlerts.h"
#include "RealtimeTrafficInfoPlugin.h"

#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_confirm_dialog.h"

static RTTrafficInfos gTrafficInfoTable;
static RTTrafficLines gRTTrafficInfoLinesTable;
static RoadMapTileCallback 		TileCbNext = NULL;
static RoadMapUnitChangeCallback sNextUnitChangeCb = NULL;

#define MAX_POINTS 100

static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo);
static BOOL RTTrafficInfo_DeleteAlert(int iID);
static void RTTrafficInfo_TileReceivedCb( int tile_id );
static BOOL RTTrafficInfo_InstrumentSegment(int iLine);
static void RTTrafficInfo_TileRequest( int tile_id, int version );
static void RTTrafficInfo_UnitChangeCb (void);

 /**
 * Initialize the Traffic info structure
 * @param pTrafficInfo - pointer to the Traffic info
 * @return None
 */
void RTTrafficInfo_InitRecord(RTTrafficInfo *pTrafficInfo)
{
	pTrafficInfo->iID            = -1;
	pTrafficInfo->iType        = -1;
	pTrafficInfo->iSpeed      = 0;
	pTrafficInfo->sCity[0]    =  0;
	pTrafficInfo->sStreet[0] =  0;
	pTrafficInfo->sEnd[0]    =  0;
	pTrafficInfo->sStart[0]   = 0;
	pTrafficInfo->sDescription[0] = 0;
	pTrafficInfo->iNumGeometryPoints = 0;
	pTrafficInfo->iUserContribution = 0;
	pTrafficInfo->boundingBox.east = 0;
	pTrafficInfo->boundingBox.north = 0;
	pTrafficInfo->boundingBox.west = 0;
	pTrafficInfo->boundingBox.south = 0;
	
}

/**
 * Initialize the Traffic Info
 * @param None
 * @return None
 */
void RTTrafficInfo_Init()
{
	int i;
	
	roadmap_log (ROADMAP_DEBUG, "RTTrafficInfo_Init()");
	
	gTrafficInfoTable.iCount = 0;
	for (i=0; i< RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT; i++){
		gTrafficInfoTable.pTrafficInfo[i] = NULL;
	}

   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0;i <RT_TRAFFIC_INFO_MAX_LINES; i++){
		gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = NULL;
	}

   TileCbNext = roadmap_tile_register_callback( RTTrafficInfo_TileReceivedCb );

   RealtimeTrafficInfoPluginInit();
   sNextUnitChangeCb = roadmap_math_register_unit_change_callback (RTTrafficInfo_UnitChangeCb);
}

/**
 * Terminate the TrafficInfo
 * @param None
 * @return None
 */
void RTTrafficInfo_Term()
{
	RTTrafficInfo_ClearAll();
	RealtimeTrafficInfoPluginTerm();
}

/**
 * Reset the TrafficInfo table
 * @param None
 * @return None
 */
void RTTrafficInfo_Reset()
{
   RTTrafficInfo_ClearAll();
}

/**
 * Clears all entries in the TrafficInfo table
 * @param None
 * @return None
 */
void RTTrafficInfo_ClearAll()
{
   int i;
   RTTrafficInfo *pRTTrafficInfo;
   int count;

	roadmap_log (ROADMAP_DEBUG, "RTTrafficInfo_ClearAll()");
	
   count = gTrafficInfoTable.iCount;
   gTrafficInfoTable.iCount = 0;
   for( i=0; i< RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT; i++)
   {
      pRTTrafficInfo = gTrafficInfoTable.pTrafficInfo[i];
      if (i < count) {
       	RTTrafficInfo_DeleteAlert(pRTTrafficInfo->iID);
      	free(pRTTrafficInfo);
      }
      gTrafficInfoTable.pTrafficInfo[i] = NULL;
   }

	count = gRTTrafficInfoLinesTable.iCount;
   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0; i<RT_TRAFFIC_INFO_MAX_LINES;i++)
   {
   	if (i < count) {
   		free (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]);
   	}
   	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = NULL;
   }

}

/**
 * Checks whether the TrafficInfo table is empty
 * @param None
 * @return TRUE if the table is empty. FALSE, otherwise
 */
BOOL RTTrafficInfo_IsEmpty( )
{ return (0 == gTrafficInfoTable.iCount);}


/**
 * The number of TrafficInfo Record currently in the table
 * @param None
 * @return the number of Records
 */
int RTTrafficInfo_Count(void)
{
    return gTrafficInfoTable.iCount;
}

/**
 * Retrieves a TrafficInfo record by its ID
 * @param iInfoID - the id of the traffic info
 * @return a pointer to the roud inf, NULL if not found
 */
RTTrafficInfo *RTTrafficInfo_RecordByID(int iInfoID){
	 int i;

    for (i=0; i< gTrafficInfoTable.iCount; i++)
        if (gTrafficInfoTable.pTrafficInfo[i]->iID == iInfoID)
        {
            return (gTrafficInfoTable.pTrafficInfo[i]);
        }

    return NULL;
}

/**
 * Checks whether a traffic info record exists
 * @param iInfoID - the id of the traffic info
 * @return TRUE, the record exists. FALSE, the record does not exists
 */
BOOL  RTTrafficInfo_Exists  (int    iInfoID)
{
   if (NULL == RTTrafficInfo_RecordByID(iInfoID))
      return FALSE;

   return TRUE;
}

/**
 * Generate an Alert from TrafficInfo
  * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo)
{
	RTAlert alert;
	int speed;
	int index;

	if (pTrafficInfo->iNumGeometryPoints < 1)
		return FALSE;
		
	RTAlerts_Alert_Init(&alert);

	alert.iID = pTrafficInfo->iID +  ALERT_ID_OFFSET;

	alert.iType = RT_ALERT_TYPE_TRAFFIC_INFO;
	alert.iSubType = pTrafficInfo->iType;
	alert.iSpeed = pTrafficInfo->iSpeed;

	strncpy_safe(alert.sLocationStr, pTrafficInfo->sDescription,RT_ALERT_LOCATION_MAX_SIZE);

	speed = pTrafficInfo->iSpeed;
	sprintf(alert.sDescription, roadmap_lang_get("Average speed %d %s"), speed,  roadmap_lang_get(roadmap_math_speed_unit()) );
	alert.iDirection = RT_ALERT_MY_DIRECTION;
	alert.iReportTime = (int)time(NULL);

	index = pTrafficInfo->iNumGeometryPoints / 2;
	alert.bAlertByMe = FALSE;
	alert.iLatitude = pTrafficInfo->geometry[index].latitude;
	alert.iLongitude = pTrafficInfo->geometry[index].longitude;

	return RTAlerts_Add(&alert);

}

/**
 * Remove a Alert that was generated from TrafficInfo
 * @param iID - Id of the TrafficInfo
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */
static BOOL RTTrafficInfo_DeleteAlert(int iID)
{
	return RTAlerts_Remove(iID + ALERT_ID_OFFSET);
}




/**
 * Add a TrafficInfo Segment to list of segments
 * @param iTrafficInfoID - ID of the TrafficInfo
 * @param iSquare - tile ID of all segments
 * @param iVersion - required tile version for segment validity
 * @param nLines - number of segments to add
 * @param iLines - segments' line ids
 * @param iDirections - segment directions
 * @return TRUE operation was successful
 */
BOOL RTTrafficInfo_AddSegments( int iTrafficInfoID, int iSquare, int iVersion, int nLines, int iLines[] ){
	int i;
	BOOL added = FALSE;
	BOOL needTile = FALSE;
	RTTrafficInfoLines *pLine;

	/*
	 * locate traffic info
	 */
	RTTrafficInfo *pTrafficInfo = RTTrafficInfo_RecordByID (iTrafficInfoID);
	if (pTrafficInfo == NULL)
	{
		roadmap_log (ROADMAP_ERROR, "Trying to add segments for invalid traffic info id %d", pTrafficInfo);
		return FALSE;
	}
	
	for (i = 0; i < nLines; i++)
	{
		int index = gRTTrafficInfoLinesTable.iCount;
		if (gRTTrafficInfoLinesTable.iCount >= RT_TRAFFIC_INFO_MAX_LINES)
		{
			roadmap_log (ROADMAP_WARNING, "Too many traffic info segments");
			break;
		}
		gRTTrafficInfoLinesTable.iCount++;
		
		if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[index] == NULL)
		{
			gRTTrafficInfoLinesTable.pRTTrafficInfoLines[index] = malloc( sizeof(RTTrafficInfoLines));
		}
		pLine = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[index];
		
		pLine->iSquare = iSquare;
		pLine->iVersion = iVersion;
		if (iLines[i] >= 0)
		{
			pLine->iLine = iLines[i];
			pLine->iDirection = ROUTE_DIRECTION_WITH_LINE;
		}
		else
		{
			pLine->iLine = -iLines[i] - 1;
			pLine->iDirection = ROUTE_DIRECTION_AGAINST_LINE;
		}
		pLine->iType = pTrafficInfo->iType;
		pLine->iSpeed = pTrafficInfo->iSpeed;
		pLine->iTrafficInfoId = iTrafficInfoID;
		pLine->pTrafficInfo = pTrafficInfo;
		if (!RTTrafficInfo_InstrumentSegment (index))
			needTile = TRUE;
		added = TRUE;
	}
	
	if (needTile)
	{
		// register for tile updates
		RTTrafficInfo_TileRequest (iSquare, iVersion);
	}
	
	return added;
}

/**
 * Remove a TrafficInfo Segment from the list of segments
 * @param iTrafficInfoID - The pointer to the traffic ID
 * @return TRUE operation was successful
 */
static BOOL RTTraficInfo_DeleteSegments(int iTrafficInfoID){
	 int i = 0;
	 BOOL found = FALSE;
	 RTTrafficInfoLines *tmp;

	 //   Are we empty?
    if ( 0 == gRTTrafficInfoLinesTable.iCount)
        return FALSE;

    while (i< gRTTrafficInfoLinesTable.iCount){
    	if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iTrafficInfoId == iTrafficInfoID){
    		gRTTrafficInfoLinesTable.iCount--;
    		tmp = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i];
    		gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[gRTTrafficInfoLinesTable.iCount];
    		gRTTrafficInfoLinesTable.pRTTrafficInfoLines[gRTTrafficInfoLinesTable.iCount] = tmp;
    		found = TRUE;
    	}
    	else
    		i++;
    }

	return found;
}

typedef struct traffic_info_context_st{
   int iTrafficInfoId;
   int iUserContribution;
}traffic_info_context;


void TrafficConfirmedCallback(int exit_code, void *context){
   traffic_info_context *traffic_info = (traffic_info_context *)context;
   int iTrafficInfoId = traffic_info->iTrafficInfoId;
   int iUserContribution = traffic_info->iUserContribution;

   if (iUserContribution > MAX_POINTS)
      return;

   if (exit_code == dec_yes){
      Realtime_TrafficAlertFeedback(iTrafficInfoId, 100);
      editor_points_add_new_points(iUserContribution);
      editor_points_display_new_points_timed(iUserContribution, 5,confirm_event);
   }
   else if (exit_code == dec_no){
      Realtime_TrafficAlertFeedback(iTrafficInfoId, 0);
      editor_points_add_new_points(iUserContribution);
      editor_points_display_new_points_timed(iUserContribution, 5,confirm_event);
      RTTrafficInfo_Remove(iTrafficInfoId);
   }
   free( context );
}

static void OnUserContribution(int iUserContribution, int iTrafficInfoId){

   traffic_info_context *context;
   RoadMapGpsPosition pos;
   static BOOL ask_once = FALSE;

   roadmap_navigate_get_current(&pos, NULL, NULL);
   if (pos.speed > 20)
       return;

   if (ask_once)
      return;

   editor_points_add_new_points(iUserContribution);
   editor_points_display_new_points_timed(iUserContribution, 6, user_contribution_event);
   context = ( traffic_info_context* ) malloc( sizeof( traffic_info_context ) );
   context->iTrafficInfoId = iTrafficInfoId;
   context->iUserContribution = iUserContribution;

   ssd_confirm_dialog_timeout("Traffic detected","Are you experiencing traffic?",TRUE, TrafficConfirmedCallback, context ,10);
   ask_once = TRUE;
}

/**
 * Add a TrafficInfo element to the list of the Traffic Info table
 * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
BOOL RTTrafficInfo_Add(RTTrafficInfo *pTrafficInfo)
{

    // Full?
    if ( RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT == gTrafficInfoTable.iCount )
        return FALSE;

    // Already exists?
    if (RTTrafficInfo_Exists(pTrafficInfo->iID))
    {
        roadmap_log( ROADMAP_INFO, "RTTrafficInfo_Add - traffic_info record (%d) already exist", pTrafficInfo->iID);
        return TRUE;
    }

	if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] == NULL) { 
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = calloc(1, sizeof(RTTrafficInfo));
	   if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] == NULL)
	   {
	   	roadmap_log( ROADMAP_ERROR, "RTTrafficInfo_Add - cannot add traffic_info  (%d) calloc failed", pTrafficInfo->iID);
	      return FALSE;
	   }
	}
	
   RTTrafficInfo_InitRecord(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]);
	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iID = pTrafficInfo->iID;
	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->fSpeedMpS = pTrafficInfo->fSpeedMpS;
	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iSpeed = (int)(roadmap_math_meters_p_second_to_speed_unit( (float)pTrafficInfo->fSpeedMpS)+0.5F);
	switch (pTrafficInfo->iType){
		case 0:
		case 1:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = LIGHT_TRAFFIC;
			break;
		case 2:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = MODERATE_TRAFFIC;
			break;
		case 3:
         gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = HEAVY_TRAFFIC;
         break;
		case 4:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = STAND_STILL_TRAFFIC;
			break;
	}


	 strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity, pTrafficInfo->sCity, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, pTrafficInfo->sStreet, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, pTrafficInfo->sStart, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd, pTrafficInfo->sEnd, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);


    if ((gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet[0] != 0) & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity[0] != 0) ){
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,"%s, %s", gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity );
    }
    else if ((gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet[0] != 0) & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart[0] != 0)  & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd[0] != 0) ){
    	if (!strcmp(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd))
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,roadmap_lang_get("%s in the neighborhood of %s"), gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart );
    	else
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,roadmap_lang_get("%s between %s and %s"), gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd );
    }

    gTrafficInfoTable.iCount++;

    if (pTrafficInfo->iUserContribution != 0){
       OnUserContribution(pTrafficInfo->iUserContribution, pTrafficInfo->iID);
    }

    return TRUE;
}

/**
 * Remove a TrafficInfo Record from table
 * @param iID - Id of the TrafficInfo
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */
BOOL RTTrafficInfo_Remove(int iID)
{
   int i;
   RTTrafficInfo *tmp;

	for (i = 0; i < gTrafficInfoTable.iCount; i++)
	{
		if (gTrafficInfoTable.pTrafficInfo[i]->iID != iID) continue;

      gTrafficInfoTable.iCount--;
      
      // preserve the allocated space for later use
		tmp = gTrafficInfoTable.pTrafficInfo[i];
		gTrafficInfoTable.pTrafficInfo[i] = gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount];
		gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = tmp;

#ifndef J2ME
      RTTraficInfo_DeleteSegments(iID);
#endif
      RTTrafficInfo_DeleteAlert (iID);
      
      return TRUE;
	}

	return FALSE;
}


/**
 * Update a traffic info record after the geometry has been updated
 * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
BOOL RTTrafficInfo_UpdateGeometry(RTTrafficInfo *pTrafficInfo)
{
	int i;
	
	if (pTrafficInfo->iNumGeometryPoints < 1)
	{
		roadmap_log (ROADMAP_ERROR, "Cannot update geometry with no coordinates - ID = %d", pTrafficInfo->iID);
		return FALSE;
	}
	
	// Calculate bounding box
	pTrafficInfo->boundingBox.east = pTrafficInfo->geometry[0].longitude;
	pTrafficInfo->boundingBox.west = pTrafficInfo->geometry[0].longitude;
	pTrafficInfo->boundingBox.north = pTrafficInfo->geometry[0].latitude;
	pTrafficInfo->boundingBox.south = pTrafficInfo->geometry[0].latitude;
	for (i = 1; i < pTrafficInfo->iNumGeometryPoints; i++)
	{
		if (pTrafficInfo->geometry[i].longitude > pTrafficInfo->boundingBox.east)
		{
			pTrafficInfo->boundingBox.east = pTrafficInfo->geometry[i].longitude;
		}
		else if (pTrafficInfo->geometry[i].longitude < pTrafficInfo->boundingBox.west)
		{
			pTrafficInfo->boundingBox.west = pTrafficInfo->geometry[i].longitude;
		}
		
		if (pTrafficInfo->geometry[i].latitude > pTrafficInfo->boundingBox.north)
		{
			pTrafficInfo->boundingBox.north = pTrafficInfo->geometry[i].latitude;
		}
		else if (pTrafficInfo->geometry[i].latitude < pTrafficInfo->boundingBox.south)
		{
			pTrafficInfo->boundingBox.south = pTrafficInfo->geometry[i].latitude;
		}
	}
	
	return RTTrafficInfo_GenerateAlert (pTrafficInfo);
}

/**
 * Get a traffic info record.
 * @param index - the index of the record
 * @return pointer to the traffic info record at that index.
 */
RTTrafficInfo *RTTrafficInfo_Get(int index){
	if ((index >= RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT) || (index < 0))
		return NULL;
	return gTrafficInfoTable.pTrafficInfo[index];
}


/**
 * Get the number of lines in the lines table
 * @None
 * @return Number of lines in the lines table
 */
int RTTrafficInfo_GetNumLines(){
	return gRTTrafficInfoLinesTable.iCount;
}

/**
 * Get a line from the lines table
 * @param Record - index of th record
 * @return pointer to Line info from Lines table
 */
RTTrafficInfoLines *RTTrafficInfo_GetLine(int Record)
{
	if ((Record >= RT_TRAFFIC_INFO_MAX_LINES) || (Record < 0))
		return NULL;

	return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[Record];
}

/**
 * Find a line from the lines table
  * @param line - line id
 * @param square - the square of the line
 * @return Index of line in the LInes table, -1 if no lines is found
 */
 int RTTrafficInfo_Get_Line(int line, int square,  int against_dir){
	int i;
	int direction;

	if (gRTTrafficInfoLinesTable.iCount == 0)
		return -1;

	if (against_dir)
		direction = ROUTE_DIRECTION_AGAINST_LINE;
	else
		direction = ROUTE_DIRECTION_WITH_LINE;

	for (i = 0; i < gRTTrafficInfoLinesTable.iCount; i++){
		if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->isInstrumented &&
			 gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iLine == line &&
			 gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iDirection == direction &&
			 gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iSquare == square)
			return i;
	}

	return -1;
}

/**
 * Finds the index in the lines tables corresponsinf to a line and square
 * @param line - line id
 * @param square - the square of the line
 * @return the line_id if line is found in the lines table, -1 otherwise
 */
static int RTTrafficInfo_Get_LineNoDirection(int line, int square){
	int i;

	if (gRTTrafficInfoLinesTable.iCount == 0)
		return -1;

	for (i = 0; i < gRTTrafficInfoLinesTable.iCount; i++){
		if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->isInstrumented &&
			 gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iLine == line &&
			 gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iSquare == square)
			return i;
	}

	return -1;
}

/**
 * Get The Average speed of a line
 * @param line - line id
 * @param against_dir
 * @return Averge real  time speed if exist, 0 otherwise
 */
int RTTrafficInfo_Get_Avg_Speed(int line, int square, int against_dir){

	int lineRecord = RTTrafficInfo_Get_Line(line, square, against_dir);
	if (lineRecord != -1){
		return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[lineRecord]->iSpeed;
	}
	else
		return 0;
}

/**
 * Get The Average cross time of a line
 * @param line - line id
 * @param against_dir
 * @return Averge real  time cross time if exist, 0 otherwise
 */
int RTTrafficInfo_Get_Avg_Cross_Time (int line, int square, int against_dir) {

   int speed;
   int length, length_m;

	speed = RTTrafficInfo_Get_Avg_Speed(line, square, against_dir);
	if (speed ==0)
		return 0;

   length = roadmap_line_length (line);

   length_m = roadmap_math_to_cm(length) / 100;

   return (int)(length_m*3.6  / speed) + 1;

}

/**
 * Returns the alertId corresponding to a line
 * @param line - line id
 * @param square - the square of the line
 * @return the alert id If line_id has alert, -1 otherwise
 */
int RTTrafficInfo_GetAlertForLine(int iLineid, int iSquareId){

	int lineRecord = RTTrafficInfo_Get_LineNoDirection(iLineid, iSquareId);
	if (lineRecord != -1){
		return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[lineRecord]->iTrafficInfoId + ALERT_ID_OFFSET;
	}
	else
		return -1;
}

/**
 * Load segment properties from tile
 * @param iLine - line index in table
 * @return TRUE is successful
 */
static BOOL RTTrafficInfo_InstrumentSegment(int iLine) {
	
	RTTrafficInfoLines *segment = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLine];
	int tileVersion = roadmap_square_version (segment->iSquare);
	int i;
	
	if (tileVersion == 0 ||
		 (segment->iVersion > 0 && tileVersion != segment->iVersion)) {
		 	
		segment->isInstrumented = FALSE;
		return FALSE;	 	
	} 
	
	roadmap_square_set_current (segment->iSquare);
	roadmap_line_shapes (segment->iLine, &(segment->iFirstShape), &(segment->iLastShape));
	roadmap_line_from (segment->iLine, &(segment->positionFrom)); 	
	roadmap_line_to (segment->iLine, &(segment->positionTo));
	// Fill bounding box
	if (segment->positionFrom.longitude < segment->positionTo.longitude) {
		segment->boundingBox.east = segment->positionTo.longitude;
		segment->boundingBox.west = segment->positionFrom.longitude;
	} else {
		segment->boundingBox.west = segment->positionTo.longitude;
		segment->boundingBox.east = segment->positionFrom.longitude;
	}
	if (segment->positionFrom.latitude < segment->positionTo.latitude) {
		segment->boundingBox.north = segment->positionTo.latitude;
		segment->boundingBox.south = segment->positionFrom.latitude;
	} else {
		segment->boundingBox.south = segment->positionTo.latitude;
		segment->boundingBox.north = segment->positionFrom.latitude;
	}
	if (segment->iFirstShape >= 0) {
		RoadMapPosition pos = segment->positionFrom;
		for (i = segment->iFirstShape; i <= segment->iLastShape; i++) {
			roadmap_shape_get_position (i, &pos);
			if (pos.longitude < segment->boundingBox.west) {
				segment->boundingBox.west = pos.longitude;
			} else if (pos.longitude > segment->boundingBox.east) {
				segment->boundingBox.east = pos.longitude;
			}
			if (pos.latitude < segment->boundingBox.south) {
				segment->boundingBox.south = pos.latitude;
			} else if (pos.latitude > segment->boundingBox.north) {
				segment->boundingBox.north = pos.latitude;
			}
		}
	}
	segment->cfcc = roadmap_line_cfcc (segment->iLine);
	segment->isInstrumented = TRUE;
	return TRUE; 	
}

/**
 * Reinstrument all lines
 * @param None
 * @return None
 */
void RTTrafficInfo_RecalculateSegments(){
	int i;

	for (i=0;i<gRTTrafficInfoLinesTable.iCount; i++){
		RTTrafficInfo_InstrumentSegment ( i );
	}
}

/**
 * Instrument all lines of a specific tile
 * @param None
 * @return None
 */
void RTTrafficInfo_InstrumentSegments(int square){
	int i;

	for (i=0;i<gRTTrafficInfoLinesTable.iCount; i++){
		if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iSquare == square)
			RTTrafficInfo_InstrumentSegment ( i );
	}
}

/**
* Callback called when a new tile is received
* @param tile_id - id of received tile
* @return None
*/
static void RTTrafficInfo_TileReceivedCb( int tile_id )
{
	// Request from the tile server
	int* tile_status = roadmap_tile_status_get ( tile_id );

	if ( *tile_status & ROADMAP_TILE_STATUS_CALLBACK_RT_TRAFFIC )
	{
		RTTrafficInfo_InstrumentSegments ( tile_id );
	}

	
	/* Next callback in chain */
	if ( TileCbNext )
	{
		TileCbNext( tile_id );
	}
}

/**
* Check if a tile needs fetched for segment instrumentation
* @param tile_id - id of the tile
* @param version - required tile version
* @return None
*/
static void RTTrafficInfo_TileRequest( int tile_id, int version )
{
	int tileVersion = roadmap_square_version ( tile_id );
	
	if ( tileVersion == 0 || ( tileVersion < version ) )
	{
		int* tile_status = roadmap_tile_status_get ( tile_id );
		*tile_status = (ROADMAP_TILE_STATUS_FLAG_CALLBACK | ROADMAP_TILE_STATUS_CALLBACK_RT_TRAFFIC | *tile_status ) &
								~ROADMAP_TILE_STATUS_FLAG_UPTODATE;
	}
}


/**
* Callback called when measurement units are changed
* @return None
*/
static void RTTrafficInfo_UnitChangeCb (void)
{
	int i;
	
	for (i = 0; i < gTrafficInfoTable.iCount; i++)
	{
		gTrafficInfoTable.pTrafficInfo[i]->iSpeed = (int)(roadmap_math_meters_p_second_to_speed_unit( (float)gTrafficInfoTable.pTrafficInfo[i]->fSpeedMpS)+0.5F);
	}	
	
	if (sNextUnitChangeCb != NULL)
		sNextUnitChangeCb ();
}


