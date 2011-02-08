/* ssd_button.h - Button widget
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

#ifndef __SSD_WIDGET_BUTTON_H_
#define __SSD_WIDGET_BUTTON_H_

#include "../roadmap_canvas.h"
#include "ssd_widget.h"

#define SSD_BUTTON_SHORT_CLICK "short_click"
#define SSD_BUTTON_LONG_CLICK  "long_click"

SsdWidget ssd_button_new (const char *name, const char *value,
                          const char **bitmap_names, int num_bitmaps,
                          int flags, SsdCallback callback);

SsdWidget ssd_button_label (const char *name, const char *label,
                            int flags, SsdCallback callback);
SsdWidget ssd_button_label_custom (const char *name, const char *label,
                            int flags, SsdCallback callback, const char **button_icon, int num_bitmaps, const char *txt_color, const char *txt_focus_color, int txt_size);

int ssd_button_change_icon( SsdWidget widget, const char **bitmap_names, int num_bitmaps );
int ssd_button_change_images( SsdWidget widget, RoadMapImage* bitmap_images, const char **bitmap_names, int num_bitmaps );
void ssd_button_change_text(SsdWidget this, const char * newText); // sets the text of the label
const char *ssd_button_get_name(SsdWidget widget);

void ssd_button_enable(SsdWidget widget);
void ssd_button_disable(SsdWidget widget);
#endif // __SSD_WIDGET_BUTTON_H_

