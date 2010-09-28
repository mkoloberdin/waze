/* roadmap_search.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2009 Avi R.
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
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_factory.h"
#include "roadmap_address_tc.h"
#include "roadmap_history.h"
#include "roadmap_search.h"
#include "roadmap_geocode.h"
#include "roadmap_screen.h"
#include "roadmap_plugin.h"
#include "roadmap_math.h"
#include "roadmap_display.h"
#include "roadmap_messagebox.h"
#include "roadmap_lang.h"
#include "roadmap_navigate.h"
#include "roadmap_address.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_street.h"
#include "roadmap_screen.h"
#include "roadmap_softkeys.h"

#include "ssd/ssd_widget.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_keyboard.h"

#include "navigate/navigate_main.h"

#include "iphone/roadmap_list_menu.h"
#include "roadmap_editbox.h"

#define MAX_CONTEXT_ENTRIES 10

typedef struct {
	char 	category;
    const char *title;
} RoadMapSearchContext;


typedef struct{
	RoadMapSearchContext *context;
	void *history;
}ContextmenuContext;

static   BOOL             g_context_menu_is_active= FALSE;

void roadmap_search_history (char category, const char *title);

static RoadMapAddressNav RoadMapAddressNavigate;


// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Navigate",           search_menu_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",           search_menu_show),
   SSD_CM_INIT_ITEM  ( "Delete",   			 search_menu_delete),
   SSD_CM_INIT_ITEM  ( "Add to favorites", 	 search_menu_add_to_favorites)
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);


static void hide_our_dialogs( int exit_code)
{
   	ssd_generic_list_dialog_hide ();
	ssd_menu_hide("Search");
    ssd_dialog_hide( "Main Menu", exit_code);
}

static void roadmap_address_done (RoadMapGeocode *selected, BOOL navigate, address_info_ptr ai) {

    PluginStreet street;
    PluginLine line;
    RoadMapPosition from;
    RoadMapPosition to;

    roadmap_locator_activate (selected->fips);

    roadmap_log (ROADMAP_DEBUG, "selected address at %d.%06d %c, %d.%06d %c",
                 abs(selected->position.longitude)/1000000,
                 abs(selected->position.longitude)%1000000,
                 selected->position.longitude >= 0 ? 'E' : 'W',
                 abs(selected->position.latitude)/1000000,
                 abs(selected->position.latitude)%1000000,
                 selected->position.latitude >= 0 ? 'N' : 'S');

	 roadmap_math_adjust_zoom (selected->square);

    roadmap_plugin_set_line
       (&line, ROADMAP_PLUGIN_ID, selected->line, -1, selected->square, selected->fips);

    roadmap_trip_set_point ("Selection", &selected->position);
    roadmap_trip_set_point ("Address", &selected->position);

    if (!navigate || !RoadMapAddressNavigate) {

       roadmap_trip_set_focus ("Address");

       roadmap_display_activate
          ("Selected Street", &line, &selected->position, &street);

	   roadmap_street_extend_line_ends (&line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
	   roadmap_display_update_points ("Selected Street", &from, &to);
	   roadmap_screen_add_focus_on_me_softkey();
       roadmap_screen_refresh ();
    } else {
    	navigate_main_stop_navigation();
       if ((*RoadMapAddressNavigate) (&selected->position, &line, 0, ai) != -1) {
       }
    }
}

static int roadmap_address_show (const char *city,
                                 const char *street_name,
                                 const char *street_number_image,
                                 RoadMapSearchContext *context,
                                 BOOL navigate) {

   int i;
   int count;
   RoadMapGeocode *selections;
   char *state;
   const char *argv[4];
   address_info   ai;

   ai.state = NULL;
   ai.country = NULL;
   ai.city =city;
   ai.street = street_name;
   ai.house = street_number_image;

   state         = "IL";

   count = roadmap_geocode_address (&selections,
                                    street_number_image,
                                    street_name,
                                    city,
                                    state);
   if (count <= 0) {
      roadmap_messagebox (roadmap_lang_get ("Warning"),
                          roadmap_geocode_last_error_string());
      free (selections);
      return 0;
   }

   if(context->category == 'A'){
   	argv[0] = street_number_image;
   	argv[1] = street_name;
   	argv[2] = city;
   	argv[3] = state;

   	roadmap_history_add ('A', argv);
   }

   roadmap_address_done (selections, navigate, &ai);

   for (i = 0; i < count; ++i) {
      free (selections[i].name);
      selections[i].name = NULL;
   }

   free (selections);

   return 1;
}


static int on_navigate(void *data){
	char *city_name;
    char *street_name;
    char *street_number;
    char *argv[5];

	ContextmenuContext *context = (ContextmenuContext *)data;

	roadmap_main_show_root(0);

    roadmap_history_get (context->context->category, (void *) context->history, argv);

  	city_name = strdup (argv[2]);
   	street_name = strdup (argv[1]);
   	street_number = strdup (argv[0]);


	if (roadmap_address_show(city_name, street_name, street_number,context->context, TRUE)){
		//hide_our_dialogs(dec_close);
	}

	return TRUE;
}


static int on_show(void *data){
	char *city_name;
    char *street_name;
    char *street_number;
	ContextmenuContext *context = (ContextmenuContext *)data;

	/* We got a full address */
    char *argv[5];

    roadmap_history_get (context->context->category, (void *) context->history, argv);

  	city_name = strdup (argv[2]);
   	street_name = strdup (argv[1]);
   	street_number = strdup (argv[0]);


	if (roadmap_address_show(city_name, street_name, street_number,context->context, FALSE)){
		hide_our_dialogs(dec_close);
	}

	return TRUE;
}


