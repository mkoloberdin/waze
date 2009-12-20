/* roadmap_alert.h - Manage the alert points in DB.
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
 */

#ifndef  _ROADMAP_ALERT_H_
#define _ROADMAP_ALERT_H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_alerter.h"

#define ALERT_CATEGORY_SPEED_CAM        2
#define ALERT_CATEGORY_DUMMY_SPEED_CAM  3
#define ALERT_CATEGORY_RED_LIGHT_CAM    4

int roadmap_alert_count(void);
int roadmap_alert_get_category(int alert);
unsigned int roadmap_alert_get_speed(int alert);
int roadmap_alert_get_id(int alert);
void roadmap_alert_get_position(int alert, RoadMapPosition *position, int *steering);
int roadmap_alert_alertable(int alert);
const char * 	roadmap_alert_get_map_icon(int alert);
const char *  roadmap_alert_get_warning_icon(int alert);
const char *  roadmap_alert_get_alert_icon(int alert);
int roadmap_alert_get_distance(int record);
RoadMapSoundList  roadmap_alert_get_alert_sound(int Id);
const char *  roadmap_alert_get_string(int alert);
extern roadmap_db_handler RoadMapAlertHandler;
extern roadmap_alert_providor RoadmapAlertProvidor;

#endif // _ROADMAP_ALERT_H_
