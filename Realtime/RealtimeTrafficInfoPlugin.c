/* RealtimeTrafficInfoPlugin.c - RealTime TrafficInfo plugin interfaces
 *
 * LICENSE:
 *
 *   Copyright 2008
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
 *   See RealtimeTrafficInfoPlugin.h
 */

#include <stdlib.h>

#include "../roadmap.h"
#include "../roadmap_plugin.h"
#include "../roadmap_config.h"
#include "../roadmap_square.h"
#include "../roadmap_shape.h"
#include "../roadmap_layer.h"
#include "../roadmap_line.h"
#include "../roadmap_math.h"
#include "../roadmap_res.h"
#include "../roadmap_map_settings.h"
#include "roadmap_screen.h"
#include "roadmap_tile_status.h"
#include "roadmap_tile_manager.h"
#include "roadmap_tile.h"

#include "Realtime.h"
#include "RealtimeTrafficInfo.h"

static void RealtimeTrafficInfoScreenRepaint (int max_pen);
static void RealtimeTrafficInfoPaintGeoms (void);

#define TRAFFIC_PEN_WIDTH 4
#define MAX_SEGEMENTS 2500

static int plugId;
static  RoadMapPen pens[6];
static  RoadMapPen speed_text_pen;
static  RoadMapConfigDescriptor RouteInfoConfigRouteColorBad =
                    ROADMAP_CONFIG_ITEM("TrafficInfo", "RouteColorBad");
static  RoadMapConfigDescriptor RouteInfoConfigRouteColorMild =
                    ROADMAP_CONFIG_ITEM("TrafficInfo", "RouteColorMild");
static  RoadMapConfigDescriptor RouteInfoConfigRouteColorGood =
                    ROADMAP_CONFIG_ITEM("TrafficInfo", "RouteColorGood");
static  RoadMapConfigDescriptor RouteInfoConfigRouteColorStandStill =
                    ROADMAP_CONFIG_ITEM("TrafficInfo", "RouteColorStandStill");

RoadMapConfigDescriptor RouteInfoConfigDisplayTraffic =
                  ROADMAP_CONFIG_ITEM("TrafficInfo", "Display traffic info");


static RoadMapPluginHooks RealtimeTrafficInfoPluginHooks = {

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      &RealtimeTrafficInfoScreenRepaint,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
};



static void RealtimeTrafficInfoRegister (void) {

   plugId = roadmap_plugin_register (&RealtimeTrafficInfoPluginHooks);
}


static void RealtimeTrafficInfoUregister () {

   if (plugId != -1)
   	roadmap_plugin_unregister (plugId);
   plugId = -1;

}

int RealtimeTrafficInfoState(){
	if (roadmap_config_match(&RouteInfoConfigDisplayTraffic, "yes"))
		return 1;
	else
		return 0;

}

void RealtimeTrafficInfoPluginInit () {

   roadmap_config_declare
      ("schema", &RouteInfoConfigRouteColorGood,  "#fdf66b", NULL); //Yellow
   roadmap_config_declare
      ("schema", &RouteInfoConfigRouteColorMild,  "#f57a24", NULL); //Orange
   roadmap_config_declare
      ("schema", &RouteInfoConfigRouteColorBad,  "#FF0000", NULL); //Red
   roadmap_config_declare
      ("schema", &RouteInfoConfigRouteColorStandStill,  "#c4251f", NULL); //Red
   roadmap_config_declare_enumeration
      ("preferences", &RouteInfoConfigDisplayTraffic, NULL, "yes", "no", NULL);

   pens[LIGHT_TRAFFIC] = roadmap_canvas_create_pen ("RealtimeTrafficInfoPenGood");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RouteInfoConfigRouteColorGood));
   roadmap_canvas_set_thickness (TRAFFIC_PEN_WIDTH);

   pens[MODERATE_TRAFFIC] = roadmap_canvas_create_pen ("RealtimeTrafficInfoPenMild");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RouteInfoConfigRouteColorMild));
   roadmap_canvas_set_thickness (TRAFFIC_PEN_WIDTH);

   pens[HEAVY_TRAFFIC] = roadmap_canvas_create_pen ("RealtimeTrafficInfoPenBad");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RouteInfoConfigRouteColorBad));
   roadmap_canvas_set_thickness (TRAFFIC_PEN_WIDTH);

   pens[STAND_STILL_TRAFFIC] = roadmap_canvas_create_pen ("RealtimeTrafficInfoPenStandStill");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RouteInfoConfigRouteColorStandStill));
   roadmap_canvas_set_thickness (TRAFFIC_PEN_WIDTH);

   speed_text_pen = 	roadmap_canvas_create_pen("SpeedText");
	roadmap_canvas_set_foreground("#000000");

   if (roadmap_config_match(&RouteInfoConfigDisplayTraffic, "yes")){
   	RealtimeTrafficInfoRegister();
   	Realtime_SendTrafficInfo(1);
   }
}


