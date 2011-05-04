/* roadmap_search.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
#include "roadmap_start.h"
#include "roadmap_screen.h"
#include "roadmap_softkeys.h"
#include "roadmap_layer.h"
#include "roadmap_reminder.h"
#include "roadmap_tripserver.h"

#include "roadmap_analytics.h"

#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_list.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_separator.h"
#include "Realtime/Realtime.h"

#include "navigate/navigate_main.h"
#include "address_search/single_search_dlg.h"
#include "address_search/address_search_dlg.h"
#include "address_search/local_search_dlg.h"
#include "address_search/local_search.h"
#include "Realtime/RealtimeAltRoutes.h"

#include "roadmap_editbox.h"

#ifdef ANDROID
#include "roadmap_androidmain.h"
#endif

#ifdef IPHONE
#include "iphone/roadmap_list_menu.h"

#define MAX_CONTEXT_ENTRIES 10
#endif //IPHONE

typedef struct {
   char    category;
   const char *title;
} RoadMapSearchContext;


#define SEARCH_MENU_DLG_NAME 		"Drive to"
static RMNativeKBParams s_gNativeKBParams = {  _native_kb_type_default, 1, _native_kb_action_search };
typedef struct{
   RoadMapSearchContext *context;
   void *history;
}ContextmenuContext;

extern int main_navigator( const RoadMapPosition *point,
                           address_info_ptr       ai);

static   BOOL              s_context_menu_is_active   = FALSE;
static   BOOL              s_address_search_is_active = FALSE;
static   BOOL              s_poi_search_is_active = FALSE;


void roadmap_search_history (char category, const char *title);


// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,        Item-ID
   SSD_CM_INIT_ITEM  ( "Drive",           search_menu_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",     search_menu_show),
   SSD_CM_INIT_ITEM  ( "Delete",          search_menu_delete),
   SSD_CM_INIT_ITEM  ( "Add to favorites",search_menu_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Add Geo-Reminder",search_menu_add_geo_reminder),
   SSD_CM_INIT_ITEM  ( "Cancel",          search_menu_cancel)
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);

#ifndef IPHONE
static void add_home_work_dlg(int iType);
#else
extern void add_home_work_dlg(int iType);
#endif
static g_favorite_name;

static void hide_our_dialogs( int exit_code)
{
   ssd_generic_list_dialog_hide ();
   ssd_menu_hide("Drive to");
   ssd_dialog_hide( "Main Menu", exit_code);
}

static char const *PopupMenuItems[] = {

   "setasdestination",
   "setasdeparture",
   NULL,
};

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_next (SsdWidget widget, const char *new_value){
   ssd_dialog_hide_current(dec_close);
   roadmap_start_popup_menu ("Show Address menu", PopupMenuItems,
                             NULL, NULL, DIALOG_ANIMATION_FROM_RIGHT);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void show_address_popup(int category, address_info_ptr ai, RoadMapPosition position){
   int number;
   int layers[128];
   int layers_count;
   RoadMapNeighbour neighbours[2];
   PluginStreetProperties properties;
   char street_name[250];
   char city_name[250];
   RoadMapPosition context_save_pos;
   zoom_t context_save_zoom;
   const char *icon = "marked_location";

   roadmap_math_get_context (&context_save_pos, &context_save_zoom);

   if (!*ai->city && !*ai->city && !*ai->house){
      layers_count = roadmap_layer_all_roads(layers, 128);
       roadmap_math_set_context(&position, 20);
       number = roadmap_street_get_closest(&position, 0, layers, layers_count,
                         1, &neighbours[0], 1);
       roadmap_math_set_context (&context_save_pos, 2);
       if (number <= 0)
          return;
       roadmap_plugin_get_street_properties (&neighbours[0].line, &properties, 0);

       if (ssd_widget_rtl(NULL)){
             snprintf (street_name, sizeof(street_name), "%s %s", properties.street, properties.address);
       }else{
                 snprintf (street_name, sizeof(street_name), "%s %s", properties.address, properties.street);
       }
       strcpy(city_name, properties.city);
   }
   else{

      if (ssd_widget_rtl(NULL)){
         snprintf (street_name, sizeof(street_name), "%s %s", ai->street, ai->house);
      }else{
         snprintf (street_name, sizeof(street_name), "%s %s", ai->house, ai->street);
      }
      strcpy(city_name, ai->city);
   }

   if(category == 'F'){
      if (!strcmp(ai->name, roadmap_lang_get("Home")) || !strcmp(ai->name,"home") || !strcmp(ai->name,"Home")) {
         icon = "home";
      }else if (!strcmp(ai->name, roadmap_lang_get("Work")) || !strcmp(ai->name,"office") || !strcmp(ai->name,"work") || !strcmp(ai->name,"Work")){
         icon = "work";
      }else
         icon = "search_favorites";
   }
   else if (category == 'A'){
      if (ai->name && *ai->name){
         icon = local_search_get_icon_name();
         strncat(street_name,",", sizeof(street_name));
         strncat(street_name,city_name,sizeof(street_name));
         strcpy(city_name, ai->name);
      }
      else
         icon = "search_history";
   }


   ssd_popup_show_float("SelectDlg",
                             city_name,
                             NULL,
                             street_name,
                             NULL,
                             icon,
                             &position,
                             ADJ_SCALE(-18),
                             NULL,
                             on_next,
                             "Options",
                             NULL);

}

static void roadmap_address_done (int category, RoadMapGeocode *selected, BOOL navigate, address_info_ptr ai, BOOL show_selected_street) {

    PluginStreet street;
    PluginLine line;
    RoadMapPosition from;
    RoadMapPosition to;

    if (selected->fips != -1)
       roadmap_locator_activate (selected->fips);

    roadmap_log (ROADMAP_DEBUG, "selected address at %d.%06d %c, %d.%06d %c",
                 abs(selected->position.longitude)/1000000,
                 abs(selected->position.longitude)%1000000,
                 selected->position.longitude >= 0 ? 'E' : 'W',
                 abs(selected->position.latitude)/1000000,
                 abs(selected->position.latitude)%1000000,
                 selected->position.latitude >= 0 ? 'N' : 'S');

    if (selected->square != -1)
       roadmap_math_adjust_zoom (selected->square);
    else
       roadmap_math_zoom_set (90);
    roadmap_layer_adjust();

    if ((selected->square != -1) && (selected->line != -1))
       roadmap_plugin_set_line
          (&line, ROADMAP_PLUGIN_ID, selected->line, -1, selected->square, selected->fips);

    roadmap_trip_set_point ("Selection", &selected->position);
    roadmap_trip_set_point ("Address", &selected->position);

    if (!navigate) {
//       roadmap_trip_set_animation("Selection");
       roadmap_trip_set_focus ("Selection");
       show_address_popup(category,ai, selected->position);
//       if ((selected->line != -1) && (show_selected_street)){
//
//          roadmap_display_activate
//             ("Selected Street", &line, &selected->position, &street);
//
//          roadmap_street_extend_line_ends (&line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
//          roadmap_display_update_points ("Selected Street", &from, &to);
//       }
      roadmap_screen_add_focus_on_me_softkey();
      roadmap_screen_refresh ();
    } else {
       navigate_main_stop_navigation();
       if (main_navigator(&selected->position, ai) != -1) {
       }
    }
}

#define  ADDRESS_STRING_MAX_SIZE    (112)
static BOOL create_address_string(  char*       address,
                                    const char* city,
                                    const char* street,
                                    const char* house)
{
   if( street && house && city && (*street) && (*house) && (*city))
      snprintf(address,
               ADDRESS_STRING_MAX_SIZE,
               "%s %s, %s", street, house, city);
   else if( street && city && (*street) && (*city))
      snprintf(address,
               ADDRESS_STRING_MAX_SIZE,
               "%s, %s", street, city);
   else if( city && (*city))
      snprintf(address,
               ADDRESS_STRING_MAX_SIZE,
               "%s", city);
   else if( street && (*street))
      snprintf(address,
               ADDRESS_STRING_MAX_SIZE,
               "%s, ", street);
   else
      return FALSE;

   return TRUE;
}


static int roadmap_address_show (const char        *name,
                                 const char 			*city,
                                 const char 			*street_name,
                                 const char 			*street_number_image,
                                 const char        *state,
                                 RoadMapPosition 	*position,
                                 RoadMapSearchContext *context,
                                 BOOL navigate) {

   int i;
   int count;
   RoadMapGeocode *selections = NULL;
   const char *argv[ahi__count];
   address_info   ai;
   BOOL show_selected_str = TRUE;

   ai.name = name;
   ai.state = state;
   ai.country = NULL;
   ai.city =city;
   ai.street = street_name;
   ai.house = street_number_image;
   if ((position != NULL) && (position->latitude != 0) && (position->longitude != 0)){
      count = 0;
   }
   else{
      count = roadmap_geocode_address (&selections,
													street_number_image,
													street_name,
													city,
													state);
   }

   if (count <= 0) {

  	   if ((position != NULL) && (position->latitude != 0) && (position->longitude != 0)){
  	      int number;
  	      int layers[128];
  	      int layers_count;
  	      RoadMapNeighbour neighbours[2];

  	      count = 1;
  	      selections = (RoadMapGeocode *)
  	          calloc (count, sizeof(RoadMapGeocode));

  	      layers_count = roadmap_layer_all_roads(layers, 128);
  	      roadmap_math_set_context((RoadMapPosition *)position, 20);
  	      number = roadmap_street_get_closest(position, 0, layers, layers_count, 1,
  	                  &neighbours[0], 1);
  	      if (number > 0) {
  	               RoadMapStreetProperties properties;

  	               selections->position.latitude = position->latitude;
  	               selections->position.longitude = position->longitude;

  	               roadmap_square_set_current(neighbours[0].line.square);
  	               roadmap_street_get_properties (neighbours[0].line.line_id, &properties);
  	               selections->fips = neighbours[0].line.fips;
  	               selections->square = neighbours[0].line.square; // roadmap_square_active ();
  	               selections->line = neighbours[0].line.line_id;
  	               selections->name = strdup (roadmap_street_get_full_name (&properties));
  	               roadmap_layer_adjust();
  	      }
  	      else{
          selections->position.latitude = position->latitude;
          selections->position.longitude = position->longitude;
  	       selections->fips = -1;
  	       selections->square = -1;
  	       selections->line = -1;
  	       selections->name = strdup (street_name);
  	      }
  	   }
  	   else{
  	          static char full_address[ADDRESS_STRING_MAX_SIZE+1];
  	          full_address[0] = 0;
  	          if( create_address_string( full_address, city, street_name, street_number_image))
  	          {
  	             if( !address_search_auto_search( full_address))
  	             {
  	                roadmap_log(ROADMAP_ERROR,
  	                      "roadmap_address_show() - Failed to run 'address_search_auto_search()'");
  	             }
  	          }
  	          else
  	          {
  	             roadmap_log(ROADMAP_ERROR,
  	                      "roadmap_address_show() - Failed to create a valid address string from history entries");
  	          }

  	          if( selections)
  	             free( selections);

  	          return 0;
  	      }
   }

   if(context->category == 'A'){
      argv[0] = street_number_image;
      argv[1] = street_name;
      argv[2] = city;
      argv[3] = state;
	   argv[4] = "";
	   if (position != NULL){
	      char temp[20];
	      sprintf(temp, "%d", position->latitude);
	      argv[5] = strdup(temp);
	      sprintf(temp, "%d", position->longitude);
	      argv[6] = strdup(temp);

	   }
	   argv[ahi_synced] = "false";
      roadmap_history_add ('A', argv);
      free((void *)argv[5]);
      free((void *)argv[6]);
   }

   if(context->category == 'S'){
      show_selected_str = FALSE;
   }


   roadmap_address_done (context->category, selections, navigate, &ai, show_selected_str);

   for (i = 0; i < count; ++i) {
      free (selections[i].name);
      selections[i].name = NULL;
   }

   free (selections);

   return 1;
}


static int on_navigate(void *data){
   char *city_name;
   char *name;
   char *street_name;
   char *street_number;
   char *argv[reminder_hi__count];
   char *state;
   RoadMapPosition *position = NULL;

   ContextmenuContext *context = (ContextmenuContext *)data;;


   roadmap_history_get (context->context->category, (void *) context->history, argv);

   city_name = strdup (argv[2]);
   street_name = strdup (argv[1]);
   street_number = strdup (argv[0]);
   state = strdup (argv[3]);
   name = strdup(argv[ahi_name]);
   position = (RoadMapPosition *)
  	       calloc (1, sizeof(RoadMapPosition));
   position->latitude = atoi(argv[5]);
   position->longitude = atoi(argv[6]);

   if (roadmap_address_show(name, city_name, street_name, street_number, state, position, context->context, TRUE)){
      hide_our_dialogs(dec_close);
   }

   free(position);
   free(city_name);
   free(street_name);
   free(street_number);
   free(state);
   return TRUE;
}


static int on_show(void *data){
   char *name;
   char *city_name;
   char *street_name;
   char *street_number;
   char *state;

#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE

    ContextmenuContext *context = (ContextmenuContext *)data;
    RoadMapPosition *position = NULL;

    /* We got a full address */
    char *argv[reminder_hi__count];

    roadmap_history_get (context->context->category, (void *) context->history, argv);

    city_name = strdup (argv[2]);
    street_name = strdup (argv[1]);
    street_number = strdup (argv[0]);
    state = strdup (argv[3]);
    name = strdup(argv[ahi_name]);
    position = (RoadMapPosition *)  calloc (1, sizeof(RoadMapPosition));
    position->latitude = atoi(argv[5]);
    position->longitude = atoi(argv[6]);

    if (roadmap_address_show(name, city_name, street_name, street_number, state, position, context->context, FALSE)){
      hide_our_dialogs(dec_close);
    }

    free(position);
    free(city_name);
    free(street_name);
    free(street_number);
    free(state);
    return TRUE;
}


