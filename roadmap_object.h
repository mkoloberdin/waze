/* roadmap_object.h - manage the roadmap moving objects.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module manages a dynamic list of objects to be displayed on the map.
 *   The objects are usually imported from external applications for which
 *   a RoadMap driver has been installed (see roadmap_driver.c).
 *
 *   These objects are dynamic and not persistent (i.e. these are not points
 *   of interest).
 *
 *   It is possible for a module to register a listener function for
 *   a specific object, i.e. to get a listener function to be called
 *   each time the object has moved. The registration will fail (return
 *   NULL) if the object could not be found. A valid listener address is
 *   always returned if the registration is successful.
 *
 *   It is also possible to monitor the creation and deletion of objects.
 *   Combined with the listener registration, this makes it possible for
 *   a module to listen to a specific object when it is created, or register
 *   again when it is deleted and then re-created.
 *
 *   Listeners and monitors must be linked to each other in a daisy-chain
 *   fashion, i.e. each listener (monitor) must call the listener (monitor)
 *   that was declared before itself.
 *
 *   This module is self-initializing and can be used at any time during
 *   the initialization of RoadMap.
 */

#ifndef INCLUDE__ROADMAP_OBJECT__H
#define INCLUDE__ROADMAP_OBJECT__H

#include "roadmap_string.h"
#include "roadmap_gps.h"
#include "roadmap_gui.h"

#define OBJECT_ANIMATION_POP_IN        0x1
#define OBJECT_ANIMATION_POP_OUT       0x2
#define OBJECT_ANIMATION_FADE_IN       0x4
#define OBJECT_ANIMATION_FADE_OUT      0x8
#define OBJECT_ANIMATION_WHEN_VISIBLE  0x10
#define OBJECT_ANIMATION_DROP_IN       0x20

#define OBJECT_PRIORITY_DEFAULT 0
#define OBJECT_PRIORITY_NORMAL  1
#define OBJECT_PRIORITY_HIGH    2
#define OBJECT_PRIORITY_HIGHEST 3

void roadmap_object_add (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString      image,
                         const RoadMapGpsPosition *position,
                         const RoadMapGuiPoint    *offset,
                         int                       animation,
                         RoadMapDynamicString text);

void roadmap_object_add_with_priority (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString      image,
                         const RoadMapGpsPosition *position,
                         const RoadMapGuiPoint    *offset,
                         int                       animation,
                         RoadMapDynamicString text,
                         int priority);
void roadmap_object_move (RoadMapDynamicString id,
                          const RoadMapGpsPosition *position);

void roadmap_object_remove (RoadMapDynamicString id);

void roadmap_object_start_glow (RoadMapDynamicString id, int max_duraiton);
void roadmap_object_stop_glow (RoadMapDynamicString id);

void roadmap_object_cleanup (RoadMapDynamicString origin);


typedef void (*RoadMapObjectAction) (const char *name,
                                     const char *sprite,
                                     const char *image,
                                     const RoadMapGpsPosition *gps_position,
                                     const RoadMapGuiPoint    *offset,
                                     BOOL       is_visible,
                                     int        scale,
                                     int        opacity,
                                     int        scale_y,
                                     const char *id,
                                     const char *text);

void roadmap_object_set_action (RoadMapDynamicString id,
                                RoadMapObjectAction action);
void roadmap_object_set_zoom (RoadMapDynamicString id, int min_zoom, int max_zoom);

void roadmap_object_iterate (RoadMapObjectAction action);


typedef void (*RoadMapObjectListener) (RoadMapDynamicString id,
                                       const RoadMapGpsPosition *position);

RoadMapObjectListener roadmap_object_register_listener
                           (RoadMapDynamicString id,
                            RoadMapObjectListener listener);


typedef void (*RoadMapObjectMonitor) (RoadMapDynamicString id);

RoadMapObjectMonitor roadmap_object_register_monitor
                           (RoadMapObjectMonitor monitor);

void roadmap_object_disable_short_click(void);
void roadmap_object_enable_short_click(void);
BOOL roadmap_object_short_ckick_enabled(void);
void roadmap_object_set_zoom (RoadMapDynamicString id, int min_zoom, int max_zoom);
BOOL roadmap_object_exists(RoadMapDynamicString id);

void roadmap_object_set_no_overlapping (RoadMapDynamicString id);
BOOL roadmap_object_overlapped(RoadMapDynamicString origin, RoadMapDynamicString   image, const RoadMapGpsPosition *position, const RoadMapGuiPoint    *offset);

void roadmap_object_set_scale_factor (RoadMapDynamicString id, int min_zoom, int scale);
#endif // INCLUDE__ROADMAP_OBJECT__H

