/* roadmap_screen_obj.c - manage screen objects.
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
 *   This module manages a dynamic list of objects to be displayed on screen.
 *   An object can be pressed on which may trigger an action and/or it can
 *   display a state using an image or a sprite.
 *
 *   The implementation is not terribly optimized: there should not be too
 *   many objects.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_canvas.h"
#include "roadmap_math.h"
#include "roadmap_sprite.h"
#include "roadmap_start.h"
#include "roadmap_state.h"
#include "roadmap_pointer.h"
#include "roadmap_screen.h"
#include "roadmap_res.h"
#include "roadmap_sound.h"
#include "roadmap_main.h"
#include "roadmap_softkeys.h"
#include "roadmap_config.h"
#include "roadmap_map_settings.h"

#include "roadmap_screen_obj.h"
#include "roadmap_analytics.h"

typedef struct {
   const char   *name;
   int           min_screen_height;
} ObjectFile;






#define OBJ_FILE_PORTRAIT 					"objects"
#define OBJ_FILE_LANDSCAPE 					"objects_wide"
#define OBJ_FILE_LANDSCAPE_SK_BOTTOM 		"objects_wide_softkeys_bottom"
#define MAX_STATES 9

#define OBJ_FLAG_NO_ROTATE 0x1
#define OBJ_FLAG_REPEAT    0x2

#define OBJ_REPEAT_TIMEOUT 100

struct RoadMapScreenObjDescriptor {

   char                *name; /* Unique name of the object. */

   char                *sprites[MAX_STATES]; /* Icons for each state. */
   const char          *images[MAX_STATES]; /* Icons for each state. */

   int                  states_count;

   short                pos_x; /* position on screen */
   short                pos_y; /* position on screen */

   short                offset_x; /* offset for this */
   short                offset_y; /* position on screen */


   int                  flags;

   int                  opacity;

   const RoadMapAction *action;
   const RoadMapAction *long_action;
   const RoadMapAction *dt_action;

   RoadMapStateFn state_fn;

   RoadMapStateFn  condition_fn;
   int			   condition_value;

   RoadMapGuiRect       bbox;

   struct RoadMapScreenObjDescriptor *next;
};

static int screen_height;
static int screen_width;
static const char *current_object_name=NULL;

static RoadMapScreenObj RoadMapObjectList = NULL;
static RoadMapScreenObj RoadMapScreenObjSelected = NULL;
static int OffsetX = 0;
static int OffsetY = 0;
static BOOL initialized = FALSE;

//Map controls event
static const char* ANALYTICS_EVENT_MAPCONTROL_NAME = "MAP_CONTROL";
static const char* ANALYTICS_EVENT_MAPCONTROL_INFO = "ACTION_NAME";

static char *roadmap_object_string (const char *data, int length) {

    char *p = malloc (length+1);

    roadmap_check_allocated(p);

    strncpy (p, data, length);
    p[length] = 0;

    return p;
}


static RoadMapScreenObj roadmap_screen_obj_new
          (int argc, const char **argv, int *argl) {

   RoadMapScreenObj object = calloc(sizeof(*object), 1);

   roadmap_check_allocated(object);

   object->name = roadmap_object_string (argv[1], argl[1]);

   object->next = RoadMapObjectList;
   RoadMapObjectList = object;

   return object;
}


static void roadmap_screen_obj_decode_arg (char *output, int o_size,
                                           const char *input, int i_size) {

   int size;

   o_size -= 1;

   size = i_size < o_size ? i_size : o_size;

   strncpy (output, input, size);
   output[size] = '\0';
}


static void roadmap_screen_obj_decode_icon
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   int i;

   argc -= 1;

   if (object->states_count > MAX_STATES) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' has too many states.",
                  object->name);
      return;
   }

   for (i = 1; i <= argc; ++i) {
      char arg[256];

      roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[i], argl[i]);

      if (!object->images[object->states_count])
      {
    	  object->images[object->states_count] = roadmap_object_string( arg, argl[i] );;

      } else {
         object->sprites[object->states_count] =
            roadmap_object_string (arg, argl[i]);
      }
   }

   ++object->states_count;
}


static void roadmap_screen_obj_decode_sprite
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   int i;

   argc -= 1;

   if (object->states_count > MAX_STATES) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' has too many states.",
                  object->name);
      return;
   }

   for (i = 1; i <= argc; ++i) {
      char arg[256];

      roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[i], argl[i]);

      object->sprites[object->states_count] =
         roadmap_object_string (arg, argl[i]);
   }

   ++object->states_count;
}


