/* editor_track_known.c - handle tracks of known points
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 * 
 * SYNOPSYS:
 *
 *   See editor_track_unknown.h
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_square.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_route.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "editor_track_main.h"
#include "editor_track_util.h"

#include "editor_track_known.h"

#define MAX_DISTANCE 10000

#define MAX_ENTRIES 15
#define MAX_PATHS 10

#define DEBUG_TRACKS	0

typedef struct {
   RoadMapTracking street;
   RoadMapNeighbour line;
   int point_id;
} KnownCandidateEntry;

typedef struct {
   KnownCandidateEntry entries[MAX_ENTRIES];
   int count;
   int low_confidence;
} KnownCandidatePath;

static KnownCandidatePath KnownCandidates[MAX_PATHS];
static int KnownCandidatesCount;

#define ROADMAP_NEIGHBOURHOUD  16
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];


int editor_track_known_num_candidates (void) {

	return KnownCandidatesCount;	
}


static int resolve_add (KnownCandidatePath *path, int point_id,
                        RoadMapNeighbour *candidate,
                        RoadMapFuzzy fuzzy,
                        RoadMapTracking *street) {

   KnownCandidateEntry *entry = path->entries + path->count - 1;

   if (!roadmap_plugin_same_line
         (&entry->line.line, &candidate->line)) {

      int p = 0;
      int i;

      for (i=0; i<path->count; i++) {
         p += path->entries[i].point_id;
      }

      p = point_id - p;

      if (++path->count == MAX_ENTRIES) {
         return -1;
      }

      entry++;
      entry->point_id = p;
      assert(p >= 0);
      entry->line = *candidate;
      entry->street = *street;
      entry->street.entry_fuzzyfied = entry->street.cur_fuzzyfied = fuzzy;
      entry->street.valid = 1;
   } else {
      entry->street.cur_fuzzyfied = fuzzy;
   }

   return 0;
}


static int resolve_candidates (const RoadMapGpsPosition *gps_position,
                               int point_id) {

   int count;
   int c;
   int found;
   RoadMapFuzzy best = roadmap_fuzzy_false ();
   RoadMapFuzzy second_best;
   int i;
   int j;

   for (c = KnownCandidatesCount - 1; c >= 0; c--) {

      RoadMapTracking new_street;

      KnownCandidateEntry *entry =
         KnownCandidates[c].entries + KnownCandidates[c].count - 1;
   
      count = editor_track_util_find_street
         (gps_position, &new_street,
          &entry->street,
          &entry->line,
          RoadMapNeighbourhood,
          ROADMAP_NEIGHBOURHOUD, &found, &best, &second_best);

      if (!roadmap_fuzzy_is_good (best)) {

         if (KnownCandidatesCount == 1) {
            /* This is the last path, force use it, but lower its confidence */
            KnownCandidates[0].low_confidence = 1;

            return 0;
         }

         memmove (KnownCandidates + c, KnownCandidates + c + 1,
               (KnownCandidatesCount - c - 1) * sizeof (KnownCandidatePath));

         KnownCandidatesCount--;
         continue;
      }

      if (roadmap_fuzzy_is_good (second_best) &&
         !roadmap_plugin_same_db_line
               (&entry->line.line, &RoadMapNeighbourhood[found].line)) {

         /* We need to create new paths */

         for (i = 0; i < count; ++i) {
            RoadMapTracking candidate;
            int result;

            /* We handle the best result below this block */
            if (i == found) continue;

            result = roadmap_navigate_fuzzify
               (&candidate,
                &entry->street,
                &entry->line,
                RoadMapNeighbourhood+i,
                0,
                gps_position->steering);

            if (roadmap_fuzzy_is_good (result)) {

               if (roadmap_plugin_same_line
                  (&entry->line.line, &RoadMapNeighbourhood[i].line)) {

                  /* Skip the current line */
                  continue;
               }

               if (KnownCandidatesCount == MAX_PATHS) return -1;

               memcpy (KnownCandidates + KnownCandidatesCount,
                       KnownCandidates + c, sizeof (KnownCandidatePath));

               resolve_add (&KnownCandidates[KnownCandidatesCount],
                     point_id,
                     &RoadMapNeighbourhood[i],
                     result,
                     &candidate);

               KnownCandidatesCount++;
            }
         }
      }

      resolve_add (&KnownCandidates[c],
                   point_id,
                   &RoadMapNeighbourhood[found],
                   best,
                   &new_street);
   }

   /* check for duplicates */
   for (i = 0; i < KnownCandidatesCount-1; i++) {

      KnownCandidateEntry *entry1 =
         KnownCandidates[i].entries + KnownCandidates[i].count - 1;
      
      for (j = i+1; j < KnownCandidatesCount; j++) {

         KnownCandidateEntry *entry2 =
            KnownCandidates[j].entries + KnownCandidates[j].count - 1;

         if (roadmap_plugin_same_line
               (&entry2->line.line, &entry1->line.line)) {

            int entry1_score = 0;
            int entry2_score = 0;
            int do_switch = 0;
            int k;
            int entries_count;

            if (KnownCandidates[i].count == KnownCandidates[j].count) {
               /* don't count the last entry which is the same one */
               entries_count = KnownCandidates[i].count - 1;

            } else if (KnownCandidates[i].count < KnownCandidates[j].count) {
               entries_count = KnownCandidates[i].count;
            } else {
               entries_count = KnownCandidates[j].count;
            }

            for (k=0; k<entries_count; k++) {
               entry1_score +=
                  KnownCandidates[i].entries[k].street.entry_fuzzyfied;
               entry2_score +=
                  KnownCandidates[j].entries[k].street.entry_fuzzyfied;
            }

            if (entries_count) {
               entry1_score /= entries_count;
               entry2_score /= entries_count;
            }

            if ((abs(entry1_score - entry2_score) <
                  (roadmap_fuzzy_good () / 10)) &&
               (KnownCandidates[i].count != KnownCandidates[j].count)) {
               
               if (KnownCandidates[j].count < KnownCandidates[i].count) {
                  do_switch = 1;
               }

            } else if (entry2_score > entry1_score) {
               do_switch = 1;
            }
               
            if (do_switch) {
               memcpy (KnownCandidates + i, KnownCandidates + j,
                  sizeof (KnownCandidatePath));
            }

            /* remove the obsolete entry */
            memmove (KnownCandidates + j, KnownCandidates + j + 1,
                  (KnownCandidatesCount - j - 1) * sizeof (KnownCandidatePath));

            KnownCandidatesCount--;
            j--;

            KnownCandidates[i].low_confidence = 1;
         }
      }
   }

   return 0;
}


