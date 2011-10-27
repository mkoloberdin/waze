/* RealtimePopUp.c - Manage Idle popups
 *
 * LICENSE:
 *
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

#include "../roadmap_main.h"
#include "../roadmap_factory.h"
#include "../roadmap_config.h"
#include "../roadmap_gps.h"
#include "../roadmap_trip.h"
#include "../roadmap_navigate.h"
#include "../roadmap_message_ticker.h"
#include "../ssd/ssd_dialog.h"
#include "../roadmap_math.h"

#include "Realtime.h"
#include "RealtimeAlerts.h"
#include "RealtimeExternalPoi.h"
#include "RealtimePopUp.h"

static RoadMapConfigDescriptor RoadMapConfigSecondsToPopup =
                        ROADMAP_CONFIG_ITEM("Alerts", "Seconds to popup");

static RoadMapConfigDescriptor RoadMapConfigMinDistance =
                        ROADMAP_CONFIG_ITEM("Popups", "Min Distance");

#define MAX_SCROLLERS 5

static BOOL gStopedStolling = FALSE;

typedef struct pointer_callback {
   RealtimeIdleScroller *scroller;
   int                  priority;
} IdleScroller;

static IdleScroller scrollers[MAX_SCROLLERS];

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimePopUp_set_Stoped_Popups(void){
   gStopedStolling = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void  RealtimePopUp_Register(RealtimeIdleScroller *scroller, int priority){
   int i;

   if (scrollers[MAX_SCROLLERS-1].scroller) {
      roadmap_log (ROADMAP_ERROR, "Too many scrollers");
      return;
   }

   for (i=0; i<MAX_SCROLLERS; i++) {
      if (scrollers[i].priority <= priority) break;
   }


   memmove (&scrollers[i+1], &scrollers[i],
            sizeof(IdleScroller) * (MAX_SCROLLERS - i - 1));

   scrollers[i].scroller = scroller;
   scrollers[i].priority = priority;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void start_scrolling(void){
   int i = 0;

    while ((i<MAX_SCROLLERS) && scrollers[i].scroller) {
       if ((scrollers[i].scroller->start_scrolling) ())
          return;
       i++;
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void stop_scrolling(void){
   int i = 0;

    while ((i<MAX_SCROLLERS) && scrollers[i].scroller) {
       if ((scrollers[i].scroller->is_scrolling) ())
          (scrollers[i].scroller->stop_scrolling) ();
       i++;
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL is_scrolling(void){
   int i = 0;

    while ((i<MAX_SCROLLERS) && scrollers[i].scroller) {
       if ((scrollers[i].scroller->is_scrolling) ())
          return TRUE;
       i++;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL is_dlg_active(){

#ifdef IPHONE
   return !roadmap_main_is_root();
#else
   if (!ssd_dialog_is_currently_active()){
      return FALSE;
   }
   else{
     SsdWidget dialog = ssd_dialog_get_currently_active();
     if (dialog)
        return !(dialog->flags & SSD_DIALOG_FLOAT);
     else
        return FALSE;
   }

#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void RealtimePopUp_Timer(void){
   static int idleCount = 0;
   RoadMapGpsPosition pos;
   SsdWidget dialog;
   const char *focus = roadmap_trip_get_focus_name();
   BOOL gps_active;
   int gps_state;
   static BOOL init = FALSE;
   static BOOL drove_enough = FALSE;
   static RoadMapGpsPosition start_pos;

   gps_state = roadmap_gps_reception_state();
   gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);

   roadmap_navigate_get_current(&pos, NULL, NULL);

   // Save initial position
   if (!init){
      if (gps_active){
         start_pos = pos;
         init = TRUE;
      }
      else{
         return;
      }
   }

   // Check that the user did some driving before
   if (!drove_enough){
      int iDistance = roadmap_math_distance((const RoadMapPosition *)&pos, (const RoadMapPosition *)&start_pos);
      if (iDistance > roadmap_config_get_integer(&RoadMapConfigMinDistance)){
         drove_enough = TRUE;
      }
      else{
         return;
      }
   }


   if (gStopedStolling && pos.speed > 30)
   {
      gStopedStolling = FALSE;
   }

   if ((pos.speed < 0) || (pos.speed > 5)){
      stop_scrolling();
      idleCount = 0;
      return;
   }



   if (gStopedStolling){
      idleCount = 0;
      stop_scrolling();
      return;
   }

   if (is_dlg_active()){
      stop_scrolling();
      return;
   }

   if (roadmap_message_ticker_is_on()){
      stop_scrolling();
      return;
   }

   dialog = ssd_dialog_get_currently_active();

   if (dialog && !(dialog->flags & SSD_DIALOG_FLOAT)){
      stop_scrolling();
      return;
   }

   if ((pos.speed < 5) && (gps_active))
      idleCount++;
   else
   {
      idleCount = 0;
      stop_scrolling();
      return;
   }

   if (!focus || strcmp(focus, "GPS")){
      idleCount = 0;
      return;
   }


   if (is_scrolling()){
     idleCount = roadmap_config_get_integer(&RoadMapConfigSecondsToPopup) - 1;
     return;
  }

  //focus && !strcmp (focus, "GPS") &&
  if (idleCount == roadmap_config_get_integer(&RoadMapConfigSecondsToPopup))
  {
     start_scrolling();
  }

  if (idleCount > roadmap_config_get_integer(&RoadMapConfigSecondsToPopup))
      idleCount = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimePopUp_Init(void){

   int i;

   for (i=0; i<MAX_SCROLLERS; i++) {
      scrollers[i].scroller = NULL;
   }

   roadmap_config_declare ("preferences", &RoadMapConfigMinDistance, "1500", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigSecondsToPopup, "15", NULL);


   roadmap_main_set_periodic(1000, RealtimePopUp_Timer);

}