void RealtimeTrafficInfoPluginTerm(){
	RealtimeTrafficInfoUregister(plugId);
	plugId = -1;
}

void RealtimeRoadToggleShowTraffic(){
   if (roadmap_config_match(&RouteInfoConfigDisplayTraffic, "yes")){
		 roadmap_config_set (&RouteInfoConfigDisplayTraffic,"no");
		 Realtime_SendTrafficInfo(0);

		 //RealtimeTrafficInfoUregister(plugId);
		 //plugId = -1;
   }
   else{
		 roadmap_config_set (&RouteInfoConfigDisplayTraffic,"yes");
		 Realtime_SendTrafficInfo(1);
		//RealtimeTrafficInfoRegister();
   }
  roadmap_screen_redraw();
}

BOOL isDisplayingTrafficInfoOn(){
	if (roadmap_config_match(&RouteInfoConfigDisplayTraffic, "yes"))
		return TRUE;
	else
		return FALSE;
}

static RoadMapPosition *spCurrentPaintingLine = NULL;

static void RealtimeTrafficInfoOutlineIterator (int shape, RoadMapPosition *position) {

	*position = spCurrentPaintingLine[shape];	
}


static void RealtimeTrafficInfoPaintGeoms (void) {
	
	int i;
	int iNumInfo = RTTrafficInfo_Count ();
   int previous_type = -1;
	
	for (i = 0; i < iNumInfo; i++) {
		RoadMapPen previous_pen;
      
      RTTrafficInfo *pTrafficInfo = RTTrafficInfo_Get (i);
      
      if (pTrafficInfo->iNumGeometryPoints < 2)
      	continue;
      	
		if (!roadmap_math_is_visible (&pTrafficInfo->boundingBox))
			continue;

      previous_pen = roadmap_canvas_select_pen (pens[pTrafficInfo->iType]);
      roadmap_canvas_set_thickness (TRAFFIC_PEN_WIDTH);
      if (previous_pen) {
    	   roadmap_canvas_select_pen (previous_pen);
      }
		if (previous_type != pTrafficInfo->iType)
	  		roadmap_screen_draw_flush();
	  		
	  	previous_type = pTrafficInfo->iType;
      
      spCurrentPaintingLine = pTrafficInfo->geometry;
	  	roadmap_screen_draw_one_line (pTrafficInfo->geometry,
	                                 pTrafficInfo->geometry + pTrafficInfo->iNumGeometryPoints - 1,
	                                 0,
	                                 pTrafficInfo->geometry,
	                                 1,
	                                 pTrafficInfo->iNumGeometryPoints - 2,
	                                 RealtimeTrafficInfoOutlineIterator,
	                                 &pens[pTrafficInfo->iType],
	                                 1,
	                                 -1,
	                                 NULL,
	                                 NULL,
	                                 NULL);
	   spCurrentPaintingLine =  NULL;
      
	}
}

