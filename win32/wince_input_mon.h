/* wince_input_mon.h
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

#ifndef __WINCE_SERIAL_
#define __WINCE_SERIAL_

#include <windows.h>
#include "../roadmap_main.h"
#include "../roadmap_io.h"

typedef struct roadmap_main_io {
   RoadMapIO *io;
   RoadMapInput callback;
   int is_valid;
} roadmap_main_io;

DWORD WINAPI SerialMonThread(LPVOID lpParam);
DWORD WINAPI SocketMonThread(LPVOID lpParam);
DWORD WINAPI FileMonThread(LPVOID lpParam);

#endif //__WINCE_SERIAL_