static void roadmap_screen_obj_decode_integer (int *value, int argc,
                                               const char **argv, int *argl) {

   char arg[255];

   argc -= 1;

   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "screen object: illegal integer value - %s",
                  argv[0]);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   *value = atoi(arg);
}


static void roadmap_screen_obj_decode_bbox
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 4) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal bbox.",
                  object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   object->bbox.minx = atoi(arg);
   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   object->bbox.miny = atoi(arg);
   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[3], argl[3]);
   object->bbox.maxx = atoi(arg);
   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[4], argl[4]);
   object->bbox.maxy = atoi(arg);
}


static void roadmap_screen_obj_decode_position
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   char arg[255];
   int pos;

   argc -= 1;
   if (argc != 2) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal position.",
                  object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   pos = atoi(arg);
   object->pos_x = pos;
   object->offset_x = 0;

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   pos = atoi(arg);
   object->pos_y = pos;
   object->offset_y = 0;
}


static void roadmap_screen_obj_decode_state
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal state indicator.",
                  object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->state_fn = roadmap_state_find (arg);

   if (!object->state_fn) {
      roadmap_log (ROADMAP_ERROR,
                  "screen object:'%s' can't find state indicator.",
                  object->name);
   }
}

static void roadmap_screen_obj_decode_condition
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 2) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal condition indicator.",
                   object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->condition_fn = roadmap_state_find (arg);

   if (!object->condition_fn) {
      roadmap_log (ROADMAP_ERROR,
                   "screen object:'%s' can't find condition indicator.",
                   object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   object->condition_value = atoi(arg);
}


static void roadmap_screen_obj_decode_action
                        (RoadMapScreenObj object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc < 1) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal action.",
                  object->name);
      return;
   }

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->action = roadmap_start_find_action (arg);

   if (!object->action) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' can't find action.",
                  object->name);
   }

   if (argc == 1) return;

   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[2], argl[2]);

   object->long_action = roadmap_start_find_action (arg);

   if (!object->long_action) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' can't find action.",
                  object->name);
   }
}

static void roadmap_screen_obj_decode_dt_action
                  (RoadMapScreenObj object,
                   int argc, const char **argv, int *argl) {
   
   char arg[255];
   
   argc -= 1;
   if (argc < 1) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' illegal double tap action.",
                   object->name);
      return;
   }
   
   roadmap_screen_obj_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   
   object->dt_action = roadmap_start_find_action (arg);
   
   if (!object->dt_action) {
      roadmap_log (ROADMAP_ERROR, "screen object:'%s' can't find double tap action.",
                   object->name);
   }
}


static void roadmap_screen_obj_load (const char *data, int size) {

   int argc;
   int argl[256];
   const char *argv[256];

   const char *p;
   const char *end;

   RoadMapScreenObj object = NULL;


   end  = data + size;

   while (data < end) {

      while (data[0] == '#' || data[0] < ' ') {

         if (*(data++) == '#') {
            while ((data < end) && (data[0] >= ' ')) data += 1;
         }
         while (data < end && data[0] == '\n' && data[0] != '\r') data += 1;
      }

      argc = 1;
      argv[0] = data;

      for (p = data; p < end; ++p) {

         if (*p == ' ') {
            argl[argc-1] = p - argv[argc-1];
            argv[argc]   = p+1;
            argc += 1;
            if (argc >= 255) break;
         }

         if (p >= end || *p < ' ') break;
      }
      argl[argc-1] = p - argv[argc-1];

      while (p < end && *p < ' ' && *p > 0) ++p;
      data = p;

      if (object == NULL) {

         if (argv[0][0] != 'N') {
            roadmap_log (ROADMAP_ERROR, "object name is missing");
            break;
         }

      }

      switch (argv[0][0]) {

         case 'O':

            roadmap_screen_obj_decode_integer (&object->opacity, argc, argv,
                                               argl);
            break;

         case 'A':

            roadmap_screen_obj_decode_action (object, argc, argv, argl);
            break;
            
         case 'D':
            
            roadmap_screen_obj_decode_dt_action (object, argc, argv, argl);
            break;

         case 'B':

            roadmap_screen_obj_decode_bbox (object, argc, argv, argl);
            break;

         case 'P':

            roadmap_screen_obj_decode_position (object, argc, argv, argl);
            break;

         case 'I':

            roadmap_screen_obj_decode_icon (object, argc, argv, argl);
            break;

         case 'E':

            roadmap_screen_obj_decode_sprite (object, argc, argv, argl);
            break;

         case 'S':

            roadmap_screen_obj_decode_state (object, argc, argv, argl);
            break;

         case 'C':

            roadmap_screen_obj_decode_condition (object, argc, argv, argl);
            break;

         case 'R':

            object->flags |= OBJ_FLAG_NO_ROTATE;
            break;

         case 'T':

            object->flags |= OBJ_FLAG_REPEAT;
            break;

         case 'N':

            object = roadmap_screen_obj_new (argc, argv, argl);
            break;
      }

      while (p < end && *p < ' ') p++;
      data = p;
   }
}


