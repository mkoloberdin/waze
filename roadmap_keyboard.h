
/* roadmap_keyboard.h
 *
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


#ifndef   __ROADMAP_KEYBOARD_H__
#define   __ROADMAP_KEYBOARD_H__
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#include "roadmap.h"
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard flags:
#define  KEYBOARD_VIRTUAL_KEY             (0x00000001)
#define  KEYBOARD_ASCII                   (0x00000002)
#define  KEYBOARD_REPLACEMENT_KEY         (0x00000004)
#define  KEYBOARD_LONG_PRESS	          (0x00000008)

// ASCII constant values
#define  ENTER_KEY                        (0x0D)
#define  TAB_KEY                          (0x09)
#define  ESCAPE_KEY                       (0x1B)
#define  SPACE_KEY                        (0x20)
#define  BACKSPACE                        (0X08)

#define  KEY_EQUALS_CHAR(_char_)          \
   ((KEYBOARD_ASCII&flags) &&  (_char_==(*utf8char)))

#define  KEY_EQUALS_VCHAR(_char_)         \
   ((KEYBOARD_VIRTUAL_KEY&flags) &&  (_char_==(*utf8char)))

#define  KEY_IS_ENTER                     KEY_EQUALS_CHAR(ENTER_KEY)
#define  KEY_IS_TAB                       KEY_EQUALS_CHAR(TAB_KEY)
#define  KEY_IS_ESCAPE                    KEY_EQUALS_CHAR(ESCAPE_KEY)
#define  KEY_IS_SPACE                     KEY_EQUALS_CHAR(SPACE_KEY)
#define  KEY_IS_BACKSPACE                 KEY_EQUALS_CHAR(BACKSPACE)

#define  VKEY_IS_BACK                     KEY_EQUALS_VCHAR(VK_Back)
#define  VKEY_IS_UP                       KEY_EQUALS_VCHAR(VK_Arrow_up)
#define  VKEY_IS_DOWN                     KEY_EQUALS_VCHAR(VK_Arrow_down)

#define  VKEY_IS_SOFTKEY_LEFT             KEY_EQUALS_VCHAR(VK_Softkey_left)
#define  VKEY_IS_SOFTKEY_RIGHT            KEY_EQUALS_VCHAR(VK_Softkey_right)
// Virtual keys:
typedef enum tagEVirtualKey
{
   VK_None = 0,      // Not a virtual key

   VK_Back,
   VK_Softkey_left,
   VK_Softkey_right,
   VK_Arrow_up,
   VK_Arrow_down,
   VK_Arrow_left,
   VK_Arrow_right,
   VK_Call_Start,
   VK_Camera,
}  EVirtualKey;

const char* roadmap_keyboard_virtual_key_name( EVirtualKey key);

//   Message loop should call this method:
BOOL   roadmap_keyboard_handler__key_pressed( const char* utf8char, uint32_t flags);
/////////////////////////////////////////////////////////////////////////////////////////////////

// Tests whether the keyboard should be locked due to driving and shows the message warning if requested
BOOL roadmap_keyboard_typing_locked( BOOL show_msg );
void roadmap_keyboard_set_typing_lock_enable( BOOL is_enabled );
BOOL roadmap_keyboard_get_typing_lock_enable( void );

/////////////////////////////////////////////////////////////////////////////////////////////////
// Callback type for 'key pressed':
typedef BOOL(*CB_OnKeyPressed)( const char* utf8char, uint32_t flags);

// Register to 'key pressed' events:
BOOL  roadmap_keyboard_register_to_event__key_pressed    ( CB_OnKeyPressed cbOnKeyPressed);
BOOL  roadmap_keyboard_unregister_from_event__key_pressed( CB_OnKeyPressed cbOnKeyPressed);
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __ROADMAP_KEYBOARD_H__
