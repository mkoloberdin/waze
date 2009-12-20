
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

#include "roadmap.h"

#ifndef  TOUCH_SCREEN
#include <string.h>
#include <stdlib.h>

#include "ssd/ssd_combobox.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "roadmap_input_type.h"
#include "roadmap_street.h"
#include "roadmap_history.h"
#include "roadmap_street.h"
#include "roadmap_locator.h"
#include "roadmap_address.h"
#include "roadmap_geocode.h"
#include "roadmap_trip.h"
#include "roadmap_display.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_history.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_keyboard_dialog.h"

#include "roadmap_address_tc_defs.h"

extern void navigate_main_stop_navigation(void);
static int on_options(SsdWidget widget, const char *new_value, void *context);

static   list_items           s_history;
static   RoadMapAddressNav    s_navigator             = NULL;
static   BOOL                 s_history_was_loaded    = FALSE;
static   BOOL                 s_viewing_history       = FALSE;
static   BOOL                 s_context_menu_is_active= FALSE;
static   on_text_changed_ctx  s_on_text_changed_ctx;

// Context menu items:
static ssd_cm_item main_menu_items[] =
{
   //                  Label     ,           Item-ID
   SSD_CM_INIT_ITEM  ( "Navigate",           cmi_navigate),
   SSD_CM_INIT_ITEM  ( "Show on map",        cmi_show),
   SSD_CM_INIT_ITEM  ( "Add to favorites",   cmi_add_to_favorites),
   SSD_CM_INIT_ITEM  ( "Exit_key",           cmi_exit)
};

// Context menu:
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( main_menu_items);
///////////////////////////////////////


///////////////////////////////////////
void roadmap_address_register_nav( RoadMapAddressNav navigate)
{ s_navigator = navigate;}
///////////////////////////////////////


// Cached data of the ssd_list content
void list_items_init( list_items_ptr this)
{ memset( this, 0, sizeof(list_items));}

void list_items_free( list_items_ptr this)
{
   int i;

   // Free previously allocated strings:
   for( i = 0; i < this->size; i++)
   {
      FREE( this->labels[i])
      this->values[i] = NULL;
   }

   this->size = 0;
}
///////////////////////////////////////


// Single tab info   //////////////////
void atc_tab_info_init( atc_tab_info_ptr this, tabid id, void* parent)
{
   this->id             = id;
   this->tab            = NULL;
   list_items_init( &(this->items));
   this->list_is_empty  = TRUE;
   this->parent         = parent;
}

void atc_tab_info_free( atc_tab_info_ptr this)
{
   list_items_free( &(this->items));
   this->list_is_empty = TRUE;
}
///////////////////////////////////////


// Module context /////////////////////
void atcctx_init( atcctx_ptr this)
{
   int i;

   memset( this, 0, sizeof(atcctx));

   for( i=0; i<tab__count; i++)
      atc_tab_info_init( (this->tab_info + i), i, this);
}

void atcctx_free( atcctx_ptr this)
{
   int i;

   for( i=0; i<tab__count; i++)
      atc_tab_info_free( this->tab_info + i);
}
///////////////////////////////////////


///////////////////////////////////////
void  on_text_changed_ctx_copy(  on_text_changed_ctx_ptr this,
                                 const char*             new_text,
                                 void*                   context)
{
   on_text_changed_ctx_free( this);

   this->context = context;

   if( new_text && (*new_text))
   {
      this->new_text= malloc( strlen( new_text) + 1);
      strcpy( this->new_text, new_text);
   }
}

void  on_text_changed_ctx_free( on_text_changed_ctx_ptr this)
{
   FREE(this->new_text)
   this->context = NULL;
}
void  on_text_changed_ctx_init( on_text_changed_ctx_ptr this)
{ memset( this, 0, sizeof(on_text_changed_ctx));}
///////////////////////////////////////


///////////////////////////////////////
static void hide_our_dialogs( int exit_code)
{
   ssd_dialog_hide( ATC_NAME, exit_code);
   ssd_dialog_hide( "Main Menu", exit_code);
   ssd_dialog_hide( "Search", exit_code);
}

static void empty_list( SsdWidget tab)
{
   atc_tab_info_ptr ti = ssd_combobox_get_context(tab);

   list_items_free( &(ti->items));

   if( ti->list_is_empty)
      return;

   ssd_combobox_update_list(  tab,
                              0,
                              NULL,
                              NULL,
                              NULL);
   ti->list_is_empty = TRUE;
}

