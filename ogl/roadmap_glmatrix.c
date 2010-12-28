/* roadmap_glmatrix.c - Wrapper module for opengl calls with projection/modelview impact
 * In order to use the the enable function should be called. All !!! the subsequent calls
 * having impact on the matrices should be done through this interface
 * NOTE:: Additional functionality should be added in the future ( for scale etc ... )
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
 *   See OpenGL spec
 */

#include "roadmap.h"
#include "roadmap_glmatrix.h"
#include <string.h>


//////////////////////////////// Local defines ////////////////////////////////////
// DEBUG LOG WRAPPER
#define OGL_MTX_DEBUG_ENABLED       0

#define OGL_MTX_LOG( ... )   {                  \
    if ( OGL_MTX_DEBUG_ENABLED )                \
        roadmap_log( __VA_ARGS__ );             \
}
//////////////////////////////// Module attributes ////////////////////////////////////
static BOOL sgGlMatrixTrackingEnabled = FALSE;       // If 0 the computations will not be done

static RMGlMatrixMode sgGlMatrixMode = _gl_matrix_mode_none;

static RMGlMatrixType sgModelMat[16];
static RMGlMatrixType sgProjMat[16];

#ifdef GTK2_OGL
   static void (*sgGlMultMatrixFunc) ( const RMGlMatrixType* ) = glMultMatrixd;
   static void (*sgGlTranslateFunc) ( RMGlMatrixType, RMGlMatrixType, RMGlMatrixType ) = glTranslated;
#else
   static void (*sgGlMultMatrixFunc) ( const RMGlMatrixType* ) = glMultMatrixf;
   static void (*sgGlTranslateFunc) ( RMGlMatrixType, RMGlMatrixType, RMGlMatrixType ) = glTranslatef;
#endif

//////////////////////////////// Local interface ////////////////////////////////////
static RMGlMatrixType* get_current( void );
static RMGlMatrixType* get_matrix( RMGlMatrixMode mode );
static inline void mult_matrix( const RMGlMatrixType* mtx );

/***********************************************************
 *  Name       : void roadmap_glmatrix_enable( void  )
 *  Purpose    : Enables the computation of the GL matrices
 *  Params     : void
 *  Returns    : void
 */
void roadmap_glmatrix_enable( void )
{
   roadmap_log( ROADMAP_WARNING, "The matrix tracking is enabled!" );
   sgGlMatrixTrackingEnabled = TRUE;
}

/***********************************************************
 *  Name       : void roadmap_glmatrix_mode( RMGlMatrixMode mode )
 *  Purpose    : Sets the opengl matrix mode
 *  Params     : mode - type of the matrix mode (see RMGlMatrixMode)
 *  Returns    : void
 *             :
 */
void roadmap_glmatrix_mode( RMGlMatrixMode mode )
{
   OGL_MTX_LOG( ROADMAP_INFO, "Changing current mode from %d to %d", sgGlMatrixMode, mode );

   switch( mode )
   {
      case _gl_matrix_mode_projection:
      {
         glMatrixMode( GL_PROJECTION );
         sgGlMatrixMode = _gl_matrix_mode_projection;
         break;
      }
      case _gl_matrix_mode_modelview:
      {
         glMatrixMode( GL_MODELVIEW );
         sgGlMatrixMode = _gl_matrix_mode_modelview;
         break;
      }
      default:
      {
         roadmap_log( ROADMAP_ERROR, "Undefined matrix mode: %d", mode );
         sgGlMatrixMode = _gl_matrix_mode_none;
      }
   }
}

/***********************************************************
 *  Name       : void roadmap_glmatrix_mult( const RMGlMatrixType* mtx  )
 *  Purpose    : Matrix multiplication
 *  Params     : mtx - transformation matrix to be applied on
 *  Returns    : void
 *             :
 */
