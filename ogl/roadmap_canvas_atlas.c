/* roadmap_canvas_atlas.c - manage the textures atlas.
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
 *   See roadmap_canvas_atlas.h.
 */


#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_atlas.h"
#ifdef GTK2_OGL
#include <GL/gl.h>
#endif// GTK2_OGL


#define CANVAS_ATLAS_MAX_TREES   20

typedef struct node_t{
   struct node_t* child[2];
   RoadMapGuiRect rect;
   RoadMapImage   image;
} AtlasNode;

typedef struct root_t {
   struct node_t* root_node;
   char           hint[256];
   int            texture;
} AtlasRoot;

static AtlasRoot roots[CANVAS_ATLAS_MAX_TREES];



////////////////////////////////////////////////////////////
static AtlasNode *create_new_node(void) {
   AtlasNode *n = (AtlasNode *)malloc(sizeof(AtlasNode));
   roadmap_check_allocated(n);
   
   n->image = NULL;
   n->child[0] = NULL;
   n->child[1] = NULL;   

   return n;   
}

static AtlasRoot create_new_root (const char *hint) {
   AtlasRoot root;
   GLuint texture;
   GLubyte* temp_buf;
   int i, j;
   
   roadmap_log (ROADMAP_DEBUG, "roadmap_canvas_atlas - creating new root for hint '%s'", hint);

   glGenTextures( 1, &texture);
   roadmap_canvas_bind(texture);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   check_gl_error();
   temp_buf= malloc(4 * CANVAS_ATLAS_TEX_SIZE * CANVAS_ATLAS_TEX_SIZE);
   roadmap_check_allocated (temp_buf);
   for( j = 0; j < CANVAS_ATLAS_TEX_SIZE; j++) {
      for( i = 0; i < CANVAS_ATLAS_TEX_SIZE; i++){
         temp_buf[4*(i+j*CANVAS_ATLAS_TEX_SIZE)] = 255;
         temp_buf[4*(i+j*CANVAS_ATLAS_TEX_SIZE) +1] = 255;
         temp_buf[4*(i+j*CANVAS_ATLAS_TEX_SIZE) +2] = 255;
         temp_buf[4*(i+j*CANVAS_ATLAS_TEX_SIZE) +3] = 0;
      }
   }

   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, CANVAS_ATLAS_TEX_SIZE, CANVAS_ATLAS_TEX_SIZE, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, temp_buf );
   check_gl_error();
   free(temp_buf);
   
   root.root_node = create_new_node();
   root.root_node->rect.minx = root.root_node->rect.miny = 0;
   root.root_node->rect.maxx = root.root_node->rect.maxy = CANVAS_ATLAS_TEX_SIZE - 1;
   root.texture = texture;
   strncpy_safe (root.hint, hint, sizeof(root.hint));
   
   return root;
}

////////////////////////////////////////////////////////////
static int get_root_index (const char *hint, int start_index) {
   int i;
   static BOOL initialized = FALSE;
   
   if (!initialized) {
      for (i = 1; i < CANVAS_ATLAS_MAX_TREES; i++) {
         roots[i].root_node = NULL;
         roots[i].hint[0] = 0;
      }

      initialized = TRUE;
   }
   
   for (i = start_index; i < CANVAS_ATLAS_MAX_TREES; i++) {
      if (roots[i].root_node == NULL) {
         roots[i] = create_new_root(hint);
         return i;
      }
      
      if (!strcmp(roots[i].hint, hint))
         return i;
   }
   
   return -1;
}