static RoadMapScreenObj roadmap_screen_obj_search (const char *name) {

   RoadMapScreenObj cursor;

   for (cursor = RoadMapObjectList; cursor != NULL; cursor = cursor->next) {
      if (!strcmp(cursor->name, name)) return cursor;
   }

   return NULL;
}

static void road_screen_objects_delete(){
   RoadMapScreenObj cursor;
   for (cursor = RoadMapObjectList; cursor != NULL; cursor = cursor->next) {
      free(cursor);

      RoadMapObjectList=NULL;
   }
}

static void roadmap_screen_obj_pos (RoadMapScreenObj object,
                                    RoadMapGuiPoint *pos) {

   pos->x = object->pos_x;
   pos->y = object->pos_y;

   if (pos->x < 0) {
      pos->x += roadmap_canvas_width ();
   } else {
      pos->x += OffsetX;
   }

   if (pos->y < 0) {
      pos->y += roadmap_canvas_height ();
   } else {
      pos->y += OffsetY;
#ifdef IPHONE
      /* Apply top bar height offset */
      if (roadmap_map_settings_isShowTopBarOnTap())
         pos->y += 40;
#endif //IPHONE
   }

   /* Apply local offsets */
   pos->x += object->offset_x;
   pos->y += object->offset_y;
}

static int obj_is_active (RoadMapScreenObj object) {
   int state;
   int condition;

   if (object->state_fn) {
      state = (*object->state_fn) ();
      if ((state < 0) || (state >= MAX_STATES))
         return 0;
   }

   if (object->condition_fn) {
      condition = (*object->condition_fn) ();
      if (condition != object->condition_value)
         return 0;
   }

   return 1;
}


static RoadMapScreenObj roadmap_screen_obj_by_pos (RoadMapGuiPoint *point) {

   RoadMapScreenObj cursor;

   for (cursor = RoadMapObjectList; cursor != NULL; cursor = cursor->next) {

      RoadMapGuiPoint pos;
      roadmap_screen_obj_pos (cursor, &pos);

      if ((point->x >= (pos.x + cursor->bbox.minx)) &&
          (point->x <= (pos.x + cursor->bbox.maxx)) &&
          (point->y >= (pos.y + cursor->bbox.miny)) &&
          (point->y <= (pos.y + cursor->bbox.maxy))) {

         if (obj_is_active(cursor))
            return cursor;
      }
   }

   return NULL;
}


static void roadmap_screen_obj_repeat (void) {
   assert(RoadMapScreenObjSelected);

   if (RoadMapScreenObjSelected && RoadMapScreenObjSelected->action) {
      (*(RoadMapScreenObjSelected->action->callback)) ();
   }
}


static int roadmap_screen_obj_pressed (RoadMapGuiPoint *point) {
   int state = 0;

   RoadMapScreenObjSelected = roadmap_screen_obj_by_pos (point);

   if (!RoadMapScreenObjSelected) return 0;

   /* There is no draggable objects on the screen. In the future if there will be such an
    * objects the short/long click handling for the small drag has to be in the drag_end event
    * like in the ssd_dialog
    */
   roadmap_pointer_cancel_dragging();

   /*
    * Enable double tap only for the supporting objects
    */
   if ( RoadMapScreenObjSelected->dt_action )
   {
	   roadmap_pointer_enable_double_click();
   }

   if (!obj_is_active(RoadMapScreenObjSelected))
       return 1;

   if (RoadMapScreenObjSelected->state_fn)
      state = (*RoadMapScreenObjSelected->state_fn) ();


   if ( RoadMapScreenObjSelected->images[state] ) {
      RoadMapGuiPoint pos;
      RoadMapImage image;
      roadmap_screen_obj_pos (RoadMapScreenObjSelected, &pos);

      image = roadmap_res_get( RES_BITMAP, RES_SKIN, RoadMapScreenObjSelected->images[state] );

     if (image == NULL)
     {
        roadmap_log (ROADMAP_ERROR, "screen object:'%s' can't load image:%s.",
        		RoadMapScreenObjSelected->name, RoadMapScreenObjSelected->images[state] );
        return 1;
     }
     else
     {
#ifndef OPENGL
    	 roadmap_canvas_draw_image ( image, &pos,
                           RoadMapScreenObjSelected->opacity, IMAGE_NORMAL );
#endif
     }
   }

#ifndef OPENGL
   roadmap_canvas_refresh ();
#endif

   if (RoadMapScreenObjSelected->flags & OBJ_FLAG_REPEAT) {
      if (RoadMapScreenObjSelected->action) {
         roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, RoadMapScreenObjSelected->action->label_long);
         
         (*(RoadMapScreenObjSelected->action->callback)) ();
      }

      roadmap_main_set_periodic (OBJ_REPEAT_TIMEOUT,
            roadmap_screen_obj_repeat);
   }

   return 1;
}


