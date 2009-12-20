/* roadmap_file.h - a module to open/read/close a roadmap database file.
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

#ifndef INCLUDE__ROADMAP_IO__H
#define INCLUDE__ROADMAP_IO__H

#include <stdio.h>

#include "roadmap_file.h"
#include "roadmap_net.h"
#include "roadmap_spawn.h"
#include "roadmap_serial.h"

/* The list of supported subsystems: */
#define ROADMAP_IO_INVALID 0
#define ROADMAP_IO_FILE    1
#define ROADMAP_IO_NET     2
#define ROADMAP_IO_SERIAL  3
#define ROADMAP_IO_PIPE    4
#define ROADMAP_IO_NULL    5 /* Bottomless pitt (i.e., no IO). */

typedef struct {

   int subsystem;
   void *context;

   union {
      RoadMapFile file;
      RoadMapSocket socket;
      RoadMapSerial serial;
      RoadMapPipe   pipe;
   } os;
} RoadMapIO;


int   roadmap_io_read  (RoadMapIO *io, void *data, int size);
int   roadmap_io_write (RoadMapIO *io, const void *data, int length, int wait);
void  roadmap_io_close (RoadMapIO *io);

int roadmap_io_same (RoadMapIO *io1, RoadMapIO *io2);

#endif // INCLUDE__ROADMAP_IO__H

