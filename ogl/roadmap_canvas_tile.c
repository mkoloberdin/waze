/* roadmap_canvas_tile.c - manage the canvas tiles
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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
 * SYNOPSYS:
 *
 *   See roadmap_canvas_tile.h.
 */


#define ROADMAP_CANVAS_TILE_C

#include <math.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_time.h"
#include "roadmap_screen.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h"
#include "roadmap_canvas_tile.h"

#ifdef IPHONE_NATIVE
#include "roadmap_main.h"
#endif


#define ROUND(_r) ((_r > 0.0) ? floorf(_r + 0.5) : ceilf(_r - 0.5))

#define MAX_LAYERS         1
#define MAX_DRAW_TIME      35
#define TILE_PAD           0 //tile rows/columns to pad around screen
#define DEFAULT_SIZE       256
#define DEFAULT_SIZE_HD    512
#define MATRIX_SIZE        40
#define MATRIX_OFFSET      2
#define DRAW_TILE_EDGES    0
#define ALLOW_X2_SCALE     1
#define ZOOM_STEP          4

typedef struct {
   zoom_t               prev_zoom;
   zoom_t               new_zoom;
   zoom_t               draw_zoom;
   zoom_t               target_zoom;
   RoadMapGuiPointF     point;
   RoadMapGuiPointF     delta;
   int                  prev_orientation;
   int                  new_orientation;
   int                  prev_is_3d;
   int                  new_is_3d;
   
   RoadMapCanvasTileCb  callback;
   int                  tile_w;
   int                  tile_h;
   RoadMapGuiPoint      top_left;
   RoadMapGuiPoint      top_right;
   RoadMapGuiPoint      bottom_left;
   RoadMapGuiPoint      bottom_right;
   
   BOOL                 active;
   BOOL                 should_resize;
} canvas_layer;

static canvas_layer gs_layers[MAX_LAYERS];


#if ALLOW_X2_SCALE
#define MAX_TILES_ARR    75
#else
#define MAX_TILES_ARR    70
#endif

typedef struct {
   int               layer_id;
   int               tile_id;
   RoadMapGuiPointF  top_left_point;
   zoom_t            zoom;
   int               opacity;
   int               drawn;
   int               age;
   int               dirty;
   RoadMapImage      image;
} canvas_tile;

static canvas_tile gs_tiles[MAX_TILES_ARR];

typedef struct {
   canvas_tile       *tile;
   int               priority;
   int               scale;
} canvas_tile_matrix;


#define MAX_PRIORITIES     4

enum priority_modes {
   UPDATE_MODE,
   GENERATE_MODE,
   DRAW_MODE
};

enum tile_distances {
   TILE_DISTANCE_NEAR,
   TILE_DISTANCE_NORMAL,
   TILE_DISTANCE_FAR
};


static BOOL initialized = FALSE;
static int  gs_horizon = 0;
static RoadMapCanvasTileOnDeleteCb gs_on_delete_cb = NULL;
static int gs_last_tile_id = 0;
static BOOL gs_is_ipad = FALSE;

#define MAX_TILES (gs_is_ipad ? 75 : 45)

