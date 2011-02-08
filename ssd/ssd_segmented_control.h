/*  ssd_segmented_control.h- buttons tabs
 *
 * LICENSE:
 *
 *   Copyright 2006 Avi Ben-Shoshan
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
#ifndef SSD_SEGMENTED_CONTROL_H_
#define SSD_SEGMENTED_CONTROL_H_

typedef int (*SsdSegmentedControlCallback)(SsdWidget widget, const char *new_value, void *context);

SsdWidget ssd_segmented_control_new (const char *name, int count, const char **labels,
                          const void **values,
                          int flags,
                          SsdSegmentedControlCallback *callback, void *context, int default_button);
void ssd_segmented_control_set_focus(SsdWidget tab, int item);

SsdWidget ssd_segmented_icon_control_new (const char *name, int count, const char **labels,
                          const void **values, const char **button_icons,
                          int flags,
                          SsdSegmentedControlCallback *callback, void *context, int default_button);

#endif /* SSD_SEGMENTED_CONTROL_H_ */
