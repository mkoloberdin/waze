/* roadmap_line_speed.c - Manage line speed data
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_line.h"
#include "roadmap_math.h"
#include "roadmap_line_speed.h"

static char *RoadMapLineSpeedType = "RoadMapLineSpeedContext";

typedef struct {

   char *type;

   RoadMapLineSpeedAvg *LineSpeedAvg;
   int                  LineSpeedAvgCount;

   RoadMapLineSpeed    *LineSpeedRef;
   int                  LineSpeedRefCount;

   RoadMapLineSpeedRef *LineSpeedSlots;
   int                  LineSpeedSlotsCount;

   int                 *LineSpeedIndex;
   int                  LineSpeedIndexCount;

} RoadMapLineSpeedContext;

static RoadMapLineSpeedContext *RoadMapLineSpeedActive = NULL;


static void *roadmap_line_speed_map (const roadmap_db_data_file *file) {

   RoadMapLineSpeedContext *context;

   context =
      (RoadMapLineSpeedContext *) calloc (1, sizeof(RoadMapLineSpeedContext));

   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }

   context->type = RoadMapLineSpeedType;

   if (!roadmap_db_get_data (file,
   								  model__tile_line_speed_avg,
   								  sizeof (RoadMapLineSpeedAvg),
   								  (void**)&(context->LineSpeedAvg),
   								  &(context->LineSpeedAvgCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid line_speed/avg structure");
   }

   if (!roadmap_db_get_data (file,
   								  model__tile_line_speed_line_ref,
   								  sizeof (RoadMapLineSpeed),
   								  (void**)&(context->LineSpeedRef),
   								  &(context->LineSpeedRefCount))) {
      roadmap_log (ROADMAP_FATAL, "invalid line_speed/line_ref structure");
   }

   if (context->LineSpeedRefCount) {

	   if (!roadmap_db_get_data (file,
	   								  model__tile_line_speed_index,
	   								  sizeof (int),
	   								  (void**)&(context->LineSpeedIndex),
	   								  &(context->LineSpeedIndexCount))) {
	      roadmap_log (ROADMAP_FATAL, "invalid line_speed/index structure");
	   }

	   if (!roadmap_db_get_data (file,
	   								  model__tile_line_speed_data,
	   								  sizeof (RoadMapLineSpeedRef),
	   								  (void**)&(context->LineSpeedSlots),
	   								  &(context->LineSpeedSlotsCount))) {
	      roadmap_log (ROADMAP_FATAL, "invalid line_speed/index structure");
	   }
   }

   return context;
}

static void roadmap_line_speed_activate (void *context) {

   RoadMapLineSpeedContext *line_speed_context =
      (RoadMapLineSpeedContext *) context;

   if ((line_speed_context != NULL) &&
       (line_speed_context->type != RoadMapLineSpeedType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line speed context activated");
   }
   RoadMapLineSpeedActive = line_speed_context;
}

static void roadmap_line_speed_unmap (void *context) {

   RoadMapLineSpeedContext *line_speed_context =
      (RoadMapLineSpeedContext *) context;

   if (line_speed_context->type != RoadMapLineSpeedType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line speed context");
   }
   free (line_speed_context);
}

roadmap_db_handler RoadMapLineSpeedHandler = {
   "line_speed",
   roadmap_line_speed_map,
   roadmap_line_speed_activate,
   roadmap_line_speed_unmap
};


static int get_time_slot (time_t when) {

#ifdef J2ME
   return 24;
#else
   int time_slot;
   struct tm *t = localtime (&when);

   time_slot = t->tm_hour * 2;

   if (t->tm_min >= 30) time_slot++;

   //time_slot = 18;
   return time_slot;
#endif
}


int get_speed_ref (int line, int against_dir) {

   RoadMapLineSpeed *ref;

   if (RoadMapLineSpeedActive == NULL) return INVALID_SPEED; /* No data. */
   if (line >= RoadMapLineSpeedActive->LineSpeedRefCount) return INVALID_SPEED;

   ref = RoadMapLineSpeedActive->LineSpeedRef +line;

   if (against_dir) return ref->to_speed_ref;
   else return ref->from_speed_ref;
}


static LineRouteTime calc_cross_time (int line, int time_slot,
                                      int against_dir) {

   int speed;
   int length, length_m;
   int speed_ref = get_speed_ref (line, against_dir);

   if (speed_ref == INVALID_SPEED) return 0;

   speed = roadmap_line_speed_get (speed_ref, time_slot);

   if (!speed) return 0;

   length = roadmap_line_length (line);
   
   length_m = roadmap_math_to_cm(length) / 100;

   return (LineRouteTime)(length_m * 3.6 / speed) + 1;
}


