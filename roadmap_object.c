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
#include "roadmap_lang.h"

#include "roadmap_object.h"

enum states {
   idle,
   animate_in,
   animate_in_p2,
   animate_in_p3,
   animate_in_p4,
   animate_in_p5,
   animate_in_p6,
   animate_in_p7,
   animate_in_p8,
   animate_in_p9,
   animate_in_pending,
   animate_out
};


#define MAX_OBJ_IMAGES  10
#define MAX_OBJ_TEXTS   4


struct RoadMapObjectDescriptor {

   RoadMapDynamicString origin;

   RoadMapDynamicString id; /* Must be unique. */

   RoadMapDynamicString name; /* Name displayed, need not be unique. */

   RoadMapDynamicString sprite; /* Icon for the object on the map. */

   RoadMapDynamicString images[MAX_OBJ_IMAGES]; /* Icon for the object on the map. */
   int                  image_count;
   
   RoadMapDynamicString bg_image;
   
   ObjectText           texts[MAX_OBJ_TEXTS]; /* text. */
   int                  text_count;

   RoadMapGpsPosition position;

   RoadMapGuiPoint offset;

   RoadMapGuiPoint orig_offset;

   RoadMapObjectListener listener;

   RoadMapObjectAction action;

   long animation;
   int animation_state;
   int glow; /* -1 = disabled; 0 - 100 = scale */

   int min_zoom;
   int max_zoom;
   int scale;
   int opacity;
   int priority;
   int scale_y;
   int rotation;

   RoadMapSize image_size;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;
   struct RoadMapObjectDescriptor *child;
   
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
static void release_object (RoadMapObject *cursor);
static void roadmap_object_add_child (RoadMapDynamicString  parent_id,
                                      RoadMapDynamicString  id,
                                      RoadMapDynamicString  image,
                                      const RoadMapGuiPoint *offset,
                                      long                   animation,
                                      RoadMapDynamicString  text);

#ifdef OPENGL
static void animation_set_callback (void *context);
static void animation_ended_callback (void *context);
static void glow_animation_set_callback (void *context);
static void glow_animation_ended_callback (void *context);
static uint32_t last_animation_time = 0;



static const char *get_vip_text (int flags) {
   if (flags & OBJECT_VIP_MEGA_MAPPER) {
      return "Mega Mapper";
   } else if (flags & OBJECT_VIP_MEGA_MAPPER) {
      return "Mega Mapper";
   } else if (flags & OBJECT_VIP_MEGA_DRIVER) {
      return "Mega Driver";
   } else if (flags & OBJECT_VIP_MEGA_REPORTER) {
      return "Mega Reporter";
   } else if (flags & OBJECT_VIP_BETA_TESTER) {
      return "Beta Tester";
   } else if (flags & OBJECT_VIP_WAZE_HERO) {
      return "Waze Hero";
   } else if (flags & OBJECT_VIP_1M_POINTS) {
      return "1M Points";
   } else {
      return "";
   }
}

static RoadMapAnimationCallbacks gAnimationCallbacks =
{
   animation_set_callback,
   animation_ended_callback
};


static void animation_set_callback (void *context) {
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   int i;
   RoadMapDynamicString OBJ_STRING = roadmap_string_new(animation->object_id);
   RoadMapObject *object = roadmap_object_search(OBJ_STRING);
   roadmap_string_release(OBJ_STRING);

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
         case ANIMATION_PROPERTY_ROTATION:
            object->rotation = animation->properties[i].current;
            break; 
         default:
            break;
      }
   }
}