static void populate_list( SsdWidget   tab,
                           int         count,
                           char**      labels,
                           void**      values)
{
   atc_tab_info_ptr ti = NULL;

   if( !count)
   {
      empty_list( tab);
      return;
   }

   ssd_combobox_update_list(  tab,
                              count,
                              (const char **)labels,
                              (const void **)values,
                              NULL);

   ti                = ssd_combobox_get_context(tab);
   ti->list_is_empty = FALSE;
}

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

static int get_house_number_from_address( const char* address)
{
   const char* p     = NULL;
   int         power = 1;
   int         value = 0;

   if( !address || !(*address))
      return -1;

   p = strchr( address, ',');
   if( !p)
      return -1;

   p--;
   if(((*p) < '0') || ('9' < (*p)))
      return -1;

   do
   {
      if( ((*p) < '0') || ('9' < (*p)))
         return value;

      value += (power * ((*p) - '0'));
      power *= 10;

   }  while( --p != address);

   return value;
}

static const char* get_city_name_from_address( const char* address)
{
   const char* p = NULL;

   if( !address || !(*address))
      return NULL;

   p = strchr( address, ',');
   if( !p)
      return address;

   while( (*p) && ((' '==(*p)) || (','==(*p)) || ('\t'==(*p))))
      p++;

   return p;
}

static const char* get_street_name_from_address( const char* address, char* street)
{
   int         size;
   const char* p = NULL;

   if( !address || !(*address) || !street)
      return NULL;

   (*street) = '\0';

   p = strchr( address, ',');
   if( !p)
      return NULL;

   p--;
   while( (p != address) && ((('0' <= (*p)) && ((*p) <= '9')) || (' '==(*p)) || ('\t'==(*p))))
      p--;

   if( p == address)
      return NULL;

   size = p - address + 1;

   if( ADDRESS_STREET_NAME_MAX_SIZE < size)
   {
      assert(0);
      return NULL;
   }

   strncpy( street, address, size);
   street[size] = '\0';

   return street;
}

static SsdWidget get_house_number_widget()
{
   atcctx_ptr  tc = get_atcctx();
   SsdWidget   tab= ssd_tabcontrol_get_tab( tc->tabcontrol, tab_house);

   return ssd_widget_get( tab, ATC_HOUSETAB_NM_EDIT);
}

static const char* get_house_number()
{
   SsdWidget house_number_widget = get_house_number_widget();

   return house_number_widget->value;
}

static void set_house_number( const char* number)
{
   SsdWidget house_number_widget = get_house_number_widget();

   house_number_widget->set_value( house_number_widget, number);
}

static void invalidate_house_number()
{  set_house_number( "");}

static void invalidate_street()
{
   atcctx_ptr  tc = get_atcctx();
   SsdWidget   tab= ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street);

   empty_list( tab);
   ssd_combobox_set_text( tab, "");

   invalidate_house_number();
}

static void invalidate_city()
{
   atcctx_ptr  tc = get_atcctx();
   SsdWidget   tab= ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city);

   empty_list( tab);
   ssd_combobox_set_text( tab, "");

   invalidate_street();

   s_viewing_history = FALSE;
}

static void invalidate_all()
{
   atcctx_ptr tc = get_atcctx();

   invalidate_city();
   ssd_tabcontrol_set_active_tab ( tc->tabcontrol, tab_city);
   ssd_tabcontrol_set_title      ( tc->tabcontrol, roadmap_lang_get(ATC_TITLE));
}

int on_city_found( RoadMapString index, const char* item, void* data)
{
   list_items_ptr li = data;

   if( ATC_LIST_MAX_SIZE == li->size)
      return 0;

   li->labels[li->size++] = strdup(item);
   return 1;
}

static void update_title()
{
   atcctx_ptr  tc    = get_atcctx();
   const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const char* street= ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
   const char* house = get_house_number();
   char        address[128];

   if( create_address_string( address, city, street, house) && address[0])
      ssd_tabcontrol_set_title( tc->tabcontrol, address);
   else
      ssd_tabcontrol_set_title( tc->tabcontrol, roadmap_lang_get(ATC_TITLE));
}

