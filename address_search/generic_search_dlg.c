/* generic_search_dlg.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *
 */


#include <stdlib.h>
#include <string.h>
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_list.h"
#include "../ssd/ssd_contextmenu.h"
#include "../ssd/ssd_bitmap.h"
#include "../ssd/ssd_keyboard.h"
#include "../ssd/ssd_keyboard_dialog.h"
#include "../ssd/ssd_progress_msg_dialog.h"
#include "../roadmap_input_type.h"
#include "../roadmap.h"
#include "../roadmap_lang.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_geocode.h"
#include "../roadmap_trip.h"
#include "../roadmap_history.h"
#include "../roadmap_locator.h"
#include "../roadmap_display.h"
#include "../roadmap_main.h"
#include "../roadmap_geo_config.h"
#include "../roadmap_square.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../roadmap_map_download.h"
#include "address_search.h"
#include "../roadmap_start.h"
#include "generic_search_dlg.h"
#include "address_search_dlg.h"
#include "local_search_dlg.h"
#include "../roadmap_bar.h"
#include "../roadmap_device_events.h"

#ifdef TOUCH_SCREEN
   #define  USE_ONSCREEN_KEYBOARD
#endif   // TOUCH_SCREEN

#ifdef   USE_ONSCREEN_KEYBOARD
   #include "../ssd/ssd_keyboard.h"
#endif   // USE_ONSCREEN_KEYBOARD

typedef struct tag_search_dlg_context
{
   SsdWidget             c_dlg;
   PFN_ON_DIALOG_CLOSED  c_cb;
   void*                 c_ctx;
   const char           *c_dlg_name;
   const char           *c_dlg_title;
   RoadMapCallback       c_on_search;
   GenericSearchOnReopen c_on_reopen;
   const char           *c_result_conatiner_name;
   SsdWidget             c_rcnt;
   BOOL                  c_1st;
   PFN_ON_DIALOG_CLOSED  c_saved_cb;
   char                  c_saved_txt[0xFF];
}search_dlg_context;

search_dlg_context search_context[__search_types_count];
static   search_types s_type;
static   BOOL                 s_history_was_loaded = FALSE;
static   SsdWidget    s_kb_refs[__search_types_count];

#define  GSD_DIALOG_NAME            "address-search-dialog"
#define  GSD_INPUT_CONT_NAME        "address-search-dialog.input-container"
#define  GSD_IC_EDITBOX_NAME        "address-search-dialog.input-container.editbox"
#define  GSD_IC_EDITBOX_TOPSPACER   "address-search-dialog.input-container.editbox.topspacer"
#define  GSD_IC_EDITBOX_CNT_NAME    "address-search-dialog.input-container.editbox.container"
#define  GSD_IC_BUTTON_NAME         "address-search-dialog.input-container.button"
#define  GSD_IC_BUTTON_LABEL        "Search"

static RMNativeKBParams s_gNativeKBParams = {  _native_kb_type_default, 1, _native_kb_action_search };




static void on_device_event( device_event event, void* context );
static void update_editbox_topspace() ;

void generic_search_dlg_switch_gui(void)
{
   SsdWidget current = search_context[s_type].c_dlg->children;
   SsdWidget other   = search_context[s_type].c_dlg->context;


   ssd_widget_replace( search_context[s_type].c_dlg, current, other);
   search_context[s_type].c_dlg->context = current;

   search_context[s_type].c_1st = !search_context[s_type].c_1st;

   if (!strcmp(other->name, search_context[s_type].c_result_conatiner_name))
      ssd_dialog_set_current_scroll_flag(TRUE);
   else
   {
      SsdWidget edit_cont= ssd_widget_get( search_context[s_type].c_dlg, GSD_IC_EDITBOX_CNT_NAME);
      if ( roadmap_native_keyboard_enabled() )
      {
         roadmap_native_keyboard_show( &s_gNativeKBParams );
      }
      ssd_dialog_set_focus( edit_cont );
      ssd_dialog_set_current_scroll_flag( FALSE );
   }
   ssd_dialog_reset_offset();
   ssd_dialog_draw ();

}

