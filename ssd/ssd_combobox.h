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


#ifndef _SSD_COMBOBOX_H__
#define _SSD_COMBOBOX_H__

#include "ssd_dialog.h"
#include "ssd_list.h"

void ssd_combobox_new(  SsdWidget               main_cont,
                        const char*             title, 
                        int                     count, 
                        const char**            labels,
                        const void**            values,
                        const char**				icons,
                        PFN_ON_INPUT_CHANGED    on_text_changed,  // User modified edit-box text
                        SsdListCallback         on_list_selection,// User selected iterm from list
                        SsdListDeleteCallback   on_delete_list_item, // User is trying to delete an item from the list
                        unsigned short          input_type,       // inputtype_<xxx> combination from 'roadmap_input_type.h'
                        void*                   context);

// Release context pointer
void ssd_combobox_free( SsdWidget            main_cont); 

void ssd_combobox_update_list(
                        SsdWidget            main_cont,
                        int                  count, 
                        const char**         labels,
                        const void**         values,
                        const char**         icons);

// Reset sizes before re-using combobox
void ssd_combobox_reset_state(
                        SsdWidget            main_cont);

// Get edit-box text
const char* ssd_combobox_get_text(
                        SsdWidget            main_cont);

void        ssd_combobox_set_text(
                        SsdWidget            main_cont,
                        const char*          text);

SsdWidget   ssd_combobox_get_textbox(
                        SsdWidget            main_cont);

void*       ssd_combobox_get_context(
                        SsdWidget            main_cont);

SsdWidget   ssd_combobox_get_list(
                        SsdWidget            main_cont);
                        
                      

void        ssd_combobox_reset_focus(
                        SsdWidget            main_cont);
#endif // _SSD_COMBOBOX_H__