static void animation_ended_callback (void *context) {
   char *vip_text, *pos;
   RoadMapAnimation *animation = (RoadMapAnimation *)context;
   RoadMapDynamicString OBJ_STRING = roadmap_string_new(animation->object_id);
   RoadMapObject *object = roadmap_object_search(OBJ_STRING);
   roadmap_string_release(OBJ_STRING);

   if (object && object->animation & OBJECT_ANIMATION_POP_IN) {
      if (object->animation_state == animate_in) {
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
      } else {
            object->animation_state = idle;
      }
   } else if (object && object->animation & OBJECT_ANIMATION_VIP) {
      RoadMapAnimation *animation = NULL;
      RoadMapDynamicString child_id = NULL;
      RoadMapDynamicString image_id = NULL;
      RoadMapDynamicString text_id = NULL;
      char child_id_txt[256];
      RoadMapGuiPoint offset = {ADJ_SCALE(-20),ADJ_SCALE(-20)};

      switch (object->animation_state) {
         case animate_in:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p2;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[0].from = 15;
               animation->properties[0].to = -15;
               animation->duration = 300;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p2:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p3;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[0].from = -15;
               animation->properties[0].to = 15;
               animation->duration = 300;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p3:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p4;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[0].from = 15;
               animation->properties[0].to = -15;
               animation->duration = 300;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p4:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p5;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[0].from = -15;
               animation->properties[0].to = 0;
               animation->duration = 150;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p5:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p6;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE_Y;
               animation->properties[0].from = 100;
               animation->properties[0].to = 70;
               animation->duration = 500;
               animation->timing = 0;
               animation->delay = 500;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p6:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p7;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[0].from = 1;
               animation->properties[0].to = 360;
               animation->duration = 500;
               animation->timing = 0;
               animation->loops = 2;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p7:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p8;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 2;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE_Y;
               animation->properties[0].from = 70;
               animation->properties[0].to = 101;
               animation->properties[1].type = ANIMATION_PROPERTY_ROTATION;
               animation->properties[1].from = 1;
               animation->properties[1].to = 360;
               animation->duration = 500;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;   
         case animate_in_p8:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p9;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE_Y;
               animation->properties[0].from = 101;
               animation->properties[0].to = 100;
               animation->duration = 100;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p9:
            object->animation_state = idle;
            
            snprintf(child_id_txt, sizeof(child_id_txt), "%s_child", roadmap_string_get(object->id));
            child_id = roadmap_string_new( child_id_txt);
            image_id = roadmap_string_new("vip_star");
            
            roadmap_object_add_child(object->id, child_id, image_id, &offset,
                                     (object->animation | OBJECT_ANIMATION_VIP_STAR) &
                                     ~OBJECT_ANIMATION_VIP &
                                     ~OBJECT_ANIMATION_WHEN_FULLY_VISIBLE,
                                     text_id);
            
            offset.y -= ADJ_SCALE(2);
            vip_text = strdup(roadmap_lang_get(get_vip_text(object->animation)));
            pos = strchr(vip_text, ' ');
            if (pos) {
               pos[0] = 0;
            }
            text_id = roadmap_string_new(vip_text);
            roadmap_object_add_text(child_id, text_id, 8, FONT_TYPE_BOLD, &offset, "#000000", "#000000ff");
            roadmap_string_release(text_id);
            
            if (pos) {
               offset.y += ADJ_SCALE(8);
               text_id = roadmap_string_new(pos +1);
               roadmap_object_add_text(child_id, text_id, 8, FONT_TYPE_BOLD, &offset, "#000000", "#000000ff");
               roadmap_string_release(text_id);
            }
            
            roadmap_string_release(child_id);
            roadmap_string_release(image_id);
            break;
         default:
            object->animation_state = idle;
            break;
      }
   } else if (object && object->animation & OBJECT_ANIMATION_VIP_STAR) {
      RoadMapAnimation *animation;
      switch (object->animation_state) {
         case animate_in:  
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p2;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
               animation->properties[0].from = 120;
               animation->properties[0].to = 100;
               animation->duration = 150;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p2:
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p3;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
               animation->properties[0].from = 100;
               animation->properties[0].to = 115;
               animation->duration = 120;
               animation->timing = 0;
               animation->delay = 50;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p3:  
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p4;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 1;
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
               animation->properties[0].from = 115;
               animation->properties[0].to = 100;
               animation->duration = 120;
               animation->timing = 0;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         case animate_in_p4:  
            animation = roadmap_animation_create();
            object->animation_state = animate_in_p5;
            if (animation) {
               strncpy_safe(animation->object_id, roadmap_string_get(object->id), ANIMATION_MAX_OBJECT_LENGTH);
               animation->properties_count = 2;
               //scale
               animation->properties[0].type = ANIMATION_PROPERTY_SCALE;
               animation->properties[0].from = 100;
               animation->properties[0].to = 0;
               //opacity
               animation->properties[1].type = ANIMATION_PROPERTY_OPACITY;
               animation->properties[1].from = 255;
               animation->properties[1].to = 100;
               
               animation->duration = 300;
               animation->timing = 0;
               animation->delay = 5000;
               animation->callbacks = &gAnimationCallbacks;
               roadmap_animation_register(animation);
            }
            break;
         default:
            object->animation_state = idle;
            break;
      }
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
   RoadMapDynamicString OBJ_STRING;

   strncpy_safe(object_name, animation->object_id, strlen(animation->object_id)-5); //remove "__glow"
   OBJ_STRING = roadmap_string_new(object_name);
   object = roadmap_object_search(OBJ_STRING);
   roadmap_string_release(OBJ_STRING);

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
   RoadMapDynamicString OBJ_STRING;

   strncpy_safe(object_name, animation->object_id, strlen(animation->object_id)-5); //remove "__glow"
   OBJ_STRING = roadmap_string_new(object_name);
   object = roadmap_object_search(OBJ_STRING);
   roadmap_string_release(OBJ_STRING);

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

static BOOL is_fully_visible(RoadMapObject *cursor) {
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
      
      if (1.0 * object_coord.y / roadmap_canvas_height() < 0.5) {
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
      if (cursor->child && cursor->child->id == id) return cursor->child;
   }

   return NULL;
}

#ifdef OPENGL
void set_animation (RoadMapObject *cursor) {
   uint32_t now = roadmap_time_get_millis();
   cursor->animation_state = idle;
   
   if (cursor->animation & OBJECT_ANIMATION_POP_IN) {
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
         animation->timing = 0;
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }

   if (cursor->animation & OBJECT_ANIMATION_FADE_IN) {
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
         animation->duration = 300;
         animation->timing = 0;
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }

   if (cursor->animation & OBJECT_ANIMATION_DROP_IN) {
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
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
   
   if (cursor->animation & OBJECT_ANIMATION_VIP) {
      RoadMapAnimation *animation = roadmap_animation_create();
      if (animation) {
         cursor->animation_state = animate_in;
         
         strncpy_safe(animation->object_id, roadmap_string_get(cursor->id), ANIMATION_MAX_OBJECT_LENGTH);
         animation->properties_count = 1;
         
         //rotation
         animation->properties[0].type = ANIMATION_PROPERTY_ROTATION;
         animation->properties[0].from = 0;
         animation->properties[0].to = 15;                  
         animation->duration = 150;
         animation->timing = 0;
         animation->callbacks = &gAnimationCallbacks;
         roadmap_animation_register(animation);
      }
   }
   
   if (cursor->animation & OBJECT_ANIMATION_VIP_STAR) {
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
         animation->timing = 0;
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
                         long                     animation,
                         RoadMapDynamicString text) {


    roadmap_object_add_with_priority(origin,id,name,sprite,image,position,offset,animation,text,OBJECT_PRIORITY_DEFAULT);
}

void roadmap_object_add_image (RoadMapDynamicString id, RoadMapDynamicString image) {
   RoadMapObject *cursor = roadmap_object_search (id);
   
   if (cursor != NULL && cursor->image_count < MAX_OBJ_IMAGES) {
      cursor->images[cursor->image_count++] = image;
      roadmap_string_lock(image);
   }
}

void roadmap_object_add_text (RoadMapDynamicString id,
                              RoadMapDynamicString text,
                              int                  size,
                              int                  font_flags,
                              RoadMapGuiPoint      *offset,
                              const char           *fg_color,
                              const char           *bg_color) {
   RoadMapObject *cursor = roadmap_object_search (id);
   
   if (cursor != NULL && cursor->text_count < MAX_OBJ_TEXTS) {
      cursor->texts[cursor->text_count].text = text;
      cursor->texts[cursor->text_count].size = size;
      cursor->texts[cursor->text_count].font_flags = font_flags;
      cursor->texts[cursor->text_count].offset = *offset;
      strncpy_safe (cursor->texts[cursor->text_count].fg_color, fg_color, sizeof(cursor->texts[cursor->text_count].fg_color));
      strncpy_safe (cursor->texts[cursor->text_count].bg_color, bg_color, sizeof(cursor->texts[cursor->text_count].bg_color));
      
      roadmap_string_lock(text);
      cursor->text_count++;
   }
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

   image = roadmap_res_get (RES_BITMAP, RES_SKIN, roadmap_string_get(cursor->images[0]));
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
   cursor.images[0] = image;
   if (offset)
      cursor.offset = *offset;

   overlapped =  overlapping(&cursor, FALSE);

   return overlapped;
}

static RoadMapObject *create_init_object () {
   RoadMapObject *cursor = calloc (1, sizeof(RoadMapObject));
   roadmap_check_allocated(cursor);
   
   cursor->listener = roadmap_object_null_listener;   
   cursor->min_zoom = -1;
   cursor->max_zoom = -1;
   cursor->priority = OBJECT_PRIORITY_DEFAULT;
   cursor->scale = 100;
   cursor->scale_factor = 100;
   cursor->scale_y = 100;
   cursor->opacity = 255;
   cursor->glow = -1;
   cursor->check_overlapping = FALSE;
   cursor->rotation = 0;
   
   return cursor;
}

void roadmap_object_add_with_priority (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite,
                         RoadMapDynamicString      image,
                         const RoadMapGpsPosition *position,
                         const RoadMapGuiPoint    *offset,
                         long                       animation,
                         RoadMapDynamicString text,
                         int priority) {

   RoadMapObject *cursor;

   if (!roadmap_object_exists(id)) {
      cursor = create_init_object();
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return;
      }
      
      cursor->origin = origin;
      cursor->id     = id;
      cursor->name   = name;
      cursor->sprite = sprite;
      if (image) {
         cursor->images[0] = image;
         cursor->image_count = 1;
      }
      if (text) {
         cursor->texts[0].text = text;
         strncpy_safe (cursor->texts[0].bg_color, "#000000", sizeof(cursor->texts[0].bg_color));
         strncpy_safe (cursor->texts[0].fg_color, "#ffffff", sizeof(cursor->texts[0].fg_color));
         cursor->texts[0].offset.x = cursor->texts[0].offset.y = 0;
         cursor->texts[0].font_flags = FONT_TYPE_BOLD|FONT_TYPE_OUTLINE;
         cursor->texts[0].size = 12;
         cursor->text_count = 1;
      }
      cursor->animation = animation;
      if (offset) {
         cursor->offset = *offset;
         cursor->orig_offset = *offset;
      } else {
         cursor->offset.x = cursor->offset.y = 0;
         cursor->orig_offset.x = cursor->orig_offset.y = 0;
      }
      cursor->priority = priority;
      if (position)
          cursor->position = *position;

      roadmap_string_lock(origin);
      roadmap_string_lock(id);
      roadmap_string_lock(name);
      roadmap_string_lock(sprite);
      roadmap_string_lock(image);
      roadmap_string_lock(text);

      RoadmapObjectList = object_insert_sort(RoadmapObjectList, cursor);

      if (position) {
#ifdef OPENGL
         if (!is_visible(cursor) &&
             (animation & OBJECT_ANIMATION_WHEN_VISIBLE))
            cursor->animation_state = animate_in_pending;
         else if (!is_fully_visible(cursor) &&
                  (animation & OBJECT_ANIMATION_WHEN_FULLY_VISIBLE))
            cursor->animation_state = animate_in_pending;
         else
            set_animation(cursor);
#endif
      }

      (*RoadMapObjectFirstMonitor) (id);
   }
}

static void roadmap_object_add_child (RoadMapDynamicString  parent_id,
                                      RoadMapDynamicString  id,
                                      RoadMapDynamicString  image,
                                      const RoadMapGuiPoint *offset,
                                      long                   animation,
                                      RoadMapDynamicString  text) {
   //We currently support single child. prev child will be removed
   
   RoadMapObject *parent_object = roadmap_object_search (parent_id);
   
   if (parent_object != NULL) {
      RoadMapObject *cursor = create_init_object();
      
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return;
      }
      
      if (parent_object->child) {
         release_object(parent_object->child);
      }
      parent_object->child = cursor;
      
      cursor->id     = id;
      if (image) {
         cursor->images[0] = image;
         cursor->image_count = 1;
      }
      if (text) {
         cursor->texts[0].text = text;
         strncpy_safe (cursor->texts[0].bg_color, "#000000", sizeof(cursor->texts[0].bg_color));
         strncpy_safe (cursor->texts[0].fg_color, "#ffffff", sizeof(cursor->texts[0].fg_color));
         cursor->texts[0].offset.x = cursor->texts[0].offset.y = 0;
         cursor->texts[0].font_flags = FONT_TYPE_BOLD|FONT_TYPE_OUTLINE;
         cursor->texts[0].size = 12;
         cursor->text_count = 1;
      }
      cursor->animation = animation;
      if (offset) {
         cursor->offset = *offset;
         cursor->orig_offset = *offset;
      } else {
         cursor->offset.x = cursor->offset.y = 0;
         cursor->orig_offset.x = cursor->orig_offset.y = 0;
      }      
      
      roadmap_string_lock(id);
      roadmap_string_lock(image);
      roadmap_string_lock(text);
      
#ifdef OPENGL
      if (!is_visible(parent_object) &&
          (animation & OBJECT_ANIMATION_WHEN_VISIBLE))
         cursor->animation_state = animate_in_pending;
      else if (!is_fully_visible(parent_object) &&
               (animation & OBJECT_ANIMATION_WHEN_FULLY_VISIBLE))
         cursor->animation_state = animate_in_pending;
      else
         set_animation(cursor);
#endif
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


static void release_object (RoadMapObject *cursor) {
   int i;
   
   roadmap_string_release(cursor->origin);
   roadmap_string_release(cursor->id);
   roadmap_string_release(cursor->name);
   roadmap_string_release(cursor->sprite);
   for (i = 0; i < cursor->image_count; i++) {
      roadmap_string_release(cursor->images[i]);
   }
   for (i = 0; i < cursor->text_count; i++) {
      roadmap_string_release(cursor->texts[i].text);
   }
   
   free (cursor);
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
      
      if (cursor->child)
         release_object(cursor->child);

      release_object(cursor);
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
   int scale_factor;

   for (cursor = RoadmapObjectList; cursor->next != NULL; cursor = cursor->next);

   for ( ; cursor != NULL ; cursor = cursor->previous) {
#ifdef OPENGL
      if (is_visible(cursor)) {
         if (cursor->animation_state == animate_in_pending && cursor->animation & OBJECT_ANIMATION_WHEN_VISIBLE)
            set_animation(cursor);
         if (cursor->child && cursor->child->animation_state == animate_in_pending && cursor->child->animation & OBJECT_ANIMATION_WHEN_VISIBLE)
            set_animation(cursor->child);
      }
      if (is_fully_visible(cursor)) {
         if (cursor->animation_state == animate_in_pending && cursor->animation & OBJECT_ANIMATION_WHEN_FULLY_VISIBLE)
            set_animation(cursor);
         if (cursor->child && cursor->child->animation_state == animate_in_pending && cursor->child->animation & OBJECT_ANIMATION_WHEN_FULLY_VISIBLE)
            set_animation(cursor->child);
      }
         

      if (cursor->glow >= 0) {
         RoadMapDynamicString image_str = roadmap_string_new("object_glow");
         int glow = cursor->glow;
         RoadMapGuiPoint offset;
         offset.x = 3 * 100 / glow;
         offset.y = -27 * 100 / glow;
         (*action) ("",
                    "",
                    &image_str,
                    1,
                    &(cursor->position),
                    &offset,
                    is_visible(cursor),
                    glow,
                    255*(101 - glow)/100,
                    100,
                    NULL,
                    NULL,
                    0,
                    0);
         glow += 50;
         if (glow > 100)
            glow -= 100;
         offset.x = 3 * 100 / glow;
         offset.y = -27 * 100 / glow;
         (*action) ("",
                    "",
                    &image_str,
                    1,
                    &(cursor->position),
                    &offset,
                    is_visible(cursor),
                    glow,
                    255*(101 - glow)/100,
                    100,
                    NULL,
                    NULL,
                    0,
                    0);
         roadmap_string_release(image_str);
      }
#endif

      if (cursor->check_overlapping){
         check_overlapping(cursor);
      }

	  if  (cursor->scale_factor_min_zoom != 0 && roadmap_math_get_zoom() >= cursor->scale_factor_min_zoom)
		  scale_factor = cursor->scale_factor;
	  else
		  scale_factor = 100;
      
      if (cursor->child) {
         (*action) (roadmap_string_get(cursor->name),
                    roadmap_string_get(cursor->sprite),
                    cursor->child->images,
                    cursor->child->image_count,
                    &(cursor->position),
                    &(cursor->child->offset),
                    is_visible(cursor),
                    cursor->child->scale*scale_factor/100,
                    cursor->child->opacity,
                    cursor->child->scale_y,
                    roadmap_string_get(cursor->child->id),
                    cursor->child->texts,
                    cursor->child->text_count,
                    cursor->child->rotation);
      }

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 cursor->images,
                 cursor->image_count,
                 &(cursor->position),
                 &(cursor->offset),
                 is_visible(cursor),
                 cursor->scale*scale_factor/100,
                 cursor->opacity,
                 cursor->scale_y,
                 roadmap_string_get(cursor->id),
                 cursor->texts,
                 cursor->text_count,
                 cursor->rotation);
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

	  if ((!cursor->images[0]) ||
         (action_only && !cursor->action) ||
         out_of_zoom (cursor))
        continue;


      cursor_position.latitude = cursor->position.latitude;
      cursor_position.longitude = cursor->position.longitude;


      roadmap_math_coordinate(&cursor_position, &pos);
      roadmap_math_rotate_project_coordinate (&pos);

      image = roadmap_res_get (RES_BITMAP, RES_SKIN, roadmap_string_get(cursor->images[0]));
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
                           object->images,
                           object->image_count,
                           &(object->position),
                           &(object->offset),
                           1,
                           object->scale*scale_factor/100,
                           object->opacity,
                           object->scale_y,
                           roadmap_string_get(object->id),
                           object->texts,
                           object->text_count,
                           0);
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