/////////////////////////////////////////////////////////////////////////////////////
static void init_layer (int i) {
   gs_layers[i].prev_zoom = -1;
   gs_layers[i].new_zoom = -1;
   gs_layers[i].target_zoom = -1;
   gs_layers[i].draw_zoom = -1;
   gs_layers[i].point.x = gs_layers[i].point.y = 0;
   gs_layers[i].delta.x = gs_layers[i].delta.y = 0;
   gs_layers[i].prev_orientation = -1;
   gs_layers[i].new_orientation = -1;
   gs_layers[i].new_is_3d = -1;
   gs_layers[i].prev_is_3d = -1;
   
   if (roadmap_screen_is_hd_screen()) {
      gs_layers[i].tile_h = DEFAULT_SIZE_HD;
      gs_layers[i].tile_w = DEFAULT_SIZE_HD;
   } else {
      gs_layers[i].tile_h = DEFAULT_SIZE;
      gs_layers[i].tile_w = DEFAULT_SIZE;
   }
   
   gs_layers[i].active = FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
static void init (void) {
   int i;
   
   if (initialized)
      return;
   
#ifdef IPHONE_NATIVE
   gs_is_ipad = (roadmap_main_get_platform() == ROADMAP_MAIN_PLATFORM_IPAD);
#endif
   
   for (i = 0; i < MAX_LAYERS; i++) {
      init_layer(i);
   }
   
   for (i = 0; i < MAX_TILES; i++) {
      gs_tiles[i].layer_id = -1;
      gs_tiles[i].opacity = 0;
      gs_tiles[i].image = NULL;
      gs_tiles[i].dirty = 0;
   }
   
   roadmap_canvas_stop_draw_to_image(); //so we capture the screen buffer (workaround)
   
   initialized = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////
static BOOL valid_id (int layer_id) {
   if (layer_id > MAX_LAYERS -1 || layer_id < 0) {
      roadmap_log(ROADMAP_ERROR, "Invalid layer id: %d max expected is: %d", layer_id, MAX_LAYERS -1);
      return FALSE;
   } else {
      return TRUE;
   }

}

/////////////////////////////////////////////////////////////////////////////////////
static void delete_tile (canvas_tile *tile) {
   tile->layer_id = -1;
   
   if (gs_on_delete_cb)
      gs_on_delete_cb(tile->tile_id);
}

/////////////////////////////////////////////////////////////////////////////////////
static void refresh_zoom_step (int layer_id) {
   canvas_layer *layer = &gs_layers[layer_id];
   
   layer->new_zoom = layer->draw_zoom; //bypass this function
   return;
   
   if (!layer->active)
      return;
   
   if (layer->target_zoom == -1) {
      layer->new_zoom = layer->draw_zoom;
      return;
   }
   
   if ((layer->target_zoom > layer->draw_zoom && layer->target_zoom > layer->new_zoom && layer->new_zoom > layer->draw_zoom) || 
       (layer->target_zoom < layer->draw_zoom && layer->target_zoom < layer->new_zoom && layer->new_zoom < layer->draw_zoom)) {
      return;
   }
   
   if (layer->target_zoom / layer->draw_zoom > ZOOM_STEP || layer->new_zoom / layer->target_zoom > ZOOM_STEP) {
      if (layer->target_zoom < layer->draw_zoom) {
         layer->new_zoom = layer->draw_zoom / ZOOM_STEP;
      } else {
         layer->new_zoom = layer->draw_zoom * ZOOM_STEP;
      }
   } else if (layer->new_zoom != layer->target_zoom){
      layer->new_zoom = layer->target_zoom;
   }
   
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_set_target_zoom (int layer_id, zoom_t zoom) {
   canvas_layer *layer = &gs_layers[layer_id];
   
   if (!layer->active)
      return;
   
   if (layer->target_zoom != zoom) {
      layer->target_zoom = zoom;
   }
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_set_horizon (int horizon,int is_projection) {
   gs_horizon = -horizon;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_resize (int layer_id) {
   canvas_layer *layer = &gs_layers[layer_id];
   
   if (!layer->active)
      return;
   
   layer->should_resize = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////
static void roadmap_canvas_tile_resize_internal (int layer_id) {
   canvas_layer *layer = &gs_layers[layer_id];
   
   if (layer->new_is_3d) {
      RoadMapGuiPoint point;
      
      point.x = 0;
      point.y = gs_horizon;
      roadmap_canvas3_unproject (&point);
      layer->top_left = point;
      
      point.x = roadmap_canvas_width() -1;
      point.y = gs_horizon;
      roadmap_canvas3_unproject (&point);
      layer->top_right = point;
      
      point.x = 0;
      point.y = roadmap_canvas_height() -1;
      roadmap_canvas3_unproject (&point);
      layer->bottom_left = point;
      
      point.x = roadmap_canvas_width() -1;
      point.y = roadmap_canvas_height() -1;
      roadmap_canvas3_unproject (&point);
      layer->bottom_right = point;      
   } else {
      layer->top_left.x = 0;
      layer->top_right.x = roadmap_canvas_width() -1;;
      layer->bottom_left.x = 0;
      layer->bottom_right.x = roadmap_canvas_width() -1;
      
      layer->top_left.y = 0;
      layer->top_right.y = 0;
      layer->bottom_left.y = roadmap_canvas_height() -1;
      layer->bottom_right.y = roadmap_canvas_height() -1;
   }
}

/////////////////////////////////////////////////////////////////////////////////////
static void update_tiles_pos (int layer_id, float zoom_factor, float dx, float dy) {
   int i;

   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id == layer_id) {
         gs_tiles[i].top_left_point.x = gs_tiles[i].top_left_point.x*zoom_factor + dx;
         gs_tiles[i].top_left_point.y = gs_tiles[i].top_left_point.y*zoom_factor + dy;
         if (zoom_factor != 1.0f)
            gs_tiles[i].dirty = 1;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_reset_all(int layer_id, int dirty) {
   int i;
   
   for (i = 0; i < MAX_TILES; i++)
      if (gs_tiles[i].layer_id == layer_id) {
         if (!dirty)
            delete_tile (&gs_tiles[i]);
         else
            gs_tiles[i].dirty = 1;
      }
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_set (int layer_id, zoom_t zoom, RoadMapGuiPointF *new_delta, int orientation, int is_3d, RoadMapCanvasTileCb callback, int refresh){
   canvas_layer *layer;
   
   if (!valid_id(layer_id)) {
      return;
   }
   
   if (!initialized)
      init();
   
   layer = &gs_layers[layer_id];
   
   if (!layer->active || refresh) {
      if (zoom <= 0 || new_delta == NULL || orientation < 0 || is_3d < 0 || callback == NULL) {
         roadmap_log(ROADMAP_ERROR, "Cannot initialize new layer (id: %d) with missing properties !", layer_id);
         return;
      }
      layer->active = TRUE;
   }
   
   if ((new_delta != NULL) &&
       (new_delta->x > 10000 || new_delta->y > 10000)) {
         roadmap_canvas_tile_reset_all(layer_id, 0);
         init_layer(layer_id);
         layer->active = TRUE;
      new_delta = NULL;
   }
         
   if (zoom > 0) {
      layer->draw_zoom = zoom;
      
      refresh_zoom_step(layer_id);
      
      if (layer->prev_zoom == -1)
         layer->prev_zoom = layer->new_zoom;
      
      
   }
   
   if (orientation >= 0)
      layer->new_orientation = orientation;
   
   if (is_3d >= 0)
      layer->new_is_3d = is_3d;
   
   if (new_delta != NULL) {
      float dx, dy;
      float prev_new_factor = 1.0f * layer->prev_zoom / layer->new_zoom;
      float draw_new_factor = 1.0f * layer->draw_zoom / layer->new_zoom;
      
      //Update tiles
      dx = new_delta->x * draw_new_factor + roadmap_canvas_width()*0.5 - (roadmap_canvas_width()*0.5) * prev_new_factor;
      dy = new_delta->y * draw_new_factor + roadmap_canvas_height()*0.5 - (roadmap_canvas_height()*0.5) * prev_new_factor;
      update_tiles_pos(layer_id, prev_new_factor, dx, dy);
      
      
      //Update layer
      layer->point.x = layer->point.x * prev_new_factor + dx;
      layer->point.y = layer->point.y * prev_new_factor + dy;
   }
   
   
   if (callback != NULL)
      layer->callback = callback;
}

static canvas_tile *get_free_tile(int layer_id) {
   int i;
   int index_x = -1, index_y = -1;

   //pick unused tile
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id == -1) {
         return &gs_tiles[i];
      }
   }
   
   if (layer_id == -1)
      return NULL;
   
   //pick old tile
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].age >= 3) {
         delete_tile (&gs_tiles[i]);
         return &gs_tiles[i];
      }
   }
   
   //pick old tile
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].age >= 1) {
         delete_tile (&gs_tiles[i]);
         return &gs_tiles[i];
      }
   }
   
   //find the most unusable tile to pick
   for (i = 1; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id == layer_id &&
          gs_tiles[i].zoom != gs_layers[layer_id].new_zoom &&
          gs_tiles[i].zoom != gs_layers[layer_id].new_zoom *2) {
         if (index_x == -1) {
            index_x = index_y = i;
         } else {
            if (abs(gs_tiles[i].top_left_point.x - gs_layers[layer_id].point.x) > abs(gs_tiles[index_x].top_left_point.x - gs_layers[layer_id].point.x))
               index_x = i;
            if (abs(gs_tiles[i].top_left_point.y - gs_layers[layer_id].point.y) > abs(gs_tiles[index_y].top_left_point.y - gs_layers[layer_id].point.y))
               index_y = i;
         }
      }
   }
   
   if (index_x == -1)
      return NULL;

   if ((index_x != index_y) &&
       abs(gs_tiles[index_y].top_left_point.y - gs_layers[layer_id].point.y) > abs(gs_tiles[index_x].top_left_point.x - gs_layers[layer_id].point.x)) {
      delete_tile (&gs_tiles[index_y]);  
      return &gs_tiles[index_y];
   } else {
      delete_tile (&gs_tiles[index_x]);
      return &gs_tiles[index_x];
   }
}

