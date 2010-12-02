/* roadmap_search.h
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
#ifndef ROADMAP_SEARCH_H_
#define ROADMAP_SEARCH_H_

#include "address_search/address_search.h"
#include "ssd/ssd_widget.h"
//////////////////////////////////////////////////////////////////////////////////////////////////
// Context menu:
typedef enum search_menu_context_menu_items
{
   search_menu_navigate,
   search_menu_show,
   search_menu_properties,
   search_menu_delete_history,
   search_menu_delete,
   search_menu_exit,
   search_menu_cancel,
   search_menu_add_to_favorites,
   search_menu_add_geo_reminder,
   search_menu__count,
   search_menu__invalid

}  search_menu_context_menu_items;


void roadmap_search_menu(void);
SsdWidget create_quick_search_menu();
void search_menu_search_history(void);
void search_menu_search_favorites(void);
void search_menu_search_address(void);
void search_menu_single_search(void);
void search_menu_search_local(void);
void search_menu_my_saved_places(void);
void search_menu_geo_reminders(void);

#if defined (IPHONE) || defined (_WIN32) || defined(ANDROID)
void roamdmap_search_address_book(void);
void search_menu_set_local_search_attrs( void );
#endif
#endif /*ROADMAP_SEARCH_H_*/
