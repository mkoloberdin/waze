/* roadmap_io.c - a module to hide OS-specific IO operations.
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
 *
 * DESCRIPTION:
 *
 *   This module provides a unique interface to all types of IO
 *   supported by RoadMap. It is not assumed that the OS provides
 *   a unified API for all sort of IO, thus this module that makes
 *   sure the rest of RoadMap code does not care much about these
 *   details.
 *   The IO must still be open in a IO specific way.
 */

#include "roadmap_io.h"


int roadmap_io_read  (RoadMapIO *io, void *data, int size) {

   switch (io->subsystem) {

      case ROADMAP_IO_FILE:
         return roadmap_file_read (io->os.file, data, size);

      case ROADMAP_IO_NET:
         return roadmap_net_receive (io->os.socket, data, size);

      case ROADMAP_IO_SERIAL:
         return roadmap_serial_read (io->os.serial, data, size);

      case ROADMAP_IO_PIPE:
         return roadmap_spawn_read_pipe (io->os.pipe, data, size);

      case ROADMAP_IO_NULL:
         return 0; /* Cannot receive anything from there. */
   }
   return -1;
}


int roadmap_io_write (RoadMapIO *io, const void *data, int length, int wait) {

   switch (io->subsystem) {

      case ROADMAP_IO_FILE:
         return roadmap_file_write (io->os.file, data, length);

      case ROADMAP_IO_NET:
         return roadmap_net_send (io->os.socket, data, length, wait);

      case ROADMAP_IO_SERIAL:
         return roadmap_serial_write (io->os.serial, data, length);

      case ROADMAP_IO_PIPE:
         return roadmap_spawn_write_pipe (io->os.pipe, data, length);

      case ROADMAP_IO_NULL:
         return length; /* It's all done, since there is nothing to do. */
   }
   return -1;
}


void  roadmap_io_close (RoadMapIO *io) {

   switch (io->subsystem) {

      case ROADMAP_IO_FILE:
         roadmap_file_close (io->os.file);
         break;

      case ROADMAP_IO_NET:
         roadmap_net_close (io->os.socket);
         break;

      case ROADMAP_IO_SERIAL:
         roadmap_serial_close (io->os.serial);
         break;

      case ROADMAP_IO_PIPE:
         roadmap_spawn_close_pipe (io->os.pipe);
         break;

      case ROADMAP_IO_NULL:
         break;
   }
   io->subsystem = ROADMAP_IO_INVALID;
}


int roadmap_io_same (RoadMapIO *io1, RoadMapIO *io2) {

   /* We do not do a memcmp() here because some compilers might
    * generate holes that are not initialized in RoadMapIO.
    */
   if (io1->subsystem != io2->subsystem) return 0;

   switch (io1->subsystem) {

      case ROADMAP_IO_FILE:
         if (io1->os.file != io2->os.file) return 0;
         break;

      case ROADMAP_IO_NET:
         if (io1->os.socket != io2->os.socket) return 0;
         break;

      case ROADMAP_IO_SERIAL:
         if (io1->os.serial != io2->os.serial) return 0;
         break;

      case ROADMAP_IO_PIPE:
         if (io1->os.pipe != io2->os.pipe) return 0;
         break;

      case ROADMAP_IO_NULL:
         break; /* No reason to be any different from each other. */
   }

   return 1; /* We did not find any difference. */
}

