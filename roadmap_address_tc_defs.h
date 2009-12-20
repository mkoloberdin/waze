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

#include "ssd/ssd_tabcontrol.h"
#include "roadmap_address_tc.h"
#include "address_search/address_search.h"

// A.T.C == Address Tab-Control

#define  ATC_NAME                         ("address.search.tabcontrol")
#define  ATC_TITLE                        ("Search Address")
#define  ATC_LIST_MAX_SIZE                (25)
#define  ATC_HOUSETAB_NM_TAB              ("housetabcontainer")
#define  ATC_HOUSETAB_NM_CONT             ("housetabcontainer.container")
#define  ATC_HOUSETAB_NM_LABL             ("housetabcontainer.label")
#define  ATC_HOUSETAB_NM_ECNT             ("housetabcontainer.editcontainer")
#define  ATC_HOUSETAB_NM_EDIT             ("housetabcontainer.editbox")

#define  ADDRESS_STRING_MAX_SIZE          (112)
#define  ADDRESS_HISTORY_CATEGORY         ('A')
#define  ADDRESS_FAVORITE_CATEGORY        ('F')
#define  ADDRESS_HISTORY_STATE            ("IL")
#define  ADDRESS_STREET_NAME_MAX_SIZE     (112)

// Tabs:
typedef enum tag_tabid
{
   tab_city,
   tab_street,
   tab_house,
   
   tab__count,
   tab__invalid
   
}  tabid;

// Cached data of the ssd_list content   
//    (ssd_list does not cache its content...)
typedef struct tag_list_items
{
   char* labels[ATC_LIST_MAX_SIZE];
   void* values[ATC_LIST_MAX_SIZE];
   int   size;

}     list_items, *list_items_ptr;
void  list_items_init( list_items_ptr this);
void  list_items_free( list_items_ptr this);

// Single tab info:
typedef struct tag_atc_tab_info
{
   tabid       id;
   SsdWidget   tab;
   list_items  items;
   BOOL        list_is_empty;
   void*       parent;
   char        last_ctx_val[ADDRESS_STRING_MAX_SIZE+1];

}     atc_tab_info, *atc_tab_info_ptr;
void  atc_tab_info_init( atc_tab_info_ptr this, tabid id, void* parent);
void  atc_tab_info_free( atc_tab_info_ptr this);

// Module context:
typedef struct tag_atcctx
{
   atc_tab_info      tab_info[tab__count];
   tabid             active_tab;
   SsdTcCtx          tabcontrol;
   RoadMapCallback   on_tabcontrol_closed;
   
}     atcctx, *atcctx_ptr;
void  atcctx_init( atcctx_ptr this);
void  atcctx_free( atcctx_ptr this);

static void       insert_history_into_city_list(BOOL force_reload);
static void       insert_city_history_into_street_list();
static atcctx_ptr get_atcctx();
   
// Context menu:
typedef enum tag_context_menu_items
{
   cmi_navigate,
   cmi_show,
   cmi_add_to_favorites,
   cmi_erase_history_entry,
   cmi_exit,
   
   cmi__count,
   cmi__invalid

}  context_menu_items;
   
typedef struct tag_on_text_changed_ctx
{
   void* context;
   char* new_text;
   
}     on_text_changed_ctx, *on_text_changed_ctx_ptr;
void  on_text_changed_ctx_init( on_text_changed_ctx_ptr  this);
void  on_text_changed_ctx_free( on_text_changed_ctx_ptr  this);
void  on_text_changed_ctx_copy( on_text_changed_ctx_ptr  this,
                                const char*              new_text,
                                void*                    context);
