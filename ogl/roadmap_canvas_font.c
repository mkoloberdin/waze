/* roadmap_canvas_font.c - manage the canvas font.
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See roadmap_canvas_font.h.
 */

#define ROADMAP_CANVAS_FONT_C

#include <math.h>
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_hash.h"
#include "roadmap_screen.h"
#include "ft2build.h"
#if defined(OPENGL)
#if defined(GTK2_OGL)
#include <GL/gl.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftstroke.h>
#else
#include "freetype.h"
#include "ftglyph.h"
#include "ftstroke.h"
#endif// GTK2_OGL
#endif// OPENGL

#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h"
#include "roadmap_canvas_atlas.h"
#include "roadmap_canvas_font.h"


static BOOL initialized = FALSE;
static FT_Library library;
static FT_Face face;
static FT_Face face_nor;
static RoadMapHash *RoadMapFontHash;
static RoadMapFontImage **RoadMapFontItems;
static int RoadMapFontSize = 0;
static int RoadMapFontCount = 0;

#define INITIAL_ITEMS_SIZE 128
#define FONT_HINT          "font"


static void roadmap_canvas_font_allocate (void) {

   if (RoadMapFontSize == 0) {
      RoadMapFontSize = INITIAL_ITEMS_SIZE;
      RoadMapFontItems = calloc(RoadMapFontSize, sizeof(RoadMapFontImage*));
      RoadMapFontHash = roadmap_hash_new ("font_hash", RoadMapFontSize);
   } else {
      RoadMapFontSize += INITIAL_ITEMS_SIZE;
      RoadMapFontItems = realloc(RoadMapFontItems,
                                 RoadMapFontSize * sizeof(RoadMapFontImage*));
      roadmap_hash_resize (RoadMapFontHash, RoadMapFontSize);
   }

   if (RoadMapFontItems == NULL) {
      roadmap_log (ROADMAP_FATAL, "No memory.");
   }
}


#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

//INLINE_DEC void create_name (char *name, int name_len ,wchar_t ch, int size, int bold) {
//   snprintf(name, name_len , "%ld_%d_%d", ch, size, bold);
//}


#define MAKE_HASH(_ch_,_size_,_bold_)(_ch_*100 + _size_ + _bold_)

static RoadMapFontImage *roadmap_canvas_font_get_item (wchar_t ch, int size, int bold) {
   int hash;
   int i;
   //char name[20];


   //create_name (name, sizeof(name), ch, size, bold);
   //hash = roadmap_hash_string (name);
   
   hash = MAKE_HASH(ch, size, bold);

   for (i = roadmap_hash_get_first (RoadMapFontHash, hash);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapFontHash, i)) {

      if (hash == RoadMapFontItems[i]->hash) {

         return RoadMapFontItems[i];
      }
   }

   return NULL;

}

static void init() {
	char *full_name;
   char *path;

#ifdef IPHONE_NATIVE
   path = (char *)roadmap_path_bundle();
#else
   path = roadmap_path_user();
#endif

   if (FT_Init_FreeType( &library ))
      roadmap_log(ROADMAP_ERROR, "FT_Init_FreeType() failed");

   full_name = roadmap_path_join(path, "font.ttf");
   if (FT_New_Face( library, full_name, 0, &face ))
      roadmap_log(ROADMAP_ERROR, "FT_New_Face() failed");

	full_name = roadmap_path_join(path, "font_normal.ttf");
	if (FT_New_Face( library, full_name, 0, &face_nor ))
		roadmap_log(ROADMAP_ERROR, "FT_New_Face() failed");
   roadmap_canvas_font_allocate();

   initialized = TRUE;
}

static void set_size (int size, int bold) {
   static int last_size_bold = 0;
   static int last_size_nor = 0;

	if (bold && last_size_bold != size){
      FT_Set_Pixel_Sizes(face,
                         size,    // pixel_width
                         size);  // pixel_height
      last_size_bold = size;

	} else if (!bold && last_size_nor != size) {
      FT_Set_Pixel_Sizes(face_nor,
                         size,    // pixel_width
                         size);  // pixel_height
      last_size_nor = size;
   }
}