static void verify_child_data_is_valid( int tab)
{
   atcctx_ptr        tc = get_atcctx();
   atc_tab_info_ptr  ti = &(tc->tab_info[tab]);

   if( tab_street == tab)
   {
      const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));

      if( strlen(ti->last_ctx_val) && (0 != strcmp( city, ti->last_ctx_val)))
         invalidate_street();
   }
   else if( tab_house == tab)
   {
      const char* street = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));

      if( strlen(ti->last_ctx_val) && (0 != strcmp( street, ti->last_ctx_val)))
         invalidate_house_number();
   }
}

static void add_address_to_history( int category,
									const char* city,
                                    const char* street,
                                    const char* house,
                                    const char *name)
{
   const char* address[ahi__count];

   address[ahi_city]          = city;
   address[ahi_street]        = street;
   address[ahi_house_number]  = house;
   address[ahi_state]         = ADDRESS_HISTORY_STATE;
   if(name)
   	address[ahi_name]	= name;
   else
   	address[ahi_name]	= "";

   roadmap_history_add( category, address);
   roadmap_history_save();
}

static BOOL navigate( BOOL take_me_there)
{
   int               i;
   PluginLine        line;
   PluginStreet      street_info;
   int               count       = 0;
   RoadMapGeocode*   selections  = NULL;
   atcctx_ptr        tc          = get_atcctx();
   const char*       city        = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const char*       street      = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
   const char*       house       = get_house_number();


   count = roadmap_geocode_address( &selections,
                                    house,
                                    street,
                                    city,
                                    ADDRESS_HISTORY_STATE);
   if( count <= 0)
   {
      roadmap_log(ROADMAP_ERROR,
                  "address_tabcontrol::navigate() - 'roadmap_geocode_address()' returned %d", count);
      FREE( selections)

      roadmap_messagebox (roadmap_lang_get ("Warning"),
                          roadmap_geocode_last_error_string());
      return FALSE;
   }

   add_address_to_history(ADDRESS_HISTORY_CATEGORY, city, street, house, NULL);

   roadmap_locator_activate( selections->fips);
   roadmap_plugin_set_line( &line, ROADMAP_PLUGIN_ID, selections->line, -1,
                            selections->square, selections->fips);
   roadmap_trip_set_point ("Selection",&selections->position);
   roadmap_trip_set_point ("Address",  &selections->position);

   if( s_navigator && take_me_there)
   {
      address_info   ai;
      ai.state = NULL;
      ai.country = NULL;
      ai.city =city;
      ai.street = street;
      ai.house = house;

      if( -1 == s_navigator( &selections->position, &line, 0, &ai))
         return FALSE;
   }
   else
   {
      roadmap_trip_set_focus ("Address");
      roadmap_display_activate( "Selected Street", &line, &selections->position, &street_info);
      roadmap_screen_redraw ();
   }

   for( i=0; i<count; i++)
      FREE(selections[i].name)
   FREE(selections)

   return TRUE;
}

void on_text_changed__city( atc_tab_info_ptr ti, const char* new_text)
{
   if( new_text && (*new_text))
   {
      list_items_ptr  li = &(ti->items);

      s_viewing_history = FALSE;

      empty_list( ti->tab);

      roadmap_locator_search_city( new_text, on_city_found, li);

      if( !li->size)
         return;

      populate_list( ti->tab, li->size, (char **)li->labels, (void **)li->values);
   }
   else
      insert_history_into_city_list( FALSE);
}

int on_street_found( RoadMapString index, const char* item, void* data)
{
   atcctx_ptr     tc    = get_atcctx();
   list_items_ptr li    = data;
   const char*    city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const char*    street= NULL;
   char           buffer[ADDRESS_STREET_NAME_MAX_SIZE+1];

   if( ATC_LIST_MAX_SIZE == li->size)
      return 0;

   if( strlen(city))
      street = get_street_name_from_address( item, buffer);

   if( street)
      li->labels[li->size++] = strdup(street);
   else
      li->labels[li->size++] = strdup(item);

   return 1;
}

// If string terminate with a comma or spaces - remove them:
void remove_string_last_decorations( char* str)
{
   char* p;

   if( !str || !(*str))
      return;

   p = str + strlen(str);
   while( --p != str)
   {
      if( ((*p) != ',')&&((*p) != ' ')&&((*p) != '\t'))
         return;

      (*p) = '\0';
   }
}

