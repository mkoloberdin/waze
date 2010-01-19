/* roadmap_serial.c - a module to open/read/close a serial IO device.
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 *   NOT IMPLEMENTED YET.
 */

#include "roadmap.h"
#include "roadmap_serial.h"

RoadMapSerial roadmap_serial_open  (const char *name,
                                    const char *mode,
                                    int speed) {

   roadmap_log (ROADMAP_ERROR, "tty support not yet implemented");
   return -1;
}

int roadmap_serial_read  (RoadMapSerial serial, void *data, int size) {
   return -1;
}

int roadmap_serial_write (RoadMapSerial serial, const void *data, int length) {
   return -1;
}

void  roadmap_serial_close (RoadMapSerial serial) { }

