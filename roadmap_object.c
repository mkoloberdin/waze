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
#include "roadmap_canvas3d.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_sound.h"
#include "roadmap_screen.h"
#include "animation/roadmap_animation.h"
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

   RoadMapGuiPoint orig_offset;

   RoadMapObjectListener listener;

   RoadMapObjectAction action;

   int animation;
   int animation_state;
   int glow; /* -1 = disabled; 0 - 100 = scale */

   int min_zoom;
   int max_zoom;
   int scale;
   int opacity;
   int priority;
   int scale_y;

   RoadMapSize image_size;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;

   BOOL   check_overlapping;
	int   scale_factor;
	int   scale_factor_min_zoom;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;

static BOOL short_click_enabled = TRUE;

static RoadMapObject *RoadmapObjectList = NULL;

static BOOL initialized = FALSE;
static RoadMapObject *roadmap_object_by_pos (RoadMapGuiPoint *point, BOOL action_only, RoadMapObject *cursor);

static RoadMapObject *roadmap_object_search (RoadMapDynamicString id);
#ifdef OPENGL
static void animation_set_callback (void *context);
static void animation_ended_callback (void *context);
static void glow_animation_set_callback (void *context);
static void glow_animation_ended_callback (void *context);
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
         case ANIMATION_PROPERTY_SCALE_Y:
            object->scale_y = animation->properties[i].current;
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

      RoadMapAnimation *animation = roadmap_animation_create();
      object->animation_state = animate_in_p2;
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

static RoadMapAnimationCallbacks gGlowAnimationCallbacks =
{
   glow_animation_set_callback,
   glow_animation_ended_callback
};


static void glow_animation_set_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   int i;
   char object_name[256];
   RoadMapObject *object;

   strncpy_safe(object_name, animation->object_id, strlen(animation->object_id)-5); //remove "__glow"
   object = roadmap_object_search(roadmap_string_new(object_name));

   if (object == NULL)
      return;

   for (i = 0; i < animation->properties_count; i++) {
      switch (animation->properties[i].type) {
         case ANIMATION_PROPERTY_SCALE:
            object->glow = animation->properties[i].current;
            break;
         default:
            break;
      }
   }
}

static void glow_animation_ended_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   char object_name[256];
   RoadMapObject *object;

   strncpy_safe(object_name, animation->object_id, strlen(animation->object_id)-5); //remove "__glow"
   object = roadmap_object_search(roadmap_string_new(object_name));

   if (object == NULL)
      return;

   object->glow = -1;
}


#endif

