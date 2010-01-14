/* roadmap_serial.c - serial connection management.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 */

#include <windows.h>
#include "../roadmap.h"
#include "../roadmap_serial.h"
#include "win32_serial.h"
#include "listports.h"


static int serial_ports[MAX_SERIAL_ENUMS];

#define MAX_SERIAL_SPEEDS 10

static BOOL CALLBACK list_port_callback(LPVOID pCallbackValue,
                               LISTPORTS_PORTINFO* lpPortInfo) {

   int index;
   char *str = ConvertToMultiByte (lpPortInfo->lpPortName, CP_UTF8);

   index = atoi (str+3);
   free (str);

   if ((index >= 0) && (index < MAX_SERIAL_ENUMS)) {
      serial_ports [index] = 1;
   }

   return TRUE;
}


RoadMapSerial roadmap_serial_open(const char *name, const char *mode,
		int baud_rate)
{

   Win32SerialConn *conn = malloc (sizeof(Win32SerialConn));

   if (conn == NULL) return NULL;
   
   memset( conn, 0, sizeof(Win32SerialConn));

   strncpy_safe (conn->name, name, sizeof(conn->name));
   strncpy_safe (conn->mode, mode, sizeof(conn->mode));
   conn->baud_rate = baud_rate;

   conn->handle = INVALID_HANDLE_VALUE;
   conn->data_count = 0;
   conn->ref_count = 1;
   conn->valid = 1;

   return conn;
}


void roadmap_serial_close(RoadMapSerial serial)
{
   if (ROADMAP_SERIAL_IS_VALID (serial)) {

      if (serial->handle != INVALID_HANDLE_VALUE) {
         CloseHandle(serial->handle);
         serial->handle = INVALID_HANDLE_VALUE;
      }

      serial->valid = 0;
      if (!--serial->ref_count) {
         free (serial);
      }
   }
}


int roadmap_serial_read(RoadMapSerial serial, void *data, int size)
{

   if (!ROADMAP_SERIAL_IS_VALID (serial) ||
         (serial->handle == INVALID_HANDLE_VALUE)) {

      return -1;
   }

   if (serial->data_count <= 0) return serial->data_count;

   if (size > serial->data_count) size = serial->data_count;

   memcpy (data, serial->data, size);

   serial->data_count -= size;

   return size;
}


int roadmap_serial_write (RoadMapSerial serial, const void *data, int length)
{
   DWORD dwBytesWritten;

   if (!ROADMAP_SERIAL_IS_VALID (serial) ||
         (serial->handle == INVALID_HANDLE_VALUE)) {

      return -1;
   }


   if(!WriteFile(serial->handle,
                data,
                length,
                &dwBytesWritten,
                NULL)) {
      return -1;
   }

   return dwBytesWritten;
}


const int *roadmap_serial_enumerate (void)
{
   if (!serial_ports[0]) {
      
      ListPorts (&list_port_callback, NULL);
   }

   return serial_ports;
}


const char **roadmap_serial_get_speeds (void)
{
   static const char *serial_speeds[MAX_SERIAL_SPEEDS] = 
               {"4800", "9600", "19200", "38400", "57600", NULL};

   return serial_speeds;
}

