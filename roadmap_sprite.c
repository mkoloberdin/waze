/* roadmap_sprite.c - manage the roadmap sprites that represent moving objects.
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
 *   This module draws predefined graphical objects on the RoadMap canvas.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_file.h"
#include "roadmap_canvas.h"
#include "roadmap_sprite.h"


typedef struct {

   int  object_count;
   int *objects;

   int  point_count;
   RoadMapGuiPoint *points;

} RoadmapSpriteDrawingSequence;


typedef struct roadmap_plane_record {

   struct roadmap_plane_record *next;

   RoadMapPen pen;

   RoadmapSpriteDrawingSequence lines;
   RoadmapSpriteDrawingSequence disks;
   RoadmapSpriteDrawingSequence circles;
   RoadmapSpriteDrawingSequence polygons;

} RoadMapSpritePlane;

typedef struct roadmap_sprite_record {

   char *name;

   RoadMapSpritePlane  first;
   RoadMapSpritePlane *last;

   struct roadmap_sprite_record *next;

} *RoadMapSprite;

static RoadMapSprite RoadMapSpriteList = NULL;

/* The default sprite used when the sprite was not found: */

static struct roadmap_sprite_record *RoadMapSpriteDefault = NULL;


int RoadMapSpritePointCount = 0;
RoadMapGuiPoint *RoadMapSpritePoints = NULL;



static char *roadmap_sprite_string (const char *data, int length) {

    char *p = malloc (length+1);

    roadmap_check_allocated(p);

    strncpy (p, data, length);
    p[length] = 0;

    return p;
}


static void roadmap_sprite_decode_plane
               (RoadMapSprite sprite,
                const char *arg, int length, const char *thickness) {

   int   t;
   char  pen[256];
   char  color[256];

   if (length >= (int)sizeof(color)-1) {
      roadmap_log (ROADMAP_FATAL,
                   "color name too long: %*s", length, arg);
   }

   if (sprite->last == NULL) {

      sprite->last = &(sprite->first);
      sprite->first.next = NULL;

   } else {

      sprite->last->next = calloc (1, sizeof(sprite->first));
      roadmap_check_allocated(sprite->last->next);

      sprite->last = sprite->last->next;
   }

   t = atoi(thickness);
   strncpy (color, arg, length);
   color[length] = 0;

   sprintf (pen, "%s.%d.%s", sprite->name, t, color);
   sprite->last->pen = roadmap_canvas_create_pen (pen);

   roadmap_canvas_set_foreground (color);
   roadmap_canvas_set_thickness  (t);
}


static void roadmap_sprite_decode_point
               (RoadMapGuiPoint *point, const char *data) {

   char *p;

   point->x = atoi(data);
   p = strchr (data, ',');
   if (p != NULL) {
      point->y = atoi(p+1);
   } else {
      point->y = 0;
   }
}


static void roadmap_sprite_decode_sequence
               (RoadmapSpriteDrawingSequence *sequence,
                int argc, const char **argv) {

   int i;
   RoadMapGuiPoint *points;

   argc -= 1;

   sequence->object_count += 1;
   sequence->objects =
       realloc (sequence->objects,
                sequence->object_count * sizeof(*(sequence->objects)));

   sequence->objects[sequence->object_count-1] = argc;

   sequence->points =
       realloc (sequence->points,
                (sequence->point_count + argc) * sizeof(*(sequence->points)));

   points = sequence->points + sequence->point_count - 1;

   for (i = 1; i <= argc; ++i) {
      roadmap_sprite_decode_point (++points, argv[i]);
   }
   sequence->point_count += argc;
}


static void roadmap_sprite_decode_circle
               (RoadmapSpriteDrawingSequence *sequence, const char **argv) {

   sequence->object_count += 1;
   sequence->point_count += 1;
   sequence->objects =
       realloc (sequence->objects,
                sequence->object_count * sizeof(*(sequence->objects)));

   sequence->objects[sequence->object_count-1] = atoi(argv[2]);

   sequence->points =
       realloc (sequence->points,
                sequence->object_count * sizeof(*(sequence->points)));

   roadmap_sprite_decode_point
      (sequence->points+sequence->object_count-1, argv[1]);
}


static RoadMapSprite roadmap_sprite_new
          (int argc, const char **argv, int *argl) {

   RoadMapSprite sprite = malloc(sizeof(*sprite));

   roadmap_check_allocated(sprite);

   memset (sprite, 0, sizeof(*sprite));

   sprite->name = roadmap_sprite_string (argv[1], argl[1]);

   sprite->first.pen = roadmap_canvas_create_pen ("Black");

   sprite->next = RoadMapSpriteList;
   RoadMapSpriteList = sprite;

   return sprite;
}


static int roadmap_sprite_maxpoint (RoadMapSprite sprite) {

   int max_point_count = 0;

   RoadMapSpritePlane *plane;

   for (plane = &(sprite->first); plane != NULL; plane = plane->next) {

      if (max_point_count < plane->lines.point_count) {
         max_point_count = plane->lines.point_count;
      }
      if (max_point_count < plane->disks.point_count) {
         max_point_count = plane->disks.point_count;
      }
      if (max_point_count < plane->circles.point_count) {
         max_point_count = plane->circles.point_count;
      }
      if (max_point_count < plane->polygons.point_count) {
         max_point_count = plane->polygons.point_count;
      }
   }
   return max_point_count;
}


