/* ssd_confirm_dialog.c - ssd confirmation dialog (yes/no).
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 *   See ssd_confirm_dialog.h.h
 */
#include <string.h>
#include <stdlib.h>
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"

#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_softkeys.h"
#include "roadmap_screen.h"
#include "ssd_confirm_dialog.h"

static int g_seconds;
static SsdWidget s_gMsgBoxDlg;

#ifndef IPHONE
 #define TEXT_SIZE 16
 #define TITLE_TEXT_SIZE 20
#else
 #define TEXT_SIZE 14
 #define TITLE_TEXT_SIZE 18
#endif //IPHONE


typedef struct {
	void *context;
	ConfirmDialogCallback callback;
	char TxtYes[SSD_TEXT_MAXIMUM_TEXT_LENGTH];
	char TxtNo[SSD_TEXT_MAXIMUM_TEXT_LENGTH];
	BOOL default_yes;
} confirm_dialog_context;

static RoadMapCallback MessageBoxCallback = NULL;

static void kill_messagebox_timer (void) {

   if (MessageBoxCallback) {
      roadmap_main_remove_periodic (MessageBoxCallback);
      MessageBoxCallback = NULL;
   }
}

static void ssd_confirm_dialog_update (void)
{
   char button_txt[SSD_TEXT_MAXIMUM_TEXT_LENGTH];
   confirm_dialog_context *data;
   SsdWidget buttonYes, buttonNo;
   char *dlg_name;

   g_seconds--;
   if (g_seconds < 0){
      ssd_confirm_dialog_close();
      return;
   }
   dlg_name = ssd_dialog_currently_active_name();
   if (!dlg_name || strcmp(dlg_name, "confirm_dialog"))
   {
      kill_messagebox_timer();
      return;
   }

   data = (confirm_dialog_context *) ssd_dialog_context();
   if (!data){
      kill_messagebox_timer();
      return;
   }

   if (data->default_yes){
      buttonYes = ssd_widget_get(s_gMsgBoxDlg, roadmap_lang_get ("Yes")); // change the buttons to custom text
      if (!g_seconds){
         ssd_dialog_set_focus(buttonYes);
         snprintf( button_txt, SSD_TEXT_MAXIMUM_TEXT_LENGTH, "%s", roadmap_lang_get (data->TxtYes));
      }
      else{
         snprintf( button_txt, SSD_TEXT_MAXIMUM_TEXT_LENGTH, "%s (%d)", roadmap_lang_get (data->TxtYes), g_seconds);
      }
#ifdef TOUCH_SCREEN
      ssd_button_change_text(buttonYes, button_txt);
#else
      ssd_widget_set_left_softkey_text(s_gMsgBoxDlg,button_txt);
      ssd_dialog_refresh_current_softkeys();
#endif
   }
   else{
      buttonNo = ssd_widget_get(s_gMsgBoxDlg, roadmap_lang_get ("No")); // change the buttons to custom text
      if (!g_seconds){
         ssd_dialog_set_focus(buttonNo);
         snprintf( button_txt, SSD_TEXT_MAXIMUM_TEXT_LENGTH, "%s", roadmap_lang_get (data->TxtNo));
      }
      else{
         snprintf( button_txt, SSD_TEXT_MAXIMUM_TEXT_LENGTH, "%s (%d)", roadmap_lang_get (data->TxtNo), g_seconds);
      }

#ifdef TOUCH_SCREEN
      ssd_button_change_text(buttonNo, button_txt);
#else
      ssd_widget_set_right_softkey_text(s_gMsgBoxDlg,button_txt);
      ssd_dialog_refresh_current_softkeys();
#endif
   }


   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

void ssd_confirm_dialog_close (void)
{
   char *dlg_name;
   confirm_dialog_context *data;
   ConfirmDialogCallback callback = NULL;
   SsdWidget dialog;
   int exit_code = dec_ok;
   dlg_name = ssd_dialog_currently_active_name();
   if (!dlg_name || strcmp(dlg_name, "confirm_dialog"))
   {
      kill_messagebox_timer();
      return;
   }

   dlg_name = ssd_dialog_currently_active_name();
   if (!dlg_name || strcmp(dlg_name, "confirm_dialog"))
   {
      kill_messagebox_timer();
      return;
   }

   dialog = ssd_dialog_context();
   data = (confirm_dialog_context *) ssd_dialog_context();
   if (data){
      callback = (ConfirmDialogCallback)data->callback;
      if (data->default_yes)
         exit_code = dec_yes;
      else
         exit_code = dec_no;
   }

   kill_messagebox_timer ();
   /* Set the NULL to the current before hide
    * NUll context is critical as an indicator for the delayed callback in ssd_button
    * that the context is already not accessible
    */
   ssd_dialog_set_context( NULL );
   ssd_dialog_hide ("confirm_dialog", dec_ok);

   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();

   if (callback && data)
      (*callback)(exit_code, data->context);

   if (data)
   {
      free(data);
   }
}

static int yes_button_callback (SsdWidget widget, const char *new_value) {

	SsdWidget dialog;
	ConfirmDialogCallback callback;
	confirm_dialog_context *data;

	dialog = widget->parent;
	data = (confirm_dialog_context *)dialog->context;

	ssd_dialog_hide ("confirm_dialog", dec_yes);

   if (data)
	{
      callback = (ConfirmDialogCallback)data->callback;

      (*callback)(dec_yes, data->context);

      kill_messagebox_timer ();
      dialog->context = NULL;
      free(data);
	}

	return 0;
}

static int no_button_callback (SsdWidget widget, const char *new_value) {


    SsdWidget dialog;
    ConfirmDialogCallback callback;
	 confirm_dialog_context *data;

    dialog = widget->parent;
    data = (confirm_dialog_context  *)dialog->context;

    ssd_dialog_hide ("confirm_dialog", dec_no);

    if (data)
    {
       callback = (ConfirmDialogCallback)data->callback;

       (*callback)(dec_no, data->context);

       kill_messagebox_timer();
       dialog->context = NULL;

       free(data);
    }

    return 0;
}

#ifndef TOUCH_SCREEN
static int no_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return no_button_callback(widget->children, new_value);
}

