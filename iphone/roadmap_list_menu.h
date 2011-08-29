/* roadmap_list_menu.h - iPhone list menu
 *
 * LICENSE:
 *
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

#ifndef __ROADMAP_LIST_MENU__H
#define __ROADMAP_LIST_MENU__H

#include "roadmap_factory.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_generic_list_dialog.h"

#define LIST_MENU_NO_ACTION   -1
#define LIST_MENU_MAX_GROUPS  10

#define LIST_MENU_ADD_NEXT_BUTTON      0x01  //DisclosureIndicator
#define LIST_MENU_ADD_DETAIL_BUTTON    0x02  //DetailDisclosureButton
#define LIST_MENU_NO_ANIMATION         0x04  //push view without animation

typedef struct {
   int                  count;
   const char**         items;
   PFN_ON_ITEM_SELECTED on_selector;
   int                  default_index;
   
} list_menu_selector;

typedef struct {
   char              title[128];
   char              text[1024];
   char              top_image[128];
   char              bottom_image[128];
   char              button_text[128];
   RoadMapCallback   button_cb;
} list_menu_empty_message;

typedef char section_title[256];
typedef struct {
   int               count;
   int               items[LIST_MENU_MAX_GROUPS];
   section_title     titles[LIST_MENU_MAX_GROUPS];
   section_title     footers[LIST_MENU_MAX_GROUPS];
} list_menu_sections;

typedef struct {
   char*             text;
   char*             icon;
   SsdCallback       callback;
} list_menu_right_btn;


void roadmap_list_menu_simple (const char             *name,
                               const char             *items_file,
                               const char             *items[],
                               const char             *additional_item,
                               const char             *custom_labels[],
                               const char             *custom_icons[],
                               PFN_ON_DIALOG_CLOSED   on_dialog_closed,
                               const RoadMapAction    *actions,
                               int                    flags);


void roadmap_list_menu_generic_refresh (void*                  list,
                                        const char*            title,
                                        int                    count,
                                        const char**           labels,
                                        const void**           values,
                                        const char**           icons,
                                        const char**           right_labels,
                                        list_menu_selector*    selector,
                                        PFN_ON_ITEM_SELECTED   on_item_selected,
                                        PFN_ON_ITEM_SELECTED   on_item_deleted,
                                        void*                  context,
                                        SsdSoftKeyCallback     detail_button_callback,
                                        list_menu_right_btn*   right_button,
                                        int                    list_height,
                                        int                    flags,
                                        list_menu_empty_message* empty_message);

void* roadmap_list_menu_generic (const char*          title,
                                 int                  count,
                                 const char**         labels,
                                 const void**         values,
                                 const char** 		   icons,
                                 const char** 		   right_labels,
                                 list_menu_selector*  selector,
                                 PFN_ON_ITEM_SELECTED on_item_selected,
                                 PFN_ON_ITEM_SELECTED on_item_deleted,
                                 void*                context,
                                 SsdSoftKeyCallback   detail_button_callback,
                                 list_menu_right_btn* right_button,
                                 int                  list_height,
                                 int                  flags,
                                 list_menu_empty_message* empty_message);


#endif // __ROADMAP_LIST_MENU__H
