/* roadmap_input.c - Decode an ASCII data stream.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_input.h
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if ! defined (_WIN32) && ! defined (J2ME)
#include <errno.h>
#endif

#include "roadmap.h"
#include "roadmap_input.h"


static void roadmap_input_shift_to_next_line (RoadMapInputContext *context,
                                             char *from) {

   char *data_end = context->data + context->cursor;

   /* Skip the trailing end of line characters, if any. */
   while ((*from < ' ') && (from < data_end)) ++from;

   if (from > context->data) {
         
      context->cursor -= (int)(from - context->data);

      if (context->cursor <= 0) {
         context->cursor = 0; /* Play it safe. */
         return;
      }

      memmove (context->data, from, context->cursor);
      context->data[context->cursor] = 0;
   }
}


int roadmap_input_split (char *text, char separator, char *field[], int max) {

   int   i;
   char *p;

   field[0] = text;
   p = strchr (text, separator);

   for (i = 1; p != NULL && *p != 0; ++i) {

      *p = 0;
      if (i >= max) return -1;

      field[i] = ++p;
      p = strchr (p, separator);
   }
   return i;
}


int roadmap_input (RoadMapInputContext *context) {

   int result;
   int received;
   int is_binary = context->is_binary;

   char *line_start;
   char *data_end;


   /* Receive more data if available. */

   if (context->cursor < (int)sizeof(context->data) - 1) {

      received =
         roadmap_io_read (context->io,
                          context->data + context->cursor,
                          sizeof(context->data) - context->cursor - 1);

      if (received < 0) {

         /* We lost that connection. */
         roadmap_log (ROADMAP_INFO, "lost %s", context->title);
         return -1;

      } else {
         context->cursor += received;
      }
   }

   if (!is_binary) context->data[context->cursor] = 0;


   /* Remove the leading end of line characters, if any.
    * We do the same in between lines (see end of loop).
    */
   line_start = context->data;
   data_end   = context->data + context->cursor;

   if (!is_binary) {
      while ((line_start < data_end) && (*line_start < ' ')) ++line_start;
   }


   /* process each complete line in this buffer. */

   result = 0;

   while (line_start < data_end) {

      int cur_res;
      char *line_end = data_end;

      if (!is_binary) {
         char *new_line;
         char *carriage_return;


         /* Find the first end of line character coming after this line. */

         new_line = strchr (line_start, '\n');
         carriage_return = strchr (line_start, '\r');

         if (new_line == NULL) {
            line_end = carriage_return;
         } else if (carriage_return == NULL) {
            line_end = new_line;
         } else if (new_line < carriage_return) {
            line_end = new_line;
         } else {
            line_end = carriage_return;
         }

         if (line_end == NULL) {

            /* This line is not complete: shift the remaining data
             * to the beginning of the buffer and then stop.
             */

            if (line_start + strlen(line_start) != data_end) {
               roadmap_log (ROADMAP_WARNING, "GPS input has null characters.");
               roadmap_input_shift_to_next_line
                  (context, line_start + strlen(line_start) + 1);

               line_start = context->data;
               data_end   = context->data + context->cursor;
               while ((line_start < data_end) && (*line_start < ' ')) ++line_start;
               continue;
            } else {
               roadmap_input_shift_to_next_line (context, line_start);
            }
            return result;
         }

         /* Process this line. */

         *line_end = 0; /* Separate this line from the next. */
      }

      if (context->logger != NULL) {
         context->logger (line_start);
      }

      cur_res = context->decoder (context->user_context,
                                  context->decoder_context, line_start,
                                  line_end - line_start);

      if (is_binary) {
         line_start += cur_res;
         context->cursor -= cur_res;

         if (context->cursor) {
            memmove (context->data, line_start, context->cursor);
         }

         return 0;
      }

      result |= cur_res;

      /* Move to the next line. */

      line_start = line_end + 1;
      while ((*line_start < ' ') && (line_start < data_end)) ++line_start;
   }


   context->cursor = 0; /* The buffer is now empty. */
   return result;
}

