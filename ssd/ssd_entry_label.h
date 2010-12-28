/* ssd_entry_label.h - Complex text entry widget with label ( including scale adjustment for various display types )
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA),  Waze Ltd.
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

#ifndef __SSD_WIDGET_ENTRY_LABEL_H_
#define __SSD_WIDGET_ENTRY_LABEL_H_

#include "ssd_widget.h"

/********************************************************************************************
 * Interface
 */
SsdWidget ssd_entry_label_new ( const char *name, const char* label_text, int label_font,
                              int label_width, int height, int flags, const char *background_text );
SsdWidget ssd_entry_label_new_confirmed( const char *name, const char* label_text, int label_font, int label_width,
                                                         int height, int flags, const char *background_text, const char *messagebox_text );
void ssd_entry_label_set_entry_flags( SsdWidget widget, int entry_flags );
void ssd_entry_label_set_label_flags( SsdWidget widget, int label_flags );
void ssd_entry_label_set_text_flags( SsdWidget widget, int text_flags );

SsdWidget ssd_entry_label_get_entry( SsdWidget entry_label );

void ssd_entry_label_set_label_color( SsdWidget widget, const char* color );

void ssd_entry_label_set_label_offset( SsdWidget widget, int offset );

void ssd_entry_label_set_kb_params( SsdWidget widget, const char* kb_title, const char* kb_label,
                                    const char* kb_note, CB_OnKeyboardDone kb_post_done_cb, int kb_flags );

void ssd_entry_label_set_kb_title( SsdWidget widget, const char* kb_title );

void ssd_entry_label_set_editbox_title( SsdWidget widget, const char* editbox_title );

/********************************************************************************************
 * Defines
 */

/*
 *  Children's names
 */
#define SSD_ENTRY_LABEL_LBL_CNT               "Label container"
#define SSD_ENTRY_LABEL_LBL                   "Label"
#define SSD_ENTRY_LABEL_ENTRY_CNT             "Entry container"
#define SSD_ENTRY_LABEL_ENTRY                 "Entry"
#define SSD_ENTRY_LABEL_OFFSET_CNT            "Offset container"
/*
 *  Default values
 */
#define SSD_ENTRY_LABEL_SEP_SPACE              5       // Space between label and entry box
#define SSD_ENTRY_LABEL_RIGHT_MARGIN           5       // Right margin space


#endif // __SSD_WIDGET_ENTRY_LABEL_H_