static void on_erase_history_item(int exit_code, void *data){

	ContextmenuContext *context = (ContextmenuContext *)data;

	if( dec_yes != exit_code)
      return;

	roadmap_history_delete_entry(context->history);
	roadmap_history_save();
	ssd_generic_list_dialog_hide ();
	roadmap_search_history (context->context->category, context->context->title);

}

static void on_delete(void *data){
	//const char* selection= ssd_generic_list_dialog_selected_string ();
	const char* selection = "Confirm"; //TODO: fix this
	roadmap_main_show_root(0); //TODO: remove this when confirm dialog is ready
	char string[100];
	ContextmenuContext *context = (ContextmenuContext *)data;

	if (context->context->category == 'A')
		strcpy(string, "Are you sure you want to remove item from history?");
	else
		strcpy(string, "Are you sure you want to remove item from favorites?");

	ssd_confirm_dialog( selection,
                        string,
                        FALSE,
                        on_erase_history_item,
                        (void*)data);

}


static int keyboard_callback(int type, const char *new_value, void *data)
{
	char *argv[5];
	char empty[250];
	ContextmenuContext *context = (ContextmenuContext *)data;;


    if (type != SSD_KEYBOARD_OK)
        return 1;

	roadmap_history_get (context->context->category, (void *) context->history, argv);

	if (new_value[0] == 0){
		sprintf(empty, "%s %s %s",  argv[1], argv[0], argv[2]);
		new_value = empty;
	}

    argv[4] = (char *)new_value;

	roadmap_history_add ('F', (const char **)argv);
	roadmap_history_save();
    ssd_keyboard_hide();

	roadmap_main_show_root(0);

    return 1;

}

static void add_to_favorites(int exit_code, void *data){

	if( dec_yes != exit_code)
      return;
	#if defined(__SYMBIAN32__) || defined(IPHONE) || defined(ANDROID)
    	ShowEditbox(roadmap_lang_get("Name"), "",
            keyboard_callback, (void *)data, EEditBoxStandard );
	#else
  	  ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
            roadmap_lang_get("Name"), "", NULL, keyboard_callback, (void *)data);
	#endif

}