void on_text_changed__street( atc_tab_info_ptr ti, const char* new_text)
{
   atcctx_ptr     tc    = get_atcctx();
   list_items_ptr li    = &(ti->items);
   const char*    city  = NULL;
   int            count = 0;
   int            i;

   empty_list( ti->tab);

   if( !new_text || !(*new_text))
   {
      insert_city_history_into_street_list();
      return;
   }

   city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   count = roadmap_street_search( city, new_text, ATC_LIST_MAX_SIZE, on_street_found, li);

   if( !count)
   {
      assert( !li->size);
      return;
   }

   assert(li->size);

   for( i=0; i<li->size; i++)
      remove_string_last_decorations( li->labels[i]);

   populate_list( ti->tab, li->size, li->labels, li->values);
}


void on_timer__on_text_changed(void)
{
   atc_tab_info_ptr tab_info = s_on_text_changed_ctx.context;

   roadmap_main_remove_periodic( on_timer__on_text_changed);

   switch( tab_info->id)
   {
      case tab_city:
         on_text_changed__city   ( tab_info, s_on_text_changed_ctx.new_text);
         verify_child_data_is_valid( tab_street);
         break;

      case tab_street:
         on_text_changed__street ( tab_info, s_on_text_changed_ctx.new_text);
         verify_child_data_is_valid( tab_house);
         break;

      default:
         assert(0);
   }

   update_title();
   roadmap_screen_redraw();
   on_text_changed_ctx_free( &s_on_text_changed_ctx);
}

void on_text_changed(const char* new_text, void* context)
{
   on_text_changed_ctx_copy( &s_on_text_changed_ctx, new_text, context);

   roadmap_main_remove_periodic( on_timer__on_text_changed);
   roadmap_main_set_periodic( 1200, on_timer__on_text_changed);
}


void distribute_address_sections_between_tabs(  const char* address,
                                                int         calling_tab)
{
   char        buffer[ADDRESS_STREET_NAME_MAX_SIZE + 1];
   const char* city  = get_city_name_from_address     ( address);
   const char* street= get_street_name_from_address   ( address, buffer);
   int         house = get_house_number_from_address  ( address);
   atcctx_ptr  tc    = get_atcctx();
   SsdWidget   tab   = NULL;

   tab = ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city);
   if( tab_city != calling_tab)
      empty_list(tab);
   if( city)
      ssd_combobox_set_text( tab, city);
   else
      ssd_combobox_set_text( tab, "");

   tab = ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street);
   if( tab_street != tab_street)
      empty_list(tab);
   tc->tab_info[tab_street].last_ctx_val[0] = '\0';
   if( street)
   {
      ssd_combobox_set_text( tab, street);

      if( city && (*city))
         strcpy( tc->tab_info[tab_street].last_ctx_val, city);
   }
   else
      ssd_combobox_set_text( tab, "");

   tab = ssd_tabcontrol_get_tab( tc->tabcontrol, tab_house);
   tc->tab_info[tab_house].last_ctx_val[0] = '\0';
   if( -1 != house)
   {
      if( street && (*street))
         strcpy( tc->tab_info[tab_house].last_ctx_val, street);

      sprintf( buffer, "%d", house);
   }
   else
      buffer[0] = '\0';

   set_house_number( buffer);
}

void on_timer__on_select(void)
{
   atcctx_ptr  tc = get_atcctx();

   roadmap_main_remove_periodic( on_timer__on_select);

   switch(tc->active_tab)
   {
      case tab_city:
      {
         const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));

         if( city && (*city))
         {
            ssd_tabcontrol_set_active_tab ( tc->tabcontrol, tab_street);
            roadmap_screen_redraw();
         }

         break;
      }

      case tab_street:
      {
         const char* street  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));

         if( street && (*street))
         {
            ssd_tabcontrol_set_active_tab ( tc->tabcontrol, tab_house);
            roadmap_screen_redraw();
         }

         break;
      }

      case tab_house:
         on_options( NULL, NULL, NULL);
         break;

      default:
         break;
   }
}

int on_list_selection_city(SsdWidget widget, const char *new_value, const void *value)
{
   if( !new_value || !(*new_value))
      return -1;

   distribute_address_sections_between_tabs(new_value,tab_city);
   update_title();

   if( s_viewing_history)
   {
      on_options( NULL, NULL, NULL);
   }
   else
   {
      roadmap_main_remove_periodic( on_timer__on_select);
      roadmap_main_set_periodic( 300, on_timer__on_select);
   }

   return 0;
}

