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

unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride) {

   png_byte color_type;
   png_byte bit_depth;

   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes;
   png_bytep* row_pointers;
   int y;

	char header[8];	// 8 is the maximum size that can be checked
	unsigned char *buf;
   HANDLE fp;
   LPWSTR url_unicode;
   DWORD num_bytes;

   url_unicode = ConvertToWideChar(file_name, CP_UTF8);

  	fp = CreateFile (url_unicode,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	free(url_unicode);

	/* open file and test for it being a png */
	if (fp == INVALID_HANDLE_VALUE) return NULL;

   ReadFile(fp, header, 8, &num_bytes, NULL);

	if (png_sig_cmp(header, 0, 8)) {
      roadmap_log (ROADMAP_ERROR,
         "[read_png_file] File %s is not recognized as a PNG file",
         file_name);
      CloseHandle(fp);
      return NULL;
   }

	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr) {
      roadmap_log (ROADMAP_ERROR,
                  "[read_png_file] png_create_read_struct failed");
      CloseHandle(fp);
      return NULL;
   }

	info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr) {
      roadmap_log (ROADMAP_ERROR,
		      "[read_png_file] png_create_info_struct failed");
      CloseHandle(fp);
      return NULL;
   }

	if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during init_io");
      CloseHandle(fp);
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
      CloseHandle(fp);
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

   CloseHandle(fp);

	return buf;
}


