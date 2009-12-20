/*
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

#include <stdlib.h>
#include <string.h>
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_list.h"
#include "../ssd/ssd_contextmenu.h"
#include "../ssd/ssd_bitmap.h"
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
#include "../roadmap_tile.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../Realtime/Realtime.h"
#include "local_search.h"
#include "generic_search.h"
#include "generic_search_dlg.h"
#include "../roadmap_start.h"
#include "local_search_dlg.h"
#include "../roadmap_bar.h"
#include "../roadmap_device_events.h"

#ifdef TOUCH_SCREEN
   #define  USE_ONSCREEN_KEYBOARD
#endif   // TOUCH_SCREEN

#ifdef   USE_ONSCREEN_KEYBOARD
   #include "../ssd/ssd_keyboard.h"
#endif   // USE_ONSCREEN_KEYBOARD

static   BOOL                 s_menu   = FALSE;
static   BOOL                 s_searching = FALSE;
static   SsdWidget            s_result_container = NULL;

#define  LSD_DIALOG_NAME            "Local-search-dialog"
#define  LSD_DIALOG_TITLE           "Google local search"

#define  LSD_RESULTS_CONT_NAME      "local-search-dialog.results-container"
#define  LSD_RC_LIST_NAME           "local-search-dialog.results-container.list"
#define  LSD_RC_BACK_BTN_NAME       "local-search-dialog.results-container.back-btn"
#define  LSD_RC_BACK_BTN_LABEL      "Back"



static void on_search_error_message( int exit_code );
static void search_progress_message_delayed(void);


typedef enum tag_contextmenu_items
{
   cm_navigate,
   cm_show,
   cm_add_to_favorites,
   cm_cancel,
   cm__count,
   cm__invalid

}  contextmenu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Navigate",           cm_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cm_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cm_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Cancel",             cm_cancel)
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);



static int on_options(SsdWidget widget, const char *new_value, void *context);
static int on_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
   on_options( NULL, NULL, NULL);
   return 0;
}

static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results[ADSR_MAX_RESULTS];
   static   void*       indexes[ADSR_MAX_RESULTS];
   static   const char* icons[ADSR_MAX_RESULTS];

   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list;
   int       i;

   s_searching = FALSE;

   /* Close the progress message */
   ssd_progress_msg_dialog_hide();

   roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

   assert(list_cont);

   list = ssd_widget_get( list_cont, LSD_RC_LIST_NAME);

   if( succeeded != rc)
   {
      if( is_network_error( rc))
         roadmap_messagebox_cb ( roadmap_lang_get( "Search location"),
                              roadmap_lang_get( "Search requires internet connection.\r\n"
                                                "Please make sure you are connected."), on_search_error_message );



      else if( err_as_could_not_find_matches == rc)
         roadmap_messagebox_cb ( roadmap_lang_get( "Search location"),
                              roadmap_lang_get( "No results found"), on_search_error_message );
      else
      {
         char msg[64];

         sprintf( msg, "Search failed\nPlease try again later");

         roadmap_messagebox_cb ( roadmap_lang_get( "Search location"), msg, on_search_error_message );
      }

      roadmap_log(ROADMAP_ERROR,
                  "local_search_dlg::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "local_search_dlg::on_address_resolved() - NO RESULTS for the address-resolve process");
      return;
   }

   assert( size <= ADSR_MAX_RESULTS);

   for( i=0; i<size; i++)
   {
      results[i] = array[i].address;
      indexes[i] = (void*)i;
      icons[i] = "search_local";
   }

   if ( roadmap_native_keyboard_enabled() )
   {
	   roadmap_native_keyboard_hide();
   }
   ssd_list_populate(list,
                     size,
                     results,
                     (const void **)indexes,
                     icons,
                     0,
                     on_list_item_selected,
                     NULL,
                     FALSE);


   generic_search_dlg_switch_gui();
}

static void on_search(void)
{
   SsdWidget      list_cont;
   SsdWidget      edit;
   const char*    text;
   roadmap_result rc;
   SsdWidget dlg = generic_search_dlg_get_search_dlg(search_local);
   
   
   if (!RealTimeLoginState()){
      roadmap_messagebox( roadmap_lang_get( "Search location"),
                          roadmap_lang_get( "Search requires internet connection."
                                            "Please make sure you are connected."));
      return;
   }
  
   edit = generic_search_dlg_get_search_edit_box(search_local);
   
   s_searching = TRUE;

   roadmap_main_set_periodic( 100, search_progress_message_delayed );

   text     = ssd_text_get_text( edit );
   list_cont= dlg->context;

   rc = local_search_resolve_address( list_cont, on_address_resolved, text );
   if( succeeded == rc)
   {
      roadmap_main_set_cursor( ROADMAP_CURSOR_WAIT);
      roadmap_log(ROADMAP_DEBUG,
                  "local_search_dlg::on_search() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);
      s_searching = FALSE;
      roadmap_log(ROADMAP_ERROR,
                  "local_search_dlg::on_search() - Resolve process transaction failed to start");
      /* Close the progress message */
      ssd_progress_msg_dialog_hide();
      roadmap_messagebox_cb ( roadmap_lang_get( "Search location"),
                           roadmap_lang_get( err ), on_search_error_message );
   }
}

static int get_selected_list_item()
{
   SsdWidget list;
   SsdWidget dlg;
   
   if( generic_search_dlg_is_1st(search_local))
      return -1;

   dlg = generic_search_dlg_get_search_dlg(search_local);
   list = ssd_widget_get( dlg, LSD_RC_LIST_NAME);
   assert(list);

   return (int)ssd_list_selected_value( list);
}


static BOOL navigate( BOOL take_me_there)
{ 
   return navigate_with_coordinates( take_me_there, search_local, get_selected_list_item());
}

static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   contextmenu_items selection = cm__invalid;
   BOOL              close     = FALSE;
   BOOL              do_nav    = FALSE;

   s_menu = FALSE;

   if( !made_selection)
      return;

   selection = item->id;

   switch( selection)
   {
      case cm_navigate:
         do_nav = TRUE;
         // Roll down...
      case cm_show:
         close = navigate( do_nav);
         break;

      case cm_add_to_favorites:
      {
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         generic_search_add_address_to_history( ADDRESS_FAVORITE_CATEGORY,
                                                selection->city,
                                                selection->street,
                                                get_house_number__str( selection->house),
                                                selection->state,
                                                selection->name,
                                                &position);
         break;
      }

      case cm_cancel:
         close = FALSE;
         break;

      default:
         assert(0);
         break;
   }

   if( close)
   {
      ssd_dialog_hide_all( dec_close);

      if (!roadmap_screen_refresh ())
           roadmap_screen_redraw();
   }
}

static int on_back(SsdWidget widget, const char *new_value, void *context)
{
   if( generic_search_dlg_is_1st(search_local))
      ssd_dialog_hide_current( dec_cancel);
   else
      generic_search_dlg_switch_gui();

   return 0;
}

int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   BOOL add_cancel = TRUE;

#ifdef TOUCH_SCREEN
   roadmap_screen_refresh();
#endif

   assert( !s_menu);

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item( &context_menu,
                              cm_navigate,
                              !generic_search_dlg_is_1st(search_local),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_show,
                              !generic_search_dlg_is_1st(search_local),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_to_favorites,
                              !generic_search_dlg_is_1st(search_local),
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              cm_cancel,
                              add_cancel,
                              FALSE);

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default,
                           0);


   s_menu = TRUE;

   return 0;
}

/////////////////////////////////////////////////////////////////////

static SsdWidget create_results_container()
{
   SsdWidget rcnt = NULL;
   SsdWidget list = NULL;
   SsdWidget title = NULL;
   SsdWidget bitmap = NULL;
   SsdWidget text = NULL;

   rcnt = ssd_container_new(  LSD_RESULTS_CONT_NAME,
                              NULL,
                              SSD_MIN_SIZE,
                              SSD_MIN_SIZE,
                              0);
   ssd_widget_set_color(rcnt, NULL,NULL);


   title = ssd_container_new(  "Title box",
                   NULL,
                   SSD_MAX_SIZE,
                   SSD_MIN_SIZE,
                   SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);

   ssd_dialog_add_vspace(title, 5, 0);
   bitmap = ssd_bitmap_new("local search icon", "google_logo", SSD_ALIGN_VCENTER);
   ssd_dialog_add_hspace(title, 5, 0);
   ssd_widget_add(title, bitmap);
   ssd_dialog_add_hspace(title, 5, 0);
   text = ssd_text_new("Local search text", roadmap_lang_get("Local search results"), 14, SSD_ALIGN_VCENTER);
   ssd_widget_add(title, text);
   ssd_dialog_add_hspace(title, 5, 0);
   ssd_widget_add( rcnt, title);
   ssd_dialog_add_vspace(rcnt, 5, 0);
   list = ssd_list_new(       LSD_RC_LIST_NAME,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, 80);
   ssd_widget_add( rcnt, list);

   return rcnt;
}

static SsdWidget get_result_container()
{
   if( !s_result_container)
   {
      s_result_container = create_results_container();
   }
   return s_result_container;
}

void local_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   generic_search_dlg_show( search_local,
                             LSD_DIALOG_NAME,
                             LSD_DIALOG_TITLE,
                             on_options,
                             on_back,
                             get_result_container(),
                             cbOnClosed,
                             on_search,
                             local_search_dlg_show,
                             context);
}

/* Allows other windows to be closed */
static void search_progress_message_delayed(void)
{
	roadmap_main_remove_periodic( search_progress_message_delayed );
	if( s_searching )
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Searching . . . " ) );
}

/* Callback for the error message box */
static void on_search_error_message( int exit_code )
{
   generic_search_dlg_reopen_native_keyboard();
}