BOOL roadmap_object_exists(RoadMapDynamicString id) {
   if (roadmap_object_search (id) == NULL)
      return FALSE;
   else
      return TRUE;
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

#ifdef OPENGL
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
		 cursor->scale_y = 100;

         animation->duration = 300;
         if (last_animation_time == 0)
            last_animation_time = now;
         if ((int)(now - last_animation_time) < 150) {
            animation->delay = 150 - (now - last_animation_time);
         }
         last_animation_time = now + animation->delay;
         animation->timing = 0;//ANIMATION_TIMING_EASY_IN;
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
		 cursor->scale_y = 100;
         //animation->is_linear = 1;
         animation->duration = 300;
         animation->timing = 0;// ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT;
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

   if (is_visible(cursor) && (cursor->animation & OBJECT_ANIMATION_DROP_IN)) {
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         cursor->animation_state = animate_in;

         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         animation->properties_count = 1;

         //scale_y
         animation->properties[0].type = ANIMATION_PROPERTY_SCALE_Y;
         animation->properties[0].from = 0;
         animation->properties[0].to = 100;
         cursor->opacity = 255;
         cursor->scale = 100;
		   cursor->scale_y = 0;
         //animation->is_linear = 1;
         animation->duration = 350;
         animation->timing = ANIMATION_TIMING_EASY_IN | ANIMATION_TIMING_EASY_OUT;

//          if (last_animation_time == 0)
//          last_animation_time = now;
//          if ((int)(now - last_animation_time) < 100) {
//          animation->delay = 100 - (now - last_animation_time);
//          }
//          last_animation_time = now + animation->delay;

         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
}
#endif


static RoadMapObject *object_insert_sort (RoadMapObject *list_head, RoadMapObject *new_obj) {
   RoadMapObject *prev = list_head;
   RoadMapObject *curr = list_head;

   if (list_head == NULL || new_obj->priority >= list_head->priority) {
      new_obj->next = list_head;
      new_obj->previous = NULL;
      if (new_obj->next != NULL) {
         new_obj->next->previous = new_obj;
      }
      return new_obj;
   } else {
      while (curr && curr->priority > new_obj->priority) {
         prev = curr;
         curr = curr->next;
      }

      prev-> next = new_obj;
      new_obj->previous = prev;
      new_obj->next = curr;
      if (curr)
         curr->previous = new_obj;
      return list_head;
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


    roadmap_object_add_with_priority(origin,id,name,sprite,image,position,offset,animation,text,OBJECT_PRIORITY_DEFAULT);
}

BOOL overlapping(RoadMapObject *cursor, BOOL move) {
   RoadMapObject *object;
   RoadMapGuiPoint pos;
   RoadMapImage image;
   RoadMapGuiPoint point;
   RoadMapPosition cursor_position;
   int image_width, image_height;
   RoadMapPosition overlapped_position;
   RoadMapGuiPoint overlapped_pos;

   if (!cursor )
      return FALSE;

   image = roadmap_res_get (RES_BITMAP, RES_SKIN, roadmap_string_get(cursor->image));
   if (!image)
      return FALSE;

   image_height = roadmap_canvas_image_height(image);
   image_width = roadmap_canvas_image_width(image);


   cursor_position.latitude = cursor->position.latitude;
    cursor_position.longitude = cursor->position.longitude;


    roadmap_math_coordinate(&cursor_position, &pos);
    roadmap_math_rotate_project_coordinate (&pos);
    point.x = pos.x - image_width/2 + cursor->offset.x + image_width/10;
    point.y = pos.y - image_height/2 + cursor->offset.y + image_height/10;
    object = roadmap_object_by_pos (&point, TRUE, cursor);
    if (!object){
       point.x = pos.x + image_width/2 + cursor->offset.x - image_width/10;
       point.y = pos.y + image_height/2 + cursor->offset.y - image_height/10;
       object = roadmap_object_by_pos (&point, TRUE, cursor);
       if (!object){
          point.x = pos.x - image_width/2 + cursor->offset.x + image_width/10;
          point.y = pos.y + image_height/2 + cursor->offset.y - image_height/10;
          object = roadmap_object_by_pos (&point, TRUE, cursor);
          if (!object){
             point.x = pos.x + image_width/2 + cursor->offset.x - image_width/10;
             point.y = pos.y - image_height/2 + cursor->offset.y + image_height/10;
             object = roadmap_object_by_pos (&point, TRUE, cursor);
             if (!object){
                return FALSE;
             }
             else{
                if (move){
                   overlapped_position.latitude = object->position.latitude;
                   overlapped_position.longitude = object->position.longitude;
                   roadmap_math_coordinate(&overlapped_position, &overlapped_pos);
                   roadmap_math_rotate_project_coordinate (&overlapped_pos);
                   if (overlapped_pos.x+object->offset.x >  (pos.x + cursor->offset.x- image_width/10))
                      cursor->offset.x -=  2;
                   if (overlapped_pos.y+object->offset.y <  (pos.y + cursor->offset.y + image_height/10))
                      cursor->offset.y +=  2;
                }
                return TRUE;
             }
          }
          else{
             if (move){
                overlapped_position.latitude = object->position.latitude;
                overlapped_position.longitude = object->position.longitude;
                roadmap_math_coordinate(&overlapped_position, &overlapped_pos);
                roadmap_math_rotate_project_coordinate (&overlapped_pos);
                if (overlapped_pos.x+object->offset.x <  (pos.x + cursor->offset.x + image_width/10))
                   cursor->offset.x +=  2;
                if (overlapped_pos.y+object->offset.y >  (pos.y + cursor->offset.y - image_height/10))
                   cursor->offset.y -=  2;
             }
             return TRUE;
          }
       }
       else{
          if (move){
             overlapped_position.latitude = object->position.latitude;
             overlapped_position.longitude = object->position.longitude;
             roadmap_math_coordinate(&overlapped_position, &overlapped_pos);
             roadmap_math_rotate_project_coordinate (&overlapped_pos);
             if (overlapped_pos.x+object->offset.x >  (pos.x + cursor->offset.x - image_width/10))
                cursor->offset.x -=  2;
             if (overlapped_pos.y+object->offset.y >  (pos.y + cursor->offset.y - image_height/10))
                cursor->offset.y -=  2;
          }
          return TRUE;
       }
    }
    else{
       if (move){
          overlapped_position.latitude = object->position.latitude;
          overlapped_position.longitude = object->position.longitude;
          roadmap_math_coordinate(&overlapped_position, &overlapped_pos);
          roadmap_math_rotate_project_coordinate (&overlapped_pos);
          if (overlapped_pos.x+object->offset.x <  (pos.x + cursor->offset.x + image_width/10))
             cursor->offset.x +=  2;
          if (overlapped_pos.y+object->offset.y <  (pos.y  +cursor->offset.y+ image_height/10))
             cursor->offset.y +=  2;
       }
       return TRUE;
    }

}

BOOL roadmap_object_overlapped(RoadMapDynamicString origin, RoadMapDynamicString   image, const RoadMapGpsPosition *position, const RoadMapGuiPoint    *offset){
   RoadMapObject cursor;
   BOOL overlapped;
   cursor.origin = origin;
   cursor.position = *position;
   cursor.image = image;
   if (offset)
      cursor.offset = *offset;

   overlapped =  overlapping(&cursor, FALSE);

   return overlapped;
}


void roadmap_object_add_with_priority (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString      image,
                         const RoadMapGpsPosition *position,
                         const RoadMapGuiPoint    *offset,
                         int                       animation,
                         RoadMapDynamicString text,
                         int priority) {

   RoadMapObject *cursor;

   if (!roadmap_object_exists(id)) {

      cursor = calloc (1, sizeof(RoadMapObject));
      roadmap_check_allocated(cursor);

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
         cursor->orig_offset = *offset;
      } else {
         cursor->offset.x = cursor->offset.y = 0;
         cursor->orig_offset.x = cursor->orig_offset.y = 0;
      }

      cursor->listener = roadmap_object_null_listener;

      cursor->action = NULL;

      cursor->min_zoom = -1;
      cursor->max_zoom = -1;
      cursor->priority = priority;
      cursor->scale = 100;
	  cursor->scale_factor = 100;
      cursor->scale_y = 100;
      cursor->opacity = 255;
      cursor->glow = -1;
      if (position)
          cursor->position = *position;
      cursor->check_overlapping = FALSE;

      roadmap_string_lock(origin);
      roadmap_string_lock(id);
      roadmap_string_lock(name);
      roadmap_string_lock(sprite);
      roadmap_string_lock(image);
      roadmap_string_lock(text);

      RoadmapObjectList = object_insert_sort(RoadmapObjectList, cursor);
//      cursor->previous  = NULL;
//      cursor->next      = RoadmapObjectList;
//      RoadmapObjectList = cursor;
//      if (cursor->next != NULL) {
//         cursor->next->previous = cursor;
//      }

      if (position) {
         (*cursor->listener) (id, position);

#ifdef OPENGL
         set_animation(cursor);

         if (!is_visible(cursor) &&
             (animation & OBJECT_ANIMATION_WHEN_VISIBLE) &&
             ((animation & OBJECT_ANIMATION_POP_IN) ||
              (animation & OBJECT_ANIMATION_FADE_IN)))
             cursor->animation_state = animate_in_pending;
         else if (!is_visible(cursor))
            cursor->animation_state = idle;

#endif
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

#ifdef OPENGL
void roadmap_object_start_glow (RoadMapDynamicString id, int max_duraiton) {
   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL && cursor->glow == -1) {
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         strcat(animation->object_id, "__glow");
         animation->properties_count = 1;

         //glow scale
         animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
         animation->properties[0].from = 1;
         animation->properties[0].to = 100;
         cursor->glow = 1;
         animation->duration = 2000;
         animation->loops = max_duraiton*1000/animation->duration;
         animation->callbacks = &gGlowAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
}

void roadmap_object_stop_glow (RoadMapDynamicString id) {
   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL && cursor->glow >= 0) {
      RoadMapAnimation *animation = roadmap_animation_create();

      if (animation) {
         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         strcat(animation->object_id, "__glow");
         roadmap_animation_cancel(animation);
         cursor->glow = -1;
      }
   }
}
#endif

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

void check_overlapping(RoadMapObject *cursor){

//   BOOL overlapped = overlapping(cursor, FALSE);
//   int count = 0;
//   if (overlapped){
//      while (overlapping(cursor, FALSE) && (count < 10)){
//         count++;
//      }
//   }
}

void roadmap_object_iterate (RoadMapObjectAction action) {

   RoadMapObject *cursor;
   RoadMapGuiPoint offset;
   int scale_factor;

   for (cursor = RoadmapObjectList; cursor->next != NULL; cursor = cursor->next);

   for ( ; cursor != NULL ; cursor = cursor->previous) {
#ifdef OPENGL
      if (is_visible(cursor) &&
          cursor->animation_state == animate_in_pending)
         set_animation(cursor);

      if (cursor->glow >= 0) {
         int glow = cursor->glow;
         RoadMapGuiPoint offset;
         offset.x = 3 * 100 / glow;
         offset.y = -27 * 100 / glow;
         (*action) ("",
                    "",
                    "object_glow",
                    &(cursor->position),
                    //&(cursor->offset),
                    &offset,
                    is_visible(cursor),
                    glow,
                    255*(101 - glow)/100,
                    100,
                    "",
                    NULL);
         glow += 50;
         if (glow > 100)
            glow -= 100;
         offset.x = 3 * 100 / glow;
         offset.y = -27 * 100 / glow;
         (*action) ("",
                    "",
                    "object_glow",
                    &(cursor->position),
                    //&(cursor->offset),
                    &offset,
                    is_visible(cursor),
                    glow,
                    255*(101 - glow)/100,
                    100,
                    "",
                    NULL);
      }
#endif

      if (cursor->check_overlapping){
         check_overlapping(cursor);
      }

	  if  (cursor->scale_factor_min_zoom != 0 && roadmap_math_get_zoom() >= cursor->scale_factor_min_zoom)
		  scale_factor = cursor->scale_factor;
	  else
		  scale_factor = 100;

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 roadmap_string_get(cursor->image),
                 &(cursor->position),
                 &(cursor->offset),
                 is_visible(cursor),
                 cursor->scale*scale_factor/100,
                 cursor->opacity,
                 cursor->scale_y,
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

static RoadMapObject *roadmap_object_by_pos (RoadMapGuiPoint *point, BOOL action_only, RoadMapObject *org_cursor) {

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
             if (org_cursor){
                if ((cursor != org_cursor) && (org_cursor->origin == cursor->origin))
                   return cursor;
                else
                   continue;
             }
             else
                return cursor;
      }
   }

   return NULL;
}

static int roadmap_object_short_click (RoadMapGuiPoint *point) {

   RoadMapObject *object;
	int scale_factor;

   if (!short_click_enabled){
      return 0;
   }

   object = roadmap_object_by_pos (point, TRUE, NULL);



   if (!object) {
      return 0;
   }

   if (object->action) {
#ifdef PLAY_CLICK
      static RoadMapSoundList list;

      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "click");
         roadmap_res_get (RES_SOUND, 0, "click");
      }
      roadmap_sound_play_list (list);
#endif //!IPHONE && TOUCH_SCREEN

	   if  (object->scale_factor_min_zoom != 0 && roadmap_math_get_zoom() >= object->scale_factor_min_zoom)
		   scale_factor = object->scale_factor;
	   else
		   scale_factor = 100;

      (*(object->action)) (roadmap_string_get(object->name),
                           roadmap_string_get(object->sprite),
                           roadmap_string_get(object->image),
                           &(object->position),
                           &(object->offset),
                           1,
                           object->scale*scale_factor/100,
                           object->opacity,
                           object->scale_y,
                           roadmap_string_get(object->id),
                           roadmap_string_get(object->text));
      //roadmap_screen_touched();
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

void roadmap_object_set_scale_factor (RoadMapDynamicString id,
									  int min_zoom,
                                int scale) {
	RoadMapObject *object = roadmap_object_search (id);

	if (object == NULL) return;
	object->scale_factor_min_zoom = min_zoom;
	object->scale_factor = scale;

}

void roadmap_object_set_zoom (RoadMapDynamicString id, int min_zoom, int max_zoom) {
   RoadMapObject *object = roadmap_object_search (id);

   if (object == NULL) return;

   if (min_zoom != -1)
      object->min_zoom = min_zoom;
   if (max_zoom != -1)
      object->max_zoom = max_zoom;
}

void roadmap_object_set_no_overlapping (RoadMapDynamicString id) {
   RoadMapObject *object = roadmap_object_search (id);

   if (object == NULL) return;

   object->check_overlapping = TRUE;
}
