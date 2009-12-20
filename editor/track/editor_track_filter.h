/* editor_track_filter.h - Filter GPS points
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
 */

#ifndef INCLUDE__EDITOR_TRACK_FILTER__H
#define INCLUDE__EDITOR_TRACK_FILTER__H

#define ED_TRACK_END 1

struct GPSFilter;

void editor_track_filter_reset (struct GPSFilter *filter);

struct GPSFilter *editor_track_filter_new (int max_distance,
                                           int max_stand_time,
                                           int timeout,
                                           int point_distance);

int editor_track_filter_add (struct GPSFilter *filter,
                             time_t gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position);

const RoadMapGpsPosition *editor_track_filter_get (struct GPSFilter *filter);

int editor_track_filter_get_current (const struct GPSFilter *filter, RoadMapPosition *pos, time_t *time);

#endif // INCLUDE__EDITOR_TRACK_FILTER__H

