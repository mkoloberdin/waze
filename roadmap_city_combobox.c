
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

#include "roadmap.h"

#ifndef  TOUCH_SCREEN

#include "ssd/ssd_combobox_dialog.h"
#include "roadmap_locator.h"
#include "roadmap_lang.h"
#include "roadmap_input_type.h"
#include "roadmap_address_tc_defs.h"

#include "roadmap_city_combobox.h"

#define  CITYCB_TITLE                  ("Select City")

static   BOOL                          object_in_use     = FALSE;
static   PFN_ON_INPUT_DIALOG_CLOSED    caller_callback   = NULL;
static   BOOL                          first_time_round  = TRUE;
static   list_items                    city_list;

static void empty_list()
{
   ssd_combobox_dialog_update_list( 0, NULL, NULL, NULL);
   list_items_free( &city_list);
}

static int on_city_found( RoadMapString index, const char* item, void* data)
{
   if( ATC_LIST_MAX_SIZE == city_list.size)
      return 0;
   
   city_list.labels[city_list.size++] = strdup(item);
   return 1;
}

static void on_text_changed(const char* new_text, void* context)
{
   empty_list();

   if( new_text && (*new_text))
   {
      roadmap_locator_search_city( new_text, on_city_found, NULL);
      
      if( !city_list.size)
         return;
      
      ssd_combobox_dialog_update_list( city_list.size, (const char **)city_list.labels, (const void **)city_list.values, NULL);
   }      
}

static void on_combo_closed( int exit_code, const char* input, void* context)
{
   if(caller_callback)
   {
      caller_callback( exit_code, input, context);
      caller_callback= NULL;
   }
   
   object_in_use = FALSE;
}

void city_combobox_show( void* context, PFN_ON_INPUT_DIALOG_CLOSED on_combobox_closed)
{
   if(object_in_use)
   {
      roadmap_log(ROADMAP_ERROR, "city_combobox_show() - Object is already in use. Ignoring call");
      return;
   }
      
   if(first_time_round)
   {
      list_items_init( &city_list);
      first_time_round = FALSE;
   }
   else
      list_items_free( &city_list);
   
   caller_callback = on_combobox_closed;

   ssd_combobox_dialog_show(  roadmap_lang_get(CITYCB_TITLE),
                              0, 
                              NULL,
                              NULL,
                              NULL,
                              on_text_changed,
                              on_combo_closed,
                              NULL,
                              inputtype_alphabetic,   //   inputtype_<xxx> combination from 'roadmap_input_type.h'
                              NULL,
                              NULL,
                              NULL);
   object_in_use = TRUE;
}

#endif   // ~TOUCH_SCREEN
