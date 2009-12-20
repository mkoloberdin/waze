/* roadmap_state.c - manage states of different objects.
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
 * DESCRIPTION:
 *
 *   This module manages a dynamic list of states of different items / objects
 *   in RoadMap. This is mainly used to display different icons on screen
 *   according to the states.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_types.h"

#include "roadmap_state.h"


struct RoadMapStateDescriptor {

   char                *name; /* Unique name of the state indicator. */

   RoadMapStateFn      state_fn;

   struct RoadMapStateDescriptor *next;
};

typedef struct RoadMapStateDescriptor *RoadMapState;

static RoadMapState RoadMapStateList = NULL;

#define ROADMAP_STATE_CLIENTS 5
static RoadMapCallback RoadMapStateMonitors[ROADMAP_STATE_CLIENTS] = {NULL};

void roadmap_state_add (const char *name, RoadMapStateFn state_fn) {

   RoadMapState object = malloc(sizeof(*object));

   roadmap_check_allocated(object);

   object->name = strdup (name);
   object->state_fn = state_fn;

   object->next = RoadMapStateList;
   RoadMapStateList = object;
}


RoadMapStateFn roadmap_state_find (const char *name) {
   
   RoadMapState cursor;

   for (cursor = RoadMapStateList; cursor != NULL; cursor = cursor->next) {

      if (!strcmp (cursor->name, name)) {
         return cursor->state_fn;
      }
   }

   return NULL;
}


void roadmap_state_refresh (void) {
   
   int i;

   for (i = 0; i < ROADMAP_STATE_CLIENTS; ++i) {

      if (RoadMapStateMonitors[i] == NULL) break;

      (RoadMapStateMonitors[i]) ();
   }
}


void roadmap_state_monitor (RoadMapCallback monitor) {

   int i;

   for (i = 0; i < ROADMAP_STATE_CLIENTS; ++i) {
      if (RoadMapStateMonitors[i] == NULL) {
         RoadMapStateMonitors[i] = monitor;
         break;
      }
   }
}