int on_list_selection_street(SsdWidget widget, const char *new_value, const void *value)
{
   atcctx_ptr  tc = get_atcctx();
   SsdWidget   tab= ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street);

   if( !new_value || !(*new_value))
      return -1;

   if( strchr( new_value, ','))
      distribute_address_sections_between_tabs(new_value,tab_street);
   else
      ssd_combobox_set_text( tab, new_value);

   update_title();

   roadmap_main_remove_periodic( on_timer__on_select);
   roadmap_main_set_periodic( 500, on_timer__on_select);

   return 0;
}

void on_erase_history_item( int exit_code, void *context)
{
   if( dec_yes != exit_code)
      return;

   roadmap_history_delete_entry( context);
   insert_history_into_city_list( TRUE);
}
static void erase_history_entry()
{
   atcctx_ptr  tc       = get_atcctx();
   SsdWidget   list     = ssd_combobox_get_list( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const void* selected = ssd_list_selected_value ( list);
   const char* string   = ssd_list_selected_string( list);

   if( !selected || !string || !(*string))
      return;

   ssd_confirm_dialog(  string,
                        "Are you sure you want to remove item from history?",
                        FALSE,
                        on_erase_history_item,
                        (void*)selected);
}

BOOL on_tab_loose_focus(int tab)
{
   atcctx_ptr        tc = get_atcctx();
   atc_tab_info_ptr  ti = &(tc->tab_info[tab]);

   if( tab_street == tab)
   {
      const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
      if( strlen(city) < ADDRESS_STRING_MAX_SIZE)
         strcpy( ti->last_ctx_val, city);
   }
   else if( tab_house == tab)
   {
      const char* street = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
      if( strlen(street) < ADDRESS_STRING_MAX_SIZE)
         strcpy( ti->last_ctx_val, street);
   }

   return TRUE;
}

void on_tab_gain_focus(int tab)
{
   atcctx_ptr        tc = get_atcctx();

   tc->active_tab = tab;

   verify_child_data_is_valid( tab);

   if( tab_street == tab)
   {
      const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
      const char* street= ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));

      if( tc->tab_info[tab_street].list_is_empty && !(*street) && (*city))
         insert_city_history_into_street_list();
   }

   update_title();
}

int on_delete_history_item( SsdWidget widget, const char *new_value, const void *value)
{
   if( s_viewing_history)
   {
      erase_history_entry();
      return TRUE;
   }

   return FALSE;
}

static SsdWidget create_tab__city( atcctx_ptr tc)
{
   atc_tab_info_ptr  ti = (tc->tab_info + tab_city);

   ti->tab = ssd_container_new(
                     "citytabcontainer",
                     NULL,
                     SSD_MAX_SIZE,
                     SSD_MAX_SIZE,
                     0);

   ssd_combobox_new( ti->tab,
                     NULL,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     on_text_changed,        // User modified edit-box text
                     on_list_selection_city, // User selected iterm from list
                     on_delete_history_item, // User is trying to delete an item from the list
                     inputtype_alpha_spaces, // inputtype_<xxx> from 'roadmap_input_type.h'
                     ti);

   return ti->tab;
}

static SsdWidget create_tab__street( atcctx_ptr tc)
{
   atc_tab_info_ptr  ti = (tc->tab_info + tab_street);

   ti->tab = ssd_container_new(
                     "streettabcontainer",
                     NULL,
                     SSD_MAX_SIZE,
                     SSD_MAX_SIZE,
                     0);

   ssd_combobox_new( ti->tab,
                     NULL,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     on_text_changed,           // User modified edit-box text
                     on_list_selection_street,  // User selected iterm from list
                     NULL,                      // User is trying to delete an item from the list
                     inputtype_alpha_spaces,    // inputtype_<xxx> from 'roadmap_input_type.h'
                     ti);

   return ti->tab;
}

static BOOL on_key_pressed__delegate_to_editbox(
                                 SsdWidget   this,
                                 const char* utf8char,
                                 uint32_t    flags)
{
	SsdWidget   editbox = this->children;
	BOOL        handled = FALSE;

	if( editbox->key_pressed( editbox, utf8char, flags))
	{
	   handled = TRUE;
      update_title();
   }


   if( KEY_IS_ENTER)
   {
      on_options( NULL, NULL, NULL);
	   handled = TRUE;
	}

   return handled;
}

