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
#include "roadmap_animation.h"
#include "roadmap_time.h"

#include "roadmap_object.h"

enum states {
   idle,
   animate_in,
   animate_in_p2,
   animate_in_pending,
   animate_out
};


struct RoadMapObjectDescriptor {

   RoadMapDynamicString origin;

   RoadMapDynamicString id; /* Must be unique. */

   RoadMapDynamicString name; /* Name displayed, need not be unique. */

   RoadMapDynamicString sprite; /* Icon for the object on the map. */

   RoadMapDynamicString image; /* Icon for the object on the map. */

   RoadMapDynamicString text; /* text. */

   RoadMapGpsPosition position;

   RoadMapGuiPoint offset;

   RoadMapObjectListener listener;

   RoadMapObjectAction action;

   int animation;
   int animation_state;
   
   int min_zoom;
   int max_zoom;
   int scale;
   int opacity;
   
   RoadMapSize image_size;
   

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;

static BOOL short_click_enabled = TRUE;

static RoadMapObject *RoadmapObjectList = NULL;

static BOOL initialized = FALSE;

static RoadMapObject *roadmap_object_search (RoadMapDynamicString id);
static void animation_set_callback (void *context);
static void animation_ended_callback (void *context);
static uint32_t last_animation_time = 0;

static RoadMapAnimationCallbacks gAnimationCallbacks =
{
   animation_set_callback,
   animation_ended_callback
};


static void animation_set_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   int i;
   RoadMapObject *object = roadmap_object_search(roadmap_string_new(animation->object_id));
   
   if (object == NULL)
      return;
   
   for (i = 0; i < animation->properties_count; i++) {
      switch (animation->properties[i].type) {
         case ANIMATION_PROPERTY_SCALE:
            object->scale = animation->properties[i].current;
            break;
         case ANIMATION_PROPERTY_OPACITY:
            object->opacity = animation->properties[i].current;
            break;
         default:
            break;
      }
   }
}

static void animation_ended_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   RoadMapObject *object = roadmap_object_search(roadmap_string_new(animation->object_id));
   
   if (object && 
       (object->animation & OBJECT_ANIMATION_POP_IN) &&
       (object->animation_state == animate_in)) {
      object->animation_state = animate_in_p2;
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
         animation->properties_count = 1;
         animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
         animation->properties[0].from = 120;
         animation->properties[0].to = 100;
         animation->duration = 150;
         animation->timing = ANIMATION_TIMING_EASY_OUT;
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   } else if (object && 
             (object->animation & OBJECT_ANIMATION_POP_IN)) {
      object->animation_state = idle;
   }
}



void roadmap_object_disable_short_click(void){
   short_click_enabled = FALSE;
}

void roadmap_object_enable_short_click(void){
   short_click_enabled = TRUE;
}

BOOL roadmap_object_short_ckick_enabled(void){
   return short_click_enabled;
}
void roadmap_object_null_listener (RoadMapDynamicString id,
                                   const RoadMapGpsPosition *position) {}

void roadmap_object_null_monitor (RoadMapDynamicString id) {}


static RoadMapObjectMonitor RoadMapObjectFirstMonitor =
                                      roadmap_object_null_monitor;



static BOOL out_of_zoom(RoadMapObject *cursor) {
   return ((cursor->min_zoom != -1 && roadmap_math_get_zoom() < cursor->min_zoom) ||
           (cursor->max_zoom != -1 && roadmap_math_get_zoom() > cursor->max_zoom));
}

static BOOL is_visible(RoadMapObject *cursor) {
   RoadMapPosition position;
   BOOL visible;
   
   position.latitude = cursor->position.latitude;
   position.longitude = cursor->position.longitude;
   
   visible = !out_of_zoom(cursor) && roadmap_math_point_is_visible(&position);
   
#ifdef VIEW_MODE_3D_OGL
   if (roadmap_screen_get_view_mode() == VIEW_MODE_3D &&
       roadmap_canvas3_get_angle() > 0.8) {
      RoadMapGuiPoint object_coord;
      roadmap_math_coordinate(&position, &object_coord);
      roadmap_math_rotate_project_coordinate (&object_coord);

      if (1.0 * object_coord.y / roadmap_canvas_height() < 0.1) {
         visible = FALSE;
      }
   }
#endif
   
   return visible;
}

static RoadMapObject *roadmap_object_search (RoadMapDynamicString id) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {
      if (cursor->id == id) return cursor;
   }

   return NULL;
}