int editor_track_known_end_segment (PluginLine *previous_line,
                                    int last_point_id,
                                    PluginLine *line,
                                    RoadMapTracking *street,
                                    int is_new_track) {
   //TODO: add stuff
   //Notice that previous_line may not be valid (only at first)
   //
   //check for low confidence & in static update, do not use a trkseg with low confidence!

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition *current;
   int trkseg;
   int trkseg_line_id;
   int trkseg_plugin_id;
   int trkseg_square;
   int line_length;
   int segment_length;
   int percentage;
   int flags = 0;

   editor_log_push ("editor_track_end_known_segment");

   assert (last_point_id != 0);
   if (!last_point_id) return 0;

   if (editor_db_activate (line->fips) == -1) {
      editor_db_create (line->fips);
      if (editor_db_activate (line->fips) == -1) {
         editor_log_pop ();
         return 0;
      }
   }

	roadmap_street_extend_line_ends (line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
   trkseg_plugin_id = roadmap_plugin_get_id (line);
   trkseg_line_id = roadmap_plugin_get_line_id (line);
   trkseg_square = roadmap_plugin_get_square (line);
   
   line_length = editor_track_util_get_line_length (line);
   segment_length = editor_track_util_length (0, last_point_id);

   editor_log
      (ROADMAP_INFO,
         "Ending line %d (plugin_id:%d). Line length:%d, Segment length:%d",
         trkseg_line_id, trkseg_plugin_id, line_length, segment_length);

   /* End current segment if we really passed through it
    * and not only touched a part of it.
    */

	/*SRUL*: avoid this problem, see above comment 
   assert (line_length > 0);

   if (line_length == 0) {
      editor_log (ROADMAP_ERROR, "line %d (plugin_id:%d) has length of zero.",
            trkseg_line_id, trkseg_plugin_id);
      editor_log_pop ();
      return 0;
   }
	*/
   if (line_length <= 0) line_length = 1;
	
   current = track_point_pos (last_point_id);
   if (roadmap_math_distance (current, &to) >
       roadmap_math_distance (current, &from)) {

      flags = ED_TRKSEG_OPPOSITE_DIR;
#if DEBUG_TRACKS
	   printf ("Closing trkseg from %d.%06d,%d.%06d to %d.%06d,%d.%06d",
	   			to.longitude / 1000000, to.longitude % 1000000,
	   			to.latitude / 1000000, to.latitude % 1000000,
	   			from.longitude / 1000000, from.longitude % 1000000,
	   			from.latitude / 1000000, from.latitude % 1000000);

   } else {
      
	   printf ("Closing trkseg from %d.%06d,%d.%06d to %d.%06d,%d.%06d",
	   			from.longitude / 1000000, from.longitude % 1000000,
	   			from.latitude / 1000000, from.latitude % 1000000,
	   			to.longitude / 1000000, to.longitude % 1000000,
	   			to.latitude / 1000000, to.latitude % 1000000);
#endif
   }
   	
#if DEBUG_TRACKS
   printf (" %d/%d", segment_length, line_length);
#endif
   		
   if (is_new_track) {
      flags |= ED_TRKSEG_NEW_TRACK;
#if DEBUG_TRACKS
      printf (" NEW");
#endif
   }

	if (!editor_ignore_new_roads ()) {
		flags |= ED_TRKSEG_RECORDING_ON;
	}
	
   percentage = 100 * segment_length / line_length;
   if (percentage < 70) {
      editor_log (ROADMAP_INFO, "segment is too small to consider: %d%%",
            percentage);
      if (segment_length > (editor_track_point_distance ()*1.5)) {

         trkseg = editor_track_util_create_trkseg
                     (trkseg_square, trkseg_line_id, trkseg_plugin_id, 0, last_point_id,
                      flags|ED_TRKSEG_LOW_CONFID);

         editor_track_add_trkseg
            (line, trkseg, ROUTE_DIRECTION_NONE, ROUTE_CAR_ALLOWED);
         editor_log_pop ();
#if DEBUG_TRACKS
         printf (" LOW (percentage)\n");
#endif
         return 1;
      } else {

         trkseg = editor_track_util_create_trkseg
                  (trkseg_square, trkseg_line_id, trkseg_plugin_id,
                   0, last_point_id, flags|ED_TRKSEG_IGNORE);
         editor_track_add_trkseg
            (line, trkseg, ROUTE_DIRECTION_NONE, ROUTE_CAR_ALLOWED);
         editor_log_pop ();
#if DEBUG_TRACKS
         printf (" IGNORE\n");
#endif
         return 0;
      }
   }

#if 0
   if (!roadmap_fuzzy_is_good (street->entry_fuzzyfied) ||
      !roadmap_fuzzy_is_good (street->cur_fuzzyfied)) {

      flags |= ED_TRKSEG_LOW_CONFID;
#if DEBUG_TRACKS
      printf (" LOW");
      if (!roadmap_fuzzy_is_good (street->entry_fuzzyfied))
      	printf (" (entry)");
      if (!roadmap_fuzzy_is_good (street->cur_fuzzyfied))
      	printf (" (cur)");
#endif
   }
#endif

   if (trkseg_plugin_id != ROADMAP_PLUGIN_ID) flags |= ED_TRKSEG_LOW_CONFID;

   trkseg =
      editor_track_util_create_trkseg
         (trkseg_square, trkseg_line_id, trkseg_plugin_id, 0, last_point_id, flags);

   if (flags & ED_TRKSEG_OPPOSITE_DIR) {
      
      editor_log (ROADMAP_INFO, "Updating route direction: to -> from");
      editor_track_add_trkseg
         (line, trkseg, ROUTE_DIRECTION_AGAINST_LINE, ROUTE_CAR_ALLOWED);
   } else {

      editor_log (ROADMAP_INFO, "Updating route direction: from -> to");
      editor_track_add_trkseg
         (line, trkseg, ROUTE_DIRECTION_WITH_LINE, ROUTE_CAR_ALLOWED);
   }


   editor_log_pop ();

#if DEBUG_TRACKS
	printf ("\n");
#endif
   return 1;
}


int editor_track_known_resolve (void) {
   return (KnownCandidatesCount == 1);
}

void editor_track_known_reset_resolve (void) {
	KnownCandidatesCount = 0;
}

int editor_track_known_locate_point (int point_id,
                                     const RoadMapGpsPosition *gps_position,
                                     RoadMapTracking *confirmed_street,
                                     RoadMapNeighbour *confirmed_line,
                                     RoadMapTracking *new_street,
                                     RoadMapNeighbour *new_line) {

   int found;
   int count;
   int current_fuzzy;
   RoadMapFuzzy best = roadmap_fuzzy_false ();
   RoadMapFuzzy second_best;
   
   const RoadMapPosition *position = track_point_pos (point_id);
   
   RoadMapFuzzy before;
   
   if (KnownCandidatesCount > 1) {
      
      if ((resolve_candidates (gps_position, point_id) == -1) ||
            !KnownCandidatesCount) {

         KnownCandidatesCount = 0;
         editor_track_reset ();
         return 0;
      }

      if (KnownCandidatesCount > 1) {

         return 0;
      }
   }

   if (KnownCandidatesCount == 1) {

      KnownCandidateEntry *entry = KnownCandidates[0].entries;

      if (!--KnownCandidates[0].count) {
         KnownCandidatesCount = 0;
      }

      *new_line = entry->line;
      *new_street = entry->street;
      if (KnownCandidates[0].low_confidence)
         new_street->entry_fuzzyfied = roadmap_fuzzy_acceptable ();

      point_id = entry->point_id;

      memmove (entry, entry+1,
            KnownCandidates[0].count * sizeof(KnownCandidateEntry));

      if (!point_id) {

         if (confirmed_street->valid &&
            roadmap_plugin_same_line (&confirmed_line->line, &new_line->line)) {

            return 0;
         }

         *confirmed_street = *new_street;
         *confirmed_line = *new_line;
         confirmed_street->valid = 1;

         if (confirmed_street->valid) {
            /* This is probably a GPS jump or an error in resolve */
            return -1;
         }
      }

      return point_id;
   }

   if (confirmed_street->valid) {
      /* We have an existing street match: check if it is still valid. */
      
      before = confirmed_street->cur_fuzzyfied;
        
      if (!editor_track_util_get_distance (position,
                                           &confirmed_line->line,
                                           confirmed_line)) {
         current_fuzzy = 0;
         confirmed_line->distance = MAX_DISTANCE;
      } else {

         current_fuzzy = roadmap_navigate_fuzzify
                           (new_street,
                            confirmed_street,
                            confirmed_line,
                            confirmed_line,
                            0,
                            gps_position->steering);
      }

      if (current_fuzzy &&
            (new_street->line_direction == confirmed_street->line_direction) &&
            ((current_fuzzy >= before) ||
            roadmap_fuzzy_is_certain(current_fuzzy))) {

         confirmed_street->cur_fuzzyfied = current_fuzzy;
         return 0; /* We are on the same street. */
      }
   }
   
   /* We must search again for the best street match. */
   
   count = editor_track_util_find_street
            (gps_position, new_street,
             confirmed_street,
             confirmed_line,
             RoadMapNeighbourhood,
             ROADMAP_NEIGHBOURHOUD, &found, &best, &second_best);

#if 0
   if (count && (RoadMapNeighbourhood[found].distance < 30) &&
      !roadmap_fuzzy_is_acceptable (best)) {
      /* if we are on a known street, we don't want to start a new road
       * if we are still close to it. In this case we ignore the fuzzy
       */
   } else
#endif     

   if (!roadmap_fuzzy_is_good (best) &&
         roadmap_fuzzy_is_acceptable (best) &&
         confirmed_street->valid) {
      /* We're not sure that the new line is a real match.
       * Delay the decision if we're still close to the previous road.
       */
      if (confirmed_line->distance < editor_track_point_distance ()) {
         RoadMapTracking candidate;

         if (roadmap_plugin_same_line (&RoadMapNeighbourhood[found].line,
                                       &confirmed_line->line)) {
            /* We don't have any candidates other than the current line */
            return 0;
         }

         /* We have two candidates here */

         /* current line */
         KnownCandidates[0].entries[0].street = *confirmed_street;
         KnownCandidates[0].entries[0].line = *confirmed_line;
         KnownCandidates[0].entries[0].point_id = 0;
         KnownCandidates[0].count = 1;
         KnownCandidates[0].low_confidence = 0;

         /* new line */
         candidate.entry_fuzzyfied = roadmap_navigate_fuzzify
                                 (&candidate, confirmed_street,
                                  confirmed_line, RoadMapNeighbourhood+found,
                                  0,
                                  gps_position->steering);
         candidate.cur_fuzzyfied = candidate.entry_fuzzyfied;
         candidate.valid = 1;
         KnownCandidates[1].entries[0].street = candidate;
         KnownCandidates[1].entries[0].line = RoadMapNeighbourhood[found];
         KnownCandidates[1].entries[0].point_id = point_id;
         KnownCandidates[1].count = 1;
         KnownCandidates[1].low_confidence = 0;
         KnownCandidatesCount = 2;
         return 0;
      }
   }

   if ((roadmap_fuzzy_is_good (best) &&
        roadmap_fuzzy_is_good (second_best)) ||
       (!roadmap_fuzzy_is_good (best) &&
         roadmap_fuzzy_is_acceptable (second_best))) {

      /* We got too many options. Wait before deciding. */

      int i;
      RoadMapFuzzy result;
      int use_acceptable = 0;

      if (!roadmap_fuzzy_is_good (best)) use_acceptable = 1;

      for (i = 0; i < count; ++i) {
         RoadMapTracking candidate;

         result = roadmap_navigate_fuzzify
                    (&candidate,
                     confirmed_street,
                     confirmed_line,
                     RoadMapNeighbourhood+i,
                     0,
                     gps_position->steering);
      
         if (roadmap_fuzzy_is_good (result) ||
            (use_acceptable && roadmap_fuzzy_is_acceptable (result))) {

            if (confirmed_street->valid &&
                roadmap_plugin_same_line (&confirmed_line->line,
                &RoadMapNeighbourhood[i].line)) {
               
               if (result < best) {
               
                  continue;
               } else {
                  /* delay the decision as the current line is still good */
                  KnownCandidatesCount = 0;
                  return 0;
               }
            }

            candidate.valid = 1;
            candidate.entry_fuzzyfied = candidate.cur_fuzzyfied = result;
            KnownCandidates[KnownCandidatesCount].entries[0].street = candidate;
            KnownCandidates[KnownCandidatesCount].entries[0].line =
                                                   RoadMapNeighbourhood[i];

            KnownCandidates[KnownCandidatesCount].entries[0].point_id =
                                                   point_id;

            KnownCandidates[KnownCandidatesCount].count = 1;
            KnownCandidates[KnownCandidatesCount].low_confidence = 0;
            KnownCandidatesCount++;
            if (KnownCandidatesCount == MAX_PATHS) {
               roadmap_log (ROADMAP_ERROR, "ResolveCandidates - Reached max entries!");
               break;
            }
         }
      }

      if (KnownCandidatesCount > 1) {

         return 0;
      } else {
         /* We only got one candidate so fall through to use it */
         KnownCandidatesCount = 0;
      }
   }

   if (roadmap_fuzzy_is_acceptable (best)) {

      if (confirmed_street->valid &&
            (!roadmap_plugin_same_line
               (&confirmed_line->line, &RoadMapNeighbourhood[found].line) ||
               (confirmed_street->opposite_street_direction !=
                new_street->opposite_street_direction) ||
               (confirmed_street->line_direction !=
                new_street->line_direction))) {

         *new_line = RoadMapNeighbourhood[found];
         new_street->valid = 1;
         new_street->entry_fuzzyfied = new_street->cur_fuzzyfied = best;

         return point_id;
      }

      *confirmed_line   = RoadMapNeighbourhood[found];

      if (!confirmed_street->valid) confirmed_street->entry_fuzzyfied = best;
      new_street->entry_fuzzyfied = confirmed_street->entry_fuzzyfied;
      *confirmed_street = *new_street;
      
      confirmed_street->cur_fuzzyfied = best;
      confirmed_street->valid = 1;

      return 0;

   } else {

      new_street->valid = 0;

      return point_id;
   }
}

