/* ssd_icon_list.h - List view widget
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *
 */

#ifndef __SSD_WIDGET_ICON_LIST_H_
#define __SSD_WIDGET_ICON_LIST_H_
  
#include "ssd_widget.h"
typedef int (*SsdListCallback)      (SsdWidget widget, const char *new_value, const void *value);
typedef int (*SsdListDeleteCallback)(SsdWidget widget, const char *new_value, const void *value);

SsdWidget ssd_list_new( const char*             name,
                        int                     width,
                        int                     height,
                        roadmap_input_type      input_type,
                        int                     flags,
                        CB_OnWidgetKeyPressed   on_unhandled_key_press);

void ssd_list_populate( SsdWidget               list, 
                        int                     count, 
                        const char**            labels, 
                        const void**            values, 
                        const char**            icons, 
                        const int*              flags, 
                        SsdListCallback         callback, 
                        SsdListDeleteCallback   del_callback, 
                        BOOL                    add_next_button);

SsdWidget   ssd_list_get_first_item(SsdWidget  list);

void        ssd_list_taborder__set_widget_before_list  (SsdWidget list, SsdWidget      w);
void        ssd_list_taborder__set_widget_after_list   (SsdWidget list, SsdWidget      w);

SsdWidget   ssd_list_item_has_focus  ( SsdWidget list);
const char* ssd_list_selected_string ( SsdWidget list);
const void* ssd_list_selected_value  ( SsdWidget list);
BOOL        ssd_list_move_focus      ( SsdWidget list, BOOL up /* or down */);
BOOL        ssd_list_set_focus       ( SsdWidget list, BOOL first_item /* or last */);

void        ssd_list_resize(SsdWidget list, int min_height);

void ssd_list_populate_widgets (SsdWidget list, int count, const char **labels,
                        const void **values, SsdWidget *icons, const int *flags, SsdListCallback callback, SsdListDeleteCallback del_callback, BOOL add_next_button);

#endif // __SSD_ICON_WIDGET_LIST_H_