#if DRAW_TILE_EDGES
static void draw_edges (RoadMapGuiRect *rect, int priority, int scale) {
   RoadMapPen saved_pen;
   static RoadMapPen edge_pen = NULL;
   RoadMapGuiPoint points[5];
   int lines = 5;
   
   //if (!edge_pen) {
      edge_pen  = roadmap_canvas_create_pen ("CanvasTile.Edge");
      roadmap_canvas_set_thickness (20);
   if (scale == 2) {
      roadmap_canvas_set_foreground ("#ff00ff2f");
   } else if (priority == 0)
      roadmap_canvas_set_foreground ("#ff00002f");
   else if (priority == 1)
      roadmap_canvas_set_foreground ("#00ff002f");
   else
      roadmap_canvas_set_foreground ("#0000ff2f");
   //}
   
   saved_pen = roadmap_canvas_select_pen(edge_pen);
   
   points[0].x = rect->minx;
   points[0].y = rect->miny;
   points[1].x = rect->maxx;
   points[1].y = rect->miny;
   points[2].x = rect->maxx;
   points[2].y = rect->maxy;
   points[3].x = rect->minx;
   points[3].y = rect->maxy;
   points[4].x = rect->minx;
   points[4].y = rect->miny;
   
   roadmap_canvas_draw_multiple_lines(1, &lines, points, 0);
   
   roadmap_canvas_select_pen(saved_pen);
}
#endif


static inline float get_offset(float value, int size, zoom_t current_zoom, zoom_t new_zoom){
   float res;
   
   value *= current_zoom/new_zoom;
   res = (value - ((int)(value / size)) * size);
   
   if (res < 0)
      res += size;
   
   return res;
}


static int tile_distance (canvas_tile *tile, canvas_layer *layer) {
   int layer_height = layer->bottom_left.y - layer->top_left.y + 1;
   int canvas_height = roadmap_canvas_height();
   int i;
   RoadMapGuiPoint points[4];
   int minx, miny, maxx, maxy;   
   
   if ((layer_height <= canvas_height*2 && !gs_is_ipad) ||
       (layer_height <= canvas_height && gs_is_ipad))
      return TILE_DISTANCE_NORMAL;
   
   
   points[0].x = ROUND(tile->top_left_point.x);
   points[0].y = ROUND(tile->top_left_point.y);
   points[1].x = ROUND(tile->top_left_point.x);
   points[1].y = ROUND(tile->top_left_point.y + layer->tile_h -1);
   points[2].x = ROUND(tile->top_left_point.x + layer->tile_w -1);
   points[2].y = ROUND(tile->top_left_point.y + layer->tile_h -1);
   points[3].x = ROUND(tile->top_left_point.x + layer->tile_w -1);
   points[3].y = ROUND(tile->top_left_point.y);
   
   roadmap_math_rotate_coordinates(4, points);
   
   miny = points[0].y;
   maxy = points[0].y;
   for (i = 1; i < 4; i++) {
      if (points[i].y < miny)
         miny = points[i].y;
      if (points[i].y > maxy)
         maxy = points[i].y;
   }
   
   if ((layer->bottom_left.y - maxy > canvas_height*3.0 && !gs_is_ipad) ||
       (layer->bottom_left.y - maxy > canvas_height*1.5 && gs_is_ipad))
      return TILE_DISTANCE_FAR;
   
   if (layer->bottom_left.y - miny < canvas_height)
      return TILE_DISTANCE_NEAR;
   
   return TILE_DISTANCE_NORMAL;
}

