/* roadmap_fuzzy.h - implement fuzzy operators for roadmap navigation.
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
 * This module implement a fuzzy truth value that is somewhat different
 * from the textbooks: the range is 0..1024 instead of 0..1 in order to use
 * integer operations only.
 */

#ifndef INCLUDE__ROADMAP_FUZZY__H
#define INCLUDE__ROADMAP_FUZZY__H

#include "roadmap_street.h"

typedef int RoadMapFuzzy;

void roadmap_fuzzy_reset_cycle (void);
void roadmap_fuzzy_set_cycle_params (int street_accuracy, int confidence);

int roadmap_fuzzy_max_distance (void);

RoadMapFuzzy roadmap_fuzzy_direction
                 (int direction, int reference, int symetric);
RoadMapFuzzy roadmap_fuzzy_distance  (int distance);

RoadMapFuzzy roadmap_fuzzy_connected
                 (const RoadMapNeighbour *street,
                  const RoadMapNeighbour *reference,
                        int               prev_direction,
                        int               direction,
                        RoadMapPosition  *connection);

RoadMapFuzzy roadmap_fuzzy_and (RoadMapFuzzy a, RoadMapFuzzy b);
RoadMapFuzzy roadmap_fuzzy_or  (RoadMapFuzzy a, RoadMapFuzzy b);
RoadMapFuzzy roadmap_fuzzy_not (RoadMapFuzzy a);

RoadMapFuzzy roadmap_fuzzy_false (void);
int          roadmap_fuzzy_is_acceptable (RoadMapFuzzy a);
int          roadmap_fuzzy_is_good       (RoadMapFuzzy a);
int          roadmap_fuzzy_is_certain    (RoadMapFuzzy a);

RoadMapFuzzy roadmap_fuzzy_good (void);
RoadMapFuzzy roadmap_fuzzy_acceptable (void);

void roadmap_fuzzy_initialize (void);

#endif // INCLUDE__ROADMAP_FUZZY__H