void reopen_keyboard(void)
{
   if( search_context[s_type].c_saved_cb){
         (*search_context[s_type].c_on_reopen)( search_context[s_type].c_saved_cb, NULL);
   }
   search_context[s_type].c_saved_cb    = NULL;
   search_context[s_type].c_saved_txt[0]= '\0';
   roadmap_main_remove_periodic( reopen_keyboard);
}




void on_dlg_closed( int exit_code, void* context)
{
   PFN_ON_DIALOG_CLOSED cb = search_context[s_type].c_cb;
   void*                ctx= search_context[s_type].c_ctx;

   search_context[s_type].c_saved_cb = search_context[s_type].c_cb;
   search_context[s_type].c_cb     = NULL;
   search_context[s_type].c_ctx    = NULL;

   // Special case:
   // If 'ok' was sent and we are showing the 'second gui' - the list - then
   // rollback to 'first-gui' - the keyboard.
   if( (dec_ok == exit_code) && !search_context[s_type].c_1st)
   {
      SsdWidget   cnt      = search_context[s_type].c_dlg->context;
      SsdWidget   edit_cont= ssd_widget_get( cnt, GSD_IC_EDITBOX_CNT_NAME);
      SsdWidget   edit     = ssd_widget_get( edit_cont,GSD_IC_EDITBOX_NAME);
      const char* val      = ssd_text_get_text( edit);

      if( val && (*val))
         strcpy( search_context[s_type].c_saved_txt, val);

      roadmap_main_set_periodic( 50, reopen_keyboard);
      return;
   }

   if(cb)
      cb( exit_code, ctx);

   search_context[s_type].c_saved_cb   = NULL;
   search_context[s_type].c_saved_txt[0]= '\0';
   // Show the top bar
   roadmap_top_bar_show();
}

static BOOL on_key_pressed__delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;
   SsdWidget main_cont= this->parent;

   assert( this);
   assert( main_cont);
   assert( this->children);
   assert( this->children->key_pressed);

   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      (*search_context[s_type].c_on_search)();

      return TRUE;
   }

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}

#ifdef USE_ONSCREEN_KEYBOARD
static BOOL on_keyboard_pressed( void* context, const char* utf8char, uint32_t flags)
{
   SsdWidget edt = (SsdWidget)context;

   return edt->key_pressed( edt, utf8char, flags);
}

static void on_keyboard_search( void* context, const char* command)
{
   (*search_context[s_type].c_on_search)();
}
#endif   // USE_ONSCREEN_KEYBOARD

int on_btn_search( SsdWidget w, const char* v)
{
   (*search_context[s_type].c_on_search)();
   return 0;
}


