/*
 * LICENSE:
 *
 *   Copyright 2009 PazO
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
 */


#ifndef   __DYNAMICBUFFER_H__
#define   __DYNAMICBUFFER_H__

/* Efficient buffer

   Abstract:   Will allocate memory statically or dynamically, depends on the 
               requested size.
               Up to size 'EBUFFER_STATIC_SIZE', ebuffer will use the static 
               memory. Above this size data will be allocated from the heap.

   Note:       It is assumed that data can be a string, thus allocated buffer 
               will always be NULL terminated.                                                           
*/


#define  EBUFFER_ECONOMIC_BUFFER_FACTOR         (5.5F)
#define  EBUFFER_BIG_SIZE                       (0x3000) // 12K
#define  EBUFFER_STATIC_SIZE                    \
               ((int)((double)EBUFFER_BIG_SIZE/EBUFFER_ECONOMIC_BUFFER_FACTOR))

typedef struct tagebuffer
{
   char  static_buffer[EBUFFER_STATIC_SIZE+1];
   char* dynamic_buffer;
   int   size;

}  ebuffer, *ebuffer_ptr;

void  ebuffer_init            ( ebuffer_ptr   this);
char* ebuffer_alloc           ( ebuffer_ptr   this, int size);
void  ebuffer_free            ( ebuffer_ptr   this);

char* ebuffer_get_buffer      ( ebuffer_ptr   this);
int   ebuffer_get_buffer_size ( ebuffer_ptr   this);
int   ebuffer_get_string_size ( ebuffer_ptr   this);

void  ebuffer_getStatistics(  int*  StaticAllocationsCount,
                              int*  DynamicAllocationsCount);

#endif   //   __DYNAMICBUFFER_H__
