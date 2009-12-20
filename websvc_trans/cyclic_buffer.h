/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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

#ifndef   __CYCLIC_BUFFER_H__
#define   __CYCLIC_BUFFER_H__

#if defined(__SYMBIAN32__)
#define  CYCLIC_BUFFER_SIZE      (2048)   // 2K
#else
#define  CYCLIC_BUFFER_SIZE      (32768)   // 32K
#endif

typedef struct tag_cyclic_buffer
{
   char  buffer[CYCLIC_BUFFER_SIZE+1];

   //   For each 'read' iteration:
   int   read_size;
   int   read_processed;

   //   For the overall transaction:
   int   data_size;
   int   data_processed;

   //   Internal usage:
   char* next_read;
   int   free_size;

}  cyclic_buffer, *cyclic_buffer_ptr;

// Remarks:
// o  read_size      - Specify bytes read
//                      Bytes which where not processed yet.
// o  read_processed - Out of 'read_size', how much was processed.
//
// o  data_size      - Over size of all expected data
//                      This value is set once, and remains const during
//                      the transaction
// o  data_processed - Out of 'data_size', how much was processed.
//                      This value is incremented-only during the transaction
//
// When (data_processed == data_size) transaction completed

void  cyclic_buffer_init   (  cyclic_buffer_ptr this);
void  cyclic_buffer_recycle(  cyclic_buffer_ptr this);
void  cyclic_buffer_update_processed_data(
                              cyclic_buffer_ptr this,
                              const char*       data,
                              const char*       data_to_skip);
const char* cyclic_buffer_get_unprocessed_data(
                              cyclic_buffer_ptr this);
#endif   //   __CYCLIC_BUFFER_H__
