
/* ssd_keyboard_layout.h
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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

#ifndef __SSD_KEYBOARD_LAYOUT_H__
#define __SSD_KEYBOARD_LAYOUT_H__

#include "../roadmap.h"
#include "ssd_keyboard_layout_defs.h"
#include "ssd_widget.h"

typedef BOOL(*CB_OnKeyboardButtonPressed) ( void* context, const char*  utf8char, uint32_t flags);
typedef void(*CB_OnKeyboardCommand)       ( void* context, const char*  command);
typedef void(*CB_OnDrawBegin)             ( void* context, int layout_index);
typedef void(*CB_OnDrawButton)            ( void* context, int layout_index, int button_index);
typedef void(*CB_OnDrawEnd)               ( void* context, int layout_index);
typedef BOOL(*CB_OnApproveValueType)   ( void* context, int index_candidate);

SsdWidget ssd_keyboard_layout_create(
                        // SSD usage:
                        const char*                name,
                        int                        flags,
                        // Layout(s) info:
                        buttons_layout_info*       layouts[],
                        int                        layouts_count,
                        // This module:
                        CB_OnKeyboardButtonPressed cbOnKey,
                        CB_OnKeyboardCommand       cbOnCommand,
                        CB_OnDrawBegin             cbOnDrawBegin,
                        CB_OnDrawButton            cbOnDrawButton,
                        CB_OnDrawEnd               cbOnDrawEnd,
                        CB_OnApproveValueType      cbOnApproveValueType,
                        void*                      context);

void ssd_keyboard_layout_reset(     SsdWidget   kb);

/* Used only by 'ssd_keyboard.c'
int  ssd_keyboard_get_value_index(  SsdWidget   kb);
void ssd_keyboard_set_value_index(  SsdWidget   kb,
                                    int         index);
void ssd_keyboard_increment_value_index(
                                    SsdWidget   kb);

int  ssd_keyboard_get_layout(       SsdWidget   kb);
void ssd_keyboard_set_layout(       SsdWidget   kb,
                                    int         layout);*/

BOOL ssd_keyboard_update_button_data(
                              SsdWidget      kb,
                              const char*    button_name,
                              const char*    new_values,
                              unsigned long  new_flags);

void* ssd_keyboard_get_context( SsdWidget   kb);

#endif // __SSD_KEYBOARD_LAYOUT_H__
