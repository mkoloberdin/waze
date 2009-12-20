
/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */

#ifndef  __ROADMAP_PHONE_KEYBOARD_H__
#define  __ROADMAP_PHONE_KEYBOARD_H__

#include "roadmap.h"

#define     TIME_BETWEEN_MULTIPLE_KEYBOARD_PRESSES      (1500)   /* ms */

void        roadmap_phone_keyboard_init();
void        roadmap_phone_keyboard_term();
const char* roadmap_phone_keyboard_get_multiple_key_value( 
                     void*       caller,
                     const char* key_pressed,
                     uint16_t    input_type,
                     BOOL*       value_was_replaced);

#endif   // __ROADMAP_PHONE_KEYBOARD_H__