static SsdWidget create_tab__house( atcctx_ptr tc)
{
   SsdWidget            cont;
   SsdWidget            label;
   SsdWidget            ecnt;
   SsdWidget            edit;
   atc_tab_info_ptr ti = (tc->tab_info + tab_house);

   ti->tab = ssd_container_new(
                        ATC_HOUSETAB_NM_TAB,
                        NULL,
                        SSD_MAX_SIZE,
                        SSD_MAX_SIZE,
                        0);

   cont =  ssd_container_new(
                        ATC_HOUSETAB_NM_CONT,
                        NULL,
                        SSD_MAX_SIZE,
                        20,
                        SSD_END_ROW);
   label= ssd_text_new( ATC_HOUSETAB_NM_LABL,
                        roadmap_lang_get("House number"),
                        18,
                        SSD_TEXT_LABEL);
   ecnt =  ssd_container_new(
                        ATC_HOUSETAB_NM_ECNT,
                        NULL,
                        SSD_MAX_SIZE,
                        20,
                        SSD_CONTAINER_BORDER|SSD_WS_TABSTOP);
   edit = ssd_text_new( ATC_HOUSETAB_NM_EDIT,
                        "",
                        18,
                        0);

   ssd_text_set_input_type( edit, inputtype_numeric);
   ssd_text_set_readonly  ( edit, FALSE);

	//	Delegate the 'on key pressed' event to the child edit-box:
	ecnt->key_pressed = on_key_pressed__delegate_to_editbox;

	ssd_widget_add( ecnt, edit);
	ssd_widget_add( cont, label);
	ssd_widget_add( cont, ecnt);
	ssd_widget_add( ti->tab, cont);

   return ti->tab;
}

void on_tabcontrol_closed( int exit_code, void* context)
{
   atcctx_ptr tc = get_atcctx();

   if(tc->on_tabcontrol_closed)
      tc->on_tabcontrol_closed();

   list_items_free( &s_history);
}

// If tab cannot handle key pressed, move focus to tab default widget:
static BOOL on_unhandled_key_pressed(
               int         tab,
               const char* utf8char,
               uint32_t    flags)
{
   atcctx_ptr  tc             = get_atcctx();
   SsdWidget   active_tab     = tc->tab_info[tab].tab;
   SsdWidget   default_widget = NULL;

   switch(tab)
   {
      case tab_city:
      case tab_street:
         default_widget = ssd_combobox_get_textbox( active_tab)->parent;
         break;

      case tab_house:
         default_widget = ssd_widget_get( active_tab, ATC_HOUSETAB_NM_ECNT);
   }

   if( !default_widget || !ssd_dialog_set_focus( default_widget))
   {
      assert(0);
      return FALSE;
   }

   return default_widget->key_pressed( default_widget, utf8char, flags);
}

static BOOL keyboard_callback(  int         exit_code,
                                const char* value,
                                void*       context)
{
   atcctx_ptr  tc    = get_atcctx();
   const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const char* street= ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
   const char* house = get_house_number();


   if( dec_ok == exit_code)
   {
      roadmap_history_declare ('F', 7);
      add_address_to_history(ADDRESS_FAVORITE_CATEGORY, city, street, house, value);

      roadmap_history_save();
   }

   return TRUE;
}

static void add_to_favorites(int exit_code, void *data){

	if( dec_yes != exit_code)
      return;

   #if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN)) || defined(IPHONE)
    	ShowEditbox(roadmap_lang_get("Name"), "",
            keyboard_callback, (void *)data, EEditBoxStandard | EEditBoxAlphaNumeric );
	#else
      ssd_show_keyboard_dialog(  roadmap_lang_get("Name"),
                                 NULL,
                                 keyboard_callback,
                                 data);

	#endif

}

static void on_add_to_favorites(){
   atcctx_ptr  tc    = get_atcctx();
   const char* city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   const char* street= ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
   const char* house = get_house_number();
   char        address[128];
   create_address_string( address, city, street, house);

   ssd_confirm_dialog( address,
                        "Are you sure you want to Add item to favorites?",
                        FALSE,
                        add_to_favorites,
                        NULL);

}


