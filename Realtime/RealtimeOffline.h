/* RealtimeOffline.h - Save server data to file
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
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

#ifndef	__REALTIME_OFFLINE_H__
#define	__REALTIME_OFFLINE_H__

#include "roadmap.h"

void		Realtime_OfflineOpen (const char *path, const char *filename);
void		Realtime_OfflineClose (void);
void		Realtime_OfflineWrite (const char *packet);
void 		Realtime_OfflineWriteServerCookie (const char *cookie);

#endif	//	__REALTIME_OFFLINE_H__
