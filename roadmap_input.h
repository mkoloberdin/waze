/* roadmap_input.h - Decode an ASCII data stream.
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
 * PURPOSE:
 *
 *   This module is used when decoding line-oriented ASCII data input.
 *   The caller's decoder is called for each line received, except for
 *   empty lines and comments (if enabled).
 *
 *   All inputs come through a roadmap_io "channel". Each received line can
 *   be logged (optional) and will be decoded using the specified decoder
 *   function. This decoder is assumed to be calling back the user modules
 *   when relevant data has been detected (the mechanism used by the
 *   decoder is ot defined here).
 *
 *   In order to make things simple for the users of this module, there
 *   are two distinct contexts to be specified:
 *
 *   - the decoder context should be used by the decoder module to implement
 *     it's callback mechanism. The content of this context is not known by
 *     roadmap_input, it is only propagated to the decoder function.
 *
 *   - the user context should be passed to the user callback by the
 *     decoder function. It represents the context of the caller module.
 */

#ifndef INCLUDED__ROADMAP_INPUT__H
#define INCLUDED__ROADMAP_INPUT__H

#include "roadmap_io.h"


typedef void (*RoadMapInputLogger)  (const char *data);
typedef int  (*RoadMapInputDecode)  (void *user_context,
                                     void *decoder_context, char *line,
				     int length);

typedef struct roadmap_input_context {

   const char *title;

   RoadMapIO  *io;

   void       *user_context;
   void       *decoder_context;
   int         is_binary;

   RoadMapInputLogger  logger;
   RoadMapInputDecode  decoder;

   char data[5120];
   int  cursor;

} RoadMapInputContext;

int roadmap_input_split (char *text, char separator, char *field[], int max);

int roadmap_input (RoadMapInputContext *context);

#endif // INCLUDED__ROADMAP_INPUT__H