static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   context_menu_items      selection = cmi__invalid;
   int                     exit_code = dec_ok;
   BOOL                    goto_map  = FALSE;
   s_context_menu_is_active          = FALSE;

   if( !made_selection)
      return;

   selection = item->id;

   switch( selection)
   {
      case cmi_navigate:
         goto_map = navigate(TRUE);
         break;

      case cmi_show:
         goto_map = navigate(FALSE);
         break;

      case cmi_erase_history_entry:
         erase_history_entry();
         break;

     case cmi_add_to_favorites:
     	on_add_to_favorites();
     	break;

     case cmi_exit:
         goto_map = TRUE;
         break;

      default:
         break;
   }

   if( goto_map)
   {
      if( cmi_exit == selection)
         exit_code = dec_cancel;

      hide_our_dialogs( exit_code);
      roadmap_screen_redraw ();
   }
}

#ifdef ADD_ERASE
static int on_erase_char(SsdWidget widget, const char *new_value, void *context)
{
   if( !s_context_menu_is_active)
   {
      static char  key[2];

      if( BACKSPACE != key[0])
      {
         key[0] = BACKSPACE;
         key[1] = '\0';
      }

      ssd_dialog_send_keyboard_event( key, KEYBOARD_ASCII);
   }

   return 0;
}
#endif

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
   atcctx_ptr  tc                = NULL;
   const char* city              = NULL;
   const char* street            = NULL;
   BOOL        can_navigate      = TRUE;
   int         menu_x;

   if(s_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_ok);
      s_context_menu_is_active = FALSE;
   }

   tc       = get_atcctx();
   city     = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   street   = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street));
   if( (!city || !(*city)) && (!street || !(*street)))
   {
      SsdWidget   active_tab = tc->tab_info[ tc->active_tab].tab;

      can_navigate = FALSE;

      if( tab_city == tc->active_tab)
      {
         SsdWidget   list     = ssd_combobox_get_list( active_tab);
         const char* selection= ssd_list_selected_string ( list);

         if( selection && (*selection))
         {
            distribute_address_sections_between_tabs(selection,tab_city);
            update_title();
            can_navigate = TRUE;
         }
      }
   }

   ssd_contextmenu_show_item( &context_menu,
                              cmi_navigate,
                              can_navigate,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_show,
                              can_navigate,
                              FALSE);
   ssd_contextmenu_show_item( &context_menu,
                              cmi_add_to_favorites,
                              can_navigate,
                              FALSE);

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default,
                           0);

   s_context_menu_is_active = TRUE;

   return 0;
}

static void set_softkeys( SsdWidget dialog)
{

#ifdef ADD_ERASE
   ssd_widget_set_right_softkey_text      ( dialog, roadmap_lang_get("Erase"));
   ssd_widget_set_right_softkey_callback  ( dialog, on_erase_char);
#endif
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_options);
}

static SsdTcCtx create_address_tabcontrol( atcctx_ptr tc)
{
   const char* tab_titles  [tab__count];
   SsdWidget   tab_conts   [tab__count];

   // Titles initialization
   tab_titles[tab_city]    = roadmap_lang_get("City");
   tab_titles[tab_street]  = roadmap_lang_get("Street");
   tab_titles[tab_house]   = roadmap_lang_get("House number");

   // Tabs initialization
   tab_conts[tab_city]     = create_tab__city(tc);
   tab_conts[tab_street]   = create_tab__street(tc);
   tab_conts[tab_house]    = create_tab__house(tc);


   tc->tabcontrol = ssd_tabcontrol_new(
                              ATC_NAME,
                              roadmap_lang_get(ATC_TITLE),
                              on_tabcontrol_closed,
                              on_tab_loose_focus,
                              on_tab_gain_focus,
                              on_unhandled_key_pressed,
                              tab_titles,
                              tab_conts,
                              tab__count,
                              0);

   if( !tc->tabcontrol)
   {
      roadmap_log(ROADMAP_ERROR,
                  "address_tabcontrol::create_address_tabcontrol() - 'ssd_tabcontrol_new' failed");
      return NULL;
   }

   set_softkeys( ssd_tabcontrol_get_dialog( tc->tabcontrol));

   return tc->tabcontrol;
}

static atcctx_ptr get_atcctx()
{
   static atcctx_ptr s_tc = NULL;
   if( s_tc)
      return s_tc;

   s_tc = malloc( sizeof(atcctx));
   atcctx_init( s_tc);

   on_text_changed_ctx_init( &s_on_text_changed_ctx);

   if( !create_address_tabcontrol( s_tc))
   {
      FREE(s_tc)
      return NULL;
   }

   return s_tc;
}

