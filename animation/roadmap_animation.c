/* roadmap_animation.c - Animation engine
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 */



#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_screen.h"
#include "roadmap_time.h"
#include "roadmap_animation.h"

#define MAX_ANIMATIONS  300
#define FRAME_RATE_MSEC 35

static RoadMapAnimation gs_animations[MAX_ANIMATIONS];
static BOOL             gs_is_active = FALSE;
static uint32_t         gs_last_refresh_time;

enum animation_status {
   ANIMATION_STATUS_NEW,
   ANIMATION_STATUS_STARTED,
   ANIMATION_STATUS_PAUSED,
   ANIMATION_STATUS_FREE
};


///////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_animation_periodic (void) {
   roadmap_main_remove_periodic(roadmap_animation_periodic);
   roadmap_screen_redraw();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_animation_start (void) {
   if (gs_is_active)
      return;
   
   gs_is_active = TRUE;
   //if (!roadmap_main_full_animation())
      roadmap_screen_set_animating (TRUE);
   roadmap_main_set_periodic(10, roadmap_animation_periodic);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
RoadMapAnimation *roadmap_animation_create (void) {
   RoadMapAnimation *animation = NULL;
   int i;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if (gs_animations[i].status == ANIMATION_STATUS_FREE)
         break;
   }
   
   
   if (i == MAX_ANIMATIONS){
      roadmap_log(ROADMAP_WARNING, "Animation table saturated !");
   } else {
      animation = &gs_animations[i];
      
      animation->status = ANIMATION_STATUS_NEW;
      animation->properties_count = 0;
      animation->object_id[0] = 0;
      animation->duration = 1000;
      animation->loops = 1;
      animation->delay = 0;
      animation->callbacks = NULL;
      animation->timing = ANIMATION_TIMING_LINEAR;
   }
   
   return animation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void merge_animations (RoadMapAnimation *old, RoadMapAnimation *new) {
   int old_c, new_c;
   uint32_t now = roadmap_time_get_millis();
   BOOL refresh_timing = FALSE;
   
   for (new_c = 0; new_c < new->properties_count; new_c++) {
      for (old_c = 0; old_c < old->properties_count; old_c++) {
         if (old->properties[old_c].type == new->properties[new_c].type) {
            if (old->properties[old_c].to != new->properties[new_c].to){
               //printf("updating property %d of old_to: %d new_to: %d\n", old_c, old->properties[old_c].to, new->properties[new_c].to);
               old->properties[old_c].to = new->properties[new_c].to;
               
               if (old->properties[old_c].type == ANIMATION_PROPERTY_ROTATION) {
                  if (old->properties[old_c].to - old->properties[old_c].current >= 180 ||
                      old->properties[old_c].to - old->properties[old_c].current <= -180) {
                     if (360 - old->properties[old_c].current < 360 - old->properties[old_c].to)
                        old->properties[old_c].current -= 360;
                     else
                        old->properties[old_c].to -= 360;
                     //printf ("changed to -  from: %d to: %d\n", old->properties[old_c].current, old->properties[old_c].to);
                  }
               }
               //old->properties[old_c].from = old->properties[old_c].current;
               
               //refresh_timing = TRUE;
            }
            break;
         }
      }
      if (old_c == old->properties_count) { //property not found
         old->properties_count++;
         old->properties[old_c] = new->properties[new_c];
         old->properties[old_c].current = old->properties[old_c].from;
         //printf("\nadding property %d of old_to: %d new_to: %d\n", old_c, old->properties[old_c].to, new->properties[new_c].to);
      }
   }
   
   if (refresh_timing) {
      for (old_c = 0; old_c < old->properties_count; old_c++) {
         old->properties[old_c].from = old->properties[old_c].current;
      }
      
      if ((old->timing & ANIMATION_TIMING_EASY_IN) &&
          (1.0*now-old->start_time)/old->duration < 0.2)
         old->timing &= !ANIMATION_TIMING_EASY_IN;
      //else
         //printf("--->>> passed %f   timing: %d\n",(1.0*now-old->start_time)/old->duration, old->timing);
      //printf("changing duration from: %d to %d\n", old->duration);
      old->duration = new->duration;
      old->start_time = now;
   }
   new->status = ANIMATION_STATUS_FREE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_register(RoadMapAnimation *animation) {
   int i;

   if (animation == NULL ||
       animation->status != ANIMATION_STATUS_NEW)
      return;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if ((gs_animations[i].status != ANIMATION_STATUS_FREE) &&
          (&gs_animations[i] != animation) &&
          (!strcmp(animation->object_id, gs_animations[i].object_id))) {
         merge_animations(&gs_animations[i], animation);
         return;
      }
   }
   
   animation->status = ANIMATION_STATUS_STARTED;
   animation->start_time = roadmap_time_get_millis();
   for (i = 0; i < animation->properties_count; i++)
      animation->properties[i].current = animation->properties[i].from;

   roadmap_animation_start();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_initialize (void) {
   int i;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      gs_animations[i].status = ANIMATION_STATUS_FREE;
   }
}

static inline float cubic_bezier(float t, float a, float b, float c, float d) {
   return pow((1 - t), 3)*a +3*pow((1 - t),2)*t*b + 3*(1 - t)*pow(t, 2)*c + pow(t, 3)*d;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_start_repaint (void) {
   RoadMapAnimation *animation;
   int i, j;
   uint32_t now = roadmap_time_get_millis();
   uint32_t actual_start_time;
   int time_passed;
   BOOL last_cycle;
   
   if (!gs_is_active)
      return;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if (gs_animations[i].status == ANIMATION_STATUS_STARTED) {
         animation = &gs_animations[i];
         actual_start_time = animation->start_time + animation->delay;
         time_passed = now - actual_start_time;
         last_cycle = FALSE;
         
         if (time_passed < 0) {
            continue;
         }
         
         if (time_passed + FRAME_RATE_MSEC >= animation->duration) {
            last_cycle = TRUE;
         }
         for (j = 0; j < animation->properties_count; j++) {
            float current_pos = 1.0f * (time_passed + FRAME_RATE_MSEC) / animation->duration;
            if (last_cycle || current_pos >= 1.0f) {
               animation->properties[j].current = animation->properties[j].to;
            } else {
               int delta = animation->properties[j].to - animation->properties[j].from +1;
               if ((animation->timing & ANIMATION_TIMING_EASY_IN) &&
                   current_pos < 0.5) {
                  animation->properties[j].current = (1- (2 * (0.8-current_pos) - pow(0.8-current_pos, 2))) * delta + animation->properties[j].from;
               } else if ((animation->timing & ANIMATION_TIMING_EASY_OUT) &&
                          current_pos >= 0.5) {
                  animation->properties[j].current = ((2 * (current_pos -0.2) - pow(current_pos -0.2, 2))) * delta + animation->properties[j].from;
               } else {
                  animation->properties[j].current = current_pos * delta + animation->properties[j].from;
               }
            }
         }
         if (animation->callbacks && animation->callbacks->set)
            animation->callbacks->set((void *)animation);
      }
   }
   
   gs_last_refresh_time = roadmap_time_get_millis();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_end_repaint (void) {
   int i, j;
   BOOL ended;
   RoadMapAnimation *animation;
   BOOL is_active_animations = FALSE;
   uint32_t now = roadmap_time_get_millis();
   int time_to_next_cycle;
   
   if (!gs_is_active)
      return;
   
   //printf("drawing took: %d ms\n", now - gs_last_refresh_time);
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if (gs_animations[i].status == ANIMATION_STATUS_STARTED) {
         animation = &gs_animations[i];
         ended = TRUE;
         for (j = 0; j < animation->properties_count; j++) {
            if (animation->properties[j].to != animation->properties[j].current) {
               ended = FALSE;
               break;
            }
         }
         if (ended) {
            animation->status = ANIMATION_STATUS_FREE;
            if (animation->callbacks && animation->callbacks->ended)
               animation->callbacks->ended((void *)animation);
         }
      }
   }
   
   //Make sure we don't have new animations set
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if (gs_animations[i].status == ANIMATION_STATUS_STARTED) {
         is_active_animations = TRUE;
         break;
      }
   }
   
   if (!is_active_animations){
      gs_is_active = FALSE;
      roadmap_screen_set_animating (FALSE);
      roadmap_screen_refresh();
   } else {
      time_to_next_cycle = FRAME_RATE_MSEC - (now - gs_last_refresh_time);
      if (time_to_next_cycle < 5)
         time_to_next_cycle = 5;
      roadmap_main_set_periodic(time_to_next_cycle, roadmap_animation_periodic);
   }
}