static void on_add_to_favorites(void *data){
	//const char* selection= ssd_generic_list_dialog_selected_string ();
	const char* selection = "Confirm"; //TODO: fix this
	roadmap_main_show_root(0); //TODO: remove this when confirm dialog is ready
	ssd_confirm_dialog( selection,
                        "Are you sure you want to Add item to favorites?",
                        FALSE,
                        add_to_favorites,
                        (void*)data);

}


static int on_option_selected (SsdWidget widget, const char *new_value, const void *value,
							 void *context) {

   search_menu_context_menu_items   selection;
   int                  exit_code = dec_ok;
   BOOL hide = FALSE;

	ssd_cm_item_ptr item = (ssd_cm_item_ptr)value;

   g_context_menu_is_active       = FALSE;

  // if( !made_selection)
    //  return;

   selection = (search_menu_context_menu_items)item->id;


   switch( selection)
   {
	  case search_menu_navigate:
	  	on_navigate(context);
	    break;

	  case search_menu_show:
	    on_show(context);
	    break;

	  case search_menu_delete:
	  	on_delete(context);
	  	break;

	  case search_menu_add_to_favorites:
	  	on_add_to_favorites(context);
	    break;

	  case search_menu_exit:
	  	hide = TRUE;
	  	break;

	  case search_menu_cancel:
	  	g_context_menu_is_active = FALSE;
	  	roadmap_screen_refresh ();
	  	break;

      default:
        break;
   }

   if (hide){
   	  ssd_dialog_hide_all( exit_code);
      roadmap_screen_refresh ();
   }

	return TRUE;
}

void get_context_item (search_menu_context_menu_items id, ssd_cm_item_ptr *item){
	int i;

	for( i=0; i<context_menu.item_count; i++)
	{
		*item = context_menu.item + i;
		if ((*item)->id == id) {
			return;
		}
	}

	//NSLog(@"not item found");
	//TODO: handle wrong id
}

static int on_options(SsdWidget widget, const char *new_value, void *search_context){

	static ContextmenuContext menu_context;
	static char *labels[MAX_CONTEXT_ENTRIES];
	static void *values[MAX_CONTEXT_ENTRIES];
	static char *icons[MAX_CONTEXT_ENTRIES];
	ssd_cm_item_ptr item;

	int count = 0;

	menu_context.history = (void *)new_value;
	menu_context.context = search_context;

	get_context_item (search_menu_delete, &item);
	values[count] = item;
	labels[count] = strdup(item->label);
	//values[count] = NULL;
	icons[count] = NULL;
	count++;

	if (menu_context.context->category == 'A'){
		get_context_item (search_menu_add_to_favorites, &item);
		values[count] = item;
		labels[count] = strdup(item->label);
		//values[count] = NULL;
		icons[count] = NULL;
		count++;
	}

	roadmap_list_menu_generic (new_value,
							   count,
							   (const char **)labels,
							   (const void **)values,
							   (const char **)icons,
							   NULL,
							   on_option_selected,
							   NULL,
							   &menu_context,
							   NULL,
							   NULL, 60,0,FALSE);

	return 0;
	/*

   int menu_x;
   static ContextmenuContext menu_context;
   BOOL can_navigate;
   BOOL can_add_to_favorites;
   BOOL can_delete;
   BOOL add_exit = TRUE;
   BOOL add_cancel = FALSE;

#ifdef TOUCH_SCREEN
   add_exit = FALSE;
   add_cancel = TRUE;
   roadmap_screen_refresh();
#endif

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      g_context_menu_is_active = FALSE;
      return 0;
   }


   if (widget != NULL){
         const char* selection= ssd_generic_list_dialog_selected_string ();
         const void* value    = ssd_generic_list_dialog_selected_value  ();
         const void* list_context = ssd_generic_list_dialog_get_context();

         if( selection && (*selection))
         {
            context = (void *)list_context;
            new_value = (char *)value;
         }
         else{
#ifdef TOUCH_SCREEN
			return 0;
#endif
         }
   }

   menu_context.history = (void *)new_value;
   menu_context.context = context;
   can_navigate = FALSE;
   can_add_to_favorites = FALSE;
   can_delete = FALSE;


   if (strcmp(new_value, "generic_list")){
		  #ifndef TOUCH_SCREEN
   	         can_navigate = TRUE;
   	      #endif
   	      can_delete = TRUE;
	      if (menu_context.context->category == 'A'){
	   	   can_add_to_favorites = TRUE;
	      }
   }

   ssd_contextmenu_show_item( &context_menu,
                              search_menu_navigate,
                              can_navigate,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              search_menu_show,
                              can_navigate,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              search_menu_delete,
                              can_delete,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              search_menu_add_to_favorites,
                              can_add_to_favorites,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              search_menu_exit,
                              add_exit,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              search_menu_cancel,
                              add_cancel,
                              FALSE);

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,   // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           &menu_context,
                           dir_default);

   g_context_menu_is_active = TRUE;

   return 0;*/
}

