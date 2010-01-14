/*
 *  GSPlayer - The audio player for WindowsCE
 *  Copyright (C) 2003  Y.Nagamidori
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  based on MAD - MPEG audio decoder library
 *  Copyright (C) 2000-2003 Underbit Technologies, Inc.
 */

#ifndef __LIBMAD_H__
#define __LIBMAD_H__

typedef struct
{
	int enable;
	int preamp; //set value 63(min) to 0(max) (default 31) 
	int data[10]; //set value 63(min) to 0(max) (default 31) 
}equalizer_value;

#define MAD_FATAL_ERR -2
#define MAD_ERR -1
#define MAD_OK  0
#define MAD_NEED_MORE_INPUT 1
#define MAD_NEED_MORE_OUTPUT 2

#ifdef LIBMAD_EXPORTS
#define LIBMAD_EXPORT __declspec(dllexport)
#else
#define LIBMAD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBMAD_EXPORT HANDLE mad_init();
LIBMAD_EXPORT int mad_decode(HANDLE hMad, char *inmemory,int inmemsize, char *outmemory,
							 int outmemsize, int* read, int *done, int resolution, int halfsamplerate);
LIBMAD_EXPORT void mad_uninit(HANDLE hMad);
LIBMAD_EXPORT void mad_seteq(HANDLE hMad, equalizer_value* eq);

#ifdef __cplusplus
}
#endif

#endif // __LIBMAD_H__