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
#include "../roadmap_gps.h"
#include "../roadmap_geo_config.h"
#include "../roadmap_square.h"
#include "../roadmap_tile_manager.h"
#include "../roadmap_tile_status.h"
#include "../roadmap_map_download.h"
#include "../roadmap_reminder.h"
#include "address_search.h"
#include "generic_search_dlg.h"
#include "../roadmap_start.h"
#include "address_search_dlg.h"
#include "../roadmap_bar.h"
#include "../roadmap_device_events.h"
#include "tts/tts.h"
#include "navigate/navigate_main.h"
#include "roadmap_config.h"
#include "tts_was_provider.h"

#ifdef TOUCH_SCREEN
   #define  USE_ONSCREEN_KEYBOARD
#endif   // TOUCH_SCREEN

#ifdef   USE_ONSCREEN_KEYBOARD
   #include "../ssd/ssd_keyboard.h"
#endif   // USE_ONSCREEN_KEYBOARD

static   SsdWidget            s_result_container = NULL;
static   BOOL                 s_searching = FALSE;
static   BOOL                 s_menu   = FALSE;
static   BOOL                 s_auto_start_nav = FALSE;

#define  ASD_DIALOG_NAME            "address-search-dialog"
#define  ASD_DIALOG_TITLE           "New address"

#define  ASD_RESULTS_CONT_NAME      "address-search-dialog.results-container"
#define  ASD_RC_LIST_NAME           "address-search-dialog.results-container.list"
#define  ASD_RC_BACK_BTN_NAME       "address-search-dialog.results-container.back-btn"
#define  ASD_RC_BACK_BTN_LABEL      "Back"
#define  COULDNT_FIND_INDEX 1000
#define  COULDNT_FIND_ADDRESS_TEXT    "Couldn't find the address I wanted. Notify waze."

static void on_search_error_message( int exit_code );
static void search_progress_message_delayed(void);
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidanceType;

typedef enum tag_contextmenu_items
{
   cm_navigate,
   cm_show,
   cm_add_to_favorites,
   cm_cancel,
   cm_add_geo_reminder,
   cm__count,
   cm__invalid,
   cm_send
}  contextmenu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Drive",              cm_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cm_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cm_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Send",               cm_send),
   SSD_CM_INIT_ITEM  ( "Add Geo-Reminder",   cm_add_geo_reminder),
   SSD_CM_INIT_ITEM  ( "Cancel",             cm_cancel),
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);
static BOOL navigate( BOOL take_me_there);


static int on_options(SsdWidget widget, const char *new_value, void *context);
static int send_error_report(){
	roadmap_result rc;
	SsdWidget   edit  = generic_search_dlg_get_search_edit_box(search_address);
   const char* searched_text      = ssd_text_get_text( edit);
	rc= address_search_report_wrong_address(searched_text);
	if( succeeded == rc)
	{
	      roadmap_log(ROADMAP_DEBUG,
	                  "address_search_dialog::on_list_item_selected() - Succeeded in sending report");
    }
	else
    {
		  const char* err = roadmap_result_string( rc);
 		  roadmap_log(ROADMAP_ERROR,
              "address_search_dialog::on_list_item_selected() - Resolve process transaction failed to start: '%s'",
              err);
	}
	return 1;
}

static int on_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
   if( (new_value != NULL )&& (strcmp(new_value, roadmap_lang_get(COULDNT_FIND_ADDRESS_TEXT))==0))
   {
      send_error_report();
      generic_search_dlg_switch_gui();
   }
   else
   {
      on_options( NULL, NULL, NULL);
   }
   return 0;
}

static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results[ADSR_MAX_RESULTS+1];
   static   void*       indexes[ADSR_MAX_RESULTS+1];
   static   const char* icons[ADSR_MAX_RESULTS+1];

   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list;
   int       i;

   s_searching = FALSE;

   /* Close the progress message */
   ssd_progress_msg_dialog_hide();

   roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

   assert(list_cont);

   list = ssd_widget_get( list_cont, ASD_RC_LIST_NAME);

   if( succeeded != rc)
   {
      if( is_network_error( rc))
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                              roadmap_lang_get( "Search requires internet connection.\r\nPlease make sure you are connected."), on_search_error_message );



      else if( err_as_could_not_find_matches == rc)
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                              roadmap_lang_get( "Sorry, no results were found for this search"), on_search_error_message );
      else
      {
         char msg[128];

         snprintf( msg, sizeof(msg), "%s\n%s",roadmap_lang_get("Sorry we were unable to complete the search"), roadmap_lang_get("Please try again later"));

         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"), msg, on_search_error_message );
      }

      roadmap_log(ROADMAP_ERROR,
                  "address_search_dlg::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "address_search_dlg::on_address_resolved() - NO RESULTS for the address-resolve process");
      return;
   }

   assert( size <= ADSR_MAX_RESULTS);
   
   if (size == 1 && s_auto_start_nav) {
      s_auto_start_nav = FALSE;
      generic_search_dlg_switch_gui();
      ssd_dialog_hide_all( dec_close);
      navigate(1);
      return;
   }

   for( i=0; i<size; i++)
   {
      results[i] = array[i].address;
      indexes[i] = (void*)i;
      icons[i] = "search_address";
   }

   results[i] = roadmap_lang_get(COULDNT_FIND_ADDRESS_TEXT);
   indexes[i] = (void*)COULDNT_FIND_INDEX;
   icons[i] = "submit_logs";

   if ( roadmap_native_keyboard_enabled() )
   {
	   roadmap_native_keyboard_hide();
   }
   ssd_list_populate(list,
                     size+1,
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
   const char*		dl_prefix = roadmap_lang_get ("map:");
   SsdWidget dlg = generic_search_dlg_get_search_dlg(search_address);

   edit = generic_search_dlg_get_search_edit_box(search_address);
   if ( !strcmp( DEBUG_LEVEL_SET_PATTERN, ssd_text_get_text( edit ) ) )
   {
      roadmap_start_reset_debug_mode();
      return;
   }

   if ( !strcmp( "##@coord", ssd_text_get_text( edit ) ) )
   {
      roadmap_gps_reset_show_coordinates();
      return;
   }

   if ( !strcmp( "##@il", ssd_text_get_text( edit ) ) )
   {
      roadmap_geo_config_il(NULL);
      return;
   }
   if ( !strcmp( "##@usa", ssd_text_get_text( edit ) ) )
   {
      roadmap_geo_config_usa(NULL);
      return;
   }
   if ( !strcmp( "##@other", ssd_text_get_text( edit ) ) )
   {
      roadmap_geo_config_other(NULL);
      return;
   }
   if ( !strcmp( "##@stg", ssd_text_get_text( edit ) ) )
   {
      roadmap_geo_config_stg(NULL);
      return;
   }


    if ( !strcmp( "##@heb", ssd_text_get_text( edit ) ) )
   {
      roadmap_lang_set_system_lang("heb", TRUE);
      roadmap_messagebox("", "Language changed to Hebrew, please restart waze");
      return;
   }

   if ( !strcmp( "##@eng", ssd_text_get_text( edit ) ) )
   {
      roadmap_lang_set_system_lang("eng", TRUE);
      roadmap_messagebox("","Language changed to English, please restart waze");
      return;
   }
   if ( !strcmp( "cc@tts", ssd_text_get_text( edit ) ) )
   {
      tts_clear_cache();
      roadmap_messagebox("","TTS cache has been cleared!");
      return;
   }
   if ( !strcmp( "##@tts", ssd_text_get_text( edit ) ) )
   {
      tts_set_feature_enabled( !tts_feature_enabled() );
      if ( tts_feature_enabled() )
      {
         roadmap_messagebox("","TTS Feature is enabled!\nPlease restart WAZE.");
      }
      else
      {
         roadmap_messagebox("","TTS Feature is disabled!");

      }
      navigate_main_override_nav_settings();
      return;
   }
   if ( !strcmp( "dbg@tts", ssd_text_get_text( edit ) ) )
   {
      if ( !strcmp( tts_was_provider_voices_set(), TTS_WAS_VOICES_SET_PRODUCTION ) )
      {
         tts_was_provider_apply_voices_set( TTS_WAS_VOICES_SET_DEBUG );
         roadmap_messagebox("","TTS Feature is running in debug mode!\nPlease restart WAZE.");
      }
      else
      {
         tts_was_provider_apply_voices_set( TTS_WAS_VOICES_SET_PRODUCTION );
         roadmap_messagebox("","TTS Feature is running in production mode!\nPlease restart WAZE.");
      }
      return;
   }

   if ( !strncmp( dl_prefix, ssd_text_get_text( edit ), strlen( dl_prefix ) ) )
   {
   	roadmap_map_download_region( ssd_text_get_text( edit ) + strlen( dl_prefix ),
   										  roadmap_locator_static_county() );
      ssd_dialog_hide_all( dec_close);

      if (!roadmap_screen_refresh ())
           roadmap_screen_redraw();
   	return;
   }

   s_searching = TRUE;

   roadmap_main_set_periodic( 100, search_progress_message_delayed );

   text     = ssd_text_get_text( edit );
   list_cont=  dlg->context;

   rc = address_search_resolve_address( list_cont, on_address_resolved, text );
   if( succeeded == rc)
   {
      roadmap_main_set_cursor( ROADMAP_CURSOR_WAIT);
      roadmap_log(ROADMAP_DEBUG,
                  "address_search_dlg::on_search() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);
      s_searching = FALSE;
      roadmap_log(ROADMAP_ERROR,
                  "address_search_dlg::on_search() - Resolve process transaction failed to start");
      /* Close the progress message */
      ssd_progress_msg_dialog_hide();
      roadmap_messagebox_cb ( roadmap_lang_get( "Resolve Address"),
                           roadmap_lang_get( err ), on_search_error_message );
   }
}


static int get_selected_list_item()
{
   SsdWidget list;
   SsdWidget dlg;

   if( generic_search_dlg_is_1st(search_address))
      return -1;

   dlg = generic_search_dlg_get_search_dlg(search_address);
   list = ssd_widget_get( dlg, ASD_RC_LIST_NAME);
   assert(list);

   return (int)ssd_list_selected_value( list);
}



static BOOL navigate( BOOL take_me_there)
{
   return navigate_with_coordinates( take_me_there, search_address, get_selected_list_item());
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
                                                NULL,
                                                &position);
         break;
      }

      case cm_cancel:
         close = FALSE;
         break;

      case cm_add_geo_reminder:
      {
         int                        selected_list_item   = get_selected_list_item();
         const address_candidate*   selection            = generic_search_result( selected_list_item);
         RoadMapPosition position;
         position.latitude = (int)(selection->latitude*1000000);
         position.longitude= (int)(selection->longtitude*1000000);
         roadmap_reminder_add_at_position(&position, TRUE, TRUE);
         break;
      }
      case cm_send:
         send_error_report();
         generic_search_dlg_switch_gui();
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
   if( generic_search_dlg_is_1st(search_address))
      ssd_dialog_hide_current( dec_cancel);
   else
      generic_search_dlg_switch_gui();

   return 0;
}

int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   BOOL add_cancel = TRUE;
   BOOL b_report_wrong_address = ( get_selected_list_item()== COULDNT_FIND_INDEX ) ;
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
                              !generic_search_dlg_is_1st(search_address)&&(!b_report_wrong_address),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_show,
                              !generic_search_dlg_is_1st(search_address)&&(!b_report_wrong_address),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_to_favorites,

                              !generic_search_dlg_is_1st(search_address)&&(!b_report_wrong_address),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
							   cm_add_geo_reminder,
                              !generic_search_dlg_is_1st(search_address)&&(!b_report_wrong_address),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_geo_reminder,

                              !generic_search_dlg_is_1st(search_address)&&(!b_report_wrong_address)&&roadmap_reminder_feature_enabled(),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_send,
                              b_report_wrong_address,
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
                           0,
                           TRUE);


   s_menu = TRUE;

   return 0;
}

static SsdWidget create_results_container()
{
   SsdWidget rcnt = NULL;
   SsdWidget list = NULL;

   rcnt = ssd_container_new(  ASD_RESULTS_CONT_NAME,
                              NULL,
                              SSD_MIN_SIZE,
                              SSD_MIN_SIZE,
                              0);
   ssd_widget_set_color(rcnt, NULL,NULL);


   list = ssd_list_new(       ASD_RC_LIST_NAME,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              0,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, ssd_container_get_row_height());

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

static void on_auto_search_completed( int exit_code, void* context)
{ ssd_dialog_hide_all( dec_close);}


void address_search_dlg_show_auto( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   generic_search_dlg_show( search_address,
                            ASD_DIALOG_NAME,
                            ASD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            address_search_dlg_show,
                            context,TRUE);

}

BOOL address_search_auto_search( const char* address)
{
   SsdWidget edit    = NULL;

//   if( s_cb || s_ctx)
//   {
//      assert(0);  // Dialog is in use now
//      return FALSE;
//   }

   address_search_dlg_show_auto( on_auto_search_completed, (void*)address);

   edit  = generic_search_dlg_get_search_edit_box(search_address);

   ssd_text_set_text( edit, address);
   s_auto_start_nav = FALSE;
   on_search();

   return TRUE;
}

BOOL address_search_auto_nav( const char* address)
{
   SsdWidget edit    = NULL;

   address_search_dlg_show_auto( on_auto_search_completed, (void*)address);
   
   edit  = generic_search_dlg_get_search_edit_box(search_address);
   
   ssd_text_set_text( edit, address);
   s_auto_start_nav = TRUE;
   on_search();
   
   return TRUE;
}

void address_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   s_auto_start_nav = FALSE;
   generic_search_dlg_show( search_address,
                            ASD_DIALOG_NAME,
                            ASD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            address_search_dlg_show,
                            context,FALSE);

}

/* Allows other windows to be closed */
static void search_progress_message_delayed(void)
{
	roadmap_main_remove_periodic( search_progress_message_delayed );
	if( s_searching )
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Searching..." ) );
}

/* Callback for the error message box */
static void on_search_error_message( int exit_code )
{
   generic_search_dlg_reopen_native_keyboard();
}