SsdWidget create_input_container()
{
   SsdWidget icnt = NULL;
   SsdWidget ecnt = NULL;
   SsdWidget edit = NULL;
   SsdWidget btn  = NULL;
   SsdWidget bitmap = NULL;
   SsdWidget space  = NULL;
   int txt_box_height = 40;
   int edit_box_top_offset = ssd_keyboard_edit_box_top_offset();

#ifndef TOUCH_SCREEN
   txt_box_height = 23;
#endif
   icnt = ssd_container_new(  GSD_INPUT_CONT_NAME,
                              NULL,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              0);
   ssd_widget_set_color(icnt, NULL, NULL);

   ecnt = ssd_container_new(  GSD_IC_EDITBOX_CNT_NAME,
                              NULL,
                              SSD_MAX_SIZE,
                              txt_box_height,
                              SSD_WS_TABSTOP|SSD_CONTAINER_TXT_BOX|SSD_END_ROW|SSD_ALIGN_CENTER);

   bitmap = ssd_bitmap_new("serach", "search_icon", SSD_ALIGN_VCENTER);


   ssd_widget_add(ecnt, bitmap);

   edit = ssd_text_new     (  GSD_IC_EDITBOX_NAME,
                              "", 18, SSD_ALIGN_VCENTER|SSD_TEXT_INPUT );


   btn  = ssd_button_label (  GSD_IC_BUTTON_NAME,
                              roadmap_lang_get(GSD_IC_BUTTON_LABEL),
                              SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_WS_TABSTOP|SSD_END_ROW,
                              on_btn_search);

   //   Delegate the 'on key pressed' event to the child edit-box:
   ecnt->key_pressed = on_key_pressed__delegate_to_editbox;

   ssd_text_set_input_type( edit, inputtype_free_text);
   ssd_text_set_readonly  ( edit, FALSE);

   space = ssd_container_new ( GSD_IC_EDITBOX_TOPSPACER, NULL, SSD_MAX_SIZE, edit_box_top_offset, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   ssd_widget_add(icnt, space);

   ssd_widget_add( ecnt, edit);
   ssd_widget_add( icnt, ecnt );
   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 5, SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (space, NULL,NULL);
   ssd_widget_add(icnt, space);

  #ifdef   USE_ONSCREEN_KEYBOARD
  s_kb_refs[s_type] = ssd_create_keyboard( icnt,
					 on_keyboard_pressed,
					 on_keyboard_search,
					 roadmap_lang_get( "Search"),
					 edit);
  #endif   // USE_ONSCREEN_KEYBOARD

#ifndef TOUCH_SCREEN
   ssd_widget_add( icnt, btn);
#endif
   return icnt;
}

static void set_softkeys( SsdWidget dialog,SsdSoftKeyCallback left_sk_callback, SsdSoftKeyCallback right_sk_callback)
{
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback   ( dialog, left_sk_callback);

   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Back"));
   ssd_widget_set_right_softkey_callback  ( dialog, right_sk_callback);
}


static SsdWidget create_dlg(const char *dlg_name, const char *dlg_title, SsdWidget rcnt)
{
   SsdWidget icnt = create_input_container();
   SsdWidget dlg  = NULL;

   assert(icnt);
   assert(rcnt);

   search_context[s_type].c_dlg_name = dlg_name;
   search_context[s_type].c_dlg_title = dlg_title;
   search_context[s_type].c_rcnt = rcnt;

   dlg  = ssd_dialog_new(  dlg_name,
                           roadmap_lang_get(dlg_title),
                           on_dlg_closed,
                           SSD_DIALOG_GUI_TAB_ORDER|SSD_CONTAINER_TITLE|SSD_PERSISTENT| /* AGA TODO :: Check if persistency is necessary here */
                           SSD_DIALOG_NO_SCROLL);

   assert( dlg);
   assert(!dlg->context);

   ssd_widget_add( dlg,  icnt);
   dlg->context = rcnt;

   return dlg;
}


static SsdWidget get_dlg(const char *dlg_name, const char *dlg_title, SsdWidget rcnt)
{
   if( !ssd_dialog_exists( dlg_name ) )
   {
      search_context[s_type].c_1st = TRUE;
      search_context[s_type].c_dlg = create_dlg(dlg_name, dlg_title, rcnt);
      roadmap_device_events_register( on_device_event, NULL);
   }
   return search_context[s_type].c_dlg;
}

SsdWidget generic_search_dlg_get_search_dlg(search_types type)
{
   if( !search_context[type].c_dlg)
   {
      search_context[type].c_dlg = create_dlg(search_context[type].c_dlg_name, search_context[type].c_dlg_title, search_context[type].c_rcnt);
      roadmap_device_events_register( on_device_event, NULL);
   }
   return search_context[type].c_dlg;
}

SsdWidget generic_search_dlg_get_search_edit_box(search_types type){
   SsdWidget dlg = generic_search_dlg_get_search_dlg(type);
   SsdWidget edit = ssd_widget_get( dlg, GSD_IC_EDITBOX_NAME );
   return edit;
}


BOOL generic_search_dlg_is_1st(search_types type)
{
   return search_context[type].c_1st;
}

void generic_search_dlg_show( search_types   type,
                              const char *dlg_name,
                              const char *dlg_title,
                              SsdSoftKeyCallback left_sk_callback,
                              SsdSoftKeyCallback right_sk_callback,
                              SsdWidget rcnt,
                              PFN_ON_DIALOG_CLOSED cbOnClosed,
                              RoadMapCallback on_search,
                              GenericSearchOnReopen on_reopen,
                              void*           context)
{
   SsdWidget dialog     = NULL;
   SsdWidget edit       = NULL;
   SsdWidget edit_cont  = NULL;

   if( search_context[type].c_cb || search_context[type].c_ctx)
   {
      assert(0);  // Dialog is in use now
      return;
   }

   if( !s_history_was_loaded)
   {
      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
      roadmap_history_declare( ADDRESS_FAVORITE_CATEGORY,ahi__count);
      s_history_was_loaded = TRUE;
   }

   s_type           = type;
   search_context[s_type].c_cb         = cbOnClosed;
   search_context[s_type].c_ctx        = context;
   search_context[s_type].c_dlg_name   = dlg_name;
   search_context[s_type].c_dlg_title  = dlg_title;
   search_context[s_type].c_on_search  = on_search;
   search_context[s_type].c_on_reopen  = on_reopen;
   search_context[s_type].c_result_conatiner_name = rcnt->name;
   dialog   = get_dlg(dlg_name, dlg_title, rcnt);
   set_softkeys(dialog, left_sk_callback, right_sk_callback);
   ssd_dialog_invalidate_tab_order_by_name(GSD_DIALOG_NAME);
   ssd_dialog_activate( dlg_name, NULL);

   if( !search_context[s_type].c_1st)
      generic_search_dlg_switch_gui();

   edit_cont= ssd_widget_get( dialog, GSD_IC_EDITBOX_CNT_NAME);
   edit     = ssd_widget_get(edit_cont,GSD_IC_EDITBOX_NAME);

   if( search_context[s_type].c_saved_cb && search_context[s_type].c_saved_txt[0])
      ssd_text_set_text( edit, search_context[s_type].c_saved_txt);
   else
      ssd_text_reset_text( edit);

   ssd_dialog_set_current_scroll_flag(FALSE);

   update_editbox_topspace();
   if ( roadmap_native_keyboard_enabled() )
   {
	  if ( s_kb_refs[s_type] )
	  {
		  ssd_widget_hide( s_kb_refs[s_type] );
	  }
      roadmap_native_keyboard_show( &s_gNativeKBParams );
   }
   else
   {
	  if ( s_kb_refs[s_type] )
	  {
		  ssd_widget_show( s_kb_refs[s_type] );
	  }
   }
   roadmap_input_type_set_mode( inputtype_free_text );

   ssd_dialog_activate( dlg_name, NULL );

   ssd_dialog_set_focus( edit_cont );

   ssd_dialog_draw ();
}

/* Top space for the editbox handler. Updates the top space depending on the portrait landscape */
static void update_editbox_topspace()
{
   SsdWidget dialog = generic_search_dlg_get_search_dlg(s_type);
   SsdWidget spacer = ssd_widget_get( dialog, GSD_IC_EDITBOX_TOPSPACER );
   // SsdWidget button = ssd_widget_get( dialog, GSD_IC_BUTTON_NAME );
   int topspace = ssd_keyboard_edit_box_top_offset();

   if ( !dialog || !spacer )
      return;
   ssd_widget_set_size( spacer, SSD_MAX_SIZE, topspace );
   /*
   if ( roadmap_canvas_width() > roadmap_canvas_height() )  // Landscape
   {
      // roadmap_top_bar_hide();
      ssd_widget_hide( button );
   }
   else  // Portrait
   {
      // roadmap_top_bar_show();
      ssd_widget_show( button );
   }
   */
}


static void on_device_event( device_event event, void* context )
{
   if( device_event_window_orientation_changed == event)
   {
      update_editbox_topspace();
   }
}


void generic_search_dlg_reopen_native_keyboard(void)
{
   if ( roadmap_native_keyboard_enabled() )
   {
      roadmap_native_keyboard_show( &s_gNativeKBParams );
   }
}
