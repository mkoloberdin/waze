
/* ssd_keyboard_dialog.c
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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
 */

#include "ssd_keyboard.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"
#include "ssd_contextmenu.h"
#include "../roadmap_lang.h"

#include "ssd_keyboard_dialog.h"
#include "ssd_bitmap.h"
#include "roadmap_native_keyboard.h"
#include "roadmap_keyboard.h"

#define SSD_KB_DLG_FIELDS_VSPACE			5
#define SSD_KB_DLG_FIELDS_HOFFSET			10
#define SSD_KB_DLG_LABEL_FONT_SIZE			16
#define SSD_KB_DLG_NOTE_FONT_SIZE			14
#define SSD_KB_DLG_LABELED_ENTRY_WIDTH		190			/* Entry width in pixels. Has to be defined and configured */
														/*  for each screen size by taken into consideration the    */
														/* portrait/landscape issues		*** AGA ***			   */
#define SSD_KB_DLG_LABEL_WIDTH				100

static   SsdWidget            s_dialog       = NULL;
const    char*                s_dialog_name  = "ssd_dynamic_keyboard_dialog";
const    char*                s_dialogbox_name  = "ssd_dynamic_keyboard_dialog.box";
const    char*                s_editbox_name = "ssd_dynamic_keyboard_dialog.editbox";
const    char*                s_editcnt_name = "ssd_dynamic_keyboard_dialog.editcnt";
const    char*                s_nextbtn_name = "ssd_dynamic_keyboard_dialog.nextbtn";
const    char*                s_btncnt_name = "ssd_dynamic_keyboard_dialog.btncnt";
const    char*                s_editlabelcnt_name = "ssd_dynamic_keyboard_dialog.editlabelcnt";
const    char*                s_editlabel_name = "ssd_dynamic_keyboard_dialog.editlabel";
const    char*                s_notecnt_name = 	 "ssd_dynamic_keyboard_dialog.notecnt";
const    char*                s_notetext_name = "ssd_dynamic_keyboard_dialog.notetext";

static   CB_OnKeyboardDone s_cbOnDone     = NULL;
static   const char*       s_InitialValue = NULL;

static RMNativeKBParams s_gNativeKBParams = {  _native_kb_type_default, 1, _native_kb_action_done };

static int btn_callback( SsdWidget widget, const char *new_value );
static void set_note_widget( const char* note, int entry_width );
static void set_label_widget( const char* label, int entry_width, int label_width );

extern void ssd_keyboard_set_value_index( SsdWidget kb, int index);
extern int  ssd_keyboard_get_value_index( SsdWidget   kb);
extern int  ssd_keyboard_get_layout(      SsdWidget   kb);

typedef enum tag_context_menu_items
{
   cmi_layout_qwerty,
   cmi_layout_grid,
   cmi_input_English,
   cmi_input_Hebrew,
   cmi_input_digits,
   cmi_erase_all,

   cmi__count,
   cmi__invalid

}  context_menu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "QWERTY layout",cmi_layout_qwerty),
   SSD_CM_INIT_ITEM  ( "GRID layout",  cmi_layout_grid),
   SSD_CM_INIT_ITEM  ( "English input",cmi_input_English),
   SSD_CM_INIT_ITEM  ( "Hebrew input", cmi_input_Hebrew),
   SSD_CM_INIT_ITEM  ( "Digits input", cmi_input_digits),
   SSD_CM_INIT_ITEM  ( "Clear text",   cmi_erase_all)
};

// Context menu:
static ssd_contextmenu  context_menu            = SSD_CM_INIT_MENU( main_menu_items);
static BOOL             s_context_menu_is_active= FALSE;
static void*            s_context               = NULL;
static void on_done( void* context, const char* command);



static void on_dialog_closed( int exit_code, void* context)
{
#ifndef IPHONE
#endif //IPHONE
   if( dec_ok != exit_code)
      s_cbOnDone( dec_cancel, s_InitialValue, s_context);

   s_cbOnDone     = NULL;
   s_InitialValue = NULL;
   s_context      = NULL;
}

static BOOL on_key( void* context, const char* utf8char, uint32_t flags)
{
   SsdWidget editbox = ssd_widget_get( s_dialog, s_editbox_name);
   return editbox->key_pressed( editbox, utf8char, flags);
}


static BOOL on_key_pressed__delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget   editbox  = NULL;

   assert( this);
   assert( this->children);
   assert( this->children->key_pressed);

   if( KEY_IS_ENTER)
   {
      on_done( NULL, NULL );
      return TRUE;
   }

   editbox = this->children;


   if ( !VKEY_IS_SOFTKEY_LEFT && !VKEY_IS_SOFTKEY_RIGHT && !KEY_IS_ESCAPE && roadmap_keyboard_typing_locked( TRUE ) )
   {
	   return TRUE;
   }

   return editbox->key_pressed( editbox, utf8char, flags);
}

