/* roadmap_address.h - manage the roadmap address dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef INCLUDE__ROADMAP_ADDRESS__H
#define INCLUDE__ROADMAP_ADDRESS__H

#include "roadmap.h"
#include "roadmap_plugin.h"
#include "address_search/address_search.h"

typedef void (*RoadMapAddressSearchCB) (const char *result, void *context);
typedef int  (*RoadMapAddressNav) (const RoadMapPosition *point,
                                   const PluginLine *line,
                                   int direction,
                                   address_info_ptr ai);

void roadmap_address_destination_by_city (void);
void roadmap_address_location_by_city (void);
void roadmap_address_search_dialog (const char *city,
                                    RoadMapAddressSearchCB callback,
                                    void *data);

void roadmap_address_register_nav (RoadMapAddressNav navigate);
void roadmap_poi_register_nav (RoadMapAddressNav navigate);

#ifdef SSD
void roadmap_address_history (void);
#endif

#endif // INCLUDE__ROADMAP_ADDRESS__H
