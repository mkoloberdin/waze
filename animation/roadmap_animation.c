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

#define MAX_ANIMATIONS  500
#define FRAME_RATE_MSEC 35

static RoadMapAnimation gs_animations[MAX_ANIMATIONS];
static BOOL             gs_is_active = FALSE;
static uint32_t         gs_last_refresh_time;
static int              gs_bezier_io[101], gs_bezier_i[101], gs_bezier_o[101];

enum animation_status {
   ANIMATION_STATUS_NEW,
   ANIMATION_STATUS_STARTED,
   ANIMATION_STATUS_PAUSED,
   ANIMATION_STATUS_FREE
};

static void roadmap_animation_run (int priority);



///////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_animation_periodic (void) {
   roadmap_main_remove_periodic(roadmap_animation_periodic);
   roadmap_animation_run (ANIMATION_PRIORITY_GPS);
   roadmap_screen_redraw();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_animation_start (void) {
   if (gs_is_active)
      return;
   
   gs_is_active = TRUE;

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
      animation->priority = ANIMATION_PRIORITY_SCREEN;
   }
   
   return animation;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void merge_animations (RoadMapAnimation *old, RoadMapAnimation *new) {
   int old_c, new_c;
   uint32_t now = roadmap_time_get_millis();
   uint32_t actual_start_time;
   int time_passed;
   
   for (new_c = 0; new_c < new->properties_count; new_c++) {
      for (old_c = 0; old_c < old->properties_count; old_c++) {
         if (old->properties[old_c].type == new->properties[new_c].type) {
            if (old->properties[old_c].to != new->properties[new_c].to){
               if (old->properties[old_c].type == ANIMATION_PROPERTY_ROTATION &&
                   (old->properties[old_c].to + 360 == new->properties[new_c].to ||
                    old->properties[old_c].to - 360 == new->properties[new_c].to)) {
                      break;
               }
               
               old->properties[old_c].to = new->properties[new_c].to;
               
               if (old->properties[old_c].type == ANIMATION_PROPERTY_ROTATION) {
                  if (old->properties[old_c].to - old->properties[old_c].current >= 180 ||
                      old->properties[old_c].to - old->properties[old_c].current <= -180) {
                     if (360 - old->properties[old_c].current < 360 - old->properties[old_c].to)
                        old->properties[old_c].current -= 360;
                     else
                        old->properties[old_c].to -= 360;
                  }
               }
               
               actual_start_time = old->properties[old_c].start_time + old->delay;
               time_passed = now - actual_start_time;
               
               if (old->timing & ANIMATION_TIMING_EASY_IN)
                  old->timing &= !ANIMATION_TIMING_EASY_IN;
               
               old->timing = 0;
               
               if (0 && 1.0*time_passed/old->properties[old_c].duration < 0.95) {
                  old->properties[old_c].duration = new->duration;
                  old->properties[old_c].from = old->properties[old_c].current;
               } else {
                  old->properties[old_c].from = old->properties[old_c].current;
                  old->properties[old_c].start_time = now;
                  if (old->properties[old_c].duration - time_passed < new->duration/2)
                     old->properties[old_c].duration = new->duration/2;
                  else
                     old->properties[old_c].duration -= time_passed;
               }            
            }
            break;
         }
      }
      if (old_c == old->properties_count) { //property not found
         old->properties_count++;
         old->properties[old_c] = new->properties[new_c];
         old->properties[old_c].current = old->properties[old_c].from;
         
         old->properties[old_c].duration = new->duration;
         old->properties[old_c].start_time = now;
      }
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

   for (i = 0; i < animation->properties_count; i++) {
      animation->properties[i].current = animation->properties[i].from;
      animation->properties[i].start_time = roadmap_time_get_millis();
      animation->properties[i].duration = animation->duration;
   }

   roadmap_animation_start();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_cancel(RoadMapAnimation *animation) {
   int i;
   
   if (animation == NULL)
      return;
   
   animation->status = ANIMATION_STATUS_FREE;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if ((gs_animations[i].status == ANIMATION_STATUS_STARTED) &&
          (!strcmp(animation->object_id, gs_animations[i].object_id))) {
         gs_animations[i].status = ANIMATION_STATUS_FREE;
         return;
      }
   }
}

/*
 Four control point Bezier interpolation
 t ranges from 0 to 1, start to end of curve
 */
static void create_bezier(RoadMapGuiPoint *p1, RoadMapGuiPoint *p2, RoadMapGuiPoint *p3, RoadMapGuiPoint *p4, int *bezier)
{
   int x_index = 0;
   
   int i;
   
   for (i = 0; i < 100; i++) {
      double tm1,tm13,tm3;
      float t = 0.01*i;
      int x, y;
      
      tm1 = 1 - t;
      tm13 = tm1 * tm1 * tm1;
      tm3 = t * t * t;
      
      x = floorf(tm13*p1->x + 3*t*tm1*tm1*p2->x + 3*t*t*tm1*p3->x + tm3*p4->x);
      y = floorf(tm13*p1->y + 3*t*tm1*tm1*p2->y + 3*t*t*tm1*p3->y + tm3*p4->y);
      
      for (; x_index <= x; x_index++) {
         bezier[x_index] = y;
      }
      
      
   }
   
   for (; x_index <= 100; x_index++) {
      bezier[x_index] = 100;
   }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_initialize (void) {
   int i;
   RoadMapGuiPoint p1, p2, p3, p4;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      gs_animations[i].status = ANIMATION_STATUS_FREE;
   }
   
   //Create bezier curves
   p1.x = 0;   p1.y = 0;   //start
   p2.x = 50;  p2.y = 0;   //control 1
   p3.x = 50;  p3.y = 100; //control 2
   p4.x = 100; p4.y = 100; //end
   create_bezier (&p1, &p2, &p3, &p4, gs_bezier_io);
   p1.x = 0;   p1.y = 0;   //start
   p2.x = 50;  p2.y = 0;   //control 1
   p3.x = 70;  p3.y = 70; //control 2
   p4.x = 100; p4.y = 100; //end
   create_bezier (&p1, &p2, &p3, &p4, gs_bezier_i);
   p1.x = 0;   p1.y = 0;   //start
   p2.x = 30;  p2.y = 30;  //control 1
   p3.x = 50;  p3.y = 100; //control 2
   p4.x = 100; p4.y = 100; //end
   create_bezier (&p1, &p2, &p3, &p4, gs_bezier_o);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_animation_start_repaint (void) {
   roadmap_animation_run (ANIMATION_PRIORITY_SCREEN);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void roadmap_animation_run (int priority) {
   RoadMapAnimation *animation;
   int i, j;
   uint32_t now = roadmap_time_get_millis();
   uint32_t actual_start_time;
   int time_passed;
   BOOL last_cycle;
   float current_pos;
   
   if (!gs_is_active)
      return;
   
   for (i = 0; i < MAX_ANIMATIONS; i++) {
      if (gs_animations[i].status == ANIMATION_STATUS_STARTED &&
          gs_animations[i].priority == priority) {
         animation = &gs_animations[i];
         
         for (j = 0; j < animation->properties_count; j++) {
            actual_start_time = animation->properties[j].start_time + animation->delay;
            time_passed = now - actual_start_time;
            last_cycle = FALSE;
            
            if (time_passed < 0) {
               continue;
            }
            
            if (time_passed + FRAME_RATE_MSEC >= animation->properties[j].duration) {
               last_cycle = TRUE;
            }
            
            current_pos = 1.0f * (time_passed + FRAME_RATE_MSEC) / animation->properties[j].duration;
            if (last_cycle || current_pos >= 1.0f) {
               animation->properties[j].current = animation->properties[j].to;
            } else if (animation->properties[j].from == animation->properties[j].to) {
               animation->properties[j].current = animation->properties[j].to;
            } else {
               int delta = animation->properties[j].to - animation->properties[j].from +1;
               int iPos = floorf(100 * current_pos);
               if ((animation->timing & ANIMATION_TIMING_EASY_IN) &&
                   (animation->timing & ANIMATION_TIMING_EASY_OUT)) {
                  animation->properties[j].current = gs_bezier_io[iPos]*0.01 * delta + animation->properties[j].from;
               } else if (animation->timing & ANIMATION_TIMING_EASY_IN) {
                  animation->properties[j].current = gs_bezier_i[iPos]*0.01 * delta + animation->properties[j].from;
               } else if (animation->timing & ANIMATION_TIMING_EASY_OUT) {
                  animation->properties[j].current = gs_bezier_o[iPos]*0.01 * delta + animation->properties[j].from;
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
            if (animation->loops > 1) {
               animation->loops--;
               for (j = 0; j < animation->properties_count; j++) {
                  animation->properties[j].current = animation->properties[j].from;
                  animation->properties[j].start_time = now;
               }
            } else {
               animation->status = ANIMATION_STATUS_FREE;
               if (animation->callbacks && animation->callbacks->ended)
                  animation->callbacks->ended((void *)animation);
            }
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

