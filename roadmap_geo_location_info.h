/* roadmap_geo_location_info.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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


#ifndef ROADMAP_GEO_LOCATION_INFO_H_
#define ROADMAP_GEO_LOCATION_INFO_H_

#include "roadmap.h"

#define  GEO_LOCATION_CONFIG_TYPE         ("user")
#define  GEO_LOCATION_ENABLE_CONFIG_TYPE  ("preferences")
#define  GEO_LOCATION_TAB                 ("Geo-Location")


#define  GEO_LOCATION_ENABLED_Name        ("Enabled")
#define  GEO_LOCATION_DISPLAYED_Name      ("Displayed")
#define  GEO_LOCATION_Yes    ("yes")
#define  GEO_LOCATION_No     ("no")


void roadmap_geo_location_info(void);

void roadmap_geo_location_set_metropolitan(const char *metro);

void roadmap_geo_location_set_metropolitan(const char *metro);
void roadmap_geo_location_set_state(const char *state);
void roadmap_geo_location_set_map_score(int map_score);
void roadmap_geo_location_set_map_score_str(const char* map_score_str);
void roadmap_geo_location_set_traffic_score(int traffic_score);
void roadmap_geo_location_set_traffic_score_str(const char* traffic_score_str);
void roadmap_geo_location_set_usage_score(int usage_score);
void roadmap_geo_location_set_usage_score_str(const char* usage_score_str);
void roadmap_geo_location_set_overall_score(int overall_score);

#endif /* ROADMAP_GEO_LOCATION_INFO_H_ */