static void on_erase_history_item(int exit_code, void *data){

   ContextmenuContext *context = (ContextmenuContext *)data;

   if( dec_yes != exit_code)
      return;

   if (context->context->category == 'F'){
#ifndef IPHONE
		const char* selection= ssd_generic_list_dialog_selected_string ();
#else
      char *argv[reminder_hi__count];
      roadmap_history_get ('S', (void *)context->history, argv);
      const char* selection= argv[4];
#endif //IPHONE
   	Realtime_TripServer_DeletePOI(selection);
   }
   else if (context->context->category == 'S'){
         char *argv[reminder_hi__count];
         roadmap_history_get ('S', (void *)context->history, argv);
         if (!strcmp(argv[reminder_hi_add_reminder],"1"))
            roadmap_reminder_delete(context->history);
   }

   roadmap_history_delete_entry(context->history);
   roadmap_history_save();
   ssd_generic_list_dialog_hide ();
   roadmap_search_history (context->context->category, context->context->title);

}

static void on_delete(void *data){
   char string[100];
   ContextmenuContext *context = (ContextmenuContext *)data;
#ifndef IPHONE
   const char* selection= ssd_generic_list_dialog_selected_string ();
#else
   char *argv[reminder_hi__count];
   char selection[100];

   roadmap_history_get (context->context->category,context->history, argv);

   if (context->context->category == 'S'){
      snprintf (selection, sizeof(selection), "%s %s, %s", argv[1], argv[0], argv[2]);
   }
   else if (context->context->category == 'A'){
      if (ssd_widget_rtl(NULL))
         snprintf (selection, sizeof(selection), "%s %s, %s", argv[1], argv[0], argv[2]);
      else
         snprintf (selection, sizeof(selection), "%s %s, %s", argv[0], argv[1], argv[2]);
   }
   else{
      snprintf (selection, sizeof(selection), "%s", argv[4]);
   }
	roadmap_main_show_root(0); //TODO: remove this when confirm dialog is ready
#endif //IPHONE


   if (context->context->category == 'S')
      strcpy(string, "Are you sure you want to remove saved location?");
   else if (context->context->category == 'A')
      strcpy(string, "Are you sure you want to remove item from history?");
   else
      strcpy(string, "Are you sure you want to remove item from favorites?");

   ssd_confirm_dialog( selection,
                        string,
                        FALSE,
                        on_erase_history_item,
                        (void*)data);

}

