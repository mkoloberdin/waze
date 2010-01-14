
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

#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_lang.h"
#include <stdlib.h>
#include <string.h>

#define CFG_KB_TYPING_LOCK_THR		10			/* Typing lock Threshold default value in miles/h */
#define KB_TYPING_LOCK_MSG_TIMEOUT	7			/* Timeout for the locked typing message in seconds */
#define KB_TYPING_LOCK_MSG_MAX_LEN	256			/* The maximum length of the lock typing message in characters */
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#define  RMKB_MAXIMUM_REGISTERED_CALLBACKS         (20)
static   CB_OnKeyPressed   gs_cbOnKeyPressed[RMKB_MAXIMUM_REGISTERED_CALLBACKS];
static   int               gs_cbOnKeyPressed_count = 0;
static   BOOL gs_TypingLockCheckEnabled = FALSE;

/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//   Message loop should call these methods:
BOOL roadmap_keyboard_handler__key_pressed( const char* utf8char, uint32_t flags)
{
   int i;

   for( i=0; i<gs_cbOnKeyPressed_count; i++)
      if( gs_cbOnKeyPressed[i]( utf8char, flags))
         return TRUE;

   return FALSE;
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_keyboard_register_to_event__general(
                     void*   Callbacks[],      //   Callbacks array
                     int*   pCallbacksCount,   //   Current size of array
                     void*   pfnNewCallback)   //   New callback
{
   if( !Callbacks || !pCallbacksCount || !pfnNewCallback)
      return FALSE;   //   Invalid parameters

   //   Verify we are not already registered:
   if( (*pCallbacksCount))
   {
      int   i;
      for( i=0; i<(*pCallbacksCount); i++)
         if( Callbacks[i] == pfnNewCallback)
            return FALSE;   //   Callback already included in our array
   }

   //   Add only if we did not reach maximum callbacks count:
   if( (*pCallbacksCount) < RMKB_MAXIMUM_REGISTERED_CALLBACKS)
   {
      Callbacks[(*pCallbacksCount)++] = pfnNewCallback;
      return TRUE;
   }

   //   Maximum count exeeded...
   return FALSE;
}

BOOL roadmap_keyboard_unregister_from_event__general(
                     void*   Callbacks[],      //   Callbacks array
                     int*   pCallbacksCount,   //   Current size of array
                     void*   pfnCallback)      //   Callback to remove
{
   BOOL bFound = FALSE;

   if( !Callbacks || !pCallbacksCount || !(*pCallbacksCount) || !pfnCallback)
      return FALSE;   //   Invalid parameters...

   if( Callbacks[(*pCallbacksCount)-1] == pfnCallback)
      bFound = TRUE;
   else
   {
      int   i;

      //   Remove event and shift all higher events one position back:
      for( i=0; i<((*pCallbacksCount)-1); i++)
      {
         if( bFound)
            Callbacks[i] = Callbacks[i+1];
         else
         {
            if( Callbacks[i] == pfnCallback)
            {
               Callbacks[i] = Callbacks[i+1];
               bFound = TRUE;
            }
         }
      }
   }

   if( bFound)
   {
      (*pCallbacksCount)--;
      Callbacks[(*pCallbacksCount)] = NULL;
   }

   return bFound;
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_keyboard_register_to_event__key_pressed( CB_OnKeyPressed cbOnKeyPressed)
{
   return roadmap_keyboard_register_to_event__general(
                  (void*)gs_cbOnKeyPressed, //   Callbacks array
                  &gs_cbOnKeyPressed_count, //   Current size of array
                  (void*)cbOnKeyPressed);   //   New callback
}

BOOL roadmap_keyboard_unregister_from_event__key_pressed( CB_OnKeyPressed cbOnKeyPressed)
{
   return roadmap_keyboard_unregister_from_event__general(
                  (void*)gs_cbOnKeyPressed, //   Callbacks array
                  &gs_cbOnKeyPressed_count, //   Current size of array
                  (void*)cbOnKeyPressed);   //   Callback to remove
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
const char* roadmap_keyboard_virtual_key_name( EVirtualKey vk)
{
   switch( vk)
   {
      case VK_Back:         return "Back";
      case VK_Softkey_left: return "Softkey-left";
      case VK_Softkey_right:return "Softkey-right";
      case VK_Arrow_up:     return "Arrow-up";
      case VK_Arrow_down:   return "Arrow-down";
      case VK_Arrow_left:   return "Arrow-left";
      case VK_Arrow_right:  return "Arrow-right";
      default:              return "<unknown>";
   }
}



//

static void on_disabling_driving_lock(int exit_code, void *data){
	if (exit_code==dec_no)
		roadmap_keyboard_set_typing_lock_enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/*************************************************************************************************
 * BOOL roadmap_keyboard_typing_locked( BOOL show_msg )
 * Checks whether the typing is locked due to speed threshold
 *
 */
BOOL roadmap_keyboard_typing_locked( BOOL show_msg )
{

	static BOOL sInitialized = FALSE;
	static RoadMapConfigDescriptor RoadMapConfigTypingLockThreshod =
	                        ROADMAP_CONFIG_ITEM( "Keyboard", "Typing lock threshold" );
	static int sTypingLockSpeedThr = CFG_KB_TYPING_LOCK_THR;

	RoadMapGpsPosition position;
	BOOL res = FALSE;
	char msg_text[KB_TYPING_LOCK_MSG_MAX_LEN];



	/* In israel - always permitted 	*/
	if ( !gs_TypingLockCheckEnabled /*&& ssd_widget_rtl( NULL )*/ )
	{
		return FALSE;
	}

	/* Load the configuration */
	if ( !sInitialized )
	{
		roadmap_config_declare( "preferences", &RoadMapConfigTypingLockThreshod, OBJ2STR( CFG_KB_TYPING_LOCK_THR ), NULL );
		sTypingLockSpeedThr = roadmap_config_get_integer( &RoadMapConfigTypingLockThreshod );
		sInitialized = TRUE;
	}

	/* Check the speed and show message if requested */
	roadmap_navigate_get_current( &position, NULL, NULL );
	res = ( position.speed > sTypingLockSpeedThr );
	if ( res && show_msg )
	{
		int cur_len = 0;
		snprintf( msg_text, KB_TYPING_LOCK_MSG_MAX_LEN, roadmap_lang_get( "Typing is disabled when driving." ) );

#ifdef TOUCH_SCREEN
		cur_len = strlen( roadmap_lang_get( "Typing is disabled when driving." ) );
		snprintf( &msg_text[cur_len], KB_TYPING_LOCK_MSG_MAX_LEN-cur_len, "\n%s", roadmap_lang_get( "You can minimize the 'Report' screen and type later." ) );
#endif

		//roadmap_messagebox_timeout( "", msg_text, KB_TYPING_LOCK_MSG_TIMEOUT );


		ssd_confirm_dialog_custom_timeout( "",
                        msg_text,
                        FALSE,
                        on_disabling_driving_lock,
                        NULL, roadmap_lang_get( "Understood!" ),
                        roadmap_lang_get( "I'm not the driver" ), KB_TYPING_LOCK_MSG_TIMEOUT );


	}
	return res;
}

void roadmap_keyboard_set_typing_lock_enable( BOOL is_enabled )
{
	gs_TypingLockCheckEnabled = is_enabled;
}

BOOL roadmap_keyboard_get_typing_lock_enable( void )
{
	return gs_TypingLockCheckEnabled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
