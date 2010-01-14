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

#define MAX_POINTS 100
#define TRAFFIC_INFO_TILE_MAP_SIZE     			1024
#define TRAFFIC_INFO_TILE_MAP_NAME     			"Traffic Info Tile Map"

typedef struct
{
	RTTrafficInfo *pTrafficInfo;			// Traffic data
	int tiles_fetch_list[RT_TRAFFIC_INFO_TILE_FETCH_LIST_MAXSIZE];
	int tiles_fetch_list_count;
} RTTrafficInfoContext;

/*
 * The queue of the traffic info waiting for the tiles
 * If some of the tiles for the traffic is unavailable,
 * it is added to this queue and removed from it when the
 * unavailable_tiles_count = 0
 */

static int sgTrafficInfoTileMapSize = 0;		//	The actual element count in the map
static int sgTrafficInfoTileMapIndex = 0;		//  The map running index
static RoadMapHash* sgTrafficInfoTileMap;



static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo, int iNodeNumber);
static BOOL RTTrafficInfo_DeleteAlert(int iID);
static void RTTrafficInfo_TileMapClear( void );
static void RTTrafficInfo_TileMapReceivedTileCb( int tile_id );
static void RTTrafficInfo_TileMapReset( void );
static BOOL RTTrafficInfo_AddTileToFetchList( RTTrafficInfoContext* pTrafficInfoCtx, int tile_id );
static BOOL RTTrafficInfo_AddFetchListToMap( RTTrafficInfoContext* pTrafficInfoCtx );
static void RTTrafficInfo_RemoveTileMapTraffic( RTTrafficInfo *pTrafficInfo );

 /**
 * Initialize the Traffic info structure
 * @param pTrafficInfo - pointer to the Traffic info
 * @return None
 */
void RTTrafficInfo_InitRecord(RTTrafficInfo *pTrafficInfo)
{
	int i;

	pTrafficInfo->iID            = -1;
	pTrafficInfo->iType        = -1;
	pTrafficInfo->fSpeed      = 0.0L;
	pTrafficInfo->sCity[0]    =  0;
	pTrafficInfo->sStreet[0] =  0;
	pTrafficInfo->sEnd[0]    =  0;
	pTrafficInfo->sStart[0]   = 0;
	pTrafficInfo->sDescription[0] = 0;
	pTrafficInfo->iNumNodes = 0;
	pTrafficInfo->iDirection = -1;
	pTrafficInfo->iUserContribution = 0;
	for ( i=0; i<RT_TRAFFIC_INFO_MAX_NODES;i++){
		pTrafficInfo->sNodes[i].iNodeId = -1;
		pTrafficInfo->sNodes[i].Position.latitude    = -1;
		pTrafficInfo->sNodes[i].Position.longitude = -1;
	}
	pTrafficInfo->iFetchListCount = 0;
}

/**
 * Initialize the Traffic Info
 * @param None
 * @return None
 */
void RTTrafficInfo_Init()
{
	int i;
	gTrafficInfoTable.iCount = 0;
	for (i=0; i< RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT; i++){
		gTrafficInfoTable.pTrafficInfo[i] = NULL;
	}

   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0;i <MAX_LINES; i++){
		gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = NULL;
	}

   sgTrafficInfoTileMap = roadmap_hash_new( TRAFFIC_INFO_TILE_MAP_NAME, TRAFFIC_INFO_TILE_MAP_SIZE );

   TileCbNext = roadmap_tile_register_callback( RTTrafficInfo_TileMapReceivedTileCb );

   RealtimeTrafficInfoPluginInit();
}

/**
 * Terminate the TrafficInfo
 * @param None
 * @return None
 */
