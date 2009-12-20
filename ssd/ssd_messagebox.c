/* ssd_messagebox.c - ssd messagebox.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   See roadmap_messagebox.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_bitmap.h"

#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_res.h"
#include "roadmap_messagebox.h"
#include "roadmap_screen.h"

#define DLG_MSG_BOX_NAME "message_box"
#define DLG_MSG_BOX_TITLE_FONT_SIZE_DFLT	20
#define DLG_MSG_BOX_TEXT_FONT_SIZE_DFLT	16
static RoadMapCallback MessageBoxCallback = NULL;
static messagebox_closed MessageBoxClosedCallback = NULL;
static SsdWidget s_gMsgBoxDlg;
static int g_seconds;

struct ssd_messagebox_data {
   const char *bitmaps;
};

void roadmap_messagebox_cb(const char *title, const char *message,
         messagebox_closed on_messagebox_closed)
{
	MessageBoxClosedCallback = on_messagebox_closed;
   roadmap_messagebox( title, message);
}

static void kill_messagebox_timer (void) {

	if (MessageBoxCallback) {
		roadmap_main_remove_periodic (MessageBoxCallback);
		MessageBoxCallback = NULL;
	}
}


static void restore_messagebox_defaults (void)
{
	SsdWidget widget_title, widget_text;

	widget_title = ssd_widget_get( s_gMsgBoxDlg, "title_text" );
	ssd_text_set_font_size( widget_title, DLG_MSG_BOX_TITLE_FONT_SIZE_DFLT );
    ssd_text_set_color( widget_title, NULL );

	widget_text = ssd_widget_get( s_gMsgBoxDlg, "text" );
	ssd_text_set_lines_space_padding( widget_text, 0 );
	ssd_text_set_font_size( widget_text, DLG_MSG_BOX_TEXT_FONT_SIZE_DFLT );
   ssd_text_set_color( widget_text, NULL );
}

static void update_button(void){
   char button_txt[20];
   SsdWidget button = ssd_widget_get(s_gMsgBoxDlg, "confirm");
   if (g_seconds != -1){
      if (!g_seconds){
         ssd_dialog_set_focus(button);
         sprintf(button_txt, "%s", roadmap_lang_get ("Ok"));
      }
      else
         sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Ok"), g_seconds);
   }else
      sprintf(button_txt, "%s", roadmap_lang_get ("Ok"));
   ssd_button_change_text(button,button_txt );
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
}

static void close_messagebox (void) {

   g_seconds--;
   if (g_seconds >= 0){
      update_button();
      return;
   }
   
   kill_messagebox_timer ();
	ssd_dialog_hide ( DLG_MSG_BOX_NAME, dec_ok );
	restore_messagebox_defaults();
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
}

static int button_callback ( SsdWidget widget, const char *new_value ) {

   kill_messagebox_timer ();
   ssd_dialog_hide ( DLG_MSG_BOX_NAME, dec_ok );
   restore_messagebox_defaults();
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
	if ( MessageBoxClosedCallback )
	{
	   messagebox_closed temp = MessageBoxClosedCallback;  
	   MessageBoxClosedCallback = NULL;
		temp( 0 );
	}
	
   return 0;
}

static SsdWidget create_messagebox ( const char* mb_name )
{

   SsdWidget dialog, text;
   char button_txt[20];
   dialog = ssd_dialog_new ( mb_name, "", NULL,
         SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK|SSD_HEADER_BLACK);

   ssd_widget_set_color (dialog, "#000000", "#ff0000000");

   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 10, SSD_END_ROW));

   text =  ssd_text_new ("text", "", DLG_MSG_BOX_TEXT_FONT_SIZE_DFLT, SSD_END_ROW|SSD_WIDGET_SPACE);
   ssd_widget_add (dialog,text);

   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer2", NULL, 0, 10, SSD_END_ROW));

   if (g_seconds != -1)
      sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Ok"), g_seconds);
   else
      sprintf(button_txt, "%s", roadmap_lang_get ("Ok"));
   
   ssd_widget_add (dialog,
      ssd_button_label ("confirm", button_txt,
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_DEFWIDGET|
                        SSD_WS_TABSTOP|SSD_END_ROW,
                        button_callback));

   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer3", NULL, 0, 10, SSD_END_ROW));

   s_gMsgBoxDlg = dialog;

   return dialog;
}

static void roadmap_messagebox_t (const char *title, const char *text)
{
   roadmap_messagebox_custom( title, text, DLG_MSG_BOX_TITLE_FONT_SIZE_DFLT, "#ffffff", DLG_MSG_BOX_TEXT_FONT_SIZE_DFLT, "#ffffff" );
}

void roadmap_messagebox (const char *title, const char *text)
{
   g_seconds = -1;
	roadmap_messagebox_custom( title, text, DLG_MSG_BOX_TITLE_FONT_SIZE_DFLT, "#ffffff", DLG_MSG_BOX_TEXT_FONT_SIZE_DFLT, "#ffffff" );
}


void roadmap_messagebox_custom( const char *title, const char *text,
				int title_font_size, char* title_color, int text_font_size, char* text_color )
{
   SsdWidget dialog = s_gMsgBoxDlg;
   SsdWidget widget_title, widget_text;
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);

   if ( !dialog )
   {
	   dialog = create_messagebox ( DLG_MSG_BOX_NAME );
	   if ( !dialog )
	   {
		   roadmap_log( ROADMAP_ERROR, "Error creating message box! " );
		   return;
	   }
   }
   else{
      update_button();
   }

   if (title[0] == 0){
      ssd_widget_hide(ssd_widget_get(dialog, "title_bar"));  
   }
   else{
      ssd_widget_show(ssd_widget_get(dialog, "title_bar"));
   }
   widget_title = ssd_widget_get( dialog, "title_text" );
   ssd_text_set_font_size( widget_title, title_font_size );
   if ( title_color != NULL )
   {
	   ssd_text_set_color( widget_title, title_color );
   }

   dialog->set_value( dialog, title );

   widget_text = ssd_widget_get( dialog, "text" );

   ssd_text_set_lines_space_padding( widget_text, 3 );

   ssd_text_set_font_size( widget_text, text_font_size );
   if ( text_color != NULL )
   {
	   ssd_text_set_color( widget_text, text_color );
   }

   ssd_text_set_text( widget_text, text );

   kill_messagebox_timer ();
   ssd_dialog_activate( DLG_MSG_BOX_NAME, NULL );
   ssd_dialog_draw ();
}


void roadmap_messagebox_timeout (const char *title, const char *text, int seconds) {

   g_seconds = seconds;
   roadmap_messagebox_t (title, text);
	MessageBoxCallback = close_messagebox;
	roadmap_main_set_periodic (1000, close_messagebox);
}



