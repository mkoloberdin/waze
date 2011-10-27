/* roadmap_analytics.c
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_analytics.h"
#include "roadmap_hash.h"
#include "Realtime/Realtime.h"

#ifdef FLURRY
extern void roadmap_flurry_log_event (const char *event_name, const char *info_name, const char *info_val);
#endif //FLURRY


#define MAX_STATS 100

typedef struct {
   int         iNumParams;
   const char  *szEventName;
   const char  *szParamName[STATS_MAX_ATTRS];
   const char  *szParamValue[STATS_MAX_ATTRS];
}StatEntry;

typedef struct {
   StatEntry statEntry[MAX_STATS];
   int iCount;
} StatsTable;

static StatsTable   gStatsTable;


/////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_analytics_clear(void) {
   int i;

   for (i= 0; i < gStatsTable.iCount; i++){
      int j;
      for (j = 0; j<= gStatsTable.statEntry[i].iNumParams; j++ ){
         if (gStatsTable.statEntry[i].szParamValue[j])
            free((void *)gStatsTable.statEntry[i].szParamValue[j]);
      }
   }
   memset(&gStatsTable, 0, sizeof(gStatsTable));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_analytics_count(void) {
   return gStatsTable.iCount;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_analytics_is_empty(void) {
   return (gStatsTable.iCount == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static StatEntry *StatsTable_get_Stat(int index) {
   return &gStatsTable.statEntry[index];
}


/////////////////////////////////////////////////////////////////////////////////////////////////
const char *StatsTable_getEventName(int index){
   StatEntry *entry = StatsTable_get_Stat(index);
   if (entry)
      return entry->szEventName;
   else
      return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int StatsTable_getNumParams(int index){
   StatEntry *entry = StatsTable_get_Stat(index);
   if (entry)
      return entry->iNumParams;
   else
      return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char **StatsTable_getParamNames(int index){
   StatEntry *entry = StatsTable_get_Stat(index);
   if (entry)
      return &entry->szParamName[0];
   else
      return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char **StatsTable_getParamValues(int index){
   StatEntry *entry = StatsTable_get_Stat(index);
   if (entry)
      return &entry->szParamValue[0];
   else
      return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void StatsTable_AddStat(const char *EventNamme, int iNumParams, const char *ParamName[], const char*ParamValue[]) {
   if (roadmap_analytics_count() < MAX_STATS){
      int i;
      roadmap_log(ROADMAP_DEBUG,"STAT %s %s,%s\n", EventNamme, ParamName[0], ParamValue[0] );
      if (iNumParams > STATS_MAX_ATTRS)
         iNumParams = STATS_MAX_ATTRS;

      gStatsTable.statEntry[roadmap_analytics_count()].iNumParams = iNumParams;
      gStatsTable.statEntry[roadmap_analytics_count()].szEventName = EventNamme;
      for (i = 0; i < iNumParams; i++){
         gStatsTable.statEntry[roadmap_analytics_count()].szParamName[i]   = ParamName[i];
         if (ParamValue[i])
            gStatsTable.statEntry[roadmap_analytics_count()].szParamValue[i]  = strdup(ParamValue[i]);
         else
            gStatsTable.statEntry[roadmap_analytics_count()].szParamValue[i]  = NULL;
      }
      gStatsTable.iCount++;
   }else{
      // Table is Full. Send List to the server.
      roadmap_log(ROADMAP_ERROR, "StatsTable_AddStat  list is full ");
      Realtime_SendAllStats(NULL);
   }
}

//////////////////////////////////////////////////////////////////
void roadmap_analytics_init(void){
   roadmap_analytics_clear();
}

void roadmap_analytics_term(void){
   if (!roadmap_analytics_is_empty())
      Realtime_SendAllStats(NULL);
}

//////////////////////////////////////////////////////////////////
void roadmap_analytics_log_event (const char *event_name, const char *info_name, const char *info_val) {
   roadmap_log (ROADMAP_DEBUG, "STAT %s %s,%s", event_name, info_name, info_val );
   StatsTable_AddStat(event_name, 1, &info_name, &info_val);
#ifdef FLURRY
   roadmap_flurry_log_event (event_name, info_name, info_val);
#endif
}


//////////////////////////////////////////////////////////////////
void roadmap_analytics_log_int_event (const char *event_name, const char *info_name, int info_val) {
   char temp[20];
   snprintf(temp, 20, "%d", info_val);
   roadmap_analytics_log_event(event_name, info_name, (const char *)&temp);
}

