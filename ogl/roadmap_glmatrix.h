/* roadmap_glmatrix.h - opengl matrices wrapper interface
 *
 *
 * LICENSE:
 *   Copyright 2010, Waze Ltd
 *   Alex Agranovich
 *
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
 * SYNOPSYS:
 *
 *   See roadmap_glmatrix.c, OpenGL spec
 */



#ifndef INCLUDE__ROADMAP_GLMATRIX__H
#define INCLUDE__ROADMAP_GLMATRIX__H

#include "roadmap_canvas_ogl.h"

typedef enum {
   _gl_matrix_mode_none,
   _gl_matrix_mode_modelview,
   _gl_matrix_mode_projection
} RMGlMatrixMode;



#ifdef GTK2_OGL
typedef  GLdouble RMGlMatrixType;
#else
typedef  GLfloat RMGlMatrixType;
#endif

void roadmap_glmatrix_mode( RMGlMatrixMode mode );
void roadmap_glmatrix_enable( void );
const RMGlMatrixType* roadmap_glmatrix_get_current( void );
const RMGlMatrixType* roadmap_glmatrix_get_matrix( RMGlMatrixMode mode  );
void roadmap_glmatrix_mult( const RMGlMatrixType* mtx  );
void roadmap_glmatrix_translate( RMGlMatrixType dx, RMGlMatrixType dy, RMGlMatrixType dz );
void roadmap_glmatrix_identity( void );
void roadmap_glmatrix_load_matrix( RMGlMatrixMode mode, RMGlMatrixType dst_mtx[16] );
void roadmap_glmatrix_print_matrix( RMGlMatrixMode mode, const char* description  );

#endif /* INCLUDE__ROADMAP_GLMATRIX__H */
