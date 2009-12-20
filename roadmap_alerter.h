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
typedef const char * (*roadmap_alerts_get_string)(int alertId);
typedef int  		(*roadmap_alerts_is_cancelable)(int alert);
typedef int  		(*roadmap_alerts_cancel)(int alert) ;
typedef int       (*roadmap_alerts_check_same_street)(int alert) ;
typedef int       (*roadmap_alerts_handle_event)(int alertId) ;

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
   roadmap_alerts_handle_event            handle_event;
} roadmap_alert_providor;

typedef struct {
	roadmap_alert_providor	*providor[20];
	int									count;
}	roadmap_alert_providors;

void 		roadmap_alerter_initialize(void) ;
int 		roadmap_alerter_get_active_alert_id();
void 		roadmap_alerter_register(roadmap_alert_providor *providor);
void 		roadmap_alerter_check(const RoadMapGpsPosition *gps_position, const PluginLine *line);
void 		roadmap_alerter_display();

#endif //_ROADMAP_ALERTS__H_