static int yes_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return yes_button_callback(widget->children, new_value);
}

static void set_soft_keys(SsdWidget dialog, const char * textYes, const char *textNo){

	ssd_widget_set_right_softkey_text(dialog,textNo);
	ssd_widget_set_right_softkey_callback(dialog,no_softkey_callback);
	ssd_widget_set_left_softkey_text(dialog, textYes);
	ssd_widget_set_left_softkey_callback(dialog,yes_softkey_callback);
}
#endif


static void create_confirm_dialog (BOOL default_yes, const char * textYes, const char *textNo) {
   SsdWidget text;
   SsdWidget dialog,widget_title;
   SsdWidget text_con;
   SsdSize dlg_size;
   SsdWidget buttonYes;
   SsdWidget buttonNo;

   // const char *question_icon[] = {"question"};

#ifndef TOUCH_SCREEN
   int yes_flags = 0;
   int no_flags = 0;

   if(default_yes)
      yes_flags|= SSD_WS_DEFWIDGET;
   else
      no_flags |= SSD_WS_DEFWIDGET;
#endif

   dialog = ssd_dialog_new ("confirm_dialog", "", NULL,
         SSD_PERSISTENT|SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_CONTAINER_TITLE|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK|SSD_POINTER_NONE);

   ssd_widget_set_color (dialog, "#000000", "#ff0000000");
   s_gMsgBoxDlg = dialog;

   ssd_widget_get_size( dialog, &dlg_size, NULL );
   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 12, SSD_END_ROW));

    text_con = ssd_container_new ("text_container", NULL,
                                  -1,	/* 90% of dialog width */
                                  SSD_MIN_SIZE,
                                  SSD_END_ROW|SSD_ALIGN_CENTER );
   ssd_widget_set_color (text_con, "#000000", "#ff0000000");


   // Text box
   text =  ssd_text_new ("text", "", 16, SSD_ALIGN_CENTER|SSD_END_ROW|SSD_WIDGET_SPACE|SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text,"#ffffff");

   ssd_widget_add (text_con,text);

  ssd_widget_add(dialog, text_con);

  widget_title = ssd_widget_get( dialog, "title_text" );
   ssd_text_set_color(widget_title,"#ffffff");
  ssd_text_set_font_size( widget_title, TITLE_TEXT_SIZE );