static LineRouteTime calc_avg_cross_time (int line, int against_dir) {

   int speed;
   int length, length_m;
   int speed_ref = get_speed_ref (line, against_dir);

   if (speed_ref == INVALID_SPEED) return 0;

   speed = roadmap_line_speed_get_avg (speed_ref);

   if (!speed) return 0;
   length = roadmap_line_length (line);
   length_m = roadmap_math_to_cm(length) / 100;
   return (LineRouteTime)(length_m * 3.6 / speed) + 1;
}



int roadmap_line_speed_get_avg (int speed_ref) {

   RoadMapLineSpeedRef *speed;
   int index;
   int count = 0;
   int total = 0;

   if (RoadMapLineSpeedActive == NULL) return 0; /* No data. */
   if (RoadMapLineSpeedActive->LineSpeedIndexCount <= speed_ref) {
      roadmap_log (ROADMAP_ERROR, "Invalid speed_ref index:%d", speed_ref);
      return 0;
   }

   index = RoadMapLineSpeedActive->LineSpeedIndex[speed_ref];
   speed = &RoadMapLineSpeedActive->LineSpeedSlots[index];

   while (1) {
      total += speed->speed;
      count++;

      if (speed->time_slot & SPEED_EOL) break;

      speed++;
   }

   return total / count;
}


int roadmap_line_speed_get (int speed_ref, int time_slot) {

   RoadMapLineSpeedRef *speed;
   int index;

   if (RoadMapLineSpeedActive == NULL) return 0; /* No data. */
   if (RoadMapLineSpeedActive->LineSpeedIndexCount <= speed_ref) {
      roadmap_log (ROADMAP_ERROR, "Invalid speed_ref index:%d", speed_ref);
      return 0;
   }

   index = RoadMapLineSpeedActive->LineSpeedIndex[speed_ref];
   speed = &RoadMapLineSpeedActive->LineSpeedSlots[index];

   while (!(speed->time_slot & SPEED_EOL) &&
      (((speed+1)->time_slot & ~SPEED_EOL) <= time_slot)) {

      speed++;
   }

   return speed->speed;
}


int roadmap_line_speed_get_cross_times (int line,
                                        LineRouteTime *from,
                                        LineRouteTime *to) {

   int time_slot;

   if (RoadMapLineSpeedActive == NULL) return -1; /* No data. */
   if (RoadMapLineSpeedActive->LineSpeedRefCount <= line) return -1;

//   *from = calc_avg_cross_time (line, route->from_speed_ref);
//   *to = calc_avg_cross_time (line, route->to_speed_ref);

   time_slot = get_time_slot (time(NULL));

   *from = calc_cross_time (line, time_slot, 0);
   *to = calc_cross_time (line, time_slot, 1);

   return 0;
}


int roadmap_line_speed_get_cross_time_at (int line, int against_dir,
                                          time_t at_time) {

   int time_slot;

   time_slot = get_time_slot (at_time);

   return calc_cross_time (line, time_slot, against_dir);
}


int roadmap_line_speed_get_cross_time (int line, int against_dir) {

   return roadmap_line_speed_get_cross_time_at (line, against_dir, time(NULL));
}


int roadmap_line_speed_get_avg_cross_time (int line, int against_dir) {

   int speed;
   int length, length_m;
   int avg_time;

   if (RoadMapLineSpeedActive == NULL) return 0; /* No data. */

   if (line < RoadMapLineSpeedActive->LineSpeedRefCount) {

      avg_time = calc_avg_cross_time (line, against_dir);

      if (avg_time) return avg_time;
   }


   if (line < RoadMapLineSpeedActive->LineSpeedAvgCount) {
      RoadMapLineSpeedAvg *avg = RoadMapLineSpeedActive->LineSpeedAvg + line;
      if (!against_dir) {
         speed = avg->from_avg_speed;
      } else {
         speed = avg->to_avg_speed;
      }

      if (!speed) speed = 30;

      length = roadmap_line_length (line);

	  length_m = roadmap_math_to_cm(length) / 100;

      return (int)(length_m * 3.6 / speed) + 1;
   }

   return -1;
}


int roadmap_line_speed_get_speed (int line, int against_dir) {

   int time_slot;
   int speed_ref = get_speed_ref (line, against_dir);

   if (speed_ref == INVALID_SPEED) return 0;

   time_slot = get_time_slot (time(NULL));

   return roadmap_line_speed_get (speed_ref, time_slot);
}


int roadmap_line_speed_get_avg_speed (int line, int against_dir) {

   int speed_ref = get_speed_ref (line, against_dir);

   if (speed_ref == INVALID_SPEED) return 0;

   return roadmap_line_speed_get_avg (speed_ref);
}