static void on_done( void* context, const char* command)
{
   SsdWidget   edit        = ssd_widget_get( s_dialog, s_editbox_name);
   const char* output_value= ssd_text_get_text( edit);

   if( s_cbOnDone( dec_ok, output_value, s_context))
      ssd_dialog_hide( s_dialog_name, dec_ok);
}

static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   context_menu_items   selection   = cmi__invalid;
   SsdWidget            kb          = context;
   s_context_menu_is_active         = FALSE;

   if( !made_selection)
      return;

   selection = item->id;

   switch( selection)
   {
      case cmi_layout_qwerty:
         ssd_keyboard_set_qwerty_layout( kb);
         break;

      case cmi_layout_grid:
         ssd_keyboard_set_grid_layout( kb);
         break;

      case cmi_input_English:
         ssd_keyboard_set_charset( kb, English_lowcase);
         break;

      case cmi_input_Hebrew:
         ssd_keyboard_set_charset( kb, Hebrew);
         break;

      case cmi_input_digits:
         ssd_keyboard_set_charset( kb, Nunbers_and_symbols);
         break;

      case cmi_erase_all:
      {
         SsdWidget edit = ssd_widget_get( s_dialog, s_editbox_name);
         ssd_text_set_text( edit, "");
         break;
      }
      default:
      {
    	  /* TODO:: Add default handling */
    	  break;
      }
   }
}

int on_options( SsdWidget widget, const char *new_value, void *context)
{
   SsdWidget   kb          = ssd_get_keyboard( s_dialog);
   int         layout_index= ssd_keyboard_get_layout     ( kb);
   int         value_index = ssd_keyboard_get_value_index( kb);
   SsdWidget   edit        = ssd_widget_get( s_dialog, s_editbox_name);
   const char*	edit_text   = ssd_text_get_text( edit);
   int         menu_x;

   if(s_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      s_context_menu_is_active = FALSE;
   }

   ssd_contextmenu_show_item( &context_menu,
                              cmi_layout_qwerty,
                              qwerty_kb_layout != layout_index,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_layout_grid,
                              qwerty_kb_layout == layout_index,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_input_English,
                              English_lowcase < value_index,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_input_Hebrew,
                              Hebrew != value_index,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_input_digits,
                              Nunbers_and_symbols != value_index,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_erase_all,
                              edit_text && (*edit_text),
                              FALSE);

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           kb,
                           dir_default,
                           0);

   s_context_menu_is_active = TRUE;

   return 0;
}

int on_cancel( SsdWidget widget, const char *new_value, void *context)
{
   ssd_dialog_hide_current( dec_cancel);
   return 0;
}

static void set_softkeys( SsdWidget dialog)
{
   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Cancel"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_cancel);
#ifdef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_options);
#endif
}

void ssd_show_keyboard_dialog(const char*       title,
                              const char*       value,
                              CB_OnKeyboardDone cbOnDone,
                              void*             context)
{
	ssd_show_keyboard_dialog_ext( title, value, NULL, NULL, cbOnDone, context, 0 );
}

