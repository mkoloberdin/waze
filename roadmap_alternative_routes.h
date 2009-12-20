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

void roadmap_alternative_routes_init(void);
void roadmap_alternative_routes_routes_dialog (void);
void roadmap_alternative_routes_suggest_route_dialog (void);
BOOL roadmap_alternative_routes_suggest_routes(void);
void roadmap_alternative_routes_set_suggest_routes(BOOL suggest_at_start);

#endif /* ROADMAP_ALTERNATIVE_ROUTES_H_ */

