/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   roadmap_main.h
 */

#include <coemain.h>
#include <eikenv.h>
#include <eikbtgpc.h>
#include <gulutil.h>
#include <stdlib.h>
#include <aknappui.h>
#include <aknutils.h>
#include <apgcli.h>

#include "FreeMapAppView.h"
#include "FreeMapAppUi.h"
#include "GSConvert.h"

extern "C" {
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_canvas.h"
#include "roadmap_io.h"
#include "roadmap_main.h"
#include "roadmap_res.h"
#include "roadmap_serial.h"
#include "roadmap_messagebox.h"
#include "roadmap_screen.h"
#include "roadmap_download.h"
#include "roadmap_lang.h"
#include "roadmap_dialog.h"
#include "roadmap_gps.h"
#include "roadmap_keyboard.h"
#include "Realtime/RealtimeAlerts.h"
#include "../editor/editor_main.h"
}

int USING_PHONE_KEYPAD = 0; 

//#include "symbian_input_mon.h"
#include "roadmap_canvas_agg.h"
//extern "C" void roadmap_canvas_configure(CBitmapContext *, CFreeMapAppView *);

//  For I/O callbacks
#include "RoadMap_NativeSocket.h"

// timer stuff
#define ROADMAP_MAX_TIMER 20
#define ROADMAP_MAX_IO 16
struct roadmap_main_timer {
	CPeriodic *periodic;
	RoadMapCallback callback;
};
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

static roadmap_main_io *RoadMapMainIo[ROADMAP_MAX_IO];

// varibles used across this module
static RoadMapKeyInput	RoadMapMainInput = NULL;
static CFreeMapAppView *View;
static CBitmapContext *Gc;

extern "C" void roadmap_start_quick_menu (void);
extern "C" void roadmap_confirmed_exit(void);

void roadmap_canvas_new (RWindow& aWindow, int initWidth, int initHeight);
BOOL roadmap_canvas_agg_is_landscape();

// for ssd login details dialog
extern "C" {
/*static*/ RoadMapConfigDescriptor RoadMapConfigUser =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "User Name");

/*static*/ RoadMapConfigDescriptor RoadMapConfigPassword =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Password");
}

