/* roadmap_alerter.h - Handles the alerting of users about nearby alerts
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

#ifndef  _ROADMAP_ALERTS__H_
#define _ROADMAP_ALERTS__H_

#include "roadmap_gps.h"
#include "roadmap_sound.h"
#include "roadmap_plugin.h"
#include "roadmap_types.h"

#define ALERTER_SQUARE_ID_UNINITIALIZED -1
#define ALERTER_SQUARE_ID_INVALID -2


#define ALERTER_PRIORITY_LOW 1
#define ALERTER_PRIORITY_MEDIUM 2
#define ALERTER_PRIORITY_HIGH 3

typedef struct {
   int square_id;
   int time_stamp;
   int line_id;
} roadmap_alerter_location_info;

typedef int 		(*roadmap_alerts_count)  (void);
typedef int 		(*roadmap_alerts_get_alert_id) (int alert);
typedef void  	(*roadmap_alerts_get_position)(int alert, RoadMapPosition *position, int *steering) ;
typedef unsigned int (*roadmap_alerts_get_speed)(int alert);
typedef const char * (*roadmap_alerts_get_map_icon)(int alertId);
typedef const char * (*roadmap_alerts_get_alert_icon)(int alertId);
typedef const char * (*roadmap_alerts_get_warning_icon)(int alertId);
typedef int  (*roadmap_alerts_get_distance)(int alert);
typedef RoadMapSoundList (*roadmap_alerts_get_sound)(int alertId);
typedef int  		(*roadmap_alerts_is_alertable)(int alert);
typedef BOOL 		(*roadmap_alerts_is_square_dependent)  (void);
typedef const char * (*roadmap_alerts_get_string)(int alertId);
typedef int  		(*roadmap_alerts_is_cancelable)(int alert);
typedef int  		(*roadmap_alerts_cancel)(int alert) ;
typedef int       (*roadmap_alerts_check_same_street)(int alert) ;
typedef int       (*roadmap_alerts_handle_event)(int alertId) ;
typedef int       (*roadmap_alerts_get_priority)  (void);
typedef roadmap_alerter_location_info * (*roadmap_alerts_get_location_info)(int alertId);
typedef BOOL      (*roadmap_alerts_distance_check)(RoadMapPosition gps_pos);
typedef BOOL      (*roadmap_alerts_can_send_thumbs_up)(int alert);
typedef int       (*roadmap_alerts_thumbs_up)(int alert) ;
typedef BOOL      (*roadmap_alerts_show_distance)(int alert);
typedef BOOL      (*roadmap_alerts_is_on_route)(int alert);
typedef struct {
   char *name;
   roadmap_alerts_count    					count;
   roadmap_alerts_get_alert_id 				get_id;
   roadmap_alerts_get_position  			   get_position;
   roadmap_alerts_get_speed					get_speed;
   roadmap_alerts_get_map_icon    			get_map_icon;
   roadmap_alerts_get_alert_icon    		get_alert_icon;
   roadmap_alerts_get_warning_icon		   get_warning_icon;
   roadmap_alerts_get_distance				get_distance;
   roadmap_alerts_get_sound					get_sound;
   roadmap_alerts_is_alertable				is_alertable;
   roadmap_alerts_get_string				   get_string;
   roadmap_alerts_is_cancelable			   is_cancelable;
   roadmap_alerts_cancel					   cancel;
   roadmap_alerts_check_same_street       check_same_street;
   roadmap_alerts_handle_event            handle_distance_event;
   roadmap_alerts_is_square_dependent     is_square_dependent;
   roadmap_alerts_get_location_info       get_location_info;
   roadmap_alerts_distance_check          distance_check;
   roadmap_alerts_get_priority            get_priority;
   roadmap_alerts_get_string              get_additional_string;
   roadmap_alerts_can_send_thumbs_up      can_send_thumbs_up;
   roadmap_alerts_thumbs_up               thumbs_up;
   roadmap_alerts_show_distance           show_distance;
   roadmap_alerts_handle_event            handle_event;
   roadmap_alerts_is_on_route             is_on_route;
   roadmap_alerts_handle_event            on_alerter_start;
   roadmap_alerts_handle_event            on_alerter_stop;
} roadmap_alert_provider;

typedef struct {
	roadmap_alert_provider	*provider[20];
	int									count;
}	roadmap_alert_providers;


void 		roadmap_alerter_initialize(void) ;
int 		roadmap_alerter_get_active_alert_id();
void 		roadmap_alerter_register(roadmap_alert_provider *provider);
void 		roadmap_alerter_check(const RoadMapGpsPosition *gps_position, const PluginLine *line);
void 		roadmap_alerter_display();
int      roadmap_alerter_get_priority();

#endif //_ROADMAP_ALERTS__H_
