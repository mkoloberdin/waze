/* roadmap_res.h - Resources manager
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#ifndef _ROADMAP_RES__H_
#define _ROADMAP_RES__H_

#define RES_BITMAP 0
#define RES_SOUND  1
#define MAX_RESOURCES 2

/* Flags */
#define RES_SKIN      0x1
#define RES_NOCACHE   0x2
#define RES_NOCREATE  0x4
#define RES_LOCK      0x8

void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name);

void roadmap_res_shutdown (void);

#endif // _ROADMAP_RES__H_