static int history_callback (SsdWidget widget, const char *new_value, const void *value,
										    void *data) {

#ifdef TOUCH_SCREEN
    static ContextmenuContext menu_context;
    menu_context.history = (void *)value;
    menu_context.context = data;
  	on_navigate(&menu_context);
   	return 0;
#endif

	on_options(NULL, value, data);

	return TRUE;
}


void roadmap_search_history (char category, const char *title) {

#define MAX_HISTORY_ENTRIES 100
   static RoadMapSearchContext search_context;


   static char *labels[MAX_HISTORY_ENTRIES];
   static void *values[MAX_HISTORY_ENTRIES];
   static char *icons[MAX_HISTORY_ENTRIES];
   static int count = -1;
   void *history;
   void *prev;




   search_context.category = category;
   search_context.title = strdup(title);

   if (count == -1){
      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
      roadmap_history_declare( ADDRESS_FAVORITE_CATEGORY, ahi__count);
   }

   count = 0;

   	history = roadmap_history_latest (category);

   while (history && (count < MAX_HISTORY_ENTRIES)) {
      char *argv[5];
      char str[100];

      roadmap_history_get (category, history, argv);
      prev = history;

	  if (category == 'A'){
      	snprintf (str, sizeof(str), "%s %s, %s", argv[1], argv[0], argv[2]);
      	icons[count] = "history";
	  }
      else{
      	snprintf (str, sizeof(str), "%s", argv[4]);
      	icons[count] = "favorite";
      }
      if (labels[count]) free (labels[count]);
      labels[count] = strdup(str);

      values[count] = history;


      count++;

      history = roadmap_history_before (category, history);
      if (history == prev) break;
   }


   roadmap_list_menu_generic (roadmap_lang_get (title),
                  count,
                  (const char **)labels,
                  (const void **)values,
                  (const char **)icons,
                  NULL,
                  history_callback,
                  NULL,
                  &search_context,
                  roadmap_lang_get("Options"),
            	  on_options, 60,0,TRUE);
}



void search_menu_search_history(void){
	roadmap_search_history ('A', "History");
}

void search_menu_search_favorites(void){
	roadmap_search_history ('F', "Favorites");
}

void search_menu_search_address(void){
	#ifdef  TOUCH_SCREEN
		roadmap_address_history();
	#else
		address_tabcontrol_show( NULL);
	#endif
}


extern ssd_contextmenu_ptr    s_main_menu;
extern const char*            grid_menu_labels[];
extern RoadMapAction          RoadMapStartActions[];
BOOL get_menu_item_names(  const char*          menu_name,
                           ssd_contextmenu_ptr  parent,
                           const char*          items[],
                           int*                 count);



void roadmap_search_menu(void){


	roadmap_list_menu_simple ("Search",
								"search",
								NULL,
								NULL,
								RoadMapStartActions,
								SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_TRANSPARENT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY);

}



void roadmap_search_register_nav (RoadMapAddressNav navigate) {
   RoadMapAddressNavigate = navigate;
}