void roadmap_glmatrix_mult( const RMGlMatrixType* mtx  )
{
   OGL_MTX_LOG( ROADMAP_INFO, "Mult matrix is called" );

   // OpenGL call
   sgGlMultMatrixFunc( mtx );

   // Tracking
   if ( sgGlMatrixTrackingEnabled )
   {
      mult_matrix( mtx );
   }
}

/***********************************************************
 *  Name       : roadmap_glmatrix_translate( RMGlMatrixType dx, RMGlMatrixType dy, RMGlMatrixType dz )
 *  Purpose    : Applies translation transformation
 *  Params     : dx, dy, dz - translation deltas for each coordinate
 *  Returns    : void
 *             :
 */
void roadmap_glmatrix_translate( RMGlMatrixType dx, RMGlMatrixType dy, RMGlMatrixType dz )
{
   OGL_MTX_LOG( ROADMAP_INFO, "Translation is called" );

   // OpenGL call
   sgGlTranslateFunc( dx, dy, dz );

   // Tracking
   if ( sgGlMatrixTrackingEnabled )
   {
      RMGlMatrixType mtx[16] = {0};
      // Diagonal
      mtx[0] = (RMGlMatrixType) 1;
      mtx[5] = (RMGlMatrixType) 1;
      mtx[10] = (RMGlMatrixType) 1;
      mtx[15] = (RMGlMatrixType) 1;
      // Translation increments ( column wise )
      mtx[12] = dx;
      mtx[13] = dy;
      mtx[14] = dz;
      // Apply the matrix
      mult_matrix( mtx );
   }
}

/***********************************************************
 *  Name       : void roadmap_glmatrix_identity( void )
 *  Purpose    : Sets current matrix to the identity
 *  Params     : void
 *  Returns    : void
 *             :
 */
void roadmap_glmatrix_identity( void )
{
   // OpenGL call
   glLoadIdentity();

   // Tracking
   if ( sgGlMatrixTrackingEnabled )
   {
      RMGlMatrixType* cur_mtx = get_current();
      memset( cur_mtx, 0, 16 * sizeof( RMGlMatrixType ) );
      cur_mtx[0] = (RMGlMatrixType) 1;
      cur_mtx[5] = (RMGlMatrixType) 1;
      cur_mtx[10] = (RMGlMatrixType) 1;
      cur_mtx[15] = (RMGlMatrixType) 1;
   }
}


/***********************************************************
 *  Name       : const RMGlMatrixType* roadmap_glmatrix_get_matrix
 *  Purpose    : Returns the pointer to the matrix of the requested mode
 *  Params     : mode - type of the matrix mode (see RMGlMatrixMode)
 *  Returns    : requested matrix
 *             :
 */
const RMGlMatrixType* roadmap_glmatrix_get_matrix( RMGlMatrixMode mode  )
{
   const RMGlMatrixType* retMtx = get_matrix( mode );
   return retMtx;
}

/***********************************************************
 *  Name       : const RMGlMatrixType* roadmap_glmatrix_get_current
 *  Purpose    : Returns the pointer to the matrix of the current mode
 *  Params     : none
 *  Returns    : requested matrix
 */
const RMGlMatrixType* roadmap_glmatrix_get_current( void )
{
   const RMGlMatrixType* retMtx = get_current();
   return retMtx;
}

/***********************************************************
 *  Name       : roadmap_glmatrix_load_matrix( RMGlMatrixMode mode, RMGlMatrixType dst_mtx[16] )
 *  Purpose    : Loads the matrix of requested mode to the given place
 *  Params     : mode - requested mode
 *             : dst_mtx - destination matrix to load to
 *  Returns    : void
 *             :
 */
