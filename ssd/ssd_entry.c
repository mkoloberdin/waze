/* ssd_entry.c - entry widget
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
 *   See ssd_entry.h.
 */

#include <string.h>
#include <stdlib.h>
#include "roadmap_lang.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "ssd_keyboard_dialog.h"
#include "ssd_confirm_dialog.h"

#include "ssd_entry.h"

typedef struct
{
	const char* 	  mb_text;			/* Message box text for the confirmed entry */
    const char* 	  kb_title;			/* Title for the extended keyboard */
    const char* 	  kb_label;			/* Label for the extended keyboard */
    const char* 	  kb_note;			/* Note for the extended keyboard */
    CB_OnKeyboardDone kb_post_done_cb; 	/* Post processing callback for the keyboard done */
    int				  kb_flags;			/* Flags for the extended keyboard */
} SsdEntryContext;


static void entry_ctx_init( SsdEntryContext* ctx )
{
	memset( ctx, 0, sizeof( SsdEntryContext ) );
}

static BOOL on_kb_closed(  int         exit_code,
                           const char* value,
                           void*       context)
{
   SsdWidget w = context;
   SsdEntryContext* ctx = ( SsdEntryContext* ) w->context;
   BOOL retVal = TRUE;

   if( dec_ok == exit_code)
      w->set_value( w, value);

   if( value && value[0])
      ssd_widget_hide( ssd_widget_get( w, "BgText"));
   else
   	ssd_widget_show( ssd_widget_get( w, "BgText"));

   if ( ctx->kb_post_done_cb )
   {
	   retVal = ctx->kb_post_done_cb( exit_code, value, context );
   }

   return retVal;
}


static int edit_callback (SsdWidget widget, const char *new_value) {

   const char *value;
   SsdEntryContext* ctx;
   /* Get the entry widget */
   widget = widget->parent;

   ctx = ( SsdEntryContext* ) widget->context;

   value = widget->get_value (widget);

#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN))
   {
	   SsdWidget text = ssd_widget_get(widget->children, "Text");

	   if ( text && (text->flags & SSD_TEXT_PASSWORD) )
	   {
	   		ShowEditbox( "", value, on_kb_closed, widget, EEditBoxPassword );
	   }
	   else
	   {
			ShowEditbox( "", value, on_kb_closed, widget, EEditBoxStandard );
	   }
   }

#else
   ssd_show_keyboard_dialog_ext(  ctx->kb_title,
								   value,
								   ctx->kb_label,
								   ctx->kb_note,
								   on_kb_closed,
								   widget,
								   ctx->kb_flags );
#endif

   return 1;
}

static void confirm_cb(int exit_code, void *context){
   if (exit_code == dec_yes ){
      edit_callback((SsdWidget)context, NULL);
   }
}

static int confirmed_edit_callback (SsdWidget widget, const char *new_value)
{
	SsdEntryContext *ctx = ( SsdEntryContext* ) widget->parent->context;
	const char* mb_text = ctx->mb_text;
    ssd_confirm_dialog("Warning",(char *) mb_text, FALSE, confirm_cb, (void *) widget);

   return 1;
}


static int set_value (SsdWidget widget, const char *value) {
   if ((value != NULL) && (value[0] != 0))
		ssd_widget_hide(ssd_widget_get(widget, "BgText"));
   else
   		ssd_widget_show(ssd_widget_get(widget, "BgText"));

   return ssd_widget_set_value (widget, "Text", value);
}


const char *get_value (SsdWidget widget) {
   return ssd_widget_get_value (widget, "Text");
}

SsdWidget ssd_entry_new (const char *name,
                         const char *value,
                         int entry_flags,
                         int text_flags,
                         int width,
                         int height,
                         const char *background_text)
{

   SsdWidget space;
   SsdWidget bg_text;
   SsdWidget entry;
   SsdWidget text_box;
   int tab_st = 0;
   int txt_box_height = 40;
   SsdEntryContext*  ctx = (SsdEntryContext*) calloc( 1, sizeof( SsdEntryContext ) );
#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif

   entry_ctx_init( ctx );

   if (entry_flags & SSD_WS_TABSTOP){
      entry_flags &= ~SSD_WS_TABSTOP;
      tab_st = SSD_WS_TABSTOP;
   }

   entry =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, entry_flags);

   text_box =
      ssd_container_new ("text_box", NULL, width,
               txt_box_height, SSD_CONTAINER_TXT_BOX|SSD_ALIGN_VCENTER|tab_st);
   ssd_widget_set_pointer_force_click( text_box );
   entry->get_value = get_value;
   entry->set_value = set_value;

   entry->bg_color = NULL;

   text_box->callback = edit_callback;
   text_box->bg_color = NULL;


   space = ssd_container_new("Space", NULL,5, SSD_MIN_SIZE, SSD_WIDGET_SPACE);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add(text_box, space);
   ssd_widget_add (text_box, ssd_text_new ("Text", value, -1, text_flags|SSD_ALIGN_VCENTER));
   if (background_text == NULL)
   		bg_text = ssd_text_new ("BgText", "", -1, SSD_ALIGN_VCENTER);
   else
   		bg_text = ssd_text_new ("BgText", background_text, -1, SSD_ALIGN_VCENTER);
   ssd_widget_set_color(bg_text, "#C0C0C0",NULL);
   ssd_widget_add (text_box, bg_text);
   entry->context = ctx;

   if ((value != NULL) && (value[0] != 0))
		ssd_widget_hide(bg_text);

   ssd_widget_add (entry, text_box);

   /* Default keyboard params */
   ssd_entry_set_kb_params( entry, background_text, NULL, NULL, NULL, 0 );


   return entry;
}

