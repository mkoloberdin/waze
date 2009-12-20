/* navigate_dglib.c - dglib implementation navigate_graph functions
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See navigate_dglib.h
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "type.h"
#include "graph.h"
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_main.h"
#include "roadmap_turns.h"
#include "roadmap_metadata.h"
#include "roadmap_messagebox.h"
#include "roadmap_line_route.h"

#include "navigate_main.h"
#include "navigate_traffic.h"
#include "navigate_graph.h"

static int fips_data_loaded = 0;
static dglGraph_s graph;
static dglSPCache_s spCache;

typedef struct {
   PluginLine from_line;
   int from_against_dir;
   int turn_restrictions;
} NavigateClip;

NavigateClip NavigateClipData;

static int  clipper     (
                        dglGraph_s *    pgraph ,
                        dglSPClipInput_s * pIn ,
                        dglSPClipOutput_s * pOut ,
                        void *          pvarg       /* caller's pointer */
                        )
{       
   NavigateClip *info = (NavigateClip *)pvarg;
   int to_line = dglEdgeGet_Id(pgraph, pIn->pnEdge);
   int from_line;
   int new_cost;

   if (pIn->pnPrevEdge != NULL) {
      
      from_line = dglEdgeGet_Id(pgraph, pIn->pnPrevEdge);
   } else {
      from_line = roadmap_plugin_get_line_id (&info->from_line);
      if (info->from_against_dir) from_line = -from_line;
   }

   /* no U turns */
   if (from_line == -to_line) return 1;
   
   new_cost = navigate_traffic_get_cross_time (abs(to_line), (to_line < 0));
   if (new_cost) pOut->nEdgeCost = new_cost;

   if (!info->turn_restrictions) return 0;

   if ( roadmap_turns_find_restriction (
            dglNodeGet_Id(pgraph, pIn->pnNodeFrom),
            pIn->pnPrevEdge != NULL ?
               dglEdgeGet_Id(pgraph, pIn->pnPrevEdge) :
               (int) pvarg,
            dglEdgeGet_Id(pgraph, pIn->pnEdge))) {
            /*
            printf( "        discarder.\n" );
            */
            return 1;
        }

   return 0;
}


int navigate_reload_data (void) {
   
   if (fips_data_loaded == 0) return 0;
      
   fips_data_loaded = 0;
   dglRelease (& graph);

   return navigate_load_data ();
}

   
int navigate_load_data (void) {
   FILE *fd;
   int nret;
   char path[255];
   int fips;
   time_t map_unix_time;
   const char *sequence;

   fips = roadmap_locator_active ();
   if (fips_data_loaded == fips) return 0;

   sequence = roadmap_path_first("maps");
   if (sequence == NULL) {
      return -1;
   }

   roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);

   do {
      snprintf (path, sizeof(path), "%s/usc%05d.dgl", sequence, fips);
      
      fd = fopen( path, "rb" );
      if ( fd != NULL ) break;

      sequence = roadmap_path_next("maps", sequence);

   } while (sequence != NULL);

   if ( fd == NULL ) {  
      roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
      roadmap_messagebox ("Error", "Can't find route data.");
      return -1;
   }

   nret = dglRead( & graph , fd );

   fclose( fd );

   roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);

   if ( nret < 0 ) {
      roadmap_messagebox ("Error", "Can't load route data.");
      return -1;
   }
   
   map_unix_time = atoi
                     (roadmap_metadata_get_attribute ("Version", "UnixTime"));

   if ((time_t) *dglGet_Opaque (&graph) != map_unix_time) {
      roadmap_messagebox ("Error", "Navigation data is too old.");
      return -1;
   }

   dglInitializeSPCache( & graph, & spCache );
   fips_data_loaded = fips;

   return 0;
}


int navigate_get_route_segments (PluginLine *from_line,
                                 int from_point,
                                 PluginLine *to_line,
                                 int to_point,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *result) {
   
   int i;
   int nret;
   int total_cost;
   int line_from_point;
   int line_to_point;
   dglSPReport_s *pReport;

   if (fips_data_loaded != roadmap_locator_active ()) return -1;

   *result = 0;

   /* save places for start & end lines */
   *size -= 2;

   NavigateClipData.from_line = *from_line;

   //FIXME add plugin support
   roadmap_line_points (from_line->line_id, &line_from_point, &line_to_point);
   if (from_point == line_from_point) {
      NavigateClipData.from_against_dir = 1;
   } else {
      NavigateClipData.from_against_dir = 0;
   }

   NavigateClipData.turn_restrictions = 1;

   nret = dglShortestPath (&graph, &pReport, from_point, to_point,
                           clipper, &NavigateClipData, NULL);
   if (nret <= 0) {

      *result = GRAPH_IGNORE_TURNS;

      NavigateClipData.turn_restrictions = 0;
      nret = dglShortestPath (&graph, &pReport, from_point, to_point,
                              clipper, &NavigateClipData, NULL);
      if (nret <= 0) {
         return nret;
      }
   }

   if (pReport->cArc > *size) return -1;
   
   total_cost = pReport->nDistance;
   *size = pReport->cArc;

   /* add starting line */
   if (abs(dglEdgeGet_Id(&graph, pReport->pArc[0].pnEdge)) !=
         from_line->line_id) {

      int tmp_from;
      int tmp_to;
      
      /* FIXME no plugin support */
      roadmap_line_points (from_line->line_id, &tmp_from, &tmp_to);

      segments[0].line = *from_line;

      if (from_point == tmp_from) {
         segments[0].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      } else {
         segments[0].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }

      segments++;
      (*size)++;
   }


   for(i=0; i < pReport->cArc ;i++) {
      segments[i].line.plugin_id = ROADMAP_PLUGIN_ID;
      segments[i].line.line_id = dglEdgeGet_Id(&graph, pReport->pArc[i].pnEdge);
      segments[i].line.fips = roadmap_locator_active ();
      segments[i].line.cfcc = *(unsigned char *)
         dglEdgeGet_Attr(&graph, pReport->pArc[i].pnEdge);
      if (segments[i].line.line_id < 0) {
         segments[i].line.line_id = abs(segments[i].line.line_id);
         segments[i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;

      } else {
         segments[i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }

/*
      printf(  "edge[%d]: from %ld to %ld - travel cost %ld - user edgeid %ld - distance from start node %ld\n" ,
            i,
            pReport->pArc[i].nFrom,
            pReport->pArc[i].nTo,
            dglEdgeGet_Cost(&graph, pReport->pArc[i].pnEdge), * this is the cost from clip() *
            dglEdgeGet_Id(&graph, pReport->pArc[i].pnEdge),
            pReport->pArc[i].nDistance
            );
            */
   }

   /* add ending line */

   /* FIXME no plugin support */
   if (segments[i-1].line.line_id != to_line->line_id) {

      int from_point;
      int to_point;
      segments[i].line = *to_line;
      roadmap_line_points (to_line->line_id, &from_point, &to_point);
      
      if (from_point == dglNodeGet_Id(&graph, &pReport->nDestinationNode)) {
         
         segments[i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      } else {

         segments[i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      }
      (*size)++;
   }

   dglFreeSPReport(&graph, pReport);

   return total_cost;
}