static BOOL keyboard_callback(  int         exit_code,
                           const char* value,
                           void*       context)
{
   char *argv[reminder_hi__count];
   char empty[250];
   RoadMapPosition coordinates;
   ContextmenuContext *ctx = context;

   if( dec_ok != exit_code)
        return TRUE;

   roadmap_history_get (ctx->context->category, (void *) ctx->history, argv);

   if (value[0] == 0){
      sprintf(empty, "%s %s %s",  argv[1], argv[0], argv[2]);
      value = empty;
   }
   else{
      coordinates.latitude = atoi(argv[5]);
      coordinates.longitude = atoi(argv[6]);
   }
    argv[4] = (char *)value;
   argv[ahi_synced] = "false";
   roadmap_history_add ('F', (const char **)argv);
   roadmap_history_save();

   if (value[0] != 0)
      roadmap_trip_server_create_poi(value, &coordinates, TRUE);

#ifdef IPHONE
	roadmap_main_show_root(0);
#endif //IPHONE

	return TRUE;
}


static void on_add_to_favorites(void *data){

   #if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE) || defined(ANDROID)
      ShowEditbox(roadmap_lang_get("Name"), "",
          keyboard_callback, (void *)data, EEditBoxStandard | EEditBoxAlphaNumeric );
    #else
      ssd_show_keyboard_dialog(  roadmap_lang_get( "Save address as:"),
                            NULL,
                            keyboard_callback,
                            data);
    #endif
}


static void on_add_to_geo_reminder(void *data){
   RoadMapPosition position;
   char *argv[reminder_hi__count];
   ContextmenuContext *ctx = data;
   roadmap_history_get (ctx->context->category, (void *) ctx->history, argv);
   position.latitude = atoi(argv[5]);
   position.longitude = atoi(argv[6]);

   roadmap_reminder_add_at_position(&position, TRUE, TRUE);
}

#ifndef IPHONE
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   search_menu_context_menu_items   selection;
   int                  exit_code = dec_ok;
   BOOL hide = FALSE;

   s_context_menu_is_active       = FALSE;

   if( !made_selection)
      return;
#else
static int on_option_selected (SsdWidget widget, const char *new_value, const void *value,
								   void *context) {

		search_menu_context_menu_items   selection;
		int                  exit_code = dec_ok;
		BOOL hide = FALSE;

		ssd_cm_item_ptr item = (ssd_cm_item_ptr)value;
#endif //IPHONE

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

     case search_menu_add_geo_reminder:
        on_add_to_geo_reminder(context);

     case search_menu_cancel:
        s_context_menu_is_active = FALSE;
        roadmap_screen_refresh ();
        break;

      default:
        break;
   }

   if (hide){
        ssd_dialog_hide_all( exit_code);
      roadmap_screen_refresh ();
   }

#ifdef IPHONE
	return TRUE;
#endif //IPHONE
}

#ifdef IPHONE
static void get_context_item (search_menu_context_menu_items id, ssd_cm_item_ptr *item){
	int i;

	for( i=0; i<context_menu.item_count; i++)
	{
		*item = context_menu.item + i;
		if ((*item)->id == id) {
			return;
		}
	}

	//wrong id
	roadmap_log(ROADMAP_ERROR, "roadmap_search - get_context_item() bad item id: %d count is: %d", id, context_menu.item_count);
	*item = context_menu.item;
}
#endif //IPHONE


