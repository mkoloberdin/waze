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

#ifndef __ROADMAP_LIBPNG_H_
#define __ROADMAP_LIBPNG_H_

#ifdef __cplusplus
extern "C"
#endif
unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride);

#endif