#ifdef TOUCH_SCREEN

   ssd_dialog_add_vspace( dialog, 14, SSD_START_NEW_ROW );

   buttonYes = ssd_button_label (roadmap_lang_get ("Yes"), textYes,
                                 SSD_ALIGN_CENTER|SSD_WS_TABSTOP,
                                 yes_button_callback);
    ssd_widget_add (dialog,buttonYes);

    buttonNo =   ssd_button_label (roadmap_lang_get ("No"), textNo,
                        SSD_ALIGN_CENTER| SSD_WS_TABSTOP,
                        no_button_callback);
   ssd_widget_add (dialog,buttonNo);

   if (default_yes){
      ssd_widget_set_color(ssd_widget_get(buttonYes,"label"),"#f6a203", "#ffffff" );
      ssd_widget_set_color(ssd_widget_get(buttonNo,"label"),"#ffffff", "#ffffff" );
   }else{
      ssd_widget_set_color(ssd_widget_get(buttonYes,"label"),"#ffffff", "#ffffff" );
      ssd_widget_set_color(ssd_widget_get(buttonNo,"label"),"#f6a203", "#ffffff" );
   }
#else
	set_soft_keys(dialog, textYes, textNo);

#endif
	// AGA TODO: Scaling factor
	ssd_dialog_add_vspace( dialog, 3, SSD_START_NEW_ROW|SSD_END_ROW );
}

void ssd_confirm_dialog_custom (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo) {

  SsdWidget dialog;
  confirm_dialog_context *data =
    (confirm_dialog_context  *)calloc (1, sizeof(*data));

  data->default_yes = default_yes;
  dialog = ssd_dialog_activate ("confirm_dialog", NULL);
  title = roadmap_lang_get (title);
  text  = roadmap_lang_get (text);

  strncpy(data->TxtYes, textYes, SSD_TEXT_MAXIMUM_TEXT_LENGTH );
  strncpy(data->TxtNo, textNo, SSD_TEXT_MAXIMUM_TEXT_LENGTH );

  if (!dialog) {
      create_confirm_dialog (default_yes,textYes,textNo);
      dialog = ssd_dialog_activate ("confirm_dialog", NULL);
  }
#ifdef TOUCH_SCREEN
   //set button text & softkeys
   SsdWidget buttonYes;
   SsdWidget buttonNo;
   buttonYes = ssd_widget_get(dialog, roadmap_lang_get ("Yes")); // change the buttons to custom text
   ssd_button_change_text(buttonYes, textYes);
   ssd_widget_loose_focus(buttonYes);
   buttonNo = ssd_widget_get(dialog, roadmap_lang_get ("No"));
   ssd_button_change_text(buttonNo, textNo);
   ssd_widget_loose_focus(buttonNo);
   if (default_yes){
      ssd_widget_set_color(ssd_widget_get(buttonYes,"label"),"#f6a203", "#ffffff" );
      ssd_widget_set_color(ssd_widget_get(buttonNo,"label"),"#ffffff", "#ffffff" );
   }else{
      ssd_widget_set_color(ssd_widget_get(buttonYes,"label"),"#ffffff", "#ffffff" );
      ssd_widget_set_color(ssd_widget_get(buttonNo,"label"),"#f6a203", "#ffffff" );
   }
#else //Non touch
   set_soft_keys(dialog, textYes, textNo); // change softkeys text to custom text
   ssd_dialog_refresh_current_softkeys();
#endif

  data->callback = callback;
  data->context = context;

  dialog->set_value (dialog, title);
  ssd_widget_set_value (dialog, "text", text);
  dialog->context = data;
  ssd_dialog_draw ();
}


void ssd_confirm_dialog (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context) {
   g_seconds = -1;
	ssd_confirm_dialog_custom(title, text, default_yes, callback, context, roadmap_lang_get("Yes"),roadmap_lang_get("No"));

}

void ssd_confirm_dialog_custom_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo, int seconds) {
   g_seconds = seconds;
   if (MessageBoxCallback){
      ssd_confirm_dialog_close();
   }
   else{
      MessageBoxCallback = ssd_confirm_dialog_update;
   }
   ssd_confirm_dialog_custom (title, text,default_yes,callback,context,textYes,textNo);
   roadmap_main_set_periodic (1000, ssd_confirm_dialog_update );
}


void ssd_confirm_dialog_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context, int seconds) {
   g_seconds = seconds;
   if (MessageBoxCallback){
      ssd_confirm_dialog_close();
   }
   else{
      MessageBoxCallback = ssd_confirm_dialog_update;
   }
   ssd_confirm_dialog_custom(title, text, default_yes, callback, context, roadmap_lang_get("Yes"),roadmap_lang_get("No"));
   roadmap_main_set_periodic (1000, ssd_confirm_dialog_update );
}

