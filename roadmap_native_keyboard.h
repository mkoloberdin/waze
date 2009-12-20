/* roadmap_native_keyboard.h - the interface for the native keyboard functionality
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
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
 * DESCRIPTION:
 *
 *
 * NOTES: Currently implemented only for android so the API is not closed
 * TODO :: Define parameter set as structure
 */

#ifndef INCLUDE__ROADMAP_NATIVE_KEYBOARD__H
#define INCLUDE__ROADMAP_NATIVE_KEYBOARD__H

#include "roadmap.h"
typedef enum
{
	_native_kb_action_default = 0,
	_native_kb_action_done,
	_native_kb_action_search,
	_native_kb_action_next
} RMNativeKBAction;

typedef enum
{
	_native_kb_type_default = 0,
	_native_kb_type_numeric
} RMNativeKBType;

typedef struct
{
	RMNativeKBType kb_type;					// Type of the keyboard
	BOOL close_on_action;					// If to close the keyboard when the action button is pressed
	RMNativeKBAction action_button;			// Action button label
} RMNativeKBParams;


BOOL roadmap_native_keyboard_enabled( void );

BOOL roadmap_native_keyboard_visible( void );

void roadmap_native_keyboard_show( RMNativeKBParams* params );

void roadmap_native_keyboard_hide( void );

void roadmap_native_keyboard_get_params( RMNativeKBParams* params_out );

#endif // INCLUDE__ROADMAP_NATIVE_KEYBOARD__H

