/* ssd_menu.h - Icons menu
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

#ifndef __SSD_MENU_H_
#define __SSD_MENU_H_

#include "roadmap_factory.h"
#include "ssd_dialog.h"

void ssd_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        SsdWidget 			 addition_conatiner,
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags,
                        int                   animation);

void ssd_list_menu_activate (const char      *name,
                        const char           *items_file,
                        const char           *items[],
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags);

SsdWidget ssd_menu_new (const char           *name,
							   SsdWidget        	 addition_conatiner,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags);

SsdWidget ssd_menu_new_cb (const char           *name,
                        SsdWidget          addition_conatiner,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags,
                               PFN_ON_DIALOG_CLOSED on_dialog_closed);
void ssd_menu_hide (const char *name);
void ssd_menu_load_images(const char   *items_file, const RoadMapAction  *actions);
void ssd_menu_set_right_text(char *name, char *item, char *text);
void ssd_menu_set_label_long_text(char *name, char *item, const char *text);
void ssd_menu_set_item_icon( SsdWidget menu, const char* item_name, const char* icon_name );
#endif // __SSD_MENU_H_
