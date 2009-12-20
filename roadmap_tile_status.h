/* roadmap_tile_status.h - Manage tile cache.
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik.
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

#ifndef _ROADMAP_TILE_STATUS__H
#define _ROADMAP_TILE_STATUS__H

#define	ROADMAP_TILE_STATUS_FLAG_ERROR		0x00000001
#define	ROADMAP_TILE_STATUS_FLAG_EXISTS		0x00000002
#define	ROADMAP_TILE_STATUS_FLAG_UPTODATE	0x00000004
#define	ROADMAP_TILE_STATUS_FLAG_ACTIVE		0x00000008
#define	ROADMAP_TILE_STATUS_FLAG_CHECKED		0x00000010
#define	ROADMAP_TILE_STATUS_FLAG_CALLBACK	0x00000020
#define	ROADMAP_TILE_STATUS_FLAG_QUEUED		0x00000040
#define	ROADMAP_TILE_STATUS_FLAG_UNFORCE		0x00000080
#define	ROADMAP_TILE_STATUS_FLAG_ROUTE		0x00000100

#define	ROADMAP_TILE_STATUS_MASK_PRIORITY			0x00FF0000
#define	ROADMAP_TILE_STATUS_PRIORITY_NONE			0x00000000
#define	ROADMAP_TILE_STATUS_PRIORITY_ON_SCREEN		0x00100000	// tile on screen

#define	ROADMAP_TILE_STATUS_PRIORITY_PREFETCH		0x00300000	// 10KM ahead on navigation route
#define	ROADMAP_TILE_STATUS_PRIORITY_NEXT_TURN		0x00400000	// contains name of next turn
#define	ROADMAP_TILE_STATUS_PRIORITY_NAV_RESOLVE	0x00500000	// needed to determine first/last navigation segment

#define	ROADMAP_TILE_STATUS_PRIORITY_NEIGHBOURS	0x00600000	// neighbouring to current GPS position
#define	ROADMAP_TILE_STATUS_PRIORITY_GPS				0x00700000	// tile of current GPS position

/* Callback types definitions */
#define	ROADMAP_TILE_STATUS_CALLBACK_RT_TRAFFIC			0x01000000	// Traffic info tiles

int *roadmap_tile_status_get (int index);

#endif // _ROADMAP_TILE_STATUS__H
