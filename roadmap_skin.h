/* roadmap_skin.h - manage skins.
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

#ifndef INCLUDE__ROADMAP_SKIN__H
#define INCLUDE__ROADMAP_SKIN__H

#include "roadmap.h"

void roadmap_skin_register (RoadMapCallback listener);
void roadmap_skin_set_subskin (const char *sub_skin);
void roadmap_skin_toggle (void);
int roadmap_skin_state(void);
int roadmap_skin_state_screen_touched(void);

void roadmap_skin_init(void);
void roadmap_skin_auto_night_mode(void);
void roadmap_skin_auto_night_mode_kill_timer(void);
BOOL roadmap_skin_auto_night_feature_enabled (void) ;
void roadmap_skin_set_scheme(const char *new_scheme);
const int roadmap_skin_get_scheme(void);
#endif // INCLUDE__ROADMAP_SKIN__H