SsdWidget ssd_confirmed_entry_new (const char *name,
                         const char *value,
                         int entry_flags,
                         int text_flags,
                         int width,
                         int height,
                         const char *messagebox_text,
                         const char *background_text) {

   const char *edit_button[] = {"edit_right", "edit_left"};
   SsdWidget button;
   SsdWidget space;
   SsdWidget bg_text;
   SsdWidget entry;
   SsdWidget text_box;
   int tab_st = 0;
   int txt_box_height = 40;
   SsdEntryContext*  ctx = (SsdEntryContext*) calloc( 1, sizeof( SsdEntryContext ) );
#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif
   entry_ctx_init( ctx );

   if (entry_flags & SSD_WS_TABSTOP){
      entry_flags &= ~SSD_WS_TABSTOP;
      tab_st = SSD_WS_TABSTOP;
   }

   entry =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, entry_flags);

   text_box =
      ssd_container_new ("text_box", NULL, width,
               txt_box_height, SSD_CONTAINER_TXT_BOX|SSD_ALIGN_VCENTER|tab_st);

   ssd_widget_set_pointer_force_click( text_box );
   entry->get_value = get_value;
   entry->set_value = set_value;

   entry->bg_color = NULL;

   text_box->callback = confirmed_edit_callback;
   text_box->bg_color = NULL;


   space = ssd_container_new("Space", NULL,5, SSD_MIN_SIZE, SSD_WIDGET_SPACE);
   ssd_widget_set_color(space, NULL, NULL);
   ssd_widget_add(text_box, space);
   ssd_widget_add (text_box, ssd_text_new ("Text", value, -1, text_flags|SSD_ALIGN_VCENTER));
   if (background_text == NULL)
         bg_text = ssd_text_new ("BgText", "", -1, SSD_ALIGN_VCENTER);
   else
         bg_text = ssd_text_new ("BgText", background_text, -1, SSD_ALIGN_VCENTER);
   ssd_widget_set_color(bg_text, "#C0C0C0",NULL);
   ssd_widget_add (text_box, bg_text);
#ifdef TOUCH_SCREEN   
   if (!ssd_widget_rtl(NULL))
      button = ssd_button_new ("edit_button", "", &edit_button[0], 1,
                          SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, edit_callback);
   else
      button = ssd_button_new ("edit_button", "", &edit_button[1], 1,
                          SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, edit_callback);

   if (!ssd_widget_rtl(NULL))
         ssd_widget_set_offset(button, 10, 0);
   else
      ssd_widget_set_offset(button, -11, 0);
#endif
   ctx->mb_text = (void *)strdup(messagebox_text);

   entry->context = ctx;

   if ((value != NULL) && (value[0] != 0))
      ssd_widget_hide(bg_text);

   ssd_widget_add (entry, text_box);

   /* Default keyboard params */
   ssd_entry_set_kb_params( entry, name, NULL, NULL, NULL, 0 );

   return entry;
}

/***********************************************************
 *  Name        : ssd_entry_set_extended_kb_params
 *  Purpose     : Initializes the keyboard params
 *  Params      : [in]	-
 *              : [out] -
 *  Returns		:
 *  Notes       :
 */
void ssd_entry_set_kb_params( SsdWidget widget,
					  		  const char* kb_title,
							  const char* kb_label,
							  const char* kb_note,
							  CB_OnKeyboardDone kb_post_done_cb,
							  int kb_flags )
{
	SsdEntryContext *ctx = (SsdEntryContext* ) widget->context;
	if ( ctx == NULL )
		return;

	ctx->kb_title = kb_title;
	ctx->kb_label = kb_label;
	ctx->kb_note = kb_note;
	ctx->kb_post_done_cb = kb_post_done_cb;
	ctx->kb_flags = kb_flags;
}

/*
 * Set entry keyboard dialog title
 */
void ssd_entry_set_kb_title( SsdWidget widget, const char* kb_title )
{
	SsdEntryContext *ctx = (SsdEntryContext* ) widget->context;
	if ( ctx == NULL )
		return;

	ctx->kb_title = kb_title;
}