void ssd_show_keyboard_dialog_ext( const char*       title,		/* Title at the top of the dialog */
								   const char*       value,		/* Value in the edit box */
								   const char*       label,
								   const char*       note,
								   CB_OnKeyboardDone cbOnDone,
								   void*             context,
								   int kb_dlg_flags )
{
#ifndef IPHONE
   SsdWidget edit = NULL;
   SsdWidget ecnt = NULL;
   SsdWidget kb   = NULL;
   SsdWidget btn_next   = NULL;
   SsdWidget box   = NULL;
   SsdWidget note_cnt = NULL;
   SsdWidget btn_cnt = NULL;
   SsdWidget lbl = NULL;
   SsdWidget elabelcnt = NULL;

   const char* special_btn_name = "Done";
   const char* btn_next_label = "Next";

   int entry_width = SSD_MAX_SIZE;
   int entry_height = 40;
   int box_height = SSD_MIN_SIZE;
   assert(cbOnDone);
  // assert(!s_cbOnDone);

#ifndef TOUCH_SCREEN
   entry_height = 23;
#endif

   s_InitialValue = value;
   s_cbOnDone     = cbOnDone;
   s_context      = context;

   /* If label for the entry box is provided - leave the space for it */
   if ( label )
   {
	   entry_width = SSD_KB_DLG_LABELED_ENTRY_WIDTH;
   }
   /* If the native keyboard is enabled - fill the screen, */
   /* otherwise - set minimum size possible to leave space for the keyboard  */
   if ( roadmap_native_keyboard_enabled() )
   {
	   box_height = SSD_MAX_SIZE;
   }
   
   roadmap_keyboard_set_typing_lock_enable(  ( kb_dlg_flags & SSD_KB_DLG_TYPING_LOCK_ENABLE ) != 0 );
   if( !s_dialog)
   {
	  // int edit_box_top_offset = ssd_keyboard_edit_box_top_offset();
      s_dialog = ssd_dialog_new( s_dialog_name,
                                 title,
                                 on_dialog_closed,
                                 SSD_DIALOG_NO_SCROLL|SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);
      set_softkeys( s_dialog);

      box = ssd_container_new ( s_dialogbox_name, NULL, SSD_MAX_SIZE,
               SSD_MIN_SIZE, SSD_END_ROW | SSD_ALIGN_CENTER | SSD_ALIGN_CENTER);
      ssd_widget_set_color(box, NULL, NULL);
      ssd_widget_set_offset( box, 0, -1 );

      ssd_dialog_add_vspace( box, SSD_KB_DLG_FIELDS_VSPACE, 0 );

      ecnt = ssd_container_new( s_editcnt_name, NULL, entry_width, entry_height, SSD_CONTAINER_TXT_BOX|SSD_WS_TABSTOP );
      ssd_widget_set_color(ecnt, NULL, NULL);
      edit = ssd_text_new     ( s_editbox_name, "", 18, SSD_ALIGN_VCENTER );
      ssd_text_set_input_type ( edit, inputtype_free_text);
      ssd_text_set_readonly   ( edit, FALSE);
      //   Delegate the 'on key pressed' event to the child edit-box:
      ecnt->key_pressed = on_key_pressed__delegate_to_editbox;

      ssd_widget_add( ecnt, edit );
      ssd_widget_add( ecnt, ssd_bitmap_new("cursor", "cursor", SSD_ALIGN_VCENTER ) );
#ifdef TOUCH_SCREEN  
      ssd_dialog_add_hspace( box, SSD_KB_DLG_FIELDS_HOFFSET-1, 0 );
#endif
      //// Label for the edit box
	  elabelcnt = ssd_container_new( s_editlabelcnt_name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE );
	  lbl = ssd_text_new( s_editlabel_name, "", SSD_KB_DLG_LABEL_FONT_SIZE, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER );
	  ssd_widget_set_color( elabelcnt, NULL, NULL );
	  ssd_widget_add( elabelcnt, lbl );
	  ssd_dialog_add_hspace( elabelcnt, SSD_KB_DLG_FIELDS_HOFFSET, SSD_ALIGN_VCENTER );
	  ssd_widget_add( box, elabelcnt );
	  ssd_widget_add( box, ecnt);


	  //// Spacing
      ssd_dialog_add_hspace( box, SSD_KB_DLG_FIELDS_HOFFSET, SSD_END_ROW|SSD_ALIGN_RIGHT );

	  //// Note under the edit box
      note_cnt = ssd_container_new( s_notecnt_name, NULL, entry_width, 1, SSD_WIDGET_SPACE|SSD_ALIGN_RIGHT );
	  ssd_widget_add( note_cnt, ssd_text_new( s_notetext_name, "", -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER ) );
	  ssd_widget_add( box, note_cnt );

#if !defined(__SYMBIAN32__) && !defined (_WIN32)
	  ssd_dialog_add_hspace( box, SSD_KB_DLG_FIELDS_HOFFSET, SSD_END_ROW );

      //// Next button
	  btn_cnt = ssd_container_new( s_notecnt_name, NULL, SSD_MIN_SIZE, 35, SSD_WIDGET_SPACE|SSD_ALIGN_RIGHT );
	  ssd_widget_set_color( btn_cnt, NULL, NULL );
      btn_next = ssd_button_label( s_nextbtn_name, roadmap_lang_get( btn_next_label ), SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, btn_callback );
      ssd_widget_add( btn_cnt, btn_next );
      ssd_widget_add( box, btn_cnt );
      ssd_dialog_add_hspace( box, SSD_KB_DLG_FIELDS_HOFFSET, SSD_END_ROW | SSD_ALIGN_RIGHT);
      ssd_dialog_add_vspace( box, 3*SSD_KB_DLG_FIELDS_VSPACE, 0 );
#endif
      ssd_widget_add( s_dialog, box );

	  ssd_dialog_set_ntv_keyboard_action( s_dialog_name, _ntv_kb_action_show );
	  ssd_dialog_set_ntv_keyboard_params( s_dialog_name, &s_gNativeKBParams );
	  kb = ssd_create_keyboard( s_dialog,
								on_key,
								on_done,
								special_btn_name,
								s_dialog);

	  ssd_widget_set_offset( kb, 0, -1 );
   }
   else
   {
      kb = ssd_get_keyboard( s_dialog );
   }
	ecnt = ssd_widget_get( s_dialog, s_editcnt_name);
	edit = ssd_widget_get( s_dialog, s_editbox_name);
	box = ssd_widget_get( s_dialog, s_dialogbox_name );

	if ( roadmap_native_keyboard_enabled() )
	{
		ssd_widget_hide( kb );
	}
	else
	{
#ifdef TOUCH_SCREEN
		ssd_widget_show( kb );
#else	
		ssd_widget_hide( kb );
#endif		
		if ( !( kb_dlg_flags & SSD_KB_DLG_LAST_KB_STATE ) )
		{
		  ssd_keyboard_reset_state( kb);

		  if ( kb_dlg_flags & SSD_KB_DLG_INPUT_ENGLISH )
		  {
			  ssd_keyboard_set_value_index( kb, English_lowcase );
		  }
		}
	}
	ssd_widget_set_size( box, SSD_MAX_SIZE, box_height );

   if( value && (*value))
      ssd_text_set_text( edit, value);
   else
      ssd_text_set_text( edit, "");

#if !defined(__SYMBIAN32__) && !defined (_WIN32)
   /* Test flags */
   btn_next = ssd_widget_get( s_dialog, s_nextbtn_name );
   if ( kb_dlg_flags & SSD_KB_DLG_SHOW_NEXT_BTN )
   {
	   RMNativeKBParams kb_prms = s_gNativeKBParams;
	   kb_prms.action_button = _native_kb_action_next;
	   kb_prms.close_on_action = 0;
 	   ssd_dialog_set_ntv_keyboard_params( s_dialog_name, &kb_prms );
	   ssd_widget_show( btn_next );
   }
   else
   {
	   ssd_dialog_set_ntv_keyboard_params( s_dialog_name, &s_gNativeKBParams );
	   ssd_widget_hide( btn_next );
   }
#endif
   /* Label handling for the edit box */
   set_label_widget( label, entry_width, SSD_KB_DLG_LABEL_WIDTH );
   /* Note handling for the edit box */
   set_note_widget( note, entry_width );

   ssd_dialog_activate( s_dialog_name, NULL);
   // Set the title
   ssd_dialog_set_value( "title_text", roadmap_lang_get( title ) );


   ssd_widget_set_focus_highlight( ecnt, FALSE );
   ssd_widget_set_focus_highlight( edit, FALSE );
   ssd_dialog_draw ();

   ssd_dialog_set_focus( ecnt );
   
#endif //IPHONE
}
/***********************************************************
 *  Adjusts the widgets according to the note data and
 *    width of the entry field
 */
