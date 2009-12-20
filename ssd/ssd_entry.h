/* ssd_entry.h - Text entry widget
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

#ifndef __SSD_WIDGET_ENTRY_H_
#define __SSD_WIDGET_ENTRY_H_


#include "ssd_keyboard_dialog.h"

SsdWidget ssd_entry_new (const char *name, const char *value, int entry_flags,
                         int text_flags, int width, int height, const char *background_text);

SsdWidget ssd_confirmed_entry_new (const char *name,const char *value, int entry_flags,
                                  int text_flags, int width, int height, const char *messagebox_text,
                                  const char *background_text);

void ssd_entry_set_kb_params( SsdWidget wisget, const char* kb_title,
							  const char* kb_label, const char* kb_note,
							  CB_OnKeyboardDone kb_post_done_cb, int kb_flags );

void ssd_entry_set_kb_title( SsdWidget widget, const char* kb_title );

#endif // __SSD_WIDGET_ENTRY_H_