void RTTrafficInfo_Term()
{
	RTTrafficInfo_ClearAll();
	RTTrafficInfo_TileMapClear();
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

   RTTrafficInfo_TileMapReset();

   count = gTrafficInfoTable.iCount;
   gTrafficInfoTable.iCount = 0;
   for( i=0; i< count; i++)
   {
      pRTTrafficInfo = gTrafficInfoTable.pTrafficInfo[i];
       RTTrafficInfo_DeleteAlert(pRTTrafficInfo->iID);
      free(pRTTrafficInfo);
      gTrafficInfoTable.pTrafficInfo[i] = NULL;
   }

   count = gRTTrafficInfoLinesTable.iCount;
   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0; i<count;i++)
   {
   	free (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]);
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
static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo, int iNodeNumber)
{
	RTAlert alert;
	int speed;

	RTAlerts_Alert_Init(&alert);

	alert.iID = pTrafficInfo->iID +  ALERT_ID_OFFSET;

	alert.iType = RT_ALERT_TYPE_TRAFFIC_INFO;
	alert.iSubType = pTrafficInfo->iType;
	alert.iSpeed = (int)(roadmap_math_meters_p_second_to_speed_unit( (float)pTrafficInfo->fSpeed)+0.5F);

	strncpy_safe(alert.sLocationStr, pTrafficInfo->sDescription,RT_ALERT_LOCATION_MAX_SIZE);

	speed = (int)(roadmap_math_meters_p_second_to_speed_unit((float)pTrafficInfo->fSpeed));
	sprintf(alert.sDescription, roadmap_lang_get("Average speed %d %s"), speed,  roadmap_lang_get(roadmap_math_speed_unit()) );
	alert.iDirection = RT_ALERT_MY_DIRECTION;
	alert.iReportTime = (int)time(NULL);

	alert.bAlertByMe = FALSE;
	alert.iLatitude = pTrafficInfo->sNodes[iNodeNumber].Position.latitude;
	alert.iLongitude = pTrafficInfo->sNodes[iNodeNumber].Position.longitude;

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
 * Adds traffic lines to the table. The prototype RoadMapStreetIterCB allows using
 * as a callback to the extend_line_ends function
 * @param line - pointer to line
 * @param context - context passed to callback
 * @param extend_flags
 *
 */
static int  RTTrafficInfo_AddTrafficLineCb( const PluginLine *line, void *context, int extend_flags )
{

 	int first_shape, last_shape;
	RoadMapPosition from;
    RoadMapPosition to;
    RoadMapShapeItr shape_itr;
    RTTrafficInfoContext *pTrafficInfoCtx = (RTTrafficInfoContext *) context;
    RTTrafficInfo *pTrafficInfo = pTrafficInfoCtx->pTrafficInfo;
	int iLinesCount;

	/*
	 * If the tile is not available - add it to the list
	 */
	if ( extend_flags & FLAG_EXTEND_CB_NO_SQUARE )
	{
		RTTrafficInfo_AddTileToFetchList( pTrafficInfoCtx, line->square );
		roadmap_log( ROADMAP_DEBUG, "Traffic ID: %d. Added tile id: %d to the list. List size: %d",
				pTrafficInfo->iID, line->square, pTrafficInfoCtx->tiles_fetch_list_count );
		return 0;
	}

	if (gRTTrafficInfoLinesTable.iCount == MAX_LINES){
		roadmap_log( ROADMAP_ERROR, "RealtimeTraffic extendCallBack line table is full");
		return 0 ;
	}

	iLinesCount = gRTTrafficInfoLinesTable.iCount;

	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount] = calloc(1, sizeof(RTTrafficInfoLines));

	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->pluginLine = *line;
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iType = pTrafficInfo->iType;
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iSpeed = (int) roadmap_math_meters_p_second_to_speed_unit((float)pTrafficInfo->fSpeed);

	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iTrafficInfoId =pTrafficInfo->iID;

	roadmap_square_set_current(line->square);

    roadmap_plugin_get_line_points (line,
									 &from, &to,
								     &first_shape, &last_shape,
									  &shape_itr );
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionFrom.longitude = from.longitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionFrom.latitude = from.latitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionTo.longitude = to.longitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionTo.latitude = to.latitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iFirstShape = first_shape;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iLastShape = last_shape;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iDirection = pTrafficInfo->iDirection;
   gRTTrafficInfoLinesTable.iCount++;
   return 0;
}

/**
 * Finds a line ID given 2 nodes
 * @param pTrafficInfo - pointer to traffic info
 * @param start - first node point
 * @param end - second node point
 * @param Line - point to line to return the found line
  * @return TRUE - If a line is found
 */
