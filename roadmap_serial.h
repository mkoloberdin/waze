/* roadmap_serial.h - a module to open/read/close a serial IO device.
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
 *   This module hides the OS specific API to access a serial device.
 */

#ifndef INCLUDE__ROADMAP_SERIAL__H
#define INCLUDE__ROADMAP_SERIAL__H

#define MAX_SERIAL_ENUMS 10

#if defined (_WIN32) && !defined (__SYMBIAN32__)

#include <windows.h>
#include "win32/win32_serial.h"

typedef struct Win32SerialConn *RoadMapSerial; /* WIN32 style. */
#define ROADMAP_SERIAL_IS_VALID(f) ((f != NULL) && (f->valid))

#else

typedef int RoadMapSerial; /* UNIX style. */
#define ROADMAP_SERIAL_IS_VALID(f) (f != (RoadMapSerial)-1)

#endif


RoadMapSerial roadmap_serial_open  (const char *name,
                                    const char *mode,
                                    int baud_rate);

int   roadmap_serial_read  (RoadMapSerial serial, void *data, int size);
int   roadmap_serial_write (RoadMapSerial serial, const void *data, int length);
void  roadmap_serial_close (RoadMapSerial serial);
const int *roadmap_serial_enumerate (void);
const char **roadmap_serial_get_speeds (void);

#endif // INCLUDE__ROADMAP_FILE__H