static void roadmap_sprite_load (const char *data, int size) {

   int argc;
   int argl[256];
   const char *argv[256];

   const char *p;
   const char *end;

   RoadMapSprite sprite = NULL;


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

      if (sprite == NULL) {

         if (argv[0][0] != 'S') {
            roadmap_log (ROADMAP_ERROR, "sprite name is missing");
            break;
         }

      } else if (sprite->last == NULL) {

         if (argv[0][0] != 'F') {
            roadmap_log (ROADMAP_ERROR,
                         "first plane of sprite %s is missing", sprite->name);
            break;
         }
      }

      switch (argv[0][0]) {

      case 'F':

         roadmap_sprite_decode_plane (sprite, argv[1], argl[1], argv[2]);
         break;

      case 'L':

         roadmap_sprite_decode_sequence (&(sprite->last->lines), argc, argv);
         break;

      case 'P':

         roadmap_sprite_decode_sequence (&(sprite->last->polygons), argc, argv);
         break;

      case 'D':

         roadmap_sprite_decode_circle (&(sprite->last->disks), argv);
         break;

      case 'C':

         roadmap_sprite_decode_circle (&(sprite->last->circles), argv);
         break;

      case 'S':

         sprite = roadmap_sprite_new (argc, argv, argl);
      }

      while (p < end && *p < ' ') p++;
      data = p;
   }
}


static void roadmap_sprite_place (RoadmapSpriteDrawingSequence *sequence,
                                  RoadMapGuiPoint *location,
                                  int orientation) {

   int i;
   int x = location->x;
   int y = location->y;
   RoadMapGuiPoint *from = sequence->points;
   RoadMapGuiPoint *to   = RoadMapSpritePoints;

   for (i = sequence->point_count - 1; i >= 0; --i) {

      to->x = x + from->x;
      to->y = y + from->y;

      to += 1;
      from += 1;
   }
   roadmap_math_rotate_object
      (sequence->point_count, RoadMapSpritePoints, location, orientation);
}


static RoadMapSprite roadmap_sprite_search (const char *name) {

   RoadMapSprite cursor;

   for (cursor = RoadMapSpriteList; cursor != NULL; cursor = cursor->next) {

      if (strcasecmp(name, cursor->name) == 0) {
         return cursor;
      }
   }

   return RoadMapSpriteDefault;
}


void roadmap_sprite_draw
        (const char *name, RoadMapGuiPoint *location, int orientation) {

   RoadMapSprite sprite = roadmap_sprite_search (name);
   RoadMapSpritePlane *plane;


   if (sprite == NULL) return;

   for (plane = &(sprite->first); plane != NULL; plane = plane->next) {

      roadmap_canvas_select_pen (plane->pen);

      if (plane->polygons.object_count > 0) {

         roadmap_sprite_place (&(plane->polygons), location, orientation);

         roadmap_canvas_draw_multiple_polygons
            (plane->polygons.object_count,
             plane->polygons.objects, RoadMapSpritePoints, 1, 0);
      }

      if (plane->disks.object_count > 0) {

         roadmap_sprite_place (&(plane->disks), location, orientation);

         roadmap_canvas_draw_multiple_circles
            (plane->disks.object_count,
             RoadMapSpritePoints, plane->disks.objects, 1, 0);
      }

      if (plane->lines.object_count > 0) {

         roadmap_sprite_place (&(plane->lines), location, orientation);

         roadmap_canvas_draw_multiple_lines
            (plane->lines.object_count,
             plane->lines.objects, RoadMapSpritePoints, 0);
      }

      if (plane->circles.object_count > 0) {

         roadmap_sprite_place (&(plane->circles), location, orientation);

         roadmap_canvas_draw_multiple_circles
            (plane->circles.object_count,
             RoadMapSpritePoints, plane->circles.objects, 0, 0);
      }
   }
}


void roadmap_sprite_initialize (void) {

   int max_point_count;
   const char *cursor;

   RoadMapSprite sprite;
   RoadMapFileContext file;


   for (cursor = roadmap_file_map ("config", "sprites", NULL, "r", &file);
        cursor != NULL;
        cursor = roadmap_file_map ("config", "sprites", cursor, "r", &file)) {

      roadmap_sprite_load (roadmap_file_base(file), roadmap_file_size(file));

      roadmap_file_unmap (&file);
   }

   for (cursor = roadmap_file_map ("user", "sprites", NULL, "r", &file);
        cursor != NULL;
        cursor = roadmap_file_map ("user", "sprites", cursor, "r", &file)) {

      roadmap_sprite_load (roadmap_file_base(file), roadmap_file_size(file));

      roadmap_file_unmap (&file);
   }

   RoadMapSpriteDefault = roadmap_sprite_search ("Default");

   if (RoadMapSpriteDefault == NULL) {

      const char *hardcoded = "S Default\nF black 1\nP -4,-4 4,-4 4,4 -4,4\n";

      roadmap_sprite_load (hardcoded, strlen(hardcoded));

      RoadMapSpriteDefault = roadmap_sprite_search ("Default");
   }


   /* Allocate the space required to draw any configured sprite: */

   RoadMapSpritePointCount = roadmap_sprite_maxpoint (RoadMapSpriteDefault);

   for (sprite = RoadMapSpriteList; sprite != NULL; sprite = sprite->next) {

      max_point_count = roadmap_sprite_maxpoint (sprite);

      if (RoadMapSpritePointCount < max_point_count) {
         RoadMapSpritePointCount = max_point_count;
      }
   }
   RoadMapSpritePoints =
      calloc (RoadMapSpritePointCount, sizeof(*RoadMapSpritePoints));
}