static BOOL RTTrafficInfo_Get_LineId( RTTrafficInfoContext *pTrafficInfoCtx, const RTTrafficInfoNode* start,
											const RTTrafficInfoNode* end, PluginLine* line_out )
{

	int start_square;
	int line_id, line_cfcc, line_count;
	int fromId, toId;
	RoadMapPosition fromPos, toPos;
	PluginLine line_ext;
	BOOL res = FALSE;
    RoadMapPosition save_pos;
    int save_zoom;
    RTTrafficInfo *pTrafficInfo = pTrafficInfoCtx->pTrafficInfo;

    roadmap_math_get_context( &save_pos, &save_zoom );

	// Find the square for the line starting point
	start_square = roadmap_tile_get_id_from_position ( 0, &start->Position );

	if ( start_square < 0 )
	{
		roadmap_log( ROADMAP_WARNING, "Cannot find square for the Node: %d. Position (%d, %d)",
				start->iNodeId, start->Position.latitude, start->Position.longitude );
		return FALSE;
	}
	if ( !roadmap_square_set_current( start_square ) )
	{

		//The tile is not available - add it to fetch list
		RTTrafficInfo_AddTileToFetchList( pTrafficInfoCtx, start_square );
//		roadmap_log( ROADMAP_DEBUG, "The tile is not available for this line. Traffic ID: %d, Tile ID: %d",
//				pTrafficInfo->iID, start_square );
		return FALSE;
	}

	line_count = roadmap_line_count();
	// Pass through all the lines in the square and match the nodes
	for ( line_id = 0; line_id < line_count; ++line_id )
	{
		// One of the ends must be the start
		roadmap_line_point_ids( line_id, &fromId, &toId );

		// Both ends differ - continue searching
		if ( ( start->iNodeId != fromId ) && ( start->iNodeId != toId ) )
		{
			continue;
		}

		// Initialize the plug in line id
		line_cfcc = roadmap_line_cfcc( line_id );
		roadmap_plugin_set_line( &line_ext, ROADMAP_PLUGIN_ID, line_id, line_cfcc, start_square, roadmap_locator_active() );

		// Both ends match - the line is found!
		if ( ( start->iNodeId == fromId ) && ( end->iNodeId == toId ) )
		{
			pTrafficInfo->iDirection = ROUTE_DIRECTION_WITH_LINE;

			roadmap_street_extend_line_ends( &line_ext, &fromPos, &toPos, FLAG_EXTEND_TO|FLAG_EXTEND_CB_NO_SQUARE,
																			RTTrafficInfo_AddTrafficLineCb, pTrafficInfoCtx );
			res = TRUE;
			break;
		}
		// Both ends match in the opposite direction - the line is found!
		if ( ( start->iNodeId == toId ) && ( end->iNodeId == fromId ) )
		{
			pTrafficInfo->iDirection = ROUTE_DIRECTION_AGAINST_LINE;

			roadmap_street_extend_line_ends( &line_ext, &fromPos, &toPos, FLAG_EXTEND_FROM|FLAG_EXTEND_CB_NO_SQUARE,
																			RTTrafficInfo_AddTrafficLineCb, pTrafficInfoCtx );
			res = TRUE;
			break;
		}
	} // while

	// Restore the zoom and square
	roadmap_math_set_context( &save_pos, save_zoom );

	// Save the output
	if ( res == TRUE )
	{
		*line_out = line_ext;
	}
	return res;
}



/**
 * Add a TrafficInfo Segment to list of segments
 * @param pTrafficInfo - pointer to the TrafficInfo
 * @param check_for_tiles - if checking of the tiles are necessary
 * @return TRUE operation was successful
 */
static BOOL RTTrafficInfo_AddSegments( RTTrafficInfo *pTrafficInfo, BOOL check_tiles ){
	int i;
	PluginLine line;
	BOOL found;

	/*
	 * Note: Pay attention - the context on stack to avoid unnecessary allocations
     * Be carefull in changing this code
	 */

	RTTrafficInfoContext traffic_info_ctx;
	traffic_info_ctx.pTrafficInfo = pTrafficInfo;
	traffic_info_ctx.tiles_fetch_list_count = 0;

	for (i=0; i < pTrafficInfo->iNumNodes-1; i++)
	{
		found = RTTrafficInfo_Get_LineId( &traffic_info_ctx, &pTrafficInfo->sNodes[i], &pTrafficInfo->sNodes[i+1], &line );

		if ( found )
		{
			RTTrafficInfo_AddTrafficLineCb( &line,  (void *)&traffic_info_ctx, 0 );
		}
	}

	// Add list to the map (if necessary) and reset
	if ( traffic_info_ctx.tiles_fetch_list_count )
	{
		RTTrafficInfo_AddFetchListToMap( &traffic_info_ctx );
		traffic_info_ctx.tiles_fetch_list_count = 0;
	}

	return TRUE;
}

