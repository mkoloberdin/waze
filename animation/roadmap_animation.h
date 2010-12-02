/* roadmap_animation.h - Animation engine
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
 
#ifndef INCLUDE__ROADMAP_ANIMATION__H
#define INCLUDE__ROADMAP_ANIMATION__H

#include <time.h>


#define ANIMATION_MAX_OBJECT_LENGTH 128
#define ANIMATION_MAX_PROPERTIES    8

#define ANIMATION_TIMING_LINEAR     0x0
#define ANIMATION_TIMING_EASY_IN    0x1
#define ANIMATION_TIMING_EASY_OUT   0x2

enum animation_property_type {
   ANIMATION_PROPERTY_POSITION_X,
   ANIMATION_PROPERTY_POSITION_Y,
   ANIMATION_PROPERTY_SCALE,
   ANIMATION_PROPERTY_ROTATION,
   ANIMATION_PROPERTY_OPACITY,
   ANIMATION_PROPERTY_FRAME
};

typedef void (*RoadMapAnimationCallbackSet)     (void *context);
typedef void (*RoadMapAnimationCallbackEnded)   (void *context);

typedef struct {
   RoadMapAnimationCallbackSet      set;
   RoadMapAnimationCallbackEnded    ended;
} RoadMapAnimationCallbacks;

typedef struct {
   int   type;
   int   from;
   int   to;
   int   current;
} RoadMapAnimationProperty;

typedef struct {
   char                       object_id[ANIMATION_MAX_OBJECT_LENGTH];
   int                        properties_count;
   RoadMapAnimationProperty   properties[ANIMATION_MAX_PROPERTIES];
   int                        duration; //msec
   int                        loops;
   int                        delay; //msec
   RoadMapAnimationCallbacks  *callbacks;
   BOOL                       timing;
   // The following are set by the controller
   int                        status;
   uint32_t                   start_time;
} RoadMapAnimation;


RoadMapAnimation *roadmap_animation_create (void);
void roadmap_animation_register(RoadMapAnimation *animation);
void roadmap_animation_initialize (void);
void roadmap_animation_start_repaint (void);
void roadmap_animation_end_repaint (void);

#endif /* INCLUDE__ROADMAP_ANIMATION__H */
