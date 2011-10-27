/*
 * Guillaume Cottenceau (gc at mandrakesoft.com)
 *
 * LICENSE:
 * Copyright 2002 MandrakeSoft
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <png.h>

#include "roadmap.h"
#include "roadmap_libpng.h"
#include "JNI/FreeMapJNI.h"

#define JAVA_RESOURCES_ACCESS_ENABLED

/*
 * Image buffer container.
 */
typedef struct
{
   unsigned char* buf;
   int size;
   int read_bytes;
}PngInStream;

static unsigned char *decode_png_buf( PngInStream* png_stream, int *width, int *height,
                             int *stride);




#ifdef JAVA_RESOURCES_ACCESS_ENABLED
unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride)
{
   int size;
   char* png_buf = WazeResManager_LoadSkin( file_name, &size );
   unsigned char* buf;
   PngInStream stream;

   if ( png_buf == NULL )
   {
      // DEBUG because some resources will be downloaded later
      roadmap_log (ROADMAP_DEBUG, "[read_png_file] Buffer is empty for image: %s", file_name );
      return NULL;
   }


   stream.buf = png_buf;
   stream.read_bytes = 0;
   stream.size = size;

   buf = decode_png_buf( &stream, width, height, stride );

   free( png_buf );

   return buf;
}
#else
unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride) {

   png_byte color_type;
   png_byte bit_depth;

   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes;
   png_bytep* row_pointers;
   int y;

   unsigned char header[8];   // 8 is the maximum size that can be checked
   unsigned char *buf;
   FILE *fp;

   /* open file and test for it being a png */
   fp = fopen(file_name, "rb");
   if (!fp) return NULL;

   fread(header, 1, 8, fp);
   if (png_sig_cmp(header, 0, 8)) {
      roadmap_log (ROADMAP_ERROR,
         "[read_png_file] File %s is not recognized as a PNG file",
         file_name);
      fclose(fp);
      return NULL;
   }

   /* initialize stuff */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (!png_ptr) {
      roadmap_log (ROADMAP_ERROR,
                  "[read_png_file] png_create_read_struct failed");
      fclose(fp);
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);

   if (!info_ptr) {
      roadmap_log (ROADMAP_ERROR,
            "[read_png_file] png_create_info_struct failed");
      fclose(fp);
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during init_io");
      fclose(fp);
      return NULL;
   }

   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, 8);

   png_read_info(png_ptr, info_ptr);

   *width = info_ptr->width;
   *height = info_ptr->height;
   *stride = info_ptr->rowbytes;
   color_type = info_ptr->color_type;
   bit_depth = info_ptr->bit_depth;

   number_of_passes = png_set_interlace_handling(png_ptr);
   png_read_update_info(png_ptr, info_ptr);

   /* read file */
   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during read_image");
      fclose(fp);
      return NULL;
   }

   row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height);
   buf = malloc (*height * info_ptr->rowbytes);
   for (y=0; y<*height; y++) {
      row_pointers[y] = (png_byte*) (buf + y * info_ptr->rowbytes);
   }

   png_read_image(png_ptr, row_pointers);
   png_read_end(png_ptr, NULL);
   png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
   free (row_pointers);

   fclose(fp);

   return buf;
}
#endif

/*
 * Custom read function. Used to read buffer memory
 *
 * Author: Alex Agranovich (AGA)
 */
static void png_read_buf_fn( png_structp png_ptr, png_bytep data, png_size_t length )
{
   PngInStream *stream = NULL;

   if ( png_ptr == NULL )
   {
      roadmap_log (ROADMAP_ERROR,
                  "[png_read_buf] Png structure pointer is invalid" );
      return;
   }

   stream = (PngInStream*)png_ptr->io_ptr;
   if ( stream == NULL )
   {
      roadmap_log (ROADMAP_ERROR,
                  "[png_read_buf] Stream is not initialized");
      return;
   }
   if ( stream->read_bytes + length > stream->size )
   {
      roadmap_log (ROADMAP_ERROR,
                        "[png_read_buf] Stream has no requested bytes. Read: %d. Size: %d. Requested: %d", stream->read_bytes, length, stream->size );
      return;
   }

   memcpy( data, &stream->buf[stream->read_bytes], length );

   stream->read_bytes += length;
}

/*
 * Decodes png image stored in buffer.
 *
 * Author: Alex Agranovich (AGA)
 */
unsigned char *decode_png_buf( PngInStream* png_stream, int *width, int *height,
                             int *stride) {

   png_byte color_type;
   png_byte bit_depth;

   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes;
   png_bytep* row_pointers;
   int y;

   unsigned char *buf;

   if ( !png_stream )
   {
      roadmap_log (ROADMAP_ERROR, "[decode_png_buf] Png stream is corrupted" );
      return NULL;
   }

   if (png_sig_cmp( png_stream->buf, 0, 8 ) ) {
      roadmap_log (ROADMAP_ERROR, "[decode_png_buf] File is not recognized as a PNG file" );
      return NULL;
   }


   png_stream->read_bytes += 8;

   /* initialize stuff */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (!png_ptr) {
      roadmap_log (ROADMAP_ERROR, "[decode_png_buf] png_create_read_struct failed");
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);

   if (!info_ptr) {
      roadmap_log (ROADMAP_ERROR, "[decode_png_buf] png_create_info_struct failed");
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[decode_png_buf] Error during init_io");
      return NULL;
   }

   png_set_read_fn( png_ptr, (void *)png_stream, png_read_buf_fn );

   png_set_sig_bytes( png_ptr, 8 );

   png_read_info( png_ptr, info_ptr);

   *width = info_ptr->width;
   *height = info_ptr->height;
   *stride = info_ptr->rowbytes;

   color_type = info_ptr->color_type;
   bit_depth = info_ptr->bit_depth;

   number_of_passes = png_set_interlace_handling(png_ptr);
   png_read_update_info(png_ptr, info_ptr);

   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during read_image");
      return NULL;
   }

   row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height);
   buf = malloc (*height * info_ptr->rowbytes);
   for (y=0; y<*height; y++) {
      row_pointers[y] = (png_byte*) (buf + y * info_ptr->rowbytes);
   }

   png_read_image(png_ptr, row_pointers);
   png_read_end(png_ptr, NULL);
   png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
   free (row_pointers);

   return buf;
}