static int tile_is_visible (canvas_tile *tile, canvas_layer *layer) {
//return value: 0 (not visible), 1 (fully visible) or -1 (partially visible).

   int i;
   RoadMapGuiPoint points[4];
   int minx, miny, maxx, maxy;
   int result = 1;
   int margin = 5;
   float scale = tile->zoom / layer->new_zoom;
   
   points[0].x = ROUND(tile->top_left_point.x);
   points[0].y = ROUND(tile->top_left_point.y);
   points[1].x = ROUND(tile->top_left_point.x);
   points[1].y = ROUND(tile->top_left_point.y + layer->tile_h * scale -1);
   points[2].x = ROUND(tile->top_left_point.x + layer->tile_w * scale -1);
   points[2].y = ROUND(tile->top_left_point.y + layer->tile_h * scale -1);
   points[3].x = ROUND(tile->top_left_point.x + layer->tile_w * scale -1);
   points[3].y = ROUND(tile->top_left_point.y);
   
   roadmap_math_rotate_coordinates(4, points);
   
   miny = points[0].y;
   maxy = points[0].y;
   minx = points[0].x;
   maxx = points[0].x;
   for (i = 1; i < 4; i++) {
      if (points[i].y < miny)
         miny = points[i].y;
      if (points[i].y > maxy)
         maxy = points[i].y;
      if (points[i].x < minx)
         minx = points[i].x;
      if (points[i].x > maxx)
         maxx = points[i].x;
   }
   
   if (0 && !layer->new_is_3d) { //2D
      if (maxx <= layer->bottom_right.x + margin &&
          maxy <= layer->bottom_right.y + margin &&
          minx >= layer->top_left.x - margin &&
          miny >= layer->top_left.y - margin)
         return 1;
      else if (((minx <= layer->bottom_right.x + margin &&
                 minx >= layer->top_left.x - margin) ||
                (maxx >= layer->top_left.x - margin &&
                 maxx <= layer->bottom_right.x + margin)) &&
               ((miny <= layer->bottom_right.y + margin &&
                 miny >= layer->top_left.y - margin) ||
                (maxy >= layer->top_left.y - margin &&
                 maxy <= layer->bottom_right.y + margin)))
         return -1;
      else
         return 0;
   } else { //3D
      if (miny > layer->bottom_right.y ||
          maxy < layer->top_left.y) {
         return 0;
      }
      
      if (maxy > layer->bottom_right.y ||
          miny < layer->top_left.y) {
         result = -1;
      }
      
      
      int deltax = layer->bottom_left.x - layer->top_left.x;
      int deltay = layer->bottom_right.y - layer->top_right.y;
      float factor = 1.0*deltax / deltay;
      if (miny < layer->top_left.y)
         miny = layer->top_left.y;//maxy;
      if (maxy > layer->bottom_left.y)
         maxy = layer->bottom_left.y;//miny;
      int midy = (maxy + miny) /2;
      int xpos_t = (layer->bottom_left.y - miny)*factor;
      int xpos = (layer->bottom_left.y - midy)*factor;
      
      if (maxx < layer->bottom_left.x - xpos_t ||
          minx > layer->bottom_right.x + xpos_t)
         return 0;
      
      if (maxx < layer->bottom_left.x - xpos ||
          minx > layer->bottom_right.x + xpos)
         return -1;
      
      if (minx <= layer->bottom_left.x - xpos ||
          maxx >= layer->bottom_right.x + xpos)
         return -1;
      
      return result;
     
   }
}