static int on_options(SsdWidget widget, const char *new_value, void *context){
#ifndef IPHONE
   int menu_x;
   static ContextmenuContext menu_context;
   BOOL can_navigate;
   BOOL can_add_to_favorites;
   BOOL can_add_reminder = TRUE;
   BOOL can_delete;
   BOOL add_cancel = FALSE;

#ifdef TOUCH_SCREEN
   add_cancel = TRUE;
   roadmap_screen_refresh();
#endif

   if(s_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      s_context_menu_is_active = FALSE;
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
   can_navigate = TRUE;
   can_add_to_favorites = FALSE;
   can_delete = FALSE;


   if (strcmp(new_value, "generic_list")){
         can_delete = TRUE;
         if ((menu_context.context->category == 'A') || (menu_context.context->category == 'S')){
            can_add_to_favorites = TRUE;
         }
   }

   if (strcmp(new_value, "generic_list")){
          if (menu_context.context->category == 'S'){
             char *argv[reminder_hi__count];
             roadmap_history_get ('S', (void *)ssd_generic_list_dialog_selected_value  (), argv);
             if (!strcmp(argv[reminder_hi_add_reminder],"1")){
                can_add_reminder = FALSE;
                can_add_to_favorites = FALSE;
             }
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
                              search_menu_cancel,
                              add_cancel,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              search_menu_add_geo_reminder,
                              can_add_reminder&&roadmap_reminder_feature_enabled(),
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
                           dir_default,
                           0,
                           TRUE);

   s_context_menu_is_active = TRUE;

#else
	static ContextmenuContext menu_context;
	static char *labels[MAX_CONTEXT_ENTRIES];
	static void *values[MAX_CONTEXT_ENTRIES];
	static char *icons[MAX_CONTEXT_ENTRIES];
	ssd_cm_item_ptr item;

	int count = 0;

	menu_context.history = (void *)new_value;
	menu_context.context = context;



	get_context_item (search_menu_navigate, &item);
	values[count] = item;
	labels[count] = strdup(item->label);
	icons[count] = NULL;
	count++;


	get_context_item (search_menu_show, &item);
	values[count] = item;
	labels[count] = strdup(item->label);
	icons[count] = NULL;
	count++;

	get_context_item (search_menu_delete, &item);
	values[count] = item;
	labels[count] = strdup(item->label);
	icons[count] = NULL;
	count++;

	if (menu_context.context->category == 'A'){
		get_context_item (search_menu_add_to_favorites, &item);
		values[count] = item;
		labels[count] = strdup(item->label);
		icons[count] = NULL;
		count++;

      if (roadmap_reminder_feature_enabled()) {
         get_context_item (search_menu_add_geo_reminder, &item);
         values[count] = item;
         labels[count] = strdup(item->label);
         icons[count] = NULL;
         count++;
      }
	} else if (menu_context.context->category == 'S'){
      char *argv[reminder_hi__count];
      roadmap_history_get ('S', menu_context.history, argv);
      if (strcmp(argv[reminder_hi_add_reminder],"1")) {

         get_context_item (search_menu_add_to_favorites, &item);
         values[count] = item;
         labels[count] = strdup(item->label);
         icons[count] = NULL;
         count++;

         if (roadmap_reminder_feature_enabled()) {
            get_context_item (search_menu_add_geo_reminder, &item);
            values[count] = item;
            labels[count] = strdup(item->label);
            icons[count] = NULL;
            count++;
         }
      }
	} else if (menu_context.context->category == 'F' && roadmap_reminder_feature_enabled()){
      get_context_item (search_menu_add_geo_reminder, &item);
      values[count] = item;
      labels[count] = strdup(item->label);
      icons[count] = NULL;
      count++;
   }


	roadmap_list_menu_generic ("Options",
                              count,
                              (const char **)labels,
                              (const void **)values,
                              (const char **)icons,
                              NULL,
                              NULL,
                              on_option_selected,
                              NULL,
                              &menu_context,
                              NULL, 60, 0, NULL);
#endif //IPHONE

   return 0;
}

static int history_callback (SsdWidget widget, const char *new_value, const void *value,
                                  void *data) {

   RoadMapSearchContext *context = (RoadMapSearchContext *)data;
#ifdef TOUCH_SCREEN
    if (context->category == 'S'){
       char *argv[reminder_hi__count];
       roadmap_history_get ('S', (void *)value, argv);
       if (!strcmp(argv[reminder_hi_add_reminder],"1")){
          static ContextmenuContext menu_context;
          menu_context.history = (void *)value;
          menu_context.context = data;
          on_show(&menu_context);
          return 0;
       }
       else{
          static ContextmenuContext menu_context;
          roadmap_analytics_log_event (ANALYTICS_EVENT_NAVIGATE, ANALYTICS_EVENT_INFO_SOURCE,  ANALYTICS_EVENT_SEARCHMARKED );
          menu_context.history = (void *)value;
          menu_context.context = data;
          on_navigate(&menu_context);
          return 0;
       }
    }
    else{
       static ContextmenuContext menu_context;
       if (context->category == 'F')
          roadmap_analytics_log_event (ANALYTICS_EVENT_NAVIGATE, ANALYTICS_EVENT_INFO_SOURCE, ANALYTICS_EVENT_SEARCHFAV );
       else if (context->category == 'A')
          roadmap_analytics_log_event (ANALYTICS_EVENT_NAVIGATE, ANALYTICS_EVENT_INFO_SOURCE,  ANALYTICS_EVENT_SEARCHHISTORY);
       menu_context.history = (void *)value;
       menu_context.context = data;
       on_navigate(&menu_context);
       return 0;
    }
#endif

   on_options(NULL, value, data);

   return TRUE;
}


static int quick_favorites_callback (SsdWidget widget, const char *new_value, const void *value) {

   static RoadMapSearchContext context ;
   static ContextmenuContext menu_context;

   if (!strcmp(value,"add home")) {
      add_home_work_dlg(0);
      return 1;
   }
   else  if (!strcmp(value,"add work")){
      add_home_work_dlg(1);
      return 1;
   }

   context.category = 'F';
   menu_context.history = (void *)value;
   menu_context.context = &context;
   roadmap_analytics_log_event (ANALYTICS_EVENT_NAVIGATE, ANALYTICS_EVENT_INFO_SOURCE,  ANALYTICS_EVENT_SEARCHQUICK );

   on_navigate(&menu_context);
   return TRUE;
}

void inverse(void  *inver_a[],int j)
{
   void *temp;
   int i;
   j--;
   for(i=0;i<(j/2);i++)
   {
      temp=inver_a[i];
      inver_a[i]=inver_a[j];
      inver_a[j]=temp;
      j--;
   }
}

static void swap(void *arr[],int i, int j){
	void * temp = arr[i];
	arr[i] = arr[j];
	arr[j] = temp;

}

static void show_empty_list_message(const char * title){
   SsdWidget dialog;
   SsdWidget group;
   SsdWidget text;
   SsdWidget space;
   if ( !ssd_dialog_exists( title ) )
   {
	   dialog = ssd_dialog_new (title,title, NULL,
								 SSD_CONTAINER_TITLE);
	   group = ssd_container_new ("empty_title_group", NULL,
				SSD_MAX_SIZE, SSD_MAX_SIZE,SSD_WIDGET_SPACE|SSD_ALIGN_CENTER|SSD_END_ROW);
	   ssd_widget_set_color(group, NULL, NULL);

	   space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, 10, SSD_WIDGET_SPACE|SSD_END_ROW);
	   ssd_widget_set_color (space, NULL,NULL);
	   ssd_widget_add (group, space);

	   text = ssd_text_new ("empty list text", roadmap_lang_get("There are no saved addresses"), 22,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

	   ssd_text_set_color(text, "#24323a");

	   ssd_widget_add (group, text);

	   ssd_widget_add( dialog, group);
   }
   ssd_dialog_activate (title, NULL);
}

int on_add_favorite_cb (SsdWidget widget, const char *new_value){
   add_home_work_dlg(2);
}


void roadmap_search_history (char category, const char *title) {

#define MAX_HISTORY_ENTRIES 100
   static RoadMapSearchContext context;
   const char *button_icon[]   = {"button_plus", "button_plus_s"};
   SsdWidget button;
   SsdWidget text;
   static char *labels[MAX_HISTORY_ENTRIES];
   static void *values[MAX_HISTORY_ENTRIES];
   static char *icons[MAX_HISTORY_ENTRIES];
   static int count = -1;
   void *history;
   void *prev;
   int height = ssd_container_get_row_height();
   int homeIndex = -1; // these will hold the places of the home and work places in the list, if they stay -1
   int workIndex = -1;	// they don't exist

#ifndef TOUCH_SCREEN
   height = 44;
#endif
   context.category = category;
   context.title = strdup(title);

   if (count == -1){
            roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
            roadmap_history_declare( ADDRESS_FAVORITE_CATEGORY, ahi__count);
            roadmap_history_declare ('S', reminder_hi__count);
   }

   count = 0;

   history = roadmap_history_latest (category);

   while (history && (count < MAX_HISTORY_ENTRIES)) {
      char *argv[reminder_hi__count];
      char str[350];
      char tmp1[250],tmp[250],tmp2[250];

      roadmap_history_get (category, history, argv);
      prev = history;

      if (category == 'S'){
         if (!strcmp(argv[reminder_hi_add_reminder],"1")){
            if ((argv[1][1] != 0) && (argv[2][0] != 0))
               snprintf (str, sizeof(str), "%s: %s, %s \n %s %s, %s",roadmap_lang_get("Reminder"), argv[reminder_hi_title], argv[reminder_hi_description],roadmap_lang_get("Near"), argv[reminder_hi_street], argv[reminder_hi_city]);
            else if (argv[2][0] != 0)
               snprintf (str, sizeof(str), "%s: %s, %s \n %s %s", roadmap_lang_get("Reminder"),argv[reminder_hi_title], argv[reminder_hi_description],roadmap_lang_get("Near"), argv[reminder_hi_city]);
            else if (argv[1][0] != 0)
               snprintf (str, sizeof(str), "%s: %s, %s \n %s %s", roadmap_lang_get("Reminder"), argv[reminder_hi_title], argv[reminder_hi_description],roadmap_lang_get("Near"), argv[reminder_hi_street]);
            icons[count] = "geo_reminders";
         }
         else{
            if (argv[4][0] != 0)
               snprintf (str, sizeof(str), "%s \n%s %s, %s", argv[4], argv[1], argv[0], argv[2]);
            else
               snprintf (str, sizeof(str), "%s %s, %s", argv[1], argv[0], argv[2]);
            icons[count] = "marked_location";
         }
      }
      else if (category == 'A'){
         snprintf (tmp1, sizeof(tmp1), "%s%s%s", argv[1], argv[0], argv[2]);
         snprintf (tmp, sizeof(tmp), "%s", argv[1]);
         if (!*tmp1)
            snprintf (tmp2, sizeof(tmp2), "-");
         else if (!*tmp)
            snprintf (tmp2, sizeof(tmp2), "%s",argv[2]);
         else if (ssd_widget_rtl(NULL))
            snprintf (tmp2, sizeof(tmp2), "%s %s, %s", argv[1], argv[0], argv[2]);
         else
            snprintf (tmp2, sizeof(tmp2), "%s %s, %s", argv[0], argv[1], argv[2]);

        if (argv[4][0] != 0){
           snprintf (str, sizeof(str), "%s \n%s", argv[4], tmp2);
           icons[count] = local_search_get_icon_name();
        }else{
           snprintf (str, sizeof(str), "%s", tmp2);
           icons[count] = "history";
        }

      }
      else{
         snprintf (str, sizeof(str), "%s", argv[4]);
         if (!strcmp(str, roadmap_lang_get("Home")) || !strcmp(str,"home") || !strcmp(str,"Home")) {
         	icons[count] = "home";
         	homeIndex = count;
         }else if (!strcmp(str, roadmap_lang_get("Work")) || !strcmp(str,"office") || !strcmp(str,"work") || !strcmp(str,"Work")){
         	icons[count] = "work";
         	workIndex= count;
         }else{
         	icons[count] = "favorite";
         }
      }
      if (labels[count]) free (labels[count]);
      labels[count] = strdup(str);

      values[count] = history;


      count++;

      history = roadmap_history_before (category, history);
      if (history == prev) break;
   }

   if (category =='F'){
   	    if (homeIndex != -1){  // Home entry exists
   	    		swap(&values[0],homeIndex,0);
   	    		swap((void **)&labels[0], homeIndex,0);
 				swap((void **)&icons[0], homeIndex,0);
   	    }
   	    if ( workIndex != -1){ // Work entry exists
   	    	   int newWorkIndex = 0;
   	    	   if (homeIndex != -1){
   	    	   		 newWorkIndex = 1; // Home also exists, so work will be pushed second
   	    	   		 if (workIndex==0)
   	    	   		 	workIndex = homeIndex; // end case - the work index actually changed in the homeIndex swap above
   	    	   }
   	    	   swap(&values[0],workIndex,newWorkIndex);
   	    	   swap((void **)&labels[0], workIndex,newWorkIndex);
 			   swap((void **)&icons[0], workIndex,newWorkIndex);
   	    }
   }

#ifndef IPHONE //TODO: handle empty list
   if (count == 0)
      show_empty_list_message(roadmap_lang_get (title));
   else
      ssd_generic_icon_list_dialog_show (roadmap_lang_get (title),
                  count,
                  (const char **)labels,
                  (const void **)values,
                  (const char **)icons,
                  NULL,
                  history_callback,
                  NULL,
                  &context,
                  roadmap_lang_get("Options"),
                  on_options, height,0,TRUE);

#ifdef TOUCH_SCREEN
   if (category =='F'){
      ssd_dialog_change_button("right_title_button", button_icon, 2);
      button = ssd_dialog_right_title_button();
      if (button){
         button->callback = on_add_favorite_cb;
         if (ssd_widget_rtl(NULL))
            ssd_widget_set_offset(button, 10, 0);
         else
            ssd_widget_set_offset(button, -10, 0);
         ssd_widget_show(button);
      }
   }
   else{
      button = ssd_dialog_right_title_button();
      if (button)
         ssd_widget_hide(button);
   }
#endif
#else
	roadmap_list_menu_generic (roadmap_lang_get (title),
                              count,
                              (const char **)labels,
                              (const void **)values,
                              (const char **)icons,
                              NULL,
                              NULL,
                              history_callback,
                              NULL,
                              &context,
                              on_options, 60, LIST_MENU_ADD_DETAIL_BUTTON, NULL);
#endif //IPHONE
}



void search_menu_search_history(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHHISTORY, NULL, NULL);

   roadmap_search_history ('A', "Recent searches");
}

void search_menu_search_favorites(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHFAV, NULL, NULL);

   roadmap_search_history ('F', "My favorites");
}

