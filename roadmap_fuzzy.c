/* roadmap_fuzzy.c - implement fuzzy operators for roadmap navigation.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_fuzzy.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_line.h"
#include "roadmap_math.h"
#include "roadmap_locator.h"
#include "roadmap_plugin.h"
#include "roadmap_line_route.h"
#include "roadmap_street.h"

#include "roadmap_fuzzy.h"


#define FUZZY_TRUTH_MAX 1024


static RoadMapConfigDescriptor RoadMapConfigConfidence =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Confidence");

static RoadMapConfigDescriptor RoadMapConfigAccuracyStreet =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Street");

static int RoadMapAccuracyStreet;
static int RoadMapConfidence;
static int RoadMapError = 10;


void roadmap_fuzzy_set_cycle_params (int street_accuracy, int confidence) {

    RoadMapAccuracyStreet = street_accuracy;
    RoadMapConfidence = confidence;
}


void roadmap_fuzzy_reset_cycle (void) {

    RoadMapAccuracyStreet =
        roadmap_config_get_integer (&RoadMapConfigAccuracyStreet);
    RoadMapConfidence =
        roadmap_config_get_integer (&RoadMapConfigConfidence);
}


int roadmap_fuzzy_max_distance (void) {

    return RoadMapAccuracyStreet;
}


/* Angle fuzzyfication:
 * max fuzzy value if same angle as reference, 0 if 60 degree difference,
 * constant slope in between.
 */
RoadMapFuzzy roadmap_fuzzy_direction
         (int direction, int reference, int symetric) {

    int delta = (direction - reference);

    if (symetric) {
       
       delta = delta % 180;

       /* The membership function is symetrical around the zero point,
        * and each side is symetrical around the 90 point: use these
        * properties to fold the delta.
        */
       while (delta < 0) delta += 180;
       if (delta > 90) delta = 180 - delta;

    } else {

       delta = delta % 360;
       while (delta < 0) delta += 360;
       if (delta > 180) delta = 360 - delta;
    }

    if (delta >= 90) return 0;

    return (FUZZY_TRUTH_MAX * (100 - delta)) / 100;
}


/* Distance fuzzyfication:
 * max fuzzy value if distance is small, 0 if objects far away,
 * constant slope in between.
 */
RoadMapFuzzy roadmap_fuzzy_distance  (int distance) {

    if (distance >= RoadMapAccuracyStreet) return 0;

    if (distance <= RoadMapError) return FUZZY_TRUTH_MAX;

    distance *= 2;

    if (distance >= RoadMapAccuracyStreet) return FUZZY_TRUTH_MAX / 3;

    return (FUZZY_TRUTH_MAX * (RoadMapAccuracyStreet - distance)) /
                                  RoadMapAccuracyStreet - RoadMapError;
}


RoadMapFuzzy roadmap_fuzzy_connected
                 (const RoadMapNeighbour *street,
                  const RoadMapNeighbour *reference,
                        int               prev_direction,
                        int               direction,
                        RoadMapPosition  *connection) {

    /* The logic for the connection membership function is that
     * the line is the most connected to itself, is well connected
     * to lines to which it share a common point and is not well
     * connected to other lines.
     * The weight of each state is a matter fine tuning.
     */
    RoadMapPosition line_point[2];
    RoadMapPosition reference_point[2];

    int i, j;


    if (roadmap_plugin_same_db_line (&street->line, &reference->line))
       return (FUZZY_TRUTH_MAX * 3) / 4;

	 roadmap_street_extend_line_ends (&street->line,&(line_point[0]), &(line_point[1]), FLAG_EXTEND_BOTH, NULL, NULL); 
	 roadmap_street_extend_line_ends (&reference->line,&(reference_point[0]), &(reference_point[1]), FLAG_EXTEND_BOTH, NULL, NULL); 

    if (direction == ROUTE_DIRECTION_AGAINST_LINE) {
       i = 1;
    } else {
       i = 0;
    }

    if (prev_direction == ROUTE_DIRECTION_AGAINST_LINE) {
       j = 0;
    } else {
       j = 1;
    }

    if ((line_point[i].latitude == reference_point[j].latitude) &&
         (line_point[i].longitude == reference_point[j].longitude)) {

       *connection = line_point[i];

       return (FUZZY_TRUTH_MAX * 2) / 3;
    } else if ((line_point[i].latitude == reference_point[!j].latitude) &&
         (line_point[i].longitude == reference_point[!j].longitude)) {

       *connection = line_point[i];

       return FUZZY_TRUTH_MAX / 2;

    } else {
       RoadMapPosition pos1;
       RoadMapPosition pos2;
       int d;

       pos1 = line_point[i];
       pos2 = reference_point[j];

       d = roadmap_math_distance (&pos1, &pos2);

       if (roadmap_math_distance (&pos1, &pos2) < 50) {
          connection->latitude  = 0;
          connection->longitude = 0;

          return FUZZY_TRUTH_MAX * 2 / 3;
       }
    }

    connection->latitude  = 0;
    connection->longitude = 0;

    return FUZZY_TRUTH_MAX / 3;
}


RoadMapFuzzy roadmap_fuzzy_and (RoadMapFuzzy a, RoadMapFuzzy b) {

   return a*b/FUZZY_TRUTH_MAX;
    if (a < b) return a;

    return b;
}


RoadMapFuzzy roadmap_fuzzy_or (RoadMapFuzzy a, RoadMapFuzzy b) {

    if (a > b) return a;

    return b;
}


RoadMapFuzzy roadmap_fuzzy_not (RoadMapFuzzy a) {

    return FUZZY_TRUTH_MAX - a;
}


RoadMapFuzzy roadmap_fuzzy_false (void) {
    return 0;
}


int roadmap_fuzzy_is_acceptable (RoadMapFuzzy a) {
    return (a >= RoadMapConfidence);
}


int roadmap_fuzzy_is_good (RoadMapFuzzy a) {
    return (a >= FUZZY_TRUTH_MAX / 2);
}

int roadmap_fuzzy_is_certain (RoadMapFuzzy a) {

    return (a >= (FUZZY_TRUTH_MAX * 2) / 3);
}


void roadmap_fuzzy_initialize (void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigAccuracyStreet, "120", NULL);

    roadmap_config_declare
        ("preferences", &RoadMapConfigConfidence, "50", NULL);
}


RoadMapFuzzy roadmap_fuzzy_good (void) {
   return FUZZY_TRUTH_MAX / 2;
}


RoadMapFuzzy roadmap_fuzzy_acceptable (void) {
   return RoadMapConfidence;
}