static void prioritize_matrix (canvas_tile_matrix *matrix, canvas_layer *layer, int mode) {
   int i, j, k;
   canvas_tile temp_tile, *ptile;
   float x_offset, y_offset;
   int is_visible;
   int distance;
  
   x_offset = get_offset(layer->point.x - layer->top_left.x, layer->tile_w, layer->new_zoom, layer->new_zoom);
   y_offset = get_offset(layer->point.y - layer->top_left.y, layer->tile_h, layer->new_zoom, layer->new_zoom);
   
   for (i = 0; i < MATRIX_SIZE; i++) { //rows
      for (j = 0; j < MATRIX_SIZE; j++) { //columns
         canvas_tile_matrix *m = &matrix[i*MATRIX_SIZE + j];
         if (m->tile != NULL) {
            ptile = m->tile;
         } else {
            ptile = &temp_tile;
            ptile->top_left_point.x = layer->top_left.x + (j - MATRIX_OFFSET -1) * layer->tile_w + x_offset;
            ptile->top_left_point.y = layer->top_left.y + (i - MATRIX_OFFSET -1) * layer->tile_h + y_offset;
            ptile->zoom = layer->new_zoom;
         }

         is_visible = tile_is_visible(ptile, layer);
         distance = tile_distance(ptile, layer);
         switch (mode) {
            case UPDATE_MODE:
               if (is_visible != 0) {
                  m->priority = 0;
               } else {
                  m->priority = -1;
               }
               break;
            case GENERATE_MODE:
               if (is_visible == 1) {
                  if (distance == TILE_DISTANCE_NEAR)
                     m->priority = 0;
                  else
                     m->priority = 1;
               } else if (is_visible == -1) {
                  if (distance == TILE_DISTANCE_NEAR)
                     m->priority = 0;
                  else
                     m->priority = 2;
               } else {
                  m->priority = -1;
               }
               break;
            case DRAW_MODE:
               if (m->tile != NULL &&
                   is_visible != 0) {
                  m->priority = 0;
               } else if (m->tile != NULL &&
                          is_visible != 0) {
                  m->priority = 1;
               } else if (is_visible != 0) {
                  m->priority = 2;
               } else {
                  m->priority = -1;
               }
               break;
               
            default:
               break;
         }
#if ALLOW_X2_SCALE
         if (distance == TILE_DISTANCE_FAR)
         {
            if (mode == GENERATE_MODE)
               m->scale = 0; //flag for scale 2
            else
               m->scale = 2;
         }
         else
#endif //ALLOW_X2_SCALE
         {
            m->scale = 1;
         }
      }
   }      

   if (mode == GENERATE_MODE) {
      
      for (i = 0; i < MATRIX_SIZE - 1; i++) { //rows
         for (j = 0; j < MATRIX_SIZE - 1; j++) { //columns
            if (matrix[i*MATRIX_SIZE + j].priority != -1 &&
                matrix[i*MATRIX_SIZE + j].scale == 0 &&
                matrix[i*MATRIX_SIZE + j].tile != NULL) {
      
               matrix[i*MATRIX_SIZE + j].scale = 2;

               if (matrix[(i+1)*MATRIX_SIZE + (j)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + j].scale == 0) {
                  matrix[(i+1)*MATRIX_SIZE + j].priority = -1;
                  matrix[(i+1)*MATRIX_SIZE + j].scale = 2;
               }
               
               if (matrix[(i+1)*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + (j+1)].scale == 0) {
                  matrix[(i+1)*MATRIX_SIZE + (j+1)].priority = -1;
                  matrix[(i+1)*MATRIX_SIZE + (j+1)].scale = 2;
               }
               
               if (matrix[i*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[i*MATRIX_SIZE + (j+1)].scale == 0) {
                  matrix[i*MATRIX_SIZE + (j+1)].priority = -1;
                  matrix[i*MATRIX_SIZE + (j+1)].scale = 2;
               }
            }
         }
      }
      
      for (i = 0; i < MATRIX_SIZE - 1; i++) { //rows
         for (j = 0; j < MATRIX_SIZE - 1; j++) { //columns
            if (matrix[i*MATRIX_SIZE + j].priority != -1 &&
                matrix[i*MATRIX_SIZE + j].scale == 0) {
                   matrix[i*MATRIX_SIZE + j].scale = 2;
               
               if (matrix[(i+1)*MATRIX_SIZE + (j)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + j].scale == 0) {
                  matrix[(i+1)*MATRIX_SIZE + j].priority = -1;
                  matrix[(i+1)*MATRIX_SIZE + j].scale = 2;
               }
               
               if (matrix[(i+1)*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + (j+1)].scale == 0) {
                  matrix[(i+1)*MATRIX_SIZE + (j+1)].priority = -1;
                  matrix[(i+1)*MATRIX_SIZE + (j+1)].scale = 2;
               }
               
               if (matrix[i*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[i*MATRIX_SIZE + (j+1)].scale == 0) {
                  matrix[i*MATRIX_SIZE + (j+1)].priority = -1;
                  matrix[i*MATRIX_SIZE + (j+1)].scale = 2;
               }
            }
         }
      }
   } else if (mode == DRAW_MODE) {
      for (i = 0; i < MATRIX_SIZE - 1; i++) { //rows
         for (j = 0; j < MATRIX_SIZE - 1; j++) { //columns
            if (matrix[i*MATRIX_SIZE + j].priority != -1 &&
                matrix[i*MATRIX_SIZE + j].scale == 2 &&
                matrix[i*MATRIX_SIZE + j].tile != NULL) {
               
               if (matrix[(i+1)*MATRIX_SIZE + (j)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + j].scale == 2) {
                  matrix[(i+1)*MATRIX_SIZE + j].priority = -1;
               }
               
               if (matrix[(i+1)*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[(i+1)*MATRIX_SIZE + (j+1)].scale == 2) {
                  matrix[(i+1)*MATRIX_SIZE + (j+1)].priority = -1;
               }
               
               if (matrix[i*MATRIX_SIZE + (j+1)].priority != -1 &&
                   matrix[i*MATRIX_SIZE + (j+1)].scale == 2) {
                  matrix[i*MATRIX_SIZE + (j+1)].priority = -1;
               }
            }
         }
      }
   }
         

   //Add padding (priority 2)
   if (mode != DRAW_MODE) {
      int pad;
      switch (mode) {
         case UPDATE_MODE:
            pad = TILE_PAD + 1;
            break;
         case GENERATE_MODE:
            pad = TILE_PAD;
            break;
         default:
            break;
      }
      for (i = TILE_PAD; i < MATRIX_SIZE - TILE_PAD; i++) { //rows
         for (j = TILE_PAD; j < MATRIX_SIZE - TILE_PAD; j++) { //columns
            canvas_tile_matrix *m = &matrix[i*MATRIX_SIZE + j];
            canvas_tile_matrix *m2;
            
            if (m->priority >= 0 && m->priority < 2) {
               for (k = 1; k <= pad; k++) {
                  m2 = &matrix[(i-k)*MATRIX_SIZE + j];
                  if (m2->priority < 0)
                     m2->priority = 2;
                  m2 = &matrix[i*MATRIX_SIZE + (j-k)];
                  if (m2->priority < 0)
                     m2->priority = 2;
                  m2 = &matrix[(i+k)*MATRIX_SIZE + j];
                  if (m2->priority < 0)
                     m2->priority = 2;
                  m2 = &matrix[i*MATRIX_SIZE + (j+k)];
                  if (m2->priority < 0)
                     m2->priority = 2;
                  if (k == 1) {
                     m2 = &matrix[(i-k)*MATRIX_SIZE + (j-k)];
                     if (m2->priority < 0)
                        m2->priority = 2;
                     m2 = &matrix[(i+k)*MATRIX_SIZE + (j+k)];
                     if (m2->priority < 0)
                        m2->priority = 2;
                     m2 = &matrix[(i+k)*MATRIX_SIZE + (j-k)];
                     if (m2->priority < 0)
                        m2->priority = 2;
                     m2 = &matrix[(i-k)*MATRIX_SIZE + (j+k)];
                     if (m2->priority < 0)
                        m2->priority = 2;
                  }
               }
            }
         }
      }
   }      
}
     

static void dump_obsolete_tiles (int layer_id) {
   int i;
   canvas_layer *layer = &gs_layers[layer_id];
   int count_prev_zoom = 0;
   int count_other_zoom = 0;
   
   if (gs_is_ipad)
      return;
   
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id == layer_id &&
          gs_tiles[i].zoom != layer->new_zoom &&
          gs_tiles[i].zoom != layer->new_zoom *2 &&
          gs_tiles[i].zoom != layer->prev_zoom) {
         count_other_zoom++;
      } else if (gs_tiles[i].layer_id == layer_id &&
                 gs_tiles[i].zoom == layer->prev_zoom) {
         count_prev_zoom++;
      }
   }
   
   if (count_other_zoom > count_prev_zoom) {
      for (i = 0; i < MAX_TILES; i++) {
         if (gs_tiles[i].layer_id == layer_id &&
             gs_tiles[i].zoom == layer->prev_zoom) {
            delete_tile (&gs_tiles[i]);
         }
      }
   } else {
      for (i = 0; i < MAX_TILES; i++) {
         if (gs_tiles[i].layer_id == layer_id &&
             gs_tiles[i].zoom != layer->new_zoom &&
             gs_tiles[i].zoom != layer->new_zoom *2 &&
             gs_tiles[i].zoom != layer->prev_zoom) {
            delete_tile (&gs_tiles[i]);
         }
      }
   }

}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_canvas_tile_draw (int layer_id, int fast_refresh) {
   canvas_tile_matrix tile_matrix[MATRIX_SIZE*MATRIX_SIZE];
   canvas_tile *tile;
   canvas_layer *layer;
   int i, j, p;
   int width, height;
   float x_offset, y_offset;
   BOOL new_tile_image_drawn = FALSE;
   uint32_t start_time = roadmap_time_get_millis();
   int full_draw = 0;
   int refresh_tiles = 0;
   int image_mode;
   int saved_orientation = roadmap_math_get_orientation();
   static int last_horizon = -1;
   BOOL stop;
   static BOOL zoom_changed = FALSE;
   static uint32_t last_refrsh = 0;
   int scale;
   int dirty;
   //printf("starting\n");
   if (!valid_id(layer_id)) {
      return 1;
   }
   
   layer = &gs_layers[layer_id];
   fast_refresh = (layer->target_zoom != layer->draw_zoom);
   if (!initialized) {
      roadmap_log(ROADMAP_ERROR, "Requested layer draw before init !");
      return 1;
   }
   
   if (layer->new_zoom <= 0)
      return 1;
   
   if (!fast_refresh) {
      last_refrsh = start_time;
      zoom_changed = FALSE;
   } else if (layer->prev_zoom != layer->new_zoom) {
      zoom_changed = TRUE;
   }
   
   if (zoom_changed && start_time - last_refrsh > 1500 &&
       layer->prev_zoom == layer->new_zoom) {
      last_refrsh = start_time;
      zoom_changed = FALSE;
   }
   
   if (layer->prev_zoom <= 0 ||
       layer->new_is_3d != layer->prev_is_3d) {      
      //Dump all tiles
      roadmap_canvas_tile_reset_all(layer_id, 0);
      
      refresh_tiles = 1;
   }

   if (refresh_tiles ||
       layer->new_is_3d && gs_horizon != last_horizon ||
       layer->should_resize) {
      last_horizon = gs_horizon;
      layer->should_resize = FALSE;
   
      //recalc size
      roadmap_canvas_tile_resize_internal(layer_id);
   }
   
   if (layer->new_zoom != layer->prev_zoom) {
      dump_obsolete_tiles (layer_id);
   }
   
   height = layer->tile_h;
   width = layer->tile_w;

   x_offset = get_offset(layer->point.x - layer->top_left.x, layer->tile_w, layer->new_zoom, layer->new_zoom);
   y_offset = get_offset(layer->point.y - layer->top_left.y, layer->tile_h, layer->new_zoom, layer->new_zoom);
   
   //init matrix
   for (i = 0; i < MATRIX_SIZE*MATRIX_SIZE; i++) {
      tile_matrix[i].tile = NULL;
   }
   
   prioritize_matrix(&tile_matrix[0], layer, UPDATE_MODE);
   //update tiles status
   int count_mapped_tiles = 0;
   if (!refresh_tiles){
      //Scan all tiles, adjust offset and check for missing tiles
      for (i = 0; i < MAX_TILES; i++) {
         if (gs_tiles[i].layer_id == layer_id) {
            int row, column;
            
            gs_tiles[i].drawn = 0;
            
            //if (gs_tiles[i].zoom == layer->new_zoom) {
            row = ROUND((gs_tiles[i].top_left_point.y - y_offset - layer->top_left.y) / height +1);
            column = ROUND((gs_tiles[i].top_left_point.x - x_offset - layer->top_left.x) / width +1);
            
            if (row + MATRIX_OFFSET < 0 || row + MATRIX_OFFSET >= MATRIX_SIZE || column + MATRIX_OFFSET < 0 || column + MATRIX_OFFSET >= MATRIX_SIZE ||
                tile_matrix[(row + MATRIX_OFFSET)*MATRIX_SIZE + (column + MATRIX_OFFSET)].priority == -1) {
            } else {
               //tile is in screen
               canvas_tile_matrix *m = &tile_matrix[(row + MATRIX_OFFSET)*MATRIX_SIZE + (column + MATRIX_OFFSET)];
               if (m->tile != NULL) {
                  gs_tiles[i].age++;
               } else if (gs_tiles[i].zoom / m->scale == layer->new_zoom ){
                  m->tile = &gs_tiles[i];
                  count_mapped_tiles++;
               }
            }
         }
      }
   }
   
   //Generate missing tiles
   stop = FALSE;
   if (!(fast_refresh && zoom_changed)) {
      int last_scale = -1;
      prioritize_matrix(&tile_matrix[0], layer, GENERATE_MODE);
      for (dirty = 0; dirty <= 1; dirty++) {//TODO: remove this loop and use the priority instead
         for (scale = 1; scale <= 2; scale++) {
            for (p = 0; p < MAX_PRIORITIES; p++) {
               for (i = MATRIX_SIZE -1; i >= 0; i--) {
                  for (j = MATRIX_SIZE -1; j >= 0; j--) {
                     canvas_tile_matrix *m = &tile_matrix[i*MATRIX_SIZE + j];
                     BOOL generated_tile = FALSE;
                     if (m->priority == p &&
                         m->scale == scale &&
                         (m->tile == NULL || m->tile->dirty)) {
                        //Missing tile, we need to draw it to buffer
                        RoadMapGuiRect rect;
                        float minx, miny;
                        if (m->tile == NULL) {
                           if (!new_tile_image_drawn || gs_is_ipad)
                              tile = get_free_tile(layer_id);
                           else {
                              tile = get_free_tile(-1);
                           }
                           
                           if (!tile) {
                              stop = TRUE;
                              break;
                           }
                           tile->dirty = 0;
                        } else {
                           if (m->tile->dirty != dirty)
                              continue;
                           tile = m->tile;
                        }
                        
                        if (!tile->image) {
                           tile->image = roadmap_canvas_new_image (width, height);
                           if (!tile->image) {
                              roadmap_log(ROADMAP_ERROR, "Buffer image not created !");
                              return 1;
                           }
                        }
                        
                        if (last_scale != scale) {
                           roadmap_math_zoom_set(layer->new_zoom * scale);
                           roadmap_layer_adjust();
                           last_scale = scale;
                        }
                        
                        if (tile->dirty) {
                           delete_tile(tile);
                           tile->dirty = 0;
                        } else {
                           tile->opacity = 0;
                        }
                        
                        tile->layer_id = layer_id;
                        tile->tile_id = ++gs_last_tile_id;
                        tile->age = 0;
                        tile->drawn = 0;
                        tile->zoom = layer->new_zoom * scale;
                        
                        tile->top_left_point.x = layer->top_left.x + (j - MATRIX_OFFSET -1) * width + x_offset;
                        tile->top_left_point.y = layer->top_left.y + (i - MATRIX_OFFSET -1) * height + y_offset;
                        
                        minx = tile->top_left_point.x / scale + roadmap_canvas_width()*0.5 - (roadmap_canvas_width()*0.5) / scale;
                        miny = tile->top_left_point.y / scale + roadmap_canvas_height()*0.5 - (roadmap_canvas_height()*0.5) / scale;
                        rect.minx = ROUND(minx);
                        rect.miny = ROUND(miny);
                        rect.maxx = rect.minx + width -1;
                        rect.maxy = rect.miny + height -1;
                        
                        if (!new_tile_image_drawn) {
                           roadmap_math_set_orientation(0);
                           roadmap_math_set_tile_visibility(1);
                        }
                        
                        roadmap_canvas_begin_draw_to_image (tile->image);
                        
                        glTranslatef(-rect.minx, -rect.miny, 0);
                        layer->callback(&rect);
#if DRAW_TILE_EDGES
                        draw_edges (&rect, p, scale);
#endif
                        
                        new_tile_image_drawn = TRUE;
                        
                        m->tile = tile;
                        generated_tile = TRUE;
                     }
                     if (generated_tile &&
                         roadmap_time_get_millis() - start_time > MAX_DRAW_TIME * (gs_is_ipad ? 1.5 : 1)) {
                        stop = TRUE;
                        break;
                     }
                  }
                  if (stop) {
                     break;
                  } //next j
               } //next i
               if (stop) {
                  break;
               }
            } //next p
            if (stop) {
               break;
            }
         } //next scale
         if (stop) {
            break;
         }
      } //next dirty
      if (!stop) {
         full_draw = 1;
      }
   }
   
   roadmap_math_zoom_set(layer->draw_zoom);
   roadmap_layer_adjust();

   if (new_tile_image_drawn) {
      roadmap_canvas_stop_draw_to_image();
      roadmap_math_set_orientation(saved_orientation);
      roadmap_math_set_tile_visibility(0);
   }
   
   
   glTranslatef(roadmap_canvas_width()/2, roadmap_canvas_height()/2, 0);
   glRotatef(layer->new_orientation, 0, 0, 1);
   glTranslatef(-roadmap_canvas_width()/2, -roadmap_canvas_height()/2, 0);

   prioritize_matrix(&tile_matrix[0], layer, DRAW_MODE);
   
   int drawn = 0;
   for (p = 2; p >= 2; p--) {
      for (i = 0; i < MATRIX_SIZE; i++) {
         for (j = 0; j < MATRIX_SIZE; j++) {
            canvas_tile_matrix *m = &tile_matrix[i*MATRIX_SIZE + j];
            canvas_tile *tile = m->tile;
            
            if (m->priority != p)
               continue;
            
            //we don't have a tile in this zoom, draw different zoom if available
            for (int n = 0; n < MAX_TILES; n++) {
               if (gs_tiles[n].layer_id == layer_id &&
                   gs_tiles[n].zoom > 0 &&
                   gs_tiles[n].zoom != layer->new_zoom &&
                   gs_tiles[n].drawn != 1) {
                  tile = &gs_tiles[n];
                  RoadMapGuiPoint top_left;
                  RoadMapGuiPoint bottom_right;
                  RoadMapGuiPoint mat_top_left;
                  RoadMapGuiPoint mat_bot_right;
                  RoadMapGuiPointF top_left_f;
                  
                  top_left_f.x = tile->top_left_point.x * layer->new_zoom / layer->draw_zoom + roadmap_canvas_width()*0.5 - (roadmap_canvas_width()*0.5) * layer->new_zoom / layer->draw_zoom;
                  top_left_f.y = tile->top_left_point.y * layer->new_zoom / layer->draw_zoom + roadmap_canvas_height()*0.5 - (roadmap_canvas_height()*0.5) * layer->new_zoom / layer->draw_zoom;
                  top_left.x = ROUND(top_left_f.x);
                  top_left.y = ROUND(top_left_f.y);
                  bottom_right.x = ROUND(top_left_f.x + 1.0* width * tile->zoom / layer->draw_zoom);
                  bottom_right.y = ROUND(top_left_f.y + 1.0* height * tile->zoom / layer->draw_zoom);
                  
                  mat_top_left.x = layer->top_left.x + (j - MATRIX_OFFSET -1) * width + x_offset;
                  mat_top_left.y = layer->top_left.y + (i - MATRIX_OFFSET -1) * height + y_offset;
                  mat_bot_right.x = mat_top_left.x + width -1;
                  mat_bot_right.y = mat_top_left.y + height -1;
                  if (top_left.x <= mat_bot_right.x &&
                      top_left.y <= mat_bot_right.y &&
                      bottom_right.x >= mat_top_left.x &&
                      bottom_right.y >= mat_top_left.y) {
                     tile->opacity = 255;
                     image_mode = IMAGE_NOBLEND;
                     roadmap_canvas_draw_image_scaled(tile->image, &top_left, &bottom_right, tile->opacity, image_mode);
                     gs_tiles[n].drawn = 1;
                     drawn++;
                  }

               }               
            }
         }
      }
   }
   
   
   //Draw the tiles
   drawn = 0;
   for (p = 1; p >= 0; p--) {
      for (i = 0; i < MATRIX_SIZE; i++) {
         for (j = 0; j < MATRIX_SIZE; j++) {
            int is_visible = 0;
            canvas_tile_matrix *m = &tile_matrix[i*MATRIX_SIZE + j];
            canvas_tile *tile = m->tile;
            
            if (m->priority != p)
               continue;

            
            RoadMapGuiPoint pos;
            pos.x = ROUND(tile->top_left_point.x);
            pos.y = ROUND(tile->top_left_point.y);
            
            is_visible = tile_is_visible(tile, layer);
            if (tile->opacity < 255) {
                  tile->opacity = 255;
            }
            
            if (tile->opacity < 255) {
               full_draw = 0;
               image_mode = IMAGE_NORMAL;
            } else {
               image_mode = IMAGE_NOBLEND;
            }
            
            RoadMapGuiPoint top_left;
            RoadMapGuiPoint bottom_right;
            RoadMapGuiPointF top_left_f;
            top_left_f.x = tile->top_left_point.x * layer->new_zoom / layer->draw_zoom + roadmap_canvas_width()*0.5 - (roadmap_canvas_width()*0.5) * layer->new_zoom / layer->draw_zoom;
            top_left_f.y = tile->top_left_point.y * layer->new_zoom / layer->draw_zoom + roadmap_canvas_height()*0.5 - (roadmap_canvas_height()*0.5) * layer->new_zoom / layer->draw_zoom;
            top_left.x = ROUND(top_left_f.x);
            top_left.y = ROUND(top_left_f.y);
            bottom_right.x = ROUND(top_left_f.x + 1.0* width * tile->zoom / layer->draw_zoom);
            bottom_right.y = ROUND(top_left_f.y + 1.0* height * tile->zoom / layer->draw_zoom);
            
            
            //roadmap_canvas_draw_image(tile->image, &pos, tile->opacity, image_mode);
            roadmap_canvas_draw_image_scaled(tile->image, &top_left, &bottom_right, tile->opacity, image_mode);
            
            tile->drawn = 1;
            tile->age = 0;
            drawn++;
         }
      }
   }
   glTranslatef(roadmap_canvas_width()/2, roadmap_canvas_height()/2, 0);
   glRotatef(-layer->new_orientation, 0, 0, 1);
   glTranslatef(-roadmap_canvas_width()/2, -roadmap_canvas_height()/2, 0);
   
   for (i = 0; i < MAX_TILES; i++) {
      canvas_tile *tile = &gs_tiles[i];
      if (tile != NULL && tile->layer_id == layer_id) {
         if (full_draw) {
            if (tile->zoom != layer->new_zoom &&
                tile->zoom != layer->new_zoom * 2&&
                !tile->drawn)
               tile->age++;
         } else if ( tile->drawn) {
            tile->age = 0;
         } else {
            tile->age++;                           
         }
         
         if (
             tile->age > 5){
            delete_tile (&gs_tiles[i]);
         }
      }
   }
   
   layer->delta.x = 0;
   layer->delta.y = 0;
   layer->prev_zoom = layer->new_zoom;
   layer->prev_orientation = layer->new_orientation;
   layer->prev_is_3d = layer->new_is_3d;
   
   return full_draw;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_canvas_tile_cached (int layer_id, RoadMapGuiPoint *point, zoom_t zoom) {
   int i;
   
   canvas_layer *layer = &gs_layers[layer_id];
   
   if (point->x < layer->top_left.x - (TILE_PAD +1)*layer->tile_w ||
       point->x > layer->top_right.x + (TILE_PAD +1)*layer->tile_w ||
       point->y < layer->top_left.y - (TILE_PAD +1)*layer->tile_h ||
       point->y > layer->bottom_left.y + (TILE_PAD +1)*layer->tile_h)
      return 0;
   
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id == layer_id &&
          gs_tiles[i].zoom == zoom) {
         if (gs_tiles[i].top_left_point.x <= point->x &&
             gs_tiles[i].top_left_point.y <= point->y &&
             gs_tiles[i].top_left_point.x + layer->tile_w -1 >= point->x &&
             gs_tiles[i].top_left_point.y + layer->tile_h -1 >= point->y)
            return 1;
      }
   }
   
   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_register_on_delete (RoadMapCanvasTileOnDeleteCb callback) {
   if (gs_on_delete_cb == NULL) //we currently support only single callback for this event
      gs_on_delete_cb = callback;
}

/////////////////////////////////////////////////////////////////////////////////////
int roadmap_canvas_tile_get_id (void) {
   return gs_last_tile_id;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_canvas_tile_free_all (void) {
   int i;
   
   roadmap_log(ROADMAP_WARNING, "Freeing all canvas tiles");
   
   for (i = 0; i < MAX_TILES; i++) {
      if (gs_tiles[i].layer_id != -1) {
            delete_tile (&gs_tiles[i]);
      }
      
      if (gs_tiles[i].image != NULL) {
         roadmap_canvas_free_image(gs_tiles[i].image);
         gs_tiles[i].image = NULL;
      }
   }
}