void search_menu_my_saved_places(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHMARKED, NULL, NULL);

	roadmap_search_history ('S', "Saved locations");
}

void search_menu_geo_reminders(void){
   roadmap_search_history ('R', "Geo-Reminders");
}

static void on_dlg_closed_address(int exit_code, void* context)
{
   s_address_search_is_active = FALSE;

   if(( dec_cancel != exit_code) && (dec_close != exit_code) && (dec_ok != exit_code))
      ssd_dialog_hide_all( dec_close);
}

static void on_dlg_closed_local(int exit_code, void* context)
{
   s_poi_search_is_active = FALSE;

   if(( dec_cancel != exit_code) && (dec_close != exit_code) && (dec_ok != exit_code))
      ssd_dialog_hide_all( dec_close);
}

void search_menu_search_address(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHADDR, NULL, NULL);

#ifndef IPHONE
   if( s_address_search_is_active)
   {
      assert(0);
      return;
   }

   address_search_dlg_show( on_dlg_closed_address, NULL);
   s_address_search_is_active = TRUE;
#else
	address_search_dlg_show(NULL, NULL);
#endif //IPHONE
}

void search_menu_single_search(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHADDR, NULL, NULL);

#ifndef IPHONE
   if( s_address_search_is_active)
   {
      assert(0);
      return;
   }

   single_search_dlg_show( on_dlg_closed_address, NULL);
   s_address_search_is_active = TRUE;
#else
   single_search_dlg_show(NULL, NULL);
#endif //IPHONE
}


void search_menu_single_search_favorite(){
#ifndef IPHONE
   if( s_address_search_is_active)
   {
      assert(0);
      return;
   }

   single_search_dlg_show_favorite( g_favorite_name, on_dlg_closed_address, NULL);
   s_address_search_is_active = TRUE;
#else
   single_search_dlg_show(NULL, NULL);
#endif //IPHONE
}


void search_menu_search_local(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHLOCAL, NULL, NULL);

#ifndef IPHONE
   if( s_poi_search_is_active)
   {
      assert(0);
      return;
   }

   local_search_dlg_show( on_dlg_closed_local, NULL);
   s_poi_search_is_active = TRUE;
#else
	local_search_dlg_show(NULL, NULL);
#endif //IPHONE
}

/////////////////////////////////////////////////////////////////////
void roamdmap_search_address_book(void){
   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHAB, NULL, NULL);
#ifdef IPHONE_NATIVE

   address_book_dlg_show(NULL, NULL);
#endif //IPHONE_NATIVE
#ifdef ANDROID
   roadmap_main_show_contacts();
#endif
#if defined (_WIN32) && !defined (EMBEDDED_CE)
   roadmap_main_search_contacts();
#endif

}


SsdWidget create_quick_search_menu(){

    return NULL;
}

extern ssd_contextmenu_ptr    s_main_menu;

#ifdef  TOUCH_SCREEN
static SsdWidget s_search_menu = NULL;
extern const char*            grid_menu_labels[];
extern RoadMapAction          RoadMapStartActions[];
BOOL get_menu_item_names(  const char*          menu_name,
                           ssd_contextmenu_ptr  parent,
                           const char*          items[],
                           int*                 count);

