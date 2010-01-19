/* roadmap_native_keyboard.c - Android keyboard specific functionality
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
 *   Ehud Shabtai
 *
 *   This file is part of RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   See FreeMapJNI.h
 */

#include "roadmap_native_keyboard.h"
#include "ssd/ssd_widget.h"
#include "JNI/FreeMapJNI.h"
#include "roadmap_config.h"

static BOOL sgNtvKeyboardVisible = FALSE;
static RMNativeKBParams sgLastParams = {0};		// Last used params

extern RoadMapConfigDescriptor RoadMapConfigUseNativeKeyboard;
/***********************************************************
 *  Name        : roadmap_native_keyboard_enabled
 *  Purpose     : If the native keyboard enabled
 *	Params		: void
 *
 */
BOOL roadmap_native_keyboard_enabled()
{
	BOOL res = roadmap_config_match( &RoadMapConfigUseNativeKeyboard, "yes" );
	return res;
}

/***********************************************************
 *  Name        : roadmap_native_keyboard_show_search
 *  Purpose     : Shows the Android native keyboard with the search action button
 *	Params		: params - set of parameters for the keyboard representation
 *
 */
void roadmap_native_keyboard_show( RMNativeKBParams* params )
{
	android_kb_action_type type;

	if ( !roadmap_native_keyboard_enabled() )
		return;

	switch ( params->action_button )
	{
		case _native_kb_action_default:
		{
			type = _andr_ime_action_none;
			break;
		}
		case _native_kb_action_done:
		{
			type = _andr_ime_action_done;
		}	break;
		case _native_kb_action_search:
		{
			type = _andr_ime_action_search;
			break;
		}
		case _native_kb_action_next:
		{
			type = _andr_ime_action_next;
			break;
		}

		default:
		{
			type = _andr_ime_action_none;
			break;
		}
	} // switch

	FreeMapNativeManager_ShowSoftKeyboard( type, params->close_on_action );

	sgLastParams = *params;
	sgNtvKeyboardVisible = TRUE;
}

/***********************************************************
 *  Name        : roadmap_keyboard_hide
 *  Purpose     : Hide native keyboard
 *
 */
void roadmap_native_keyboard_hide()
{
	if ( !roadmap_native_keyboard_enabled() )
		return;

	FreeMapNativeManager_HideSoftKeyboard();

	sgNtvKeyboardVisible = FALSE;
}


/***********************************************************
 *  Name        : roadmap_keyboard_hide
 *  Purpose     : Returns true if the native keyboard is currently visible
 *
 */
BOOL roadmap_native_keyboard_visible( void )
{
	return sgNtvKeyboardVisible;
}

/***********************************************************
 *  Name        : roadmap_native_keyboard_get_params
 *  Purpose     : Returns params set ( bitwise copy )
 *
 */
void roadmap_native_keyboard_get_params( RMNativeKBParams* params_out )
{
	*params_out = sgLastParams;
}
