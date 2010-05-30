/* win32_serial.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


#ifndef __WIN32_SERIAL_
#define __WIN32_SERIAL_

#include <windows.h>

typedef struct Win32SerialConn {
   
   HANDLE handle;
   char name[20];
   char mode[10];
   int baud_rate;
   char data[10240];
   int data_count;
   int ref_count;
   int valid;
   
} Win32SerialConn;

#endif //__WIN32_SERIAL_
