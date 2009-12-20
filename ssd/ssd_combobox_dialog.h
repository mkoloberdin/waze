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

#ifndef _SSD_COMBOBOX_DIALOG_H__
#define _SSD_COMBOBOX_DIALOG_H__

#include "ssd_combobox.h"

void ssd_combobox_dialog_show(
            const char*                title, 
            int                        count, 
            const char**               labels,
            const void**               values,
            const char**               icons,
            PFN_ON_INPUT_CHANGED       on_text_changed,
            PFN_ON_INPUT_DIALOG_CLOSED on_combobox_closed,
            SsdListCallback            on_list_selection,
            unsigned short             input_type,   //   txttyp_<xxx> combination from 'roadmap_input_type.h'
            void*                      context,
            const char*                left_softkey_text, 
            SsdSoftKeyCallback         left_softkey_callback );

void ssd_combobox_dialog_update_list(
            int                        count, 
            const char**               labels,
            const void**               values,
            const char**               icons);

SsdWidget   ssd_combobox_dialog_get_textbox();
void*       ssd_combobox_dialog_get_context();
SsdWidget   ssd_combobox_dialog_get_list();                              
const char* ssd_combobox_dialog_get_text();
void        ssd_combobox_dialog_set_text(const char* text);

#endif // _SSD_COMBOBOX_DIALOG_H__