void roadmap_search_top_three_fav (char ***labels_o, void ***values_o, char ***icons_o, int *count_o, SsdListCallback *callback_o) {
   //TODO remove this code from get_menu_item_names and use this function instead
   int count = 0;
   void *history;
   void *prev;
   static char *labels[MAX_HISTORY_ENTRIES];
   static void *values[MAX_HISTORY_ENTRIES];
   static char *icons[MAX_HISTORY_ENTRIES];
   int homeIndex = -1; // these will hold the places of the home and work places in the list, if they stay -1
   int workIndex = -1;  // they don't exist

   history = roadmap_history_latest ('F');
   while (history && (count < MAX_HISTORY_ENTRIES)) {
      char *argv[ahi__count];
      char str[350];
      roadmap_history_get ('F', history, argv);
      prev = history;
      snprintf (str, sizeof(str), "%s", argv[4]);
      if (!strcmp(str, roadmap_lang_get("Home")) || !strcmp(str,"home") || !strcmp(str,"Home")) {
         icons[count] = "home";
         homeIndex = count;
      }else if ( !strcmp(str, roadmap_lang_get("Work")) || !strcmp(str,"work") || !strcmp(str,"Work") || !strcmp(str,"office") ){
         icons[count] = "work";
         workIndex= count;
      }else{
         icons[count] = "favorite";
      }
      if (labels[count]) free (labels[count]);
      labels[count] = strdup(str);

      values[count] = history;

      count++;

      history = roadmap_history_before ('F', history);
      if (history == prev) break;
   }

   if (homeIndex != -1){  // Home entry exists
      swap(&values[0],homeIndex,0);
      swap((void **)&labels[0], homeIndex,0);
      swap((void **)&icons[0], homeIndex,0);
   }
   if ( workIndex != -1){ // Work entry exists
      int newWorkIndex = 0;
      if (homeIndex != -1){
         newWorkIndex = 1; // Home also exists, so work will be pushed second
         if (workIndex==0)
            workIndex = homeIndex; // end case - the work index actually changed in the homeIndex swap above
      }
      swap(&values[0],workIndex,newWorkIndex);
      swap((void **)&labels[0], workIndex,newWorkIndex);
      swap((void **)&icons[0], workIndex,newWorkIndex);
   }

   if (count > 3)
      count = 3;

   if (count == 0){
      icons[0] = "home";
      labels[0] = strdup(roadmap_lang_get("Home (Touch to add)"));
      icons[1] = "work";
      labels[1] = strdup(roadmap_lang_get("Work (Touch to add)"));
      values[0] = "add home";
      values[1] = "add work";
      count = 2;
   }
   else if (count < 3){
      if (homeIndex == -1){
         swap(&values[0],count,0);
         swap((void **)&labels[0], count,0);
         swap((void **)&icons[0], count,0);
         icons[0] = "home";
         labels[0] = strdup(roadmap_lang_get("Home (Touch to add)"));
         values[0] = "add home";
         homeIndex = 0;
         count++;
      }

      if  ((count < 3) && (workIndex == -1)){
         workIndex = count;
         icons[count] = "work";
         labels[count] = strdup(roadmap_lang_get("Work (Touch to add)"));
         values[count] = "add work";

         if (count != 1){
            swap(&values[0],count,1);
            swap((void **)&labels[0], count,1);
            swap((void **)&icons[0], count,1);
         }
         count++;
      }
   }

   *labels_o = labels;
   *values_o = values;
   *icons_o = icons;
   *count_o = count;
   *callback_o = quick_favorites_callback;
}

#ifndef IPHONE_NATIVE
static SsdWidget get_favorites_widget( SsdWidget list, int *f_count){
   SsdWidget favorites_container = NULL;
   static RoadMapSearchContext context;
   int count = 0;
   void *history;
   void *prev;
   static char *labels[MAX_HISTORY_ENTRIES];
   static void *values[MAX_HISTORY_ENTRIES];
   static char *icons[MAX_HISTORY_ENTRIES];
   int homeIndex = -1; // these will hold the places of the home and work places in the list, if they stay -1
   int workIndex = -1;  // they don't exist
   int width = ssd_container_get_width();


   context.category = 'F';
   context.title = strdup("My favorites");

   history = roadmap_history_latest ('F');
   while (history && (count < MAX_HISTORY_ENTRIES)) {
      char *argv[ahi__count];
      char str[350];
      roadmap_history_get ('F', history, argv);
      prev = history;
      snprintf (str, sizeof(str), "%s", argv[4]);
      if (!strcmp(str, roadmap_lang_get("Home"))){
          icons[count] = "home";
          homeIndex = count;
       }else if (!strcmp(str, roadmap_lang_get("Work"))){
          icons[count] = "work";
          workIndex= count;
       }else{
          icons[count] = "favorite";
       }
       if (labels[count]) free (labels[count]);
       labels[count] = strdup(str);

       values[count] = history;

       count++;

       history = roadmap_history_before ('F', history);
       if (history == prev) break;
   }

   if (homeIndex != -1){  // Home entry exists
       swap(&values[0],homeIndex,0);
       swap((void **)&labels[0], homeIndex,0);
       swap((void **)&icons[0], homeIndex,0);
   }
   if ( workIndex != -1){ // Work entry exists
        int newWorkIndex = 0;
        if (homeIndex != -1){
            newWorkIndex = 1; // Home also exists, so work will be pushed second
            if (workIndex==0)
                workIndex = homeIndex; // end case - the work index actually changed in the homeIndex swap above
        }
        swap(&values[0],workIndex,newWorkIndex);
        swap((void **)&labels[0], workIndex,newWorkIndex);
        swap((void **)&icons[0], workIndex,newWorkIndex);
   }

   if (count > 3)
      count = 3;

   if (count == 0){
        icons[0] = "home";
        labels[0] = strdup(roadmap_lang_get("Home (Touch to add)"));
        icons[1] = "work";
        labels[1] = strdup(roadmap_lang_get("Work (Touch to add)"));
        values[0] = "add home";
        values[1] = "add work";
        count = 2;
   }
   else if (count < 3){
      if (homeIndex == -1){
         swap(&values[0],count,0);
         swap((void **)&labels[0], count,0);
         swap((void **)&icons[0], count,0);
         icons[0] = "home";
         labels[0] = strdup(roadmap_lang_get("Home (Touch to add)"));
         values[0] = "add home";
         homeIndex = 0;
         count++;
      }

      if  ((count < 3) && (workIndex == -1)){
         workIndex = count;
         icons[count] = "work";
         labels[count] = strdup(roadmap_lang_get("Work (Touch to add)"));
         values[count] = "add work";

         if (count != 1){
            swap(&values[0],count,1);
            swap((void **)&labels[0], count,1);
            swap((void **)&icons[0], count,1);
         }
         count++;
      }
   }

   if (list == NULL){
      list = ssd_list_new(       "__favorites_additional_container_list",
                                 SSD_MAX_SIZE,
                                 SSD_MAX_SIZE,
                                 inputtype_free_text,
                                 0,
                                 NULL);
      //ssd_widget_set_color(list, NULL,NULL);
      ssd_list_resize( list, ssd_container_get_row_height());
      if (count > 0){
         //Quick Setting Container
         favorites_container = ssd_container_new ("__favorites_additional_container", NULL, width, SSD_MIN_SIZE,
               SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);
         ssd_list_populate (list, count, (const char **)labels, (const void **)values, (const char **)icons, NULL, quick_favorites_callback, NULL, FALSE);

         ssd_widget_add( favorites_container, list);
      }

   }
   else{
      ssd_list_populate (list, count, (const char **)labels, (const void **)values, (const char **)icons, NULL, quick_favorites_callback, NULL, FALSE);

   }

   return favorites_container;
}

static void update_favorites_widget(SsdWidget container){
   int count;
   get_favorites_widget(ssd_widget_get(container, "__favorites_additional_container_list"), &count);
}

static BOOL on_key_pressed__delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;
   SsdWidget main_cont= this->parent;
   SsdWidget edit;

   assert( this);
   assert( main_cont);
   assert( this->children);
   assert( this->children->key_pressed);

   editbox = this->children;
   ssd_widget_hide(ssd_widget_get(editbox, "BgText"));

   edit = ssd_widget_get(editbox,"edit" );
   edit->flags |= SSD_TEXT_INPUT;
   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
     single_search_auto_search( ssd_text_get_text( edit));

      return TRUE;
   }

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}

