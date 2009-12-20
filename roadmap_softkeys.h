/* roadmap_softkeys.h - manage the roadmap address dialogs.
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

#ifndef INCLUDE__ROADMAP_SOFTKEYS__H
#define INCLUDE__ROADMAP_SOFTKEYS__H

#include "roadmap.h"

typedef struct{
	char 			text[50];
	RoadMapCallback callback; 
}Softkey;

#define SOFT_KEYS_ON_BOTTOM 0
#define SOFT_KEYS_ON_RIGHT	1
#define SOFT_KEYS_ON_LEFT   2
#define SOFT_KEYS_ON_TOP    3

int roadmap_softkeys_on_bottom_state(void);
int roadmap_softkeys_orientation(void);

void roadmap_softkeys_set_right_soft_key(const char *name, Softkey *s );
void roadmap_softkeys_set_left_soft_key(const char *name, Softkey *s );

void roadmap_softkeys_remove_left_soft_key(const char *name );
void roadmap_softkeys_remove_right_soft_key(const char *name );

const char *roadmap_softkeys_get_right_soft_key_text(void);
const char *roadmap_softkeys_get_left_soft_key_text(void);

void roadmap_softkeys_left_softkey_callback(void);
void roadmap_softkeys_right_softkey_callback(void);

void roamdap_softkeys_chnage_left_text(const char *name, const char *new_text);

#endif // INCLUDE__ROADMAP_SOFTKEYS__H
