/* editor_points.h - New roads points 
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 */
 
#ifndef INCLUDE__EDITOR_POINTS__H
#define INCLUDE__EDITOR_POINTS__H
 
void editor_points_initialize (void);
int  editor_points_get_new_points(void);
void editor_points_display_new_points(int points);
void editor_points_display_new_points_timed(int points, int seconds, int event);
void editor_points_add_new_points(int points);
void editor_points_hide (void);
void editor_points_display(int road_length);
void editor_points_add(int road_length);
int editor_points_get(void);

int editor_points_get_old_points(void);
void editor_points_set_old_points(int points, int timeStamp);

int editor_points_get_total_points(void);
int editor_points_reset_munching (void);

#endif // INCLUDE__EDITOR_POINTS__H
