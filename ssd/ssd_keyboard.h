
/* ssd_keyboard.h
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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
 *
 */

#ifndef __SSD_KEYBOARD_H__
#define __SSD_KEYBOARD_H__

#include "ssd_keyboard_layout.h"

typedef enum tag_kb_layout_type
{
   qwerty_kb_layout,
   grid_kb_layout,
   widegrid_kb_layout

}  kb_layout_type;

typedef enum tag_kb_characters_set
{
   English_highcase,
   English_lowcase,
   Hebrew,
   Nunbers_and_symbols

}  kb_characters_set;

SsdWidget   ssd_create_keyboard( SsdWidget                  container,
                                 CB_OnKeyboardButtonPressed cbOnKey,
                                 CB_OnKeyboardCommand       cbOnSpecialButton,
                                 const char*                special_button_name,
                                 void*                      context);

SsdWidget   ssd_get_keyboard( SsdWidget container);

void        ssd_keyboard_set_grid_layout  ( SsdWidget kb);
void        ssd_keyboard_set_qwerty_layout( SsdWidget kb);
void        ssd_keyboard_set_charset      ( SsdWidget kb, kb_characters_set cset);
void        ssd_keyboard_reset_state      ( SsdWidget kb);
int 		ssd_keyboard_edit_box_top_offset( void );
#endif // __SSD_KEYBOARD_H__
