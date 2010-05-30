/*  roadmap_map_settings_settings.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Dan Friedman
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


#ifndef ROADMAP_MAP_SETTINGS_H_
#define ROADMAP_MAP_SETTINGS_H_

#include "roadmap_config.h"

void roadmap_map_settings_show(void);
void roadmap_map_settings_init(void);
BOOL roadmap_map_settings_isEnabled(RoadMapConfigDescriptor descriptor);
BOOL roadmap_map_settings_isShowWazers(void);
BOOL roadmap_map_settings_isShowReports(void);
BOOL roadmap_map_settings_isShowTopBarOnTap(void);
BOOL roadmap_map_settings_isAutoShowStreetBar(void);
BOOL roadmap_map_settings_show_report(int iType);
BOOL roadmap_map_settings_color_roads();
BOOL roadmap_map_settings_isShowSpeedCams();
int roadmap_map_settings_allowed_alerts(int outAlertsUserCanToggle[]);
void roadmap_map_settings_alert_string(char *outAlertString[]);
BOOL roadmap_map_settings_isShowSpeedometer();
BOOL roadmap_map_settings_road_goodies();
#endif /*ROADMAP_MAP_SETTINGS_H_*/