void set_animation (RoadMapObject *cursor) {
   uint32_t now = roadmap_time_get_millis();
   if (is_visible(cursor) && (cursor->animation & OBJECT_ANIMATION_POP_IN)) {
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         cursor->animation_state = animate_in;

         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         animation->properties_count = 2;
         
         //scale
         animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
         animation->properties[0].from = 20;
         animation->properties[0].to = 120;
         cursor->scale = 1;
         //opacity
         animation->properties[1].type = ANIMATION_PROPERTY_OPACITY;
         animation->properties[1].from = 100;
         animation->properties[1].to = 255;
         cursor->opacity = 1;
         
         //animation->is_linear = 1;
         animation->duration = 300;
         if (last_animation_time == 0)
            last_animation_time = now;
         if ((int)(now - last_animation_time) < 150) {
            animation->delay = 150 - (now - last_animation_time);
         }
         last_animation_time = now + animation->delay;
         animation->timing = ANIMATION_TIMING_EASY_IN;
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
   
   if (is_visible(cursor) && (cursor->animation & OBJECT_ANIMATION_FADE_IN)) {
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         cursor->animation_state = animate_in;
         
         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         animation->properties_count = 1;
         
         //opacity
         animation->properties[0].type = ANIMATION_PROPERTY_OPACITY;
         animation->properties[0].from = 1;
         animation->properties[0].to = 255;
         cursor->opacity = 1;
         
         //animation->is_linear = 1;
         animation->duration = 300;
         animation->timing = ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT;
         /*
          if (last_animation_time == 0)
          last_animation_time = now;
          if ((int)(now - last_animation_time) < 100) {
          animation->delay = 100 - (now - last_animation_time);
          }
          last_animation_time = now + animation->delay;
          */
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
}


void roadmap_object_add (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString      image,
                         const RoadMapGpsPosition *position,
                         const RoadMapGuiPoint    *offset,
                         int                       animation,
                         RoadMapDynamicString text) {

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
      cursor->text = text;
      cursor->animation = animation;
      if (offset) {
         cursor->offset = *offset;
      } else {
         cursor->offset.x = cursor->offset.y = 0;
      }

      cursor->listener = roadmap_object_null_listener;

      cursor -> action = NULL;
      cursor->min_zoom = -1;
      cursor->max_zoom = -1;
      cursor->scale = 100;
      cursor->opacity = 255;

      roadmap_string_lock(origin);
      roadmap_string_lock(id);
      roadmap_string_lock(name);
      roadmap_string_lock(sprite);
      roadmap_string_lock(image);
      roadmap_string_lock(text);

      cursor->previous  = NULL;
      cursor->next      = RoadmapObjectList;
      RoadmapObjectList = cursor;

      if (cursor->next != NULL) {
         cursor->next->previous = cursor;
      }

      if (position) {
         cursor->position = *position;
         (*cursor->listener) (id, position);
         
         set_animation(cursor);
         
         if (!is_visible(cursor) &&
             (animation & OBJECT_ANIMATION_WHEN_VISIBLE) &&
             ((animation & OBJECT_ANIMATION_POP_IN) ||
              (animation & OBJECT_ANIMATION_FADE_IN)))
             cursor->animation_state = animate_in_pending;
         else if (!is_visible(cursor))
            cursor->animation_state = idle;
         
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
      roadmap_string_release(cursor->text);
      roadmap_string_release(cursor->image);

      free (cursor);
   }
}


void roadmap_object_iterate (RoadMapObjectAction action) {

   RoadMapObject *cursor;
/*
   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 roadmap_string_get(cursor->image),
                 &(cursor->position),
                 is_visible(cursor),
                 cursor->scale,
                 roadmap_string_get(cursor->id));
   }
 */
   for (cursor = RoadmapObjectList; cursor->next != NULL; cursor = cursor->next);
   
   for ( ; cursor != NULL ; cursor = cursor->previous) {
      if (is_visible(cursor) &&
         cursor->animation_state == animate_in_pending)
         set_animation(cursor);
      
      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 roadmap_string_get(cursor->image),
                 &(cursor->position),
                 &(cursor->offset),
                 is_visible(cursor),
                 cursor->scale,
                 cursor->opacity,
                 roadmap_string_get(cursor->id),
                 roadmap_string_get(cursor->text));
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

	  if ((!cursor->image) ||
         (action_only && !cursor->action) ||
         out_of_zoom (cursor))
        continue;


      cursor_position.latitude = cursor->position.latitude;
      cursor_position.longitude = cursor->position.longitude;


      roadmap_math_coordinate(&cursor_position, &pos);
      roadmap_math_rotate_project_coordinate (&pos);

      image = roadmap_res_get (RES_BITMAP, RES_SKIN, roadmap_string_get(cursor->image));
      if (!image) continue;

      image_height = roadmap_canvas_image_height(image);
	  image_width = roadmap_canvas_image_width(image);

      if ((touched_point.x >= (pos.x - image_width/2 + cursor->offset.x)) &&
          (touched_point.x <= (pos.x + image_width/2 + cursor->offset.x)) &&
          (touched_point.y >= (pos.y - image_height/2 + cursor->offset.y)) &&
          (touched_point.y <= (pos.y + image_height/2 + cursor->offset.y))) {
               return cursor;
      }
   }

   return NULL;
}

static int roadmap_object_short_click (RoadMapGuiPoint *point) {

   RoadMapObject *object;

   if (!short_click_enabled){
      return 0;
   }

   object = roadmap_object_by_pos (point, TRUE);



   if (!object) {
      return 0;
   }

   if (object->action) {
      static RoadMapSoundList list;

#ifdef PLAY_CLICK
      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "click");
         roadmap_res_get (RES_SOUND, 0, "click");
      }
      roadmap_sound_play_list (list);
#endif //!IPHONE && TOUCH_SCREEN
      (*(object->action)) (roadmap_string_get(object->name),
                           roadmap_string_get(object->sprite),
                           roadmap_string_get(object->image),
                           &(object->position),
                           &(object->offset),
                           1,
                           object->scale,
                           object->opacity,
                           roadmap_string_get(object->id),
                           roadmap_string_get(object->text));
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

void roadmap_object_set_zoom (RoadMapDynamicString id, int min_zoom, int max_zoom) {
   RoadMapObject *object = roadmap_object_search (id);
   
   if (object == NULL) return;
   
   if (min_zoom != -1)
      object->min_zoom = min_zoom;
   if (max_zoom != -1)
      object->max_zoom = max_zoom;
}