static void RealtimeTrafficInfoScreenRepaint (int max_pen) {
   int i;
   int iNumLines;
   int width = TRAFFIC_PEN_WIDTH;
   int previous_with = -1;
   int previous_type = -1;
   int lastRequestedTile = -1;

   if (!isDisplayingTrafficInfoOn())
   	return;
   if (!(roadmap_map_settings_color_roads())) // see if user chose not to display color on map
   	 return;
	if (RTTrafficInfo_IsEmpty()) return;

	if (roadmap_square_current_scale_factor() > 1/*roadmap_math_get_zoom () >= 100*/) {
		RealtimeTrafficInfoPaintGeoms();
		return;
	}
	
   iNumLines = RTTrafficInfo_GetNumLines();


    for (i=0; i<iNumLines; i++) {
		RTTrafficInfoLines *pTrafficLine = RTTrafficInfo_GetLine(i);
		RoadMapGuiPoint seg_middle;
		RoadMapPen previous_pen;
      RoadMapPen layer_pen;

		if (!roadmap_math_is_visible (&pTrafficLine->pTrafficInfo->boundingBox))
			continue;
			
		if (!pTrafficLine->isInstrumented) {

			if (pTrafficLine->iSquare != lastRequestedTile) {
				RoadMapArea tileBox;
				roadmap_tile_edges (pTrafficLine->iSquare, &tileBox.west, &tileBox.east, &tileBox.south, &tileBox.north);
				if (!roadmap_math_is_visible (&tileBox))
					continue;
					
				if (roadmap_square_version (pTrafficLine->iSquare) < pTrafficLine->iVersion) {
					roadmap_tile_request (pTrafficLine->iSquare, ROADMAP_TILE_STATUS_PRIORITY_ON_SCREEN, TRUE, NULL);
					lastRequestedTile = pTrafficLine->iSquare;
				}
			}
			continue;
		}
			
		if (!roadmap_math_is_visible (&pTrafficLine->boundingBox))
			continue;

		if (!roadmap_layer_is_visible (pTrafficLine->cfcc, 0))
			continue;

		roadmap_square_set_current(pTrafficLine->iSquare);


		layer_pen = roadmap_layer_get_pen (pTrafficLine->cfcc, 0, 0);
#ifndef ROUTE_ARROWS
      if (layer_pen)
         	width = roadmap_canvas_get_thickness (layer_pen)+2;
      else
         	width = TRAFFIC_PEN_WIDTH;
#else
       if (layer_pen)
          width = roadmap_canvas_get_thickness (layer_pen)*4/5;
       else
          width = TRAFFIC_PEN_WIDTH;
#endif //ROUTE_ARROWS

      if (width < TRAFFIC_PEN_WIDTH) {
            width = TRAFFIC_PEN_WIDTH;
      }

      previous_pen = roadmap_canvas_select_pen (pens[pTrafficLine->iType]);
      roadmap_canvas_set_thickness (width);

      if (previous_pen) {
    	       roadmap_canvas_select_pen (previous_pen);
      	}


		if ((width != previous_with) || (previous_type != pTrafficLine->iType))
	  		roadmap_screen_draw_flush();

	  	previous_with = width;
	  	previous_type = pTrafficLine->iType;


      roadmap_screen_draw_one_line (&pTrafficLine->positionFrom,
   	                                &pTrafficLine->positionTo,
      	                            0,
      	                            &pTrafficLine->positionFrom,
            	                    pTrafficLine->iFirstShape,
               	                    pTrafficLine->iLastShape,
                  	                NULL,
                     	            &pens[pTrafficLine->iType],
                        	        1,
                                   -1,
                           	        NULL,
                              	    &seg_middle,
                                 	NULL);

      	if ((width >= 4) && !roadmap_screen_fast_refresh()) {
      			static const char *direction_colors[4] = {"#fd9f0b", "#FFF380", "#FFFFFF", "#FFFFFF"};
	 		roadmap_screen_draw_line_direction (&pTrafficLine->positionFrom,
   												&pTrafficLine->positionTo,
   												&pTrafficLine->positionFrom,
                  									pTrafficLine->iFirstShape,
                  									pTrafficLine->iLastShape,
                  									NULL,
		                  							width,
      		            							pTrafficLine->iDirection,
      		            							direction_colors[pTrafficLine->iType]);
       }
   }
}