static void RTTraficInfo_RemoveSegments(int iRecordNumber){
 	int i;

    free(gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iRecordNumber]);

    for (i=iRecordNumber; i<(gRTTrafficInfoLinesTable.iCount-1); i++) {
            gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i+1];
    }

     gRTTrafficInfoLinesTable.iCount--;

     gRTTrafficInfoLinesTable.pRTTrafficInfoLines[gRTTrafficInfoLinesTable.iCount] = NULL;
}

/**
 * Remove a TrafficInfo Segment from the list of segments
 * @param iTrafficInfoID - The pointer to the traffic ID
 * @return TRUE operation was successful
 */
static BOOL RTTraficInfo_DeleteSegments(int iTrafficInfoID){
	 int i = 0;
	 BOOL found = FALSE;

	 //   Are we empty?
    if ( 0 == gRTTrafficInfoLinesTable.iCount)
        return FALSE;

    while (i< gRTTrafficInfoLinesTable.iCount){
    	if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iTrafficInfoId == iTrafficInfoID){
    		RTTraficInfo_RemoveSegments(i);
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

    gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = calloc(1, sizeof(RTTrafficInfo));
    if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] == NULL)
    {
        roadmap_log( ROADMAP_ERROR, "RTTrafficInfo_Add - cannot add traffic_info  (%d) calloc failed", pTrafficInfo->iID);
        return FALSE;
    }

    RTTrafficInfo_InitRecord(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]);
	 gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iID = pTrafficInfo->iID;
	 gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->fSpeed = pTrafficInfo->fSpeed;
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

    gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iNumNodes = pTrafficInfo->iNumNodes;
    memcpy(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes,
          pTrafficInfo->sNodes, pTrafficInfo->iNumNodes * sizeof(*pTrafficInfo->sNodes));

#if 0
    for (i=0; i<pTrafficInfo->iNumNodes; i++){
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].iNodeId = pTrafficInfo->sNodes[i].iNodeId;
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].Position.latitude= pTrafficInfo->sNodes[i].Position.latitude;
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].Position.longitude = pTrafficInfo->sNodes[i].Position.longitude;
    }
#endif

#ifndef J2ME
    RTTrafficInfo_AddSegments( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount], TRUE );
#endif

    RTTrafficInfo_GenerateAlert(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount], (int)(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iNumNodes -1)/2);

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
    BOOL bFound= FALSE;

    //   Are we empty?
    if ( 0 == gTrafficInfoTable.iCount)
        return FALSE;

    if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount-1]->iID == iID)
    {
    	// Remove from the tile map
    	RTTrafficInfo_RemoveTileMapTraffic( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount-1] );

        free(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount-1]);
        bFound = TRUE;
    }
    else
    {
        int i;

        for (i=0; i<(gTrafficInfoTable.iCount-1); i++)
        {
            if (bFound)
                gTrafficInfoTable.pTrafficInfo[i] = gTrafficInfoTable.pTrafficInfo[i+1];
            else
            {
                if (gTrafficInfoTable.pTrafficInfo[i]->iID == iID)
                {
                	// Remove from the tile map
                	RTTrafficInfo_RemoveTileMapTraffic( gTrafficInfoTable.pTrafficInfo[i] );

                    free(gTrafficInfoTable.pTrafficInfo[i]);
                    gTrafficInfoTable.pTrafficInfo[i] = gTrafficInfoTable.pTrafficInfo[i+1];
                    bFound = TRUE;
                }
            }
        }
    }

    if (bFound)
    {
        gTrafficInfoTable.iCount--;

        gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = NULL;

#ifndef J2ME
        RTTraficInfo_DeleteSegments(iID);
#endif

        RTTrafficInfo_DeleteAlert (iID);

	 }

    return TRUE;
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
	if ((Record >= MAX_LINES) || (Record < 0))
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
		if ((gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.line_id == line) &&
			  (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iDirection == direction) &&
			  (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.square == square))
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
		if ((gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.line_id == line) && (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.square == square))
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
 * Recalculate all line IDs
 * @param None
 * @return None
 */
void RTTrafficInfo_RecalculateSegments(){
	int i;

	for (i=0;i<gRTTrafficInfoLinesTable.iCount; i++){
			RTTraficInfo_RemoveSegments(i);
	}

	for (i=0;i<gTrafficInfoTable.iCount; i++){
		RTTrafficInfo_AddSegments( gTrafficInfoTable.pTrafficInfo[i], FALSE );
	}
}