void roadmap_canvas_font_metrics (int size, int *ascent, int *descent, int bold) {
   FT_FaceRec *faceRec;
   FT_Face used_face;

   if (!initialized) {
      init();
   }

	if (bold){
		used_face = (FT_Face)face;
	}else {
		used_face = (FT_Face)face_nor;
	}

   faceRec = (FT_FaceRec*)used_face;
   *ascent = (int)(faceRec->ascender * size / faceRec->height);
   *descent = abs((int)(faceRec->descender * size / faceRec->height));
}

RoadMapFontImage *roadmap_canvas_font_tex (wchar_t ch, int size, int bold) {

   FT_Face used_face;
   RoadMapFontImage *fontImage = NULL;
   RoadMapImage image;
   int width;
   int height;
   int i, j;
   FT_BitmapGlyph bitmap_glyph;
   FT_Glyph glyph;
   FT_Bitmap bitmap;
   FT_Stroker stroker;
   int error;

   if (!initialized) {
      init();
   }
   
   size = ACTUAL_FONT_SIZE(size);

   if (bold){
		used_face = (FT_Face)face;
	}else {
		used_face = (FT_Face)face_nor;
	}

   fontImage = roadmap_canvas_font_get_item(ch, size, bold);

   if (fontImage != NULL)
      return fontImage;

   set_size(size, bold);

   //roadmap_log (ROADMAP_INFO, "Creating character: %ld_%d_%d", ch, size, bold);

   fontImage = malloc(sizeof(RoadMapFontImage));
   roadmap_check_allocated (fontImage);

   if (FT_Load_Glyph( used_face, FT_Get_Char_Index( used_face, ch ), FT_LOAD_DEFAULT ))
		roadmap_log(ROADMAP_ERROR, "FT_Load_Glyph() failed");

	if(FT_Get_Glyph( used_face->glyph, &glyph ))
		roadmap_log(ROADMAP_ERROR, "FT_Get_Glyph() failed");

   if( ( error = FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 ) ) )
		roadmap_log(ROADMAP_ERROR, "FT_Glyph_To_Bitmap() failed. Error: %d", error );


	bitmap_glyph = (FT_BitmapGlyph)glyph;


   fontImage->left = bitmap_glyph->left;
   fontImage->top = bitmap_glyph->top;
   fontImage->advance_x = used_face->glyph->advance.x >> 6;

	bitmap=bitmap_glyph->bitmap;

   width = bitmap.width;
   height = bitmap.rows;

   if (width && height) {
      GLubyte* temp_buf = malloc(4 * width * height);
      roadmap_check_allocated (temp_buf);
      for( j = 0; j < height; j++) {
         for( i = 0; i < width; i++){
            temp_buf[4*(i+j*width)]= temp_buf[4*(i+j*width) +1] =
            temp_buf[4*(i+j*width) +2]= 255;
            temp_buf[4*(i+j*width) +3] = bitmap.buffer[i + bitmap.width*j];
         }
      }

      image = malloc(sizeof(roadmap_canvas_image));
      roadmap_check_allocated (image);
      image->width = bitmap.width;
      image->height = bitmap.rows;
      image->buf = temp_buf;
      image->frame_buf = 0;
      image->full_path = "";
      
      if (!roadmap_canvas_atlas_insert (FONT_HINT, &image, GL_LINEAR, GL_LINEAR)) {
         roadmap_log (ROADMAP_ERROR, "Could not cache font in texture atlas !");
         fontImage->image = NULL;
         free (image);
      } else {
         fontImage->image = image;
      }
      
      free (image->buf);
      image->buf = NULL;
   } else { //Not an error if the character is <space>
      fontImage->image = NULL;
   }

   FT_Done_Glyph(glyph);
   
   
   
   //Font outline
   if (FT_Load_Glyph( used_face, FT_Get_Char_Index( used_face, ch ), FT_LOAD_DEFAULT ))
		roadmap_log(ROADMAP_ERROR, "FT_Load_Glyph() failed");
   
	if(FT_Get_Glyph( used_face->glyph, &glyph ))
		roadmap_log(ROADMAP_ERROR, "FT_Get_Glyph() failed");
   
   FT_Stroker_New(library, &stroker);
   FT_Stroker_Set(stroker,
                  (int)(ADJ_SCALE(2) * 64),
                  FT_STROKER_LINECAP_ROUND,
                  FT_STROKER_LINEJOIN_ROUND,
                  0);
   FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1);
   FT_Stroker_Done(stroker);
   
   if( ( error = FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 ) ) )
		roadmap_log(ROADMAP_ERROR, "FT_Glyph_To_Bitmap() failed. Error: %d", error );
   
   
	FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
   
	bitmap_glyph = (FT_BitmapGlyph)glyph;
	bitmap = bitmap_glyph->bitmap;
   
   width = bitmap.width;
   height = bitmap.rows;
   
   if (width && height) {
      GLubyte* temp_buf = malloc(4 * width * height);
      roadmap_check_allocated (temp_buf);
      for( j = 0; j < height; j++) {
         for( i = 0; i < width; i++){
            temp_buf[4*(i+j*width)]= temp_buf[4*(i+j*width) +1] =
            temp_buf[4*(i+j*width) +2]= 255;
            temp_buf[4*(i+j*width) +3] = bitmap.buffer[i + bitmap.width*j];
         }
      }
      
      image = malloc(sizeof(roadmap_canvas_image));
      roadmap_check_allocated (image);
      image->width = bitmap.width;
      image->height = bitmap.rows;
      image->buf = temp_buf;
      image->frame_buf = 0;
      image->full_path = "";
      
      if (!roadmap_canvas_atlas_insert (FONT_HINT, &image, GL_LINEAR, GL_LINEAR)) {
         roadmap_log (ROADMAP_ERROR, "Could not cache font in texture atlas !");
         fontImage->outline_image = NULL;
         free (image);
      } else {
         fontImage->outline_image = image;
      }
      
      free (image->buf);
      image->buf = NULL;
   } else { //Not an error if the character is <space>
      fontImage->outline_image = NULL;
   }
   
   FT_Done_Glyph(glyph);
   

   //Cache the new texture
   if (RoadMapFontCount == RoadMapFontSize) {
      roadmap_canvas_font_allocate();
   }

   //create_name (fontImage->name, sizeof(fontImage->name) ,ch, size, bold);
   //hash = roadmap_hash_string (fontImage->name);
   fontImage->hash = MAKE_HASH(ch, size, bold);
   
   roadmap_hash_add (RoadMapFontHash, fontImage->hash, RoadMapFontCount);
   RoadMapFontItems[RoadMapFontCount] = fontImage;

   RoadMapFontCount++;

   //roadmap_log(ROADMAP_DEBUG, "Font count: %d. Image: 0x%x", RoadMapFontCount, image );

   return fontImage;
}


void roadmap_canvas_font_shutdown()
{
	int i;
	RoadMapImage fontImage;

	roadmap_log( ROADMAP_WARNING, "Shutting down fonts. Font images count %d", RoadMapFontCount );

	for( i = 0; i < RoadMapFontCount; ++i )
	{
		fontImage = RoadMapFontItems[i]->image;
		if ( fontImage )
		{
			free( fontImage );
			free( RoadMapFontItems[i] );
		}
		RoadMapFontItems[i] = NULL;
	}
	RoadMapFontCount = 0;
	roadmap_hash_clean( RoadMapFontHash );
   roadmap_canvas_atlas_clean (FONT_HINT);
}
