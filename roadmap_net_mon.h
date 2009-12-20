/* roadmap_net_mon.h - Network activity monitor
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
 */

#ifndef INCLUDE__ROADMAP_NET_MON__H
#define INCLUDE__ROADMAP_NET_MON__H

#include "roadmap.h"

typedef enum tag_roadmap_net_mon_state{

   NET_MON_DISABLED,
   NET_MON_OFFLINE,
   NET_MON_START,
   NET_MON_IDLE,
   NET_MON_CONNECT,
   NET_MON_DATA,
   NET_MON_ERROR
	
} ROADMAP_NET_MON_STATE;

void roadmap_net_mon_start (void);
void roadmap_net_mon_connect (void);
void roadmap_net_mon_send (size_t size);
void roadmap_net_mon_recv (size_t size);
void roadmap_net_mon_disconnect (void);
void roadmap_net_mon_destroy (void);
void roadmap_net_mon_error (const char *text);
ROADMAP_NET_MON_STATE roadmap_net_mon_get_status (void);
size_t roadmap_net_mon_get_count (void);
void roadmap_net_mon_offline (void);

#endif // INCLUDE__ROADMAP_NET_MON__H