/**
* Initialization of the tile map
* @param void
* @return None
*/
static void RTTrafficInfo_TileMapReset( void )
{
	int i;
	for ( i = 0; i < TRAFFIC_INFO_TILE_MAP_SIZE; ++i )
	{
		sgTrafficInfoTileMap->next[i] = -1;
	}
    for ( i = 0; i < ROADMAP_HASH_MODULO; i++ )
    {
    	sgTrafficInfoTileMap->head[i] = -1;
    }
	sgTrafficInfoTileMapIndex = 0;
	sgTrafficInfoTileMapSize = 0;
}


/**
* Deallocate the tile hash map
* @param void
* @return None
*/
static void RTTrafficInfo_TileMapClear( void )
{
	roadmap_hash_free( sgTrafficInfoTileMap );
}

/**
* Deallocate the tile hash map
* @param void
* @return None
*/
static void RTTrafficInfo_TileMapReceivedTileCb( int tile_id )
{
	int i;
	RTTrafficInfoTileMapEntry* pEntry;
	RTTrafficInfo* pData;

	// Request from the tile server
	int* tile_status = roadmap_tile_status_get ( tile_id );

	if ( *tile_status & ROADMAP_TILE_STATUS_CALLBACK_RT_TRAFFIC )
	{

//		roadmap_log( ROADMAP_DEBUG, "Tile ID: %d is ready. Current index: %d. Current size: %d", tile_id,
//				sgTrafficInfoTileMapIndex, sgTrafficInfoTileMapSize );

		if ( sgTrafficInfoTileMapIndex == 0 )
			return;

		i = roadmap_hash_get_first( sgTrafficInfoTileMap, tile_id );
		while ( i >= 0 )
		{
			pEntry = roadmap_hash_get_value( sgTrafficInfoTileMap, i );
			if ( pEntry->tile_id == tile_id )
			{
				pData = pEntry->pData;
				/* One more tile is ready */
				pData->iFetchListCount--;
				/* The last reference to the data - all the tiles are available now */
				if ( pData->iFetchListCount == 0 )
				{
					RTTrafficInfo_AddSegments( pData, FALSE );
				}
				else
				{
					roadmap_log( ROADMAP_DEBUG, "Traffic info ID: %d still lacks %d tiles", pData->iID,
							pData->iFetchListCount );
				}
				/* Remove the entry from the hash */
				roadmap_hash_remove( sgTrafficInfoTileMap, tile_id, i );
				roadmap_hash_set_value( sgTrafficInfoTileMap, i, NULL );
				sgTrafficInfoTileMapSize--;
			}
			i = roadmap_hash_get_next( sgTrafficInfoTileMap, i );
		}
		/* All the tiles are fetched and processed - the hash can be reused */
		if ( sgTrafficInfoTileMapSize == 0 )
		{
			RTTrafficInfo_TileMapReset();
		}
	}
	/* Next callback in chain */
	if ( TileCbNext )
	{
		TileCbNext( tile_id );
	}
}

/**
* Add the tile to the tile map of the traffic info
* @param pData - pointer to the traffic info data to be added
* @param tile_id - id of the tile - the key to the hash
* @return TRUE - added successfully
*/
static BOOL RTTrafficInfo_TileMapAdd( RTTrafficInfo* pData, int tile_id )
{
	RTTrafficInfoTileMapEntry* pEntry;
	// Check the hash size. Resize if necessary
	if ( sgTrafficInfoTileMapIndex == TRAFFIC_INFO_TILE_MAP_SIZE )
	{
			/*
			 * Not supposed to exceed the maximum value! Or error in tiles download or logical bug in software
			 */
			roadmap_log( ROADMAP_ERROR, "Maximum index of the Traffic info tile map has exceeded: Tiles download error or bug!" );
			return FALSE;
	}

	// Prepare the entry
	if ( pData->iFetchListCount < RT_TRAFFIC_INFO_TILE_FETCH_LIST_MAXSIZE )
	{

		pEntry = &pData->sTileFetchList[pData->iFetchListCount];
		pEntry->hash_index = sgTrafficInfoTileMapIndex;
		pEntry->tile_id = tile_id;
		pEntry->pData = pData;
		pData->iFetchListCount++;

		// Add to the hash map
		roadmap_hash_add( sgTrafficInfoTileMap, tile_id, pEntry->hash_index );
		roadmap_hash_set_value( sgTrafficInfoTileMap, pEntry->hash_index, (void*) pEntry );

		sgTrafficInfoTileMapIndex++;
		sgTrafficInfoTileMapSize++;
	}
	else
	{
		// TODO :: Add log here
	}
	return TRUE;
}