inline void roadmap_glmatrix_load_matrix( RMGlMatrixMode mode, RMGlMatrixType dst_mtx[16] )
{
   if ( sgGlMatrixTrackingEnabled )
   {
      RMGlMatrixType* cur_mtx = get_matrix( mode );
      memcpy( dst_mtx, cur_mtx, 16 * sizeof( RMGlMatrixType ) );
   }
   else
   {
      GLenum gl_mode = 0;
      switch( mode )
      {
         case _gl_matrix_mode_modelview:
         {
            gl_mode = GL_MODELVIEW_MATRIX;
            break;
         }
         case _gl_matrix_mode_projection:
         {
            gl_mode = GL_PROJECTION_MATRIX;
            break;
         }
         default:
         {
            roadmap_log( ROADMAP_ERROR, "Undefined matrix mode: %d", mode );
         }
      }
      if ( gl_mode > 0 )
      {
#ifdef GTK2_OGL
         glGetDoublev( gl_mode, dst_mtx );
#else
         glGetFloatv( gl_mode, dst_mtx );
#endif// GTK2_OGL
      }
   }
}

/***********************************************************
 *  Name       : roadmap_glmatrix_print_matrix( RMGlMatrixMode mode, const char* description  )
 *  Purpose    : Dumps the matrix to the log
 *  Params     : mode - the requested mode
 *             : description - additional description in the starting string
 *  Return     : void
 *             :
 */
void roadmap_glmatrix_print_matrix( RMGlMatrixMode mode, const char* description  )
{
   int i, j;
   RMGlMatrixType mtx[16];

   roadmap_log_raw_data_fmt( " The %s matrix values for mode: %d\n", SAFE_STR( description ), mode );
   roadmap_glmatrix_load_matrix( mode, mtx );

   for( i = 0; i < 4; ++i )
   {
      for( j = 0; j < 4; ++j )
      {
         // Column wise
         roadmap_log_raw_data_fmt( "   %f", *( mtx + ( i + 4*j ) ) );
      }
      roadmap_log_raw_data_fmt( "\n" );
   }
   roadmap_log_raw_data_fmt( "\n" );
}




//////////////////////////////// Local interface ////////////////////////////////////



/***********************************************************
 *  Name       : const RMGlMatrixType* get_matrix
 *  Purpose    : Returns the pointer to the matrix of the requested mode
 *  Params     : mode - type of the matrix mode (see RMGlMatrixMode)
 *  Return     : Matrix pointer
 *             : Auxiliary
 */
static RMGlMatrixType* get_matrix( RMGlMatrixMode mode )
{
   RMGlMatrixType* retMtx = NULL;
   switch( mode )
   {
      case _gl_matrix_mode_modelview:
      {
         retMtx = sgModelMat;
         break;
      }
      case _gl_matrix_mode_projection:
      {
         retMtx = sgProjMat;
         break;
      }
      default:
      {
         roadmap_log( ROADMAP_ERROR, "Undefined matrix mode: %d", mode );
      }
   }
   return retMtx;
}
/***********************************************************
 *  Name       : RMGlMatrixType* get_current( void )
 *  Purpose    : Returns the pointer to the matrix of the current mode
 *  Params     : void
 *  Return     : Matrix pointer
 *             : Auxiliary
 */
static RMGlMatrixType* get_current( void )
{
   return get_matrix( sgGlMatrixMode );
}

/***********************************************************
 *  Name       : mult_matrix( const RMGlMatrixType* mtx  )
 *  Purpose    : Multiplies the internal matrix only
 *  Params     : mtx - Right hand matrix to multiply on
 *  Return     : void
 *             : Auxiliary
 */
static inline void mult_matrix( const RMGlMatrixType* mtx  )
{
   RMGlMatrixType res_mtx[16] = {0};
   RMGlMatrixType* cur_mtx = get_current();
   int k, i, row, col;
   for( k = 0; k<16; ++k )
   {
      row = ( k >> 2 ); // Row in the current matrix
      col = ( k % 4 );  // Column in the current matrix
      for ( i = 0; i < 4; ++i )
      {
         // Column wise result - multiply each row of the current by the
         // next column of the right hand matrix. All the sources and result are columnwise
         res_mtx[k] += ( cur_mtx[ col + 4*i] * mtx[i + 4*row] );
      }
   }
   // Set the result
   memcpy( cur_mtx, res_mtx, sizeof( RMGlMatrixType ) * 16 );
}