static BOOL on_add_favorite_key_pressed__delegate_to_editbox(
                     SsdWidget   this,
                     const char* utf8char,
                     uint32_t    flags)
{
   SsdWidget editbox  = NULL;
   SsdWidget main_cont= this->parent;
   SsdWidget edit;

   assert( this);
   assert( main_cont);
   assert( this->children);
   assert( this->children->key_pressed);

   editbox = this->children;
   ssd_widget_hide(ssd_widget_get(editbox, "BgText"));

   edit = ssd_widget_get(editbox,"edit" );
   edit->flags |= SSD_TEXT_INPUT;
   //   Special case:   move focus to the list
   if( KEY_IS_ENTER)
   {
      single_search_add_favorite( ssd_text_get_text( edit), g_favorite_name);

      return TRUE;
   }

   editbox = this->children;
   return editbox->key_pressed( editbox, utf8char, flags);
}
static void reset_edit_box(SsdWidget widget)
{
   SsdWidget edit;
   const char *text;
// AGA TEMPORARY SOLUTION
#ifdef ANDROID
   search_menu_single_search();
   return TRUE;
#endif
   if ( !roadmap_native_keyboard_enabled() ){
            search_menu_single_search();
            return ;
   }
   edit = ssd_widget_get(widget,"edit" );

   if (!widget->in_focus){
      ssd_widget_hide(ssd_widget_get(widget, "BgText"));
      edit->flags |= SSD_TEXT_INPUT;
      ssd_dialog_set_focus(widget);
      roadmap_native_keyboard_show( &s_gNativeKBParams );
   }
   else{
      text = ssd_text_get_text( edit);
      if (text && *text==0)
         ssd_widget_show(ssd_widget_get(widget, "BgText"));
      edit->flags &= ~SSD_TEXT_INPUT;
      ssd_widget_loose_focus(widget);
      roadmap_native_keyboard_hide();
   }

}
static int on_input_pointer_down(SsdWidget widget, const RoadMapGuiPoint *point)
{
   if ( !roadmap_native_keyboard_enabled() ){
            search_menu_single_search();
            return TRUE;
   }

   reset_edit_box(widget);
   return TRUE;
}

static int on_add_favorite_input_pointer_down(SsdWidget widget, const RoadMapGuiPoint *point)
{
   if ( !roadmap_native_keyboard_enabled() ){

      search_menu_single_search_favorite();
            return TRUE;
   }
#ifdef ANDROID
   search_menu_single_search_favorite();
#else
   reset_edit_box(widget);
#endif
   return TRUE;
}


