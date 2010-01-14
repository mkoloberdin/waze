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


#ifndef __SSD_CONTEXT_MENU_H__
#define __SSD_CONTEXT_MENU_H__

#include "ssd_contextmenu_defs.h"

typedef enum tag_menu_open_direction
{
   dir_right,
   dir_left,
   dir_default
   
}  menu_open_direction;

typedef void(*SsdOnContextMenu)( BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context);
   
void ssd_context_menu_show(int                  x,
                           int                  y,
                           ssd_contextmenu_ptr  menu,
                           SsdOnContextMenu     on_menu_closed,
                           void*                context,
                           menu_open_direction  dir,
                           unsigned short       flags,
                           BOOL 				close_on_selection);

void ssd_context_menu_set_size( int size);
void close_all_popup_menus( ssd_contextmenu_ptr menu);
#define  SSD_CMDLG_DIALOG_NAME                  ("contextmenu_dialog")
#endif // __SSD_CONTEXT_MENU_H__
