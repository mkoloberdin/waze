/* roadmap_object.c - manage the roadmap moving objects.
 *
 * LICENSE:
 *
 *   Copyright 2005 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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
 *   The implementation is not terribly optimized: there should not be too
 *   many objects.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_pointer.h"
#include "roadmap_math.h"
#include "roadmap_canvas.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_sound.h"
#include "roadmap_screen.h"

#include "roadmap_object.h"


struct RoadMapObjectDescriptor {

   RoadMapDynamicString origin;

   RoadMapDynamicString id; /* Must be unique. */

   RoadMapDynamicString name; /* Name displayed, need not be unique. */

   RoadMapDynamicString sprite; /* Icon for the object on the map. */
   
   RoadMapDynamicString image; /* Icon for the object on the map. */

   RoadMapGpsPosition position;

   RoadMapObjectListener listener;
   
   RoadMapObjectAction action;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;


static RoadMapObject *RoadmapObjectList = NULL;

static BOOL initialized = FALSE;



void roadmap_object_null_listener (RoadMapDynamicString id,
                                   const RoadMapGpsPosition *position) {}

void roadmap_object_null_monitor (RoadMapDynamicString id) {}


static RoadMapObjectMonitor RoadMapObjectFirstMonitor =
                                      roadmap_object_null_monitor;


static RoadMapObject *roadmap_object_search (RoadMapDynamicString id) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {
      if (cursor->id == id) return cursor;
   }

   return NULL;
}


void roadmap_object_add (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString image) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) {

      cursor = calloc (1, sizeof(RoadMapObject));
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return;
      }
      cursor->origin = origin;
      cursor->id     = id;
      cursor->name   = name;
      cursor->sprite = sprite;
      cursor->image = image;

      cursor->listener = roadmap_object_null_listener;
      
      cursor -> action = NULL;

      roadmap_string_lock(origin);
      roadmap_string_lock(id);
      roadmap_string_lock(name);
      roadmap_string_lock(sprite);
      roadmap_string_lock(image);

      cursor->previous  = NULL;
      cursor->next      = RoadmapObjectList;
      RoadmapObjectList = cursor;

      if (cursor->next != NULL) {
         cursor->next->previous = cursor;
      }

      (*RoadMapObjectFirstMonitor) (id);
   }
}


void roadmap_object_move (RoadMapDynamicString id,
                          const RoadMapGpsPosition *position) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      if ((cursor->position.longitude != position->longitude) ||
          (cursor->position.latitude  != position->latitude)  ||
          (cursor->position.altitude  != position->altitude)  ||
          (cursor->position.steering  != position->steering)  ||
          (cursor->position.speed     != position->speed)) {

         cursor->position = *position;
         (*cursor->listener) (id, position);
      }
   }
}


void roadmap_object_remove (RoadMapDynamicString id) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      if (cursor->next != NULL) {
          cursor->next->previous = cursor->previous;
      }
      if (cursor->previous != NULL) {
         cursor->previous->next = cursor->next;
      } else {
         RoadmapObjectList = cursor->next;
      }

      roadmap_string_release(cursor->origin);
      roadmap_string_release(cursor->id);
      roadmap_string_release(cursor->name);
      roadmap_string_release(cursor->sprite);
      
      roadmap_string_release(cursor->image);
      
      free (cursor);
   }
}


void roadmap_object_iterate (RoadMapObjectAction action) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 roadmap_string_get(cursor->image),
                 &(cursor->position),
                 roadmap_string_get(cursor->id));
   }
}


void roadmap_object_cleanup (RoadMapDynamicString origin) {

   RoadMapObject *cursor;
   RoadMapObject *next;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = next) {

      next = cursor->next;

      if (cursor->origin == origin) {
         roadmap_object_remove (cursor->id);
      }
   }
}


RoadMapObjectListener roadmap_object_register_listener
                           (RoadMapDynamicString id,
                            RoadMapObjectListener listener) {

   RoadMapObjectListener previous;
   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) return NULL;

   previous = cursor->listener;
   cursor->listener = listener;

   return previous;
}


RoadMapObjectMonitor roadmap_object_register_monitor
                           (RoadMapObjectMonitor monitor) {

   RoadMapObjectMonitor previous = RoadMapObjectFirstMonitor;

   RoadMapObjectFirstMonitor = monitor;
   return previous;
}

static RoadMapObject *roadmap_object_by_pos (RoadMapGuiPoint *point, BOOL action_only) {
   
   RoadMapObject *cursor;
   RoadMapGuiPoint touched_point = *point;
   
   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {
	  int image_height;
	  int image_width;
	  RoadMapPosition cursor_position;
      RoadMapGuiPoint pos;
      RoadMapImage image;

	  if (!cursor->image) continue;
      if (action_only && !cursor->action) continue;
      
      
      cursor_position.latitude = cursor->position.latitude;
      cursor_position.longitude = cursor->position.longitude;
      

      roadmap_math_coordinate(&cursor_position, &pos);
      roadmap_math_rotate_coordinates (1, &pos);

      image = roadmap_res_get (RES_BITMAP, RES_SKIN, roadmap_string_get(cursor->image));
      if (!image) continue;
      
      image_height = roadmap_canvas_image_height(image);
	  image_width = roadmap_canvas_image_width(image);
      
      if ((touched_point.x >= (pos.x - image_width/2)) &&
          (touched_point.x <= (pos.x + image_width/2)) &&
          (touched_point.y >= (pos.y - image_height/2)) &&
          (touched_point.y <= (pos.y + image_height/2))) {
               return cursor;
      }
   }
   
   return NULL;
}

static int roadmap_object_short_click (RoadMapGuiPoint *point) {
   
   RoadMapObject *object = roadmap_object_by_pos (point, TRUE);
   
   if (!object) {
      return 0;
   }
   
   if (object->action) {
      static RoadMapSoundList list;
      
      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "click");
         roadmap_res_get (RES_SOUND, 0, "click");
      }
#ifndef IPHONE
      roadmap_sound_play_list (list);
#endif //IPHONE
      (*(object->action)) (roadmap_string_get(object->name),
                           roadmap_string_get(object->sprite),
                           roadmap_string_get(object->image),
                           &(object->position),
                           roadmap_string_get(object->id));
      roadmap_screen_touched();
   } 
   
   return 1;
}

void roadmap_object_set_action (RoadMapDynamicString id,
                                RoadMapObjectAction action) {
   RoadMapObject *object = roadmap_object_search (id);
   
   if (object == NULL) return;
   
   if (action) {
      object->action = action;
      if (!initialized) {
         roadmap_pointer_register_short_click
            (roadmap_object_short_click, POINTER_NORMAL);
         roadmap_pointer_register_enter_key_press
            (roadmap_object_short_click, POINTER_HIGH);   
         initialized = TRUE;
      }
   }
}