static int roadmap_screen_obj_released (RoadMapGuiPoint *point) {
   RoadMapScreenObj object = RoadMapScreenObjSelected;

   if (!RoadMapScreenObjSelected) {
      return 0;
   }

   if (object->flags & OBJ_FLAG_REPEAT) {
      roadmap_main_remove_periodic (roadmap_screen_obj_repeat);
      RoadMapScreenObjSelected = 0;
   }

   return 1;
}


static int roadmap_screen_obj_short_click (RoadMapGuiPoint *point) {

   RoadMapScreenObj object = RoadMapScreenObjSelected;

   if (!RoadMapScreenObjSelected) {
      return 0;
   }

   if (object->flags & OBJ_FLAG_REPEAT) return 1;

   RoadMapScreenObjSelected = NULL;

   if (!obj_is_active(object))
      return 0;

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
      roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, object->action->label_long);
      
      (*(object->action->callback)) ();
      roadmap_screen_touched();
   }



   return 1;
}


static int roadmap_screen_obj_long_click (RoadMapGuiPoint *point) {

   static RoadMapSoundList list;
   RoadMapScreenObj object = RoadMapScreenObjSelected;

   if (!RoadMapScreenObjSelected) {
      return 0;
   }

   if (RoadMapScreenObjSelected->flags & OBJ_FLAG_REPEAT) return 1;

   if (!obj_is_active(object)) return 0;

   RoadMapScreenObjSelected = NULL;

   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click_long");
      roadmap_res_get (RES_SOUND, 0, "click_long");
   }

   if (object->long_action) {
#ifndef IPHONE
      roadmap_sound_play_list (list);
#endif //IPHONE
      roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, object->long_action->label_long);
      
      (*(object->long_action->callback)) ();

   } else if (object->action) {
#ifndef IPHONE
      roadmap_sound_play_list (list);
#endif //IPHONE
      roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, object->action->label_long);
      
      (*(object->action->callback)) ();
   }

   return 1;
}

static int roadmap_screen_obj_double_click (RoadMapGuiPoint *point) {
   
   static RoadMapSoundList list;
   RoadMapScreenObj object = RoadMapScreenObjSelected;
   
   if (!RoadMapScreenObjSelected) {
      return 0;
   }
   
   if (RoadMapScreenObjSelected->flags & OBJ_FLAG_REPEAT) return 1;
   
   if (!obj_is_active(object)) return 0;
   
   RoadMapScreenObjSelected = NULL;
   
   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click_long");
      roadmap_res_get (RES_SOUND, 0, "click_long");
   }
   
   if (object->dt_action) {
#ifndef IPHONE
      roadmap_sound_play_list (list);
#endif //IPHONE
      roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, object->dt_action->label_long);
      
      (*(object->dt_action->callback)) ();
      
   } else if (object->action) {
#ifndef IPHONE
      roadmap_sound_play_list (list);
#endif //IPHONE
      roadmap_analytics_log_event(ANALYTICS_EVENT_MAPCONTROL_NAME, ANALYTICS_EVENT_MAPCONTROL_INFO, object->action->label_long);
      
      (*(object->action->callback)) ();
   }
   
   return 1;
}