static void set_note_widget( const char* note, int entry_width )
{
	   SsdWidget note_cnt = ssd_widget_get( s_dialog, s_notecnt_name );

	   if ( note == NULL )
	   {
		   ssd_widget_hide( ssd_widget_get( s_dialog, s_notecnt_name  ) );
	   }
	   else
	   {
		   SsdWidget note_text = ssd_widget_get( s_dialog, s_notetext_name );

		   ssd_widget_show( note_cnt );
		   // Adjust the size of the note and set the text
		   ssd_text_set_text( note_text, note );
	   }
	   ssd_widget_set_size( note_cnt, entry_width, SSD_MIN_SIZE );
}

/***********************************************************
 *  Adjusts the widgets according to the label data and
 *    width parameters of the label and entry
 */
static void set_label_widget( const char* label, int entry_width, int label_width )
{
	int entry_height = 40;
	SsdWidget ecnt;
#ifndef TOUCH_SCREEN
	entry_height = 23;
#endif
	ecnt = ssd_widget_get( s_dialog, s_editcnt_name);
	if ( label == NULL )
	{
	   ssd_widget_hide( ssd_widget_get( s_dialog, s_editlabelcnt_name  ) );
	   ssd_widget_set_size( ecnt, entry_width, entry_height );
	}
	else
	{
	   SsdWidget label_cnt = ssd_widget_get( s_dialog, s_editlabelcnt_name );
	   SsdWidget label_text = ssd_widget_get( s_dialog, s_editlabel_name );
	   ssd_widget_show( label_cnt );
	   // Adjust the sizes of the label and the entry
	   ssd_widget_set_size( ecnt, entry_width, entry_height );
	   ssd_widget_set_size( label_cnt, label_width, entry_height );
	   ssd_text_set_text( label_text, label );
	}
}

static int btn_callback( SsdWidget widget, const char *new_value )
{
	on_done( s_context, "" );
	return 1;
}
