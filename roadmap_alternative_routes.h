/* roadmap_alternative_routes.h
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


#ifndef ROADMAP_ALTERNATIVE_ROUTES_H_
#define ROADMAP_ALTERNATIVE_ROUTES_H_
#include "ssd/ssd_widget.h"

#define ALT_ROUTESS_MIN_ZOOM_IN 60

BOOL roadmap_alternative_feature_enabled (void);
void roadmap_alternative_routes_init(void);
void roadmap_alternative_routes_routes_dialog (void);
void roadmap_alternative_routes_suggested_trip (void);
void roadmap_alternative_routes_suggest_route_dialog (void);
BOOL roadmap_alternative_routes_suggest_routes(void);
void roadmap_alternative_routes_set_suggest_routes(BOOL suggest_at_start);
void add_routes_selection(SsdWidget dialog);
int on_routes_selection_route (SsdWidget widget, const RoadMapGuiPoint *point);

#ifdef IPHONE_NATIVE
int roadmap_alternative_route_select (int index);
#endif //IPHONE_NATIVE

void roadmap_alternative_route_get_src (RoadMapPosition *way_point);
void roadmap_alternative_route_get_waypoint (int distance, RoadMapPosition *way_point) ;
void highligh_selection(int route);
BOOL roadmap_alternative_routes_suggest_dlg_active(void);
#endif /* ROADMAP_ALTERNATIVE_ROUTES_H_ */