static void roadmap_screen_obj_reload (void) {

   const char *cursor;
   RoadMapFileContext file;
   int height = roadmap_canvas_height ();
   int width = roadmap_canvas_width();
   const char *object_name = OBJ_FILE_PORTRAIT;		/* Portrait is the default */

   RoadMapObjectList = NULL;

   screen_height = height;
   screen_width = width;

  if ( height < width )		/* Landscape */
  {
	 if ( roadmap_softkeys_orientation() == SOFT_KEYS_ON_BOTTOM )
		 object_name = OBJ_FILE_LANDSCAPE_SK_BOTTOM;
	 else
		 object_name = OBJ_FILE_LANDSCAPE;
  }

  /* AGA reduce debug level */
  roadmap_log( ROADMAP_DEBUG, "Current object file: %s", object_name );

   if (!object_name) {
      roadmap_log
         (ROADMAP_ERROR, "Can't find object file for screen height: %d",
          height);
      return;
   }

   if (current_object_name){
   		if (!strcmp(current_object_name, object_name))
   			road_screen_objects_delete();
   		else
   			return;
   }

   for (cursor = roadmap_file_map ("skin", object_name, NULL, "r", &file);
        cursor != NULL;
        cursor = roadmap_file_map ("skin", object_name, cursor, "r", &file)) {

      roadmap_screen_obj_load (roadmap_file_base(file), roadmap_file_size(file));

      roadmap_file_unmap (&file);
      return;
   }
}


void roadmap_screen_obj_move (const char *name,
                              const RoadMapGuiPoint *position) {

   RoadMapScreenObj cursor = roadmap_screen_obj_search (name);

   if (cursor != NULL) {

      cursor->pos_x = position->x;
      cursor->pos_y = position->y;
   }
}


void roadmap_screen_obj_offset ( const char* obj_name, int offset_x, int offset_y )
{
   RoadMapScreenObj cursor = roadmap_screen_obj_search( obj_name );
   if (cursor){
      cursor->offset_x = offset_x;
      cursor->offset_y = offset_y;
   }
}


void roadmap_screen_obj_global_offset (int x, int y) {

   OffsetX += x;
   OffsetY += y;
}


#if 0
void roadmap_object_iterate (RoadMapObjectAction action) {

   RoadMapScreenObj *cursor;

   for (cursor = RoadMapObjectList; cursor != NULL; cursor = cursor->next) {

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 &(cursor->position));
   }
}
#endif


void roadmap_screen_obj_initialize (void) {

   roadmap_pointer_register_pressed
      (roadmap_screen_obj_pressed, POINTER_HIGH);
   roadmap_pointer_register_released
      (roadmap_screen_obj_released, POINTER_HIGH);
   roadmap_pointer_register_short_click
      (roadmap_screen_obj_short_click, POINTER_HIGH);

   // pressing enter on client will emulate a short click on objects
   roadmap_pointer_register_enter_key_press
       (roadmap_screen_obj_short_click, POINTER_HIGH);

   roadmap_pointer_register_long_click
      (roadmap_screen_obj_long_click, POINTER_HIGH);
   
   roadmap_pointer_register_double_click
      (roadmap_screen_obj_double_click, POINTER_HIGH);

   roadmap_screen_obj_reload ();

   initialized = TRUE;
}


void roadmap_screen_obj_draw (void) {

   RoadMapScreenObj cursor;
   int height = roadmap_canvas_height ();
   int width = roadmap_canvas_width();

   if (!initialized)
      return;

   if ((height != screen_height) && (width != screen_width)){
	   roadmap_screen_obj_reload();
   }

   for (cursor = RoadMapObjectList; cursor != NULL; cursor = cursor->next) {
      int state = 0;
      int image_mode = IMAGE_NORMAL;
      RoadMapGuiPoint pos;

      if (!obj_is_active(cursor))
         continue;

      if (cursor->state_fn)
         state = (*cursor->state_fn) ();


      if (cursor == RoadMapScreenObjSelected) {
         image_mode = IMAGE_SELECTED;
      }

      roadmap_screen_obj_pos (cursor, &pos);

      if (cursor->images[state]) {
    	  RoadMapImage image;
          image = roadmap_res_get( RES_BITMAP, RES_SKIN, cursor->images[state] );
          if (image == NULL)
          {
             roadmap_log (ROADMAP_ERROR, "screen object:'%s' can't load image:%s.",
            		cursor->name, cursor->images[state] );
          }
          else
          {
        	  roadmap_canvas_draw_image ( image, &pos, cursor->opacity, image_mode );
          }
      }

      if (cursor->sprites[state]) {

         if (cursor->flags & OBJ_FLAG_NO_ROTATE) {
            roadmap_sprite_draw (cursor->sprites[state], &pos,
                                 -roadmap_math_get_orientation());
         } else {
            roadmap_sprite_draw (cursor->sprites[state], &pos, 0);
         }
      }
   }
}



