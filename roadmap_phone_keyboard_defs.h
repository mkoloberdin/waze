
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

#ifndef  __ROADMAP_PHONE_KEYBOARD_DEFS_H__
#define  __ROADMAP_PHONE_KEYBOARD_DEFS_H__

#include "roadmap.h"

// Multiple values for a single key:
typedef struct tag_multiple_key_info
{
   char**   values;  // Array of (utf8) values for a single phone key
   int      count;   // Array size 

}     multiple_key_info, *multiple_key_info_ptr;
void  multiple_key_info_init  (  multiple_key_info_ptr this);
void  multiple_key_info_free  (  multiple_key_info_ptr this);
int   multiple_key_info_get_next_valid_key
                              (  multiple_key_info_ptr this, 
                                 int                   start_from, 
                                 uint16_t              input_type,
                                 BOOL*                 single_option);

// Phone keys:
typedef enum tag_phone_keys
{
   phonekey_0,
   phonekey_1,
   phonekey_2,
   phonekey_3,
   phonekey_4,
   phonekey_5,
   phonekey_6,
   phonekey_7,
   phonekey_8,
   phonekey_9,
   phonekey_star,
   phonekey_ladder,
   
   phonekey__count,
   phonekey__invalid
   
}           phone_keys;
phone_keys  phone_keys_from_string  ( const char*  key);
const char* phone_keys_tag_name     ( phone_keys   key);

// Phone keyboard current state:
typedef struct tag_phone_keyboard_state
{
   phone_keys  key;
   int         seq;
   int         ms;

}     phone_keyboard_state, *phone_keyboard_state_ptr;
void  phone_keyboard_state_init( phone_keyboard_state_ptr this);

// Information on all keys and current state:
typedef struct tag_phone_keyboard
{
   multiple_key_info    keys[phonekey__count];
   phone_keyboard_state state;
   void*                caller;

}     phone_keyboard, *phone_keyboard_ptr;
void  phone_keyboard_init( phone_keyboard_ptr this);
void  phone_keyboard_free( phone_keyboard_ptr this);
void  phone_keyboard_load( phone_keyboard_ptr this);

#endif   // __ROADMAP_PHONE_KEYBOARD_DEFS_H__