////////////////////////////////////////////////////////////
static AtlasNode *insert(RoadMapImage *image, AtlasNode *node) {
   
   int n_width = node->rect.maxx - node->rect.minx + 1;
   int n_height = node->rect.maxy - node->rect.miny + 1;
   int width = (*image)->width;
   int height = (*image)->height;
   int dw, dh;
   AtlasNode *new_node;
   
   
   if (node->child[0] != NULL && 
       node->child[1] != NULL) {
      new_node = insert(image, node->child[0]);
      if (new_node != NULL)
         return new_node;
      
      return insert(image, node->child[1]);
   }
   

   if (node->image != NULL)
      return NULL;
   
   if (height > n_height -1 ||
       width > n_width -1)
      return NULL;
   
   if (height == n_height -1 &&
       width == n_width -1)
      return node;
   
   node->child[0] = create_new_node();
   node->child[1] = create_new_node();
   
   dw = n_width - width;
   dh = n_height - height;
  
   if (dw > dh) {
      node->child[0]->rect.minx = node->rect.minx;
      node->child[0]->rect.miny = node->rect.miny;
      node->child[0]->rect.maxx = node->rect.minx + width;
      node->child[0]->rect.maxy = node->rect.maxy;
      node->child[1]->rect.minx = node->rect.minx + width +1;
      node->child[1]->rect.miny = node->rect.miny;
      node->child[1]->rect.maxx = node->rect.maxx;
      node->child[1]->rect.maxy = node->rect.maxy;
   } else {
      node->child[0]->rect.minx = node->rect.minx;
      node->child[0]->rect.miny = node->rect.miny;
      node->child[0]->rect.maxx = node->rect.maxx;
      node->child[0]->rect.maxy = node->rect.miny + height;
      node->child[1]->rect.minx = node->rect.minx;
      node->child[1]->rect.miny = node->rect.miny + height +1;
      node->child[1]->rect.maxx = node->rect.maxx;
      node->child[1]->rect.maxy = node->rect.maxy;
   }
  
   return insert(image, node->child[0]);
}

BOOL roadmap_canvas_atlas_insert (const char* hint, RoadMapImage *image) {
   int root_index = 0;
   AtlasRoot *root = NULL;
   AtlasNode *node = NULL;
   RoadMapImage img = (*image);
   
   while (node == NULL && root_index != -1) {
      root_index = get_root_index(hint, root_index);
      if (root_index != -1) {
         root = &roots[root_index];         
         node = insert(image, root->root_node);
         root_index++;
      }
   }
   
   if (node == NULL) {
      roadmap_log (ROADMAP_ERROR, "roadmap_canvas_atlas_insert() - no space for image !!!");
      return FALSE;
   }
   
   node->image = img;

   roadmap_canvas_bind(root->texture);
   glTexSubImage2D(GL_TEXTURE_2D, 0, node->rect.minx, node->rect.miny, img->width, img->height,
                   GL_RGBA, GL_UNSIGNED_BYTE, img->buf);
   check_gl_error();
   
   img->offset.x = node->rect.minx;
   img->offset.y = node->rect.miny;
   img->texture = root->texture;
   
   
   return TRUE;
}

void roadmap_canvas_atlas_clean (const char* hint) {
   int i, count;
   GLuint texture;
   
   roadmap_log (ROADMAP_DEBUG, "roadmap_canvas_atlas - cleaning hint '%s'", hint);
   
   for (i = 0; i < CANVAS_ATLAS_MAX_TREES; i++) {
      if (roots[i].root_node == NULL) {
         continue;
      }
      
      if (!strcmp(roots[i].hint, hint)) {
         free (roots[i].root_node);
         roots[i].root_node = NULL;
         roots[i].hint[0] = 0;
         texture = roots[i].texture;
         glDeleteTextures( 1, &texture );
         roots[i].texture = 0;
      }
   }
   
   //optimize; must be done for correct roots scan when adding image
   count = 0;
   for (i = 0; i < CANVAS_ATLAS_MAX_TREES; i++) {
      if (roots[i].root_node != NULL &&
          i > count) {
         
         roots[count] = roots[i];
         
         roots[i].root_node = NULL;
         roots[i].hint[0] = 0;
         roots[i].texture = 0;
         
         count++;
      }
   }
}