/**
* Adds the tile to the queue of the unavailable tiles checking if already has one
* @param tile_id - the tile id of the tile to add
* @return TRUE - if the tile added, FALSE - already in the list or list is full
*/
static BOOL RTTrafficInfo_AddTileToFetchList( RTTrafficInfoContext* pTrafficInfoCtx, int tile_id )
{
	BOOL res = FALSE;
	int j;
	int fetch_list_count = pTrafficInfoCtx->tiles_fetch_list_count;
	if (  fetch_list_count >= RT_TRAFFIC_INFO_TILE_FETCH_LIST_MAXSIZE )
	{
		roadmap_log( ROADMAP_INFO, "The maximum size of the tiles fetch list cannot be exceeded!" );
		// Just ignore
		return res;
	}

	// Check if already in the list
	for ( j = 0; j < fetch_list_count; ++j )
	{
		if ( pTrafficInfoCtx->tiles_fetch_list[j] == tile_id )
			break;
	}
	// If not in the list - add it
	if ( j == pTrafficInfoCtx->tiles_fetch_list_count )
	{
		res = TRUE;
		pTrafficInfoCtx->tiles_fetch_list[j] = tile_id;
		pTrafficInfoCtx->tiles_fetch_list_count++;
	}
	return res;
}

/**
* Process tiles list. Prepare the data entries and add them to the map
* @param pTrafficInfo - the pointer to the traffic data
* @return TRUE - if the tiles in the list are added successfully to the map
*/
static BOOL RTTrafficInfo_AddFetchListToMap( RTTrafficInfoContext *pTrafficInfoCtx )
{
	BOOL res = FALSE;
	int tile_id, j;
	RTTrafficInfo *pTrafficInfo = pTrafficInfoCtx->pTrafficInfo;
	int fetch_list_count = pTrafficInfoCtx->tiles_fetch_list_count;
	// The list is ready - prepare the data entries and add them
	// Create the data entry if necessary
	if (  fetch_list_count > 0 )
	{
		int *tile_status;

		// Prepare the data entry
		for ( j = 0; j < fetch_list_count; ++j )
		{
			tile_id = pTrafficInfoCtx->tiles_fetch_list[j];
			// Add to map
			RTTrafficInfo_TileMapAdd( pTrafficInfo, tile_id );
			// Request from the tile server
			tile_status = roadmap_tile_status_get ( tile_id );
			(*tile_status) |= ROADMAP_TILE_STATUS_FLAG_CALLBACK|ROADMAP_TILE_STATUS_CALLBACK_RT_TRAFFIC;
			roadmap_tile_request( tile_id, ROADMAP_TILE_STATUS_PRIORITY_NONE, 1, NULL );
		}

		roadmap_log( ROADMAP_DEBUG, "Traffic Info ID: %d, Nodes #%d. Requested %d tiles. ", pTrafficInfo->iID, pTrafficInfo->iNumNodes, fetch_list_count );
		res = TRUE;
	}
	else
	{
		roadmap_log( ROADMAP_DEBUG, "Traffic Info ID: %d, Nodes #%d. All the tiles are available.", pTrafficInfo->iID, pTrafficInfo->iNumNodes  );
	}
	return res;
}


/**
* Removes the traffic from the tile map
* @param pTrafficInfo - the pointer to the traffic data
* @return VOID
*/
static void RTTrafficInfo_RemoveTileMapTraffic( RTTrafficInfo *pTrafficInfo )
{
	int i;
	RTTrafficInfoTileMapEntry* pEntry;

	roadmap_log( ROADMAP_DEBUG, "Removing traffic info: %d, current fetch list size: %d", pTrafficInfo->iID, pTrafficInfo->iFetchListCount );

	for ( i = 0; i < pTrafficInfo->iFetchListCount; ++i )
	{
		pEntry = &pTrafficInfo->sTileFetchList[i];
		roadmap_hash_remove( sgTrafficInfoTileMap, pEntry->tile_id, pEntry->hash_index );
		roadmap_hash_set_value( sgTrafficInfoTileMap, pEntry->hash_index, NULL );
	}

	pTrafficInfo->iFetchListCount = 0;
}