#ifndef IPHONE
static void add_home_work_dlg(int iType){
   SsdWidget dialog;
   SsdWidget search_container = NULL;
   SsdWidget search_widget;
   SsdWidget favorites;
   SsdWidget icnt;
   SsdWidget ecnt;
   SsdWidget bg_text;
   SsdWidget edit = NULL;
   SsdWidget bitmap = NULL;
   SsdWidget box;
   SsdWidget text;
   int f_count;
   int height = ADJ_SCALE(45);
   int txt_box_height = ADJ_SCALE(37);
   int container_height = ADJ_SCALE(60);
   const char *title;


   if (iType == 0){
      title = roadmap_lang_get("My Home");
      g_favorite_name = roadmap_lang_get("Home");
   }
   else if (iType == 1){
      title = roadmap_lang_get("My Work");
      g_favorite_name = roadmap_lang_get("Work");
   }
   else{
      title = roadmap_lang_get("Add to favorites");
      g_favorite_name = NULL;
   }
   dialog = ssd_dialog_new ("Add HomeWork",title, NULL, SSD_CONTAINER_TITLE);

   search_container = ssd_container_new ("__search_additional_container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
           SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color(search_container, NULL, NULL);
   ssd_widget_set_offset(search_container, 0, -5);
   search_widget = ssd_container_new ("__search_additional_container.search_widget", NULL, SSD_MAX_SIZE, container_height,
           SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color(search_widget, "#b2b2b2", "#b2b2b2");

   icnt = ssd_container_new(  "input_container",
                              NULL,
                              SSD_MAX_SIZE,
                              SSD_MAX_SIZE,
                              SSD_ALIGN_VCENTER);
   ssd_widget_set_color(icnt, NULL, NULL);

   ecnt = ssd_container_new(  "input_container",
                              NULL,
                              roadmap_canvas_width() - 15,
                              txt_box_height,
                              SSD_WS_TABSTOP|SSD_CONTAINER_SEARCH_BOX|SSD_END_ROW|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
   ssd_widget_set_focus_highlight( ecnt, FALSE );
   ecnt->pointer_down = on_add_favorite_input_pointer_down;
   bitmap = ssd_bitmap_new("serach", "search_icon", SSD_ALIGN_VCENTER);


   ssd_widget_add(ecnt, bitmap);

   edit = ssd_text_new     (  "edit",
                              "", 18, SSD_ALIGN_VCENTER );
   bg_text = ssd_text_new ("BgText", roadmap_lang_get("Search address or Place"), -1, SSD_ALIGN_VCENTER);
   ssd_widget_set_color(bg_text, "#C0C0C0",NULL);
   ssd_widget_add(ecnt, bg_text);
   ssd_text_set_input_type( edit, inputtype_free_text);
   ssd_text_set_readonly  ( edit, FALSE);
   //   Delegate the 'on key pressed' event to the child edit-box:
   ecnt->key_pressed = on_add_favorite_key_pressed__delegate_to_editbox;

   ssd_widget_add( ecnt, edit);
   ssd_widget_add( icnt, ecnt );

   ssd_widget_add (search_widget, icnt );

   ssd_widget_add(search_container, search_widget);


   ssd_widget_add(dialog,search_container);


   box = ssd_container_new("home work box","", SSD_MAX_SIZE, SSD_MIN_SIZE, SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE);
   if (g_favorite_name != NULL){
      ssd_dialog_add_vspace(box, 10, 0);
      bitmap = ssd_bitmap_new("homework", "HW_route", SSD_ALIGN_CENTER|SSD_END_ROW);
      ssd_widget_add(box,bitmap);

      ssd_dialog_add_vspace(box, 10, 0);
      text = ssd_text_new("HW_route_Txt", roadmap_lang_get("Waze is best used for commuting."), 16,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
      ssd_text_set_color(text,"#000000");
      ssd_dialog_add_hspace(box, 10, 0);
      ssd_widget_add(box,text);

      ssd_dialog_add_vspace(box, 30, 0);
      text = ssd_text_new("HW_route_Txt2", roadmap_lang_get("Once you add 'Home' and 'Work' to your favorites, waze'll learn your preferred routes and departure times to these destinations."), 16,SSD_TEXT_NORMAL_FONT|SSD_END_ROW|SSD_ALIGN_CENTER);
      ssd_text_set_color(text,"#000000");
      ssd_dialog_add_hspace(box, 10, 0);
      ssd_text_set_lines_space_padding(text, 5);
      ssd_widget_add(box,text);
   }

   ssd_dialog_add_vspace(box, 10, SSD_END_ROW);
   text = ssd_text_new("HW_route_Txt2", roadmap_lang_get("Enter your address in the search box above."), 16,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_CENTER);
   ssd_text_set_color(text,"#000000");
   ssd_dialog_add_hspace(box, 10, 0);
   ssd_text_set_lines_space_padding(text, 5);
   ssd_widget_add(box,text);

   ssd_widget_add(dialog,box);



   ssd_dialog_activate("Add HomeWork", NULL);
}
#endif

static SsdWidget create_additional_search_container(){
    SsdWidget search_container = NULL;
    SsdWidget search_widget;
    SsdWidget favorites;
    SsdWidget icnt;
    SsdWidget ecnt;
    SsdWidget bg_text;
    SsdWidget edit = NULL;
    SsdWidget bitmap = NULL;
    int f_count;
    int height = 45;
    int txt_box_height = ADJ_SCALE(37);
    int container_height = 60;

    roadmap_history_declare( ADDRESS_FAVORITE_CATEGORY, ahi__count);

    if ( roadmap_screen_is_hd_screen() )
    {
       height = 65;
       container_height = 90;
    }

    search_container = ssd_container_new ("__search_additional_container", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
            SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW);
    ssd_widget_set_color(search_container, NULL, NULL);
    ssd_widget_set_offset(search_container, 0, -14);
    search_widget = ssd_container_new ("__search_additional_container.search_widget", NULL, SSD_MAX_SIZE, container_height,
            SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW);
    ssd_widget_set_color(search_widget, "#b2b2b2", "#b2b2b2");

    icnt = ssd_container_new(  "input_container",
                               NULL,
                               SSD_MAX_SIZE,
                               SSD_MAX_SIZE,
                               SSD_ALIGN_VCENTER);
    ssd_widget_set_color(icnt, NULL, NULL);

    ecnt = ssd_container_new(  "input_container",
                               NULL,
                               roadmap_canvas_width() - 15,
                               txt_box_height,
                               SSD_WS_TABSTOP|SSD_CONTAINER_SEARCH_BOX|SSD_END_ROW|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
    ssd_widget_set_focus_highlight( ecnt, FALSE );
    ecnt->pointer_down = on_input_pointer_down;
    bitmap = ssd_bitmap_new("serach", "search_icon", SSD_ALIGN_VCENTER);


    ssd_widget_add(ecnt, bitmap);

    edit = ssd_text_new     (  "edit",
                               "", 18, SSD_ALIGN_VCENTER );
    bg_text = ssd_text_new ("BgText", roadmap_lang_get("Search address or Place"), -1, SSD_ALIGN_VCENTER);
    ssd_widget_set_color(bg_text, "#C0C0C0",NULL);
    ssd_widget_add(ecnt, bg_text);
    ssd_text_set_input_type( edit, inputtype_free_text);
    ssd_text_set_readonly  ( edit, FALSE);
    //   Delegate the 'on key pressed' event to the child edit-box:
    ecnt->key_pressed = on_key_pressed__delegate_to_editbox;

    ssd_widget_add( ecnt, edit);
    ssd_widget_add( icnt, ecnt );

    ssd_widget_add (search_widget, icnt );

    ssd_widget_add(search_container, search_widget);

    favorites = get_favorites_widget(NULL, &f_count);
    if (favorites != NULL){
       //Quick Setting Container
       ssd_dialog_add_vspace(search_container, 5, 0);
       ssd_widget_add(search_container, favorites);
    }


    return search_container;
}
static void on_search_dlg_close (int exit_code, void* context){
   roadmap_native_keyboard_hide();
}

void roadmap_search_menu(void){
   int                  count;
   RoadMapGpsPosition   MyLocation;
   RoadMapPosition      position;
   SsdWidget            additionalSearchContainer;

   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHMENU, NULL, NULL);

   if( roadmap_navigate_get_current( &MyLocation, NULL, NULL) == -1)
   {
       position.latitude = 0;
       position.longitude = 0;
   }
   else{
       position.latitude = MyLocation.latitude;
       position.longitude = MyLocation.longitude;
   }
   if (ssd_dialog_is_currently_active()) {
      if (!strcmp(ssd_dialog_currently_active_name(), SEARCH_MENU_DLG_NAME )){
         ssd_dialog_hide_current(dec_close);
         roadmap_screen_refresh ();
         return;
      }
   }

   if( !s_main_menu)
   {
      s_main_menu = roadmap_factory_load_menu("quick.menu", RoadMapStartActions);

      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }

   if( !get_menu_item_names( "search_menu", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }


   if ( !ssd_dialog_exists( SEARCH_MENU_DLG_NAME ) )
   {
      additionalSearchContainer = create_additional_search_container();
      s_search_menu = ssd_menu_new_cb( SEARCH_MENU_DLG_NAME, additionalSearchContainer, NULL, grid_menu_labels, RoadMapStartActions, SSD_CONTAINER_TITLE,on_search_dlg_close );
      ssd_dialog_activate ( SEARCH_MENU_DLG_NAME, NULL );
   }
   else{
      SsdWidget text;
      SsdWidget dialog = ssd_dialog_activate ( SEARCH_MENU_DLG_NAME, NULL );
      update_favorites_widget(dialog);
      text = ssd_widget_get(dialog, "edit");
      if (text){
         ssd_text_set_text(text,"");
         ssd_widget_set_focus(text->parent);
       }
   }
   ssd_dialog_draw ();


}


#else
   /*
   void roadmap_search_menu(void){
   int                  count;
   RoadMapGpsPosition   MyLocation;
   RoadMapPosition      position;

   roadmap_analytics_log_event(ANALYTICS_EVENT_SEARCHMENU, NULL, NULL);

   if( roadmap_navigate_get_current( &MyLocation, NULL, NULL) == -1)
   {
       position.latitude = 0;
       position.longitude = 0;
   }
   else{
       position.latitude = MyLocation.latitude;
       position.longitude = MyLocation.longitude;
   }

	if (ssd_dialog_is_currently_active()) {
		if (!strcmp(ssd_dialog_currently_active_name(), SEARCH_MENU_DLG_NAME )){
			ssd_dialog_hide_current(dec_close);
			roadmap_screen_refresh ();
			return;
		}
	}

   if( !s_main_menu)
   {
      s_main_menu = roadmap_factory_load_menu("quick.menu", RoadMapStartActions);

      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }

   if( !get_menu_item_names( "search_menu", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }

   const char *icons[20];
   const char *labels[20];
   const char *menu_label;
   int i = 0;

   menu_label = grid_menu_labels[i];

   //replace the local search action's icon and label
   while (menu_label != NULL) {
      if (strcmp(menu_label, "search_local") == 0) {
         icons[i] = local_search_get_icon_name();
         labels[i] = local_search_get_provider_label();
      } else {
         icons[i] = NULL;
         labels[i] = NULL;
      }
      i++;
      menu_label = grid_menu_labels[i];
   }
	roadmap_list_menu_simple (SEARCH_MENU_DLG_NAME,
                             NULL,
                             grid_menu_labels,
                             NULL,
                             labels,
                             icons,
                             NULL,
                             RoadMapStartActions,
                             0);

}
    */
#endif //IPHONE_NATIVE

#else

void roadmap_search_menu(void){
}

#endif  //  TOUCH_SCREEN


/*
 * Sets the attributes of the local search menu
 */
void search_menu_set_local_search_attrs()
{
#ifdef TOUCH_SCREEN
	if ( ssd_dialog_exists( SEARCH_MENU_DLG_NAME ) && s_search_menu )
	{
		const char* icon_name = local_search_get_icon_name();
		ssd_menu_set_item_icon( s_search_menu, "search_local", icon_name );
		ssd_menu_set_label_long_text( SEARCH_MENU_DLG_NAME, "search_local", roadmap_lang_get( local_search_get_provider_label() ) );
	}
#else
	if ( s_main_menu )
	{
	   ssd_cm_item_ptr item;
	   item = roadmap_start_get_menu_item( "search_menu", "search_local", s_main_menu );
	   if (item){
	      item->icon = local_search_get_icon_name();
	      item->label = roadmap_lang_get(local_search_get_provider_label());
	   }
	}
#endif
}