static void insert_city_history_into_street_list()
{
   atcctx_ptr     tc    = get_atcctx();
   list_items_ptr li    = &(tc->tab_info[tab_street].items);
   SsdWidget      street= ssd_tabcontrol_get_tab( tc->tabcontrol, tab_street);
   const char*    city  = ssd_combobox_get_text( ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city));
   void*          cursor= NULL;
   int            count = 0;

   if( !s_history_was_loaded)
   {
      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
      s_history_was_loaded = TRUE;
   }

   list_items_free(li);
   empty_list( street);

   cursor = roadmap_history_latest( ADDRESS_HISTORY_CATEGORY);
   if( !cursor)
      return;

   count = 0;
   while( (count++ < 500) && (li->size < ATC_LIST_MAX_SIZE))
   {
      char* last = cursor;
      char* address_items[ahi__count];

      roadmap_history_get( ADDRESS_HISTORY_CATEGORY, cursor, address_items);

      if( 0 == strncmp( address_items[ahi_city], city, strlen(city)))
      {
         li->labels[li->size] = strdup(address_items[ahi_street]);
         li->values[li->size] = cursor;
         li->size++;
      }

      cursor = roadmap_history_before( ADDRESS_HISTORY_CATEGORY, cursor);
      if( cursor == last)
         break;
   }

   populate_list( street, li->size, li->labels, li->values);
}

static void insert_history_into_city_list( BOOL force_reload)
{
   atcctx_ptr  tc    = get_atcctx();
   SsdWidget   city  = ssd_tabcontrol_get_tab( tc->tabcontrol, tab_city);
   void*       cursor= NULL;
   int         count = 0;
   int 		   i;
   BOOL 	   exist;

   s_viewing_history = FALSE;

   if( !s_history_was_loaded)
   {
      roadmap_history_declare( ADDRESS_HISTORY_CATEGORY, ahi__count);
      s_history_was_loaded = TRUE;
   }

   if( !force_reload && s_history.size)
   {
      populate_list( city, s_history.size, s_history.labels, s_history.values);
      s_viewing_history = TRUE;
      return;
   }

   cursor = roadmap_history_latest( ADDRESS_HISTORY_CATEGORY);
   if( !cursor)
      return;

   count = 0;
   while( count < ATC_LIST_MAX_SIZE)
   {
      char* last = cursor;
      char* address_items[ahi__count];

      roadmap_history_get( ADDRESS_HISTORY_CATEGORY, cursor, address_items);
      exist = FALSE;
      for (i = 0; i< count; i++){
      	if (!strcmp(s_history.labels[i], address_items[ahi_city])){
      		exist = TRUE;
      		break;
      	}
      }

	  if (!exist){
      	s_history.labels[count] = strdup(address_items[ahi_city]);
      	s_history.values[count] = cursor;
      	count++;
	  }

      cursor = roadmap_history_before( ADDRESS_HISTORY_CATEGORY, cursor);
      if( cursor == last)
         break;
   }

   s_history.size = count;

   populate_list( city, count, s_history.labels, s_history.values);

   s_viewing_history = FALSE;
}

void address_tabcontrol_show( RoadMapCallback on_tabcontrol_closed)
{
   // Create/Get tab-control:
   atcctx_ptr tc = get_atcctx();

   if( !tc)
   {
      assert(0);
      return;
   }

   tc->on_tabcontrol_closed= on_tabcontrol_closed;
   tc->active_tab          = tab_city;

   // Cancel any current navigation:
   navigate_main_stop_navigation();

   // Invalidate all data in the controls (maybe from previous usage)
   invalidate_all();

   // Populate with History:
   insert_history_into_city_list( FALSE);

   // Show the tab-control:
   ssd_tabcontrol_show( tc->tabcontrol);

   // If we have history - move focus to first item:
   if( s_history.size)
   {
      SsdWidget list = ssd_combobox_get_list( tc->tab_info[tab_city].tab);
      SsdWidget item = ssd_list_get_first_item( list);

      ssd_dialog_set_focus( item);
   }
}

void roadmap_address_history(void)
{ address_tabcontrol_show( NULL);}

#endif   // ~TOUCH_SCREEN