extern "C" {
	
	void roadmap_main_configure (CBitmapContext *gc, CFreeMapAppView *view) {
		Gc = gc;
		View = view;
	}
	
	void roadmap_main_toggle_full_screen (void)
	{
	}

	void roadmap_main_new (const char */*title*/, int /*width*/, int /*height*/)
	{
	  roadmap_canvas_new(View->GetWindow(), 0, 0);
	  
	  editor_main_set (1);
  }
	
	void roadmap_main_set_keyboard
      (struct RoadMapFactoryKeyMap */*bindings*/, RoadMapKeyInput callback)
	{
		RoadMapMainInput = callback;
	}
	
	void roadmap_main_minimize( void ){
	   roadmap_confirmed_exit();
	}


TKeyResponse roadmap_main_process_key( const TKeyEvent& aKeyEvent, TEventCode aType ) 
{
	char regular_key[2];
	EVirtualKey vk      = VK_None;
	const char* pCode;
	TUint code = aKeyEvent.iScanCode;
	
	if ( aType != EEventKey ) { return EKeyWasNotConsumed;  }
  
	if ( code == EStdKeyDevice3 )
	{//handle center key here because it's not virtual
		regular_key[0] = ENTER_KEY;
		regular_key[1] = '\0';
		roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_ASCII );
		return EKeyWasConsumed;
	}
	if ( code == EStdKeyBackspace)
	{
  		regular_key[0] = BACKSPACE;
  		regular_key[1] = '\0';
  		roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_ASCII );
  		return EKeyWasConsumed;
	}
  
	// Virtual keys first
	switch(code)
	{
		case EStdKeyDevice0        : vk = VK_Softkey_left;   break;
		case EStdKeyDevice1        : vk = VK_Softkey_right;  break;
		case EStdKeyUpArrow        : vk = VK_Arrow_up;       break;
		case EStdKeyDownArrow      : vk = VK_Arrow_down;     break;
		case EStdKeyLeftArrow      : vk = VK_Arrow_left;     break;
		case EStdKeyRightArrow     : vk = VK_Arrow_right;    break;
		case EStdKeyNo             : roadmap_main_exit();     return EKeyWasConsumed;
		default:
			break;
	}
	// Handle virtual key if necessary
	if( vk != VK_None ) 
	{
		regular_key[0] = ( char ) ( vk & 0xFF );
		regular_key[1] = '\0';
		if ( roadmap_keyboard_handler__key_pressed( regular_key, KEYBOARD_VIRTUAL_KEY ) )
		{
			return EKeyWasConsumed;
		}
	}
	// Conversions for the not standard codes
	switch( code )
	{
		case 133		 	: /* This is star in Samsung's Symbian... */
		   code = '*';        break;
		case EStdKeyHash  	: code = '#';        break;
		default				: break;
	}
	// Regular keys - phone or qwerty 
	if ( USING_PHONE_KEYPAD || ( aKeyEvent.iModifiers & EModifierFunc ) )
	{
		regular_key[0] = ( char ) ( aKeyEvent.iCode & 0xFF );	/* In case of keypad iCode and iScanCode are the same */
		regular_key[1] = '\0';
		pCode =  regular_key;
	}
	else
	{	// Qwerty keyboard
		TUint16 qwertyCode = code;
		TUint32 qwertyCodeNullEnd = 0;
		CFreeMapAppUi* pAppUi = static_cast<CFreeMapAppUi*>( CEikonEnv::Static()->EikAppUi() );

		pAppUi->GetUnicodeForScanCodeL( aKeyEvent, qwertyCode );
		// Little endian: first byte = first char, null terminated
		qwertyCodeNullEnd = qwertyCode & 0xFFFF; 
		pCode =  reinterpret_cast<const char*>( &qwertyCodeNullEnd );
	}
	// Handle regular keys. pCode - pointer to the null terminated bytes
	// of the utf character
	if( roadmap_keyboard_handler__key_pressed( pCode, KEYBOARD_ASCII)) 
	{
		return EKeyWasConsumed;
	}
	else 
	{
		return EKeyWasNotConsumed;
	}
}

   RoadMapMenu roadmap_main_new_menu (void)
   {
      //return CreatePopupMenu();
   return NULL;
   }

   
	void roadmap_main_add_menu (RoadMapMenu menu, const char *label)
	{
	}
	
	
	void roadmap_main_add_menu_item (RoadMapMenu menu, const char *label,
      const char *tip, RoadMapCallback callback)
	{
	}
	
   void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {

   }

	void roadmap_main_add_separator (RoadMapMenu menu)
	{
		roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
	}
	
	void roadmap_main_add_tool (const char *label,
		const char *icon,
		const char *tip,
		RoadMapCallback callback)
	{
	}
	
	
	void roadmap_main_add_tool_space (void)
	{
	}
	
	
	void roadmap_main_add_canvas (void)
	{

	}
	
	
	void roadmap_main_add_status (void)
	{
		//TODO: do we need this?
	}
	
	
	void roadmap_main_show (void)
	{
	}
	
	
	void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback)
	{
      int i;
      
      for (i = 0; i < ROADMAP_MAX_IO; ++i) {
         if (RoadMapMainIo[i] == NULL) {
            RoadMapMainIo[i] = (roadmap_main_io *) malloc (sizeof(roadmap_main_io));
            RoadMapMainIo[i]->io = io;
            RoadMapMainIo[i]->callback = callback;
            RoadMapMainIo[i]->is_valid = 1;
            break;
         }
      }
      
      if (i == ROADMAP_MAX_IO) {
         roadmap_log (ROADMAP_FATAL, "Too many set input calls");
         return;
      }
      
     if ( io == NULL || callback == NULL )  return;
     if ( io->subsystem != ROADMAP_IO_NET ) return; 
     
     CRoadMapNativeNet* net = (CRoadMapNativeNet*)(io->os.socket);
     if ( net == NULL ) return;
     
     net->StartPolling((void*)callback, (void*)RoadMapMainIo[i]);
   }
   
   
   void roadmap_main_remove_input (RoadMapIO *io)
   {
    if ( io == NULL )  return;
    if ( io->subsystem != ROADMAP_IO_NET ) return; 

    int i;
    
    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
       if (RoadMapMainIo[i] && RoadMapMainIo[i]->io == io) {

          if (RoadMapMainIo[i]->is_valid) {
             RoadMapMainIo[i]->is_valid = 0;
          } else {
             free (RoadMapMainIo[i]);
          }

          RoadMapMainIo[i] = NULL;
          break;
       }
    }

    CRoadMapNativeNet* net = (CRoadMapNativeNet*)(io->os.socket);
    if ( net == NULL ) return;

    net->StopPolling();
   }
   
   static int roadmap_main_timeout (TAny *ptr)
   {     
      RoadMapCallback callback = (RoadMapCallback) ptr;
         
      if (callback != NULL) {
        if (global_FreeMapLock() != 0) return 0;
         (*callback) ();
         global_FreeMapUnlock();
      }
      
      return 0;
   }
   
   void roadmap_main_set_periodic (int interval, RoadMapCallback callback)
   {
      int index;
      struct roadmap_main_timer *timer = NULL;
      
      for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {
         
         if (RoadMapMainPeriodicTimer[index].callback == callback) {
            return;
         }
         if (timer == NULL) {
            if (RoadMapMainPeriodicTimer[index].callback == NULL) {
               timer = RoadMapMainPeriodicTimer + index;
            }
         }
      }
      
      if (timer == NULL) {
         roadmap_log (ROADMAP_FATAL, "Timer table saturated");
      }
      
      timer->callback = callback;
      
      TRAPD(err, timer->periodic = CPeriodic::NewL(CActive::EPriorityStandard));
      if ( err != KErrNone )
      {
        roadmap_log(ROADMAP_FATAL, "Could not instantiate timer!");
        return;
      }
      
      interval *= 1000;
      if (interval > 0 )
         timer->periodic->Start(interval, interval, TCallBack(roadmap_main_timeout, (TAny *)timer->callback));
      
   }
   
   
   void roadmap_main_remove_periodic (RoadMapCallback callback)
   {
      int index;
      
      for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {
         
         if (RoadMapMainPeriodicTimer[index].callback == callback) {
            
            RoadMapMainPeriodicTimer[index].callback = NULL;
            RoadMapMainPeriodicTimer[index].periodic->Cancel();
            delete (RoadMapMainPeriodicTimer[index].periodic);
            
            return;
         }
      }
      
      roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);
   }
   
   
   void roadmap_main_set_status (const char *text) {}
   
   void roadmap_main_flush (void) 
    {
       CEikonEnv::Static()->Flush();
    }
	
	
	void roadmap_main_exit (void)
	{
		roadmap_start_exit ();
		((CAknAppUi*)CEikonEnv::Static()->EikAppUi())->Exit();
	}

   void roadmap_main_set_cursor (int cursor)
   {
      switch (cursor) {

      case ROADMAP_CURSOR_NORMAL:
         //SetCursor(NULL);
         break;

      case ROADMAP_CURSOR_WAIT:
         //SetCursor(LoadCursor(NULL, IDC_WAIT));
         break;
      }
   }
   
   int roadmap_softkeys_orientation(){
   	return AknLayoutUtils::CbaLocation();
   }

   int roadmap_horizontal_screen_orientation(){
      return roadmap_canvas_agg_is_landscape();
   }
   
   void roadmap_internet_open_browser (char *url) {
   	  RApaLsSession apaLsSession;
   	  const TUid KOSSBrowserUidValue = {0x10008D39}; // 0x1020724D for S60 3rd Ed
   	  TUid id(KOSSBrowserUidValue);
   	  TApaTaskList taskList(CEikonEnv::Static()->WsSession());
   	  TApaTask task = taskList.FindApp(id);
   	  if(task.Exists())
   		  {
   		  task.BringToForeground();
   		  task.SendMessage(TUid::Uid(0), TPtrC8((TUint8 *)url,strlen(url))); // UID not used
   		  }
   	  else
   		  {
   		  if(!apaLsSession.Handle())
   			  {
   			  User::LeaveIfError(apaLsSession.Connect());
   			  }
   		  TThreadId thread;
   		  
   		  TBuf16<128> buf;
   		  buf.Copy(TPtrC8((TUint8 *)url,strlen(url)));
   		   		  
   		  User::LeaveIfError(apaLsSession.StartDocument(buf, KOSSBrowserUidValue, thread));
   		  apaLsSession.Close();   
   		  }
   }

} // extern "C"

