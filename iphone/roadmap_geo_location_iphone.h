/* roadmap_geo_location_iphone.h - iPhone geo location dialog
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __ROADMAP_GEO_LOCATION_IPHONE__H
#define __ROADMAP_GEO_LOCATION_IPHONE__H

typedef struct tag_geo_location_info
	{
		char    metro[128];
		char    state[50];
		char    map_score[20];
		char    traffic_score[20];
		char    usage_score[20];
		int     overall_score;
      int     wizard;
		
	}  geo_location_info;

@interface GeoInfoView : UIViewController <UIScrollViewDelegate> {	
	geo_location_info g_geo_info; 

}

- (void) onLetMeIn;

- (void) showWithMetro: (const char *)metro
                 State: (const char *)state
              MapScore: (const char *)map_score
          TrafficScore: (const char *)traffic_score
            UsageScore: (const char *)usage_score
          OverallScore: (int)overall_score;


@end


#endif // __ROADMAP_GEO_LOCATION_IPHONE__H