/* roadmap_recommend.h - The interface for the recommend/rate module
 *
 * LICENSE:
 *
 *   Copyright 2010, Waze Ltd
 *                   Alex Agranovich   (AGA)
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

#ifndef __ROADMAP_RECOMMEND__H
#define __ROADMAP_RECOMMEND__H

#include "roadmap.h"

#define RATE_SCREEN_TYPE_FIRST_TIME    0
#define RATE_SCREEN_TYPE_NEW_VER       1
#define RATE_SCREEN_TYPE_FB_SHARE      2

#ifdef IPHONE
void roadmap_recommend(void);
void roadmap_recommend_appstore(void);
void roadmap_recommend_chomp(void);
void roadmap_recommend_email(void);
#else
typedef void (*RMRateLauncherCb)( void );
void roadmap_recommend_register_launcher( RMRateLauncherCb launcher_cb );
#endif //IPHONE

void roadmap_recommend_rate_us( RoadMapCallback on_close, int type );

#endif // __ROADMAP_RECOMMEND__H