/*
 * LICENSE:
 *
 *   Copyright 2010 Avi Ben-Shoshan
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
#include "local_search.h"
#include "single_search.h"
#include "generic_search_dlg.h"
#include "../roadmap_start.h"
#include "single_search_dlg.h"
#include "../roadmap_bar.h"
#include "../roadmap_device_events.h"

#ifdef TOUCH_SCREEN
   #define  USE_ONSCREEN_KEYBOARD
#endif   // TOUCH_SCREEN

#ifdef   USE_ONSCREEN_KEYBOARD
   #include "../ssd/ssd_keyboard.h"
#endif   // USE_ONSCREEN_KEYBOARD

static   SsdWidget            s_result_container = NULL;
static   BOOL                 s_searching = FALSE;
static   BOOL                 s_menu   = FALSE;

#define  SSD_DIALOG_NAME            "single-search-dialog"
#define  SSD_DIALOG_TITLE           "Drive to"

#define  SSD_RESULTS_CONT_NAME      "single-search-dialog.results-container"
#define  SSD_RC_ADDRESSLIST_NAME    "single-search-dialog.results-container.list.address"
#define  SSD_RC_ADDRESSLIST_TITLE_NAME "single-search-dialog.results-container.adr.list.title_text"
#define  SSD_RC_LSLIST_NAME         "single-search-dialog.results-container.list.ls"
#define  SSD_RC_LSLIST_TITLE_NAME   "single-search-dialog.results-container.list.ls.title_text"
#define  SSD_RC_BACK_BTN_NAME       "single-search-dialog.results-container.back-btn"
#define  SSD_RC_BACK_BTN_LABEL      "Back"
#define  COULDNT_FIND_INDEX 1000
#define  COULDNT_FIND_ADDRESS_TEXT    "Couldn't find the address I wanted. Notify waze."

static int g_is_address = TRUE;
static void on_search_error_message( int exit_code );
static void search_progress_message_delayed(void);
static int on_addr_list_item_selected(SsdWidget widget, const char *new_value, const void *value);

typedef enum tag_contextmenu_items
{
   cm_navigate,
   cm_show,
   cm_add_to_favorites,
   cm_cancel,
   cm_add_geo_reminder,
   cm__count,
   cm__invalid
}  contextmenu_items;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Drive",              cm_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cm_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cm_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Add Geo-Reminder",   cm_add_geo_reminder),
   SSD_CM_INIT_ITEM  ( "Cancel",             cm_cancel),
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);



static int on_options(SsdWidget widget, const char *new_value, void *context);


static void update_list(SsdWidget list_cont){
   static   const char* results_adr[ADSR_MAX_RESULTS+1];
    static   void*       indexes_adr[ADSR_MAX_RESULTS+1];
    static   const char* icons_adr[ADSR_MAX_RESULTS+1];
    int      count_adr = 0;
    SsdWidget list_address;
    int       i;
    int size = generic_search_result_count();
    address_candidate*   array = generic_search_result(0);
    /* Close the progress message */
    ssd_progress_msg_dialog_hide();

    roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

    assert(list_cont);

    list_address = ssd_widget_get( list_cont, SSD_RC_ADDRESSLIST_NAME);

    for( i=0; i<size; i++)
    {
       if (array[i].type == ADDRESS_CANDIDATE_TYPE_ADRESS){
          results_adr[count_adr] = array[i].address;
          indexes_adr[count_adr] = (void*)i;
          icons_adr[count_adr] = "search_address";
          count_adr++;
       }
    }

    ssd_list_populate(list_address,
                      count_adr,
                      results_adr,
                      (const void **)indexes_adr,
                      icons_adr,
                      0,
                      on_addr_list_item_selected,
                      NULL,
                      FALSE);

    ssd_dialog_invalidate_tab_order();
    ssd_dialog_resort_tab_order();
    roadmap_screen_refresh();
}

static int on_addr_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
   if (value == -1){
      widget->in_focus = FALSE;
      widget->force_click = FALSE;
      update_list(widget);
      return 1;
   }
   g_is_address = TRUE;
   on_options( NULL, NULL, NULL);
   return 0;
}

static int on_ls_list_item_selected(SsdWidget widget, const char *new_value, const void *value)
{
   g_is_address = FALSE;
   on_options( NULL, NULL, NULL);
   return 0;
}
static void on_address_resolved( void*                context,
                                 address_candidate*   array,
                                 int                  size,
                                 roadmap_result       rc)
{
   static   const char* results_adr[ADSR_MAX_RESULTS+1];
   static   void*       indexes_adr[ADSR_MAX_RESULTS+1];
   static   const char* icons_adr[ADSR_MAX_RESULTS+1];
   static   const char* results_ls[ADSR_MAX_RESULTS+1];
   static   void*       indexes_ls[ADSR_MAX_RESULTS+1];
   static   const char* icons_ls[ADSR_MAX_RESULTS+1];
   int      count_adr = 0;
   int      count_ls = 0;
   const char* provider_icon = NULL;
   SsdWidget list_cont = (SsdWidget)context;
   SsdWidget list_address;
   SsdWidget list_ls;
   int       i;

   s_searching = FALSE;

   /* Close the progress message */
   ssd_progress_msg_dialog_hide();

   roadmap_main_set_cursor( ROADMAP_CURSOR_NORMAL);

   assert(list_cont);

   list_address = ssd_widget_get( list_cont, SSD_RC_ADDRESSLIST_NAME);
   list_ls = ssd_widget_get( list_cont, SSD_RC_LSLIST_NAME);

   if( succeeded != rc)
   {
      if( is_network_error( rc))
         roadmap_messagebox_cb ( roadmap_lang_get( "Oops"),
                              roadmap_lang_get( "Search requires internet connection.\nPlease make sure you are connected."), on_search_error_message );



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
                  "single_search_dlg::on_address_resolved() - Resolve process failed with error '%s' (%d)",
                  roadmap_result_string( rc), rc);
      return;
   }

   if( !size)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_dlg::on_address_resolved() - NO RESULTS for the address-resolve process");
      return;
   }

   assert( size <= ADSR_MAX_RESULTS);
   provider_icon = local_search_get_icon_name();
   for( i=0; i<size; i++)
   {
      if (array[i].type == ADDRESS_CANDIDATE_TYPE_ADRESS){
         results_adr[count_adr] = array[i].address;
         indexes_adr[count_adr] = (void*)i;
         icons_adr[count_adr] = "search_address";
         count_adr++;
      }
      else{
         results_ls[count_ls] = array[i].address;
         indexes_ls[count_ls] = (void*)i;
         icons_ls[count_ls] = provider_icon;
         count_ls++;
      }
   }

   if ( roadmap_native_keyboard_enabled() )
   {
      roadmap_native_keyboard_hide();
   }

   if ((count_adr > 3) && (count_ls > 0)){
      static char label[50];
      if (ssd_widget_rtl(NULL))
         snprintf(label, sizeof(label), "%s %d...", roadmap_lang_get("more"), count_adr - 3);
      else
         snprintf(label, sizeof(label), "%d %s...", count_adr - 3, roadmap_lang_get("more"));
      count_adr = 4;
      results_adr[count_adr-1] = label;
      indexes_adr[count_adr-1] = (void*)(-1);
      icons_adr[count_adr-1] = "search_address";

   }

   if (count_adr == 0){
      ssd_widget_hide(ssd_widget_get(list_cont,SSD_RC_ADDRESSLIST_TITLE_NAME));
   }
   else{
      ssd_widget_show(ssd_widget_get(list_cont,SSD_RC_ADDRESSLIST_TITLE_NAME));
   }

   ssd_list_populate(list_address,
                     count_adr,
                     results_adr,
                     (const void **)indexes_adr,
                     icons_adr,
                     0,
                     on_addr_list_item_selected,
                     NULL,
                     FALSE);

   if (count_ls == 0){
      ssd_widget_hide(ssd_widget_get(list_cont,SSD_RC_LSLIST_TITLE_NAME));
   }
   else{
      ssd_widget_show(ssd_widget_get(list_cont,SSD_RC_LSLIST_TITLE_NAME));
   }

   ssd_list_populate(list_ls,
                     count_ls,
                     results_ls,
                     (const void **)indexes_ls,
                     icons_ls,
                     0,
                     on_ls_list_item_selected,
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
   const char*    dl_prefix = roadmap_lang_get ("map:");
   SsdWidget dlg = generic_search_dlg_get_search_dlg(single_search);

   edit = generic_search_dlg_get_search_edit_box(single_search);
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

#ifdef __SYMBIAN32__
   if ( !strcmp( "##@opera", ssd_text_get_text( edit ) ) )
   {
      roadmap_browser_set_show_external(NULL);
      return;
   }
#endif
    if ( !strcmp( "##@heb", ssd_text_get_text( edit ) ) )
   {
      roadmap_lang_set_system_lang("heb");
      roadmap_messagebox("", "Language changed to Hebrew, please restart waze");
      return;
   }

   if ( !strcmp( "##@eng", ssd_text_get_text( edit ) ) )
   {
      roadmap_lang_set_system_lang("eng");
      roadmap_messagebox("","Language changed to English, please restart waze");
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

   rc = single_search_resolve_address( list_cont, on_address_resolved, text );
   if( succeeded == rc)
   {
      roadmap_main_set_cursor( ROADMAP_CURSOR_WAIT);
      roadmap_log(ROADMAP_DEBUG,
                  "single_search_dlg::on_search() - Started Web-Service transaction: Resolve address");
   }
   else
   {
      const char* err = roadmap_result_string( rc);
      s_searching = FALSE;
      roadmap_log(ROADMAP_ERROR,
                  "single_search_dlg::on_search() - Resolve process transaction failed to start");
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

   if( generic_search_dlg_is_1st(single_search))
      return -1;

   dlg = generic_search_dlg_get_search_dlg(single_search);
   if (g_is_address)
      list = ssd_widget_get( dlg, SSD_RC_ADDRESSLIST_NAME);
   else
      list = ssd_widget_get( dlg, SSD_RC_LSLIST_NAME);
   assert(list);

   return (int)ssd_list_selected_value( list);
}



static BOOL navigate( BOOL take_me_there)
{
   return navigate_with_coordinates( take_me_there, single_search, get_selected_list_item());
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
   if( generic_search_dlg_is_1st(single_search))
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
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_show,
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_to_favorites,

                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                        cm_add_geo_reminder,
                              !generic_search_dlg_is_1st(single_search),
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cm_add_geo_reminder,

                              !generic_search_dlg_is_1st(single_search)&&roadmap_reminder_feature_enabled(),
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
   SsdWidget text = NULL;

   rcnt = ssd_container_new(  SSD_RESULTS_CONT_NAME,
                              NULL,
                              SSD_MIN_SIZE,
                              SSD_MIN_SIZE,
                              0);
   ssd_widget_set_color(rcnt, NULL,NULL);


   ssd_dialog_add_vspace(rcnt,5,0);
   ssd_dialog_add_hspace(rcnt,5,0);
   text = ssd_text_new(SSD_RC_ADDRESSLIST_TITLE_NAME, roadmap_lang_get("New address"),16, SSD_END_ROW);
   ssd_widget_add( rcnt, text);


   list = ssd_list_new(       SSD_RC_ADDRESSLIST_NAME,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, 50);

   ssd_widget_add( rcnt, list);

   ssd_dialog_add_vspace(rcnt,5,0);
   ssd_dialog_add_hspace(rcnt,5,0);
   text = ssd_text_new(SSD_RC_LSLIST_TITLE_NAME, roadmap_lang_get(local_search_get_provider_label()),16, SSD_END_ROW);
   ssd_widget_add( rcnt, text);


   list = ssd_list_new(       SSD_RC_LSLIST_NAME,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              inputtype_free_text,
                              SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE,
                              NULL);
   //ssd_widget_set_color(list, NULL,NULL);
   ssd_list_resize( list, 50);

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

void single_search_dlg_show_auto( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_show,
                            context,
                            TRUE);

}

BOOL single_search_auto_search( const char* address)
{
   SsdWidget edit    = NULL;

   generic_search_dlg_update_text( address, single_search );

   single_search_dlg_show_auto( on_auto_search_completed, (void*)address);

   edit  = generic_search_dlg_get_search_edit_box(single_search);

   ssd_text_set_text( edit, address);

// AGA Don't start, just show to user. Perhaps some editing is necessary
#ifndef ANDROID
   on_search();
#endif

   return TRUE;
}

void single_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context)
{
   generic_search_dlg_show( single_search,
                            SSD_DIALOG_NAME,
                            SSD_DIALOG_TITLE,
                            on_options,
                            on_back,
                            get_result_container(),
                            cbOnClosed,
                            on_search,
                            single_search_dlg_show,
                            context,
                            FALSE);

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
