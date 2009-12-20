/* ssd_progress_msg_dialog.c - Progress message dialog source
 *
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
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
 *   See ssd_progress_msg_dialog.h
 */


#include "ssd_progress_msg_dialog.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_screen.h"
#include <stdlib.h>

//======== Local Types ========


//======== Defines ========
#define SSD_PROGRESS_MSG_DLG_NAME 		"SSD PROGRESS MESSAGE DIALOG"
#define SSD_PROGRESS_MSG_TEXT_FLD 		"Message Text"
#define SSD_PROGRESS_MSG_FONT_SIZE 		19

//======== Globals ========
static SsdWidget gProgressMsgDlg = NULL;

//======== Local interface ========
static SsdWidget ssd_progress_msg_dialog_new( void );





/***********************************************************
 *  Name        : ssd_progress_msg_dialog_show
 *
 *  Purpose     :  Shows the dialog with the given name. Creates the new one if not exists
 *
 *  Params		: [in] dlg_text - the text to be displayed in message
 *  			:
 *				:
 *  Returns 	: void
 */
void ssd_progress_msg_dialog_show( const char* dlg_text )
{

	if ( !gProgressMsgDlg  )
	{
       // Create the dialog. Return in case of failure
	   if ( !( gProgressMsgDlg = ssd_progress_msg_dialog_new() ) )
		   return;
	}

	ssd_dialog_activate( SSD_PROGRESS_MSG_DLG_NAME, NULL );

	ssd_dialog_set_value( SSD_PROGRESS_MSG_TEXT_FLD, dlg_text );

	ssd_dialog_draw();

}

/***********************************************************
 *  Name        : ssd_progress_msg_dialog_set_text
 *
 *  Purpose     :  change the text of the progress dialog
 *
 *  Params		: [in] dlg_text - the text to be displayed in message
 *  			:
 *  			:
 *				:
 *  Returns 	: void
 */
void ssd_progress_msg_dialog_set_text( const char* dlg_text )
{

   if ( !gProgressMsgDlg  )
   {
       // Create the dialog. Return in case of failure
      if ( !( gProgressMsgDlg = ssd_progress_msg_dialog_new() ) )
         return;
   }

   ssd_dialog_set_value( SSD_PROGRESS_MSG_TEXT_FLD, dlg_text );

   roadmap_screen_redraw();

}

static int on_button_hide (SsdWidget widget, const char *new_value){
   ssd_dialog_hide(SSD_PROGRESS_MSG_DLG_NAME, dec_close);
   return 1;
}

/***********************************************************
 *  Name        : ssd_progress_msg_dialog_new
 *
 *  Purpose     :  Creates the adjustable progress dialog
 *
 *  Params		: void
 *  			:
 *  			:
 *				:
 *  Returns 	: The created widget
 */

static SsdWidget ssd_progress_msg_dialog_new( void )
{
	SsdWidget dialog, group, text, spacer, button;

	int rtl_flag = 0x0;
	int         text_width;
	int         text_ascent;
	int         text_descent;
	int         text_height;
	
    dialog = ssd_dialog_new( SSD_PROGRESS_MSG_DLG_NAME, "", NULL, SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|
                                SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);
    if ( !dialog )
    {
        roadmap_log( ROADMAP_ERROR, "Error creating progress message dialog" );
        return NULL;
    }
    rtl_flag = ssd_widget_rtl( NULL );
    group = ssd_container_new( "Text Container", NULL,
                SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW|rtl_flag );
    ssd_widget_set_color ( group, NULL, NULL );


    roadmap_canvas_get_text_extents( "aAbB19Xx", SSD_PROGRESS_MSG_FONT_SIZE, &text_width, &text_ascent, &text_descent, NULL);
    text_height =  (text_ascent + text_descent);
    // Space right
    spacer = ssd_container_new ( "spacer_right", NULL, 10, 1, SSD_END_ROW);
    ssd_widget_set_color ( spacer, NULL, NULL);
    ssd_widget_add ( group, spacer );

    text = ssd_text_new( SSD_PROGRESS_MSG_TEXT_FLD, "", SSD_PROGRESS_MSG_FONT_SIZE, SSD_ALIGN_VCENTER|SSD_END_ROW );
    ssd_widget_set_color(text, "#ffffff","#ffffff");
    ssd_widget_add( group, text );

    ssd_widget_add ( dialog, group );

    // Space below
    spacer = ssd_container_new( "spacer", NULL, SSD_MAX_SIZE, 10,
          SSD_WIDGET_SPACE|SSD_END_ROW );
    ssd_widget_set_color(spacer, NULL, NULL);
    ssd_widget_add( dialog, spacer );

#ifdef TOUCH_SCREEN
    button = ssd_button_label("Hide Button", roadmap_lang_get("Hide"), SSD_ALIGN_CENTER, on_button_hide);
    ssd_widget_add(dialog, button);
#endif
    
    return dialog;
}


/***********************************************************
 *  Name        : ssd_progress_msg_dialog_hide
 *
 *  Purpose     : Hides the progress dialog
 *
 *  Params		:
 *  			:
 *				:
 *  Returns 	: void
 */
void ssd_progress_msg_dialog_hide( void )
{
	ssd_dialog_hide( SSD_PROGRESS_MSG_DLG_NAME, 0 );
}



static void hide_timer(void){
	ssd_progress_msg_dialog_hide();
	if (!roadmap_screen_refresh())
	   roadmap_screen_redraw();
	roadmap_main_remove_periodic (hide_timer);
}

/***********************************************************
 *  Name        : ssd_progress_msg_dialog_show_timed
 *
 *  Purpose     :  Shows the dialog with the given name, for a given amount of time
 *
 *  Params		: [in] dlg_text - the text to be displayed in message
 *  			: [in] seconds - the time to be displayed 
 *	Author		: Dan Friedman
 *  Returns 	: void 
 */
void ssd_progress_msg_dialog_show_timed( const char* dlg_text , int seconds)
{
	ssd_progress_msg_dialog_show(dlg_text);
	roadmap_main_set_periodic (seconds * 1000, hide_timer);
}
