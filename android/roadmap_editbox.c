/* roadmap_editbox.c - Android implementation of the editbox
 *
 * LICENSE:
 *   Copyright 2010, Waze Ltd
 *   Alex Agranovich
 *
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

#include <stdlib.h>
#include "roadmap_editbox.h"
#include "JNI/FreeMapJNI.h"
#include "JNI/WazeEditBox_JNI.h"
#include "roadmap_screen.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "roadmap_lang.h"

static int roadmap_editbox_action( int flag );
static void roadmap_editbox_dlg_show( const char* title );

#define SSD_EDITBOX_DIALOG_NAME   "EditBox Dialog"
#define SSD_EDITBOX_DIALOG_CNT_NAME   "EditBox Dialog.Container"
/***********************************************************
 *  Name       : ShowEditbox
 *  Purpose    : Native edit box customization and showing
 * Params      : aTitleUtf8 - title at the top of edit box
 *             : aTextUtf8 - initial string at the editbox
 *             : callback - Callback function to be called upon editbox enter
 *             : context - context to be passed to the callback
 *             : aBoxType - edit box customization flags (see roadmap_editbox.h)
 */
void ShowEditbox(const char* aTitleUtf8, const char* aTextUtf8, SsdKeyboardCallback callback, void *context, TEditBoxType aBoxType )
{
   int action = roadmap_editbox_action( aBoxType & EDITBOX_ACTION_MASK );
   int isEmbedded = aBoxType & EEditBoxEmbedded;
   int stayOnAction = ( ( aBoxType & EEditBoxStayOnAction ) > 0 );
   int margin = EDITBOX_TOP_MARGIN;
   int flags = 0;

   margin = roadmap_screen_adjust_height( margin );

   // Flags
   if ( aBoxType & EEditBoxPassword )
   {
      flags |= com_waze_WazeEditBox_WAZE_EDITBOX_FLAG_PASSWORD;
   }

   EditBoxContextType *pCtx = malloc( sizeof( EditBoxContextType ) );
   pCtx->callback = callback;
   pCtx->cb_context = context;


   if ( !isEmbedded )
   {
      roadmap_editbox_dlg_show( aTitleUtf8 );
   }

   FreeMapNativeManager_ShowEditBox( action, stayOnAction, aTextUtf8, (void*) pCtx, margin, flags );
}

/***********************************************************
 *  Name       : roadmap_editbox_dlg_show
 *  Purpose    : Ssd dialog to be shown at the back of the edit box
 * Params      : title - dialog title
 *
 */

static void roadmap_editbox_dlg_show( const char* title )
{
   static SsdWidget dialog = NULL;
   static SsdWidget container = NULL;
   static SsdWidget title_text = NULL;

   if ( !( dialog = ssd_dialog_activate( SSD_EDITBOX_DIALOG_NAME, NULL ) ) )
   {
      dialog = ssd_dialog_new( SSD_EDITBOX_DIALOG_NAME, roadmap_lang_get( title ), NULL, SSD_CONTAINER_TITLE );
//      container = ssd_container_new( SSD_EDITBOX_DIALOG_CNT_NAME, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE, 0 );
//      ssd_widget_add( dialog, container );
      // AGA NOTE: In case of ssd_dialog_new dialog points to the scroll_container.
      // In case of activate dialog points to the container
      dialog = ssd_dialog_activate( SSD_EDITBOX_DIALOG_NAME, NULL );
   }
   title_text = ssd_widget_get( dialog, "title_text" );
   ssd_text_set_text( title_text, roadmap_lang_get( title ) );
   ssd_dialog_draw();
}

/***********************************************************
 *  Name       : roadmap_editbox_action
 *  Purpose    : decodes the flag to the actual action value
 * Params      : flag - TEditBoxType flag
 *
 */
static int roadmap_editbox_action( int flag )
{
   int action = _andr_ime_action_done;
   switch( flag )
   {
      case EEditBoxActionDefault:
         action = _andr_ime_action_none;
         break;
      case EEditBoxActionDone:
         action = _andr_ime_action_done;
         break;
      case EEditBoxActionNext:
         action = _andr_ime_action_next;
         break;
      case EEditBoxActionSearch:
         action = _andr_ime_action_search;
         break;
   }

   return action;
}


void roadmap_editbox_dlg_hide( void )
{
   ssd_dialog_hide( SSD_EDITBOX_DIALOG_NAME, dec_ok );
}
