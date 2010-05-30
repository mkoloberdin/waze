/* roadmap_win32.h
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

#ifndef _ROADMAP_WIN32_H_
#define _ROADMAP_WIN32_H_

#include <windows.h>
#include <stdio.h>

#if defined(__GNUC__)
#define __try try
#define __except(x) catch (...)
#else
#define inline _inline
#endif

#define MENU_ID_START	               WM_USER
#define MAX_MENU_ITEMS	               (100)
#define TOOL_ID_START	               (MENU_ID_START + MAX_MENU_ITEMS + 1)
#define MAX_TOOL_ITEMS	               (100)
#define WM_FREEMAP                     (TOOL_ID_START + MAX_TOOL_ITEMS + 1)
#define WM_FREEMAP_READ                (WM_FREEMAP + 1)
#define WM_FREEMAP_SYNC                (WM_FREEMAP + 2)
#define WM_FREEMAP_SOCKET              (WM_FREEMAP + 3)
#define WM_FREEMAP_RECEIVE             (WM_FREEMAP + 4)
#define WM_FREEMAP_DEVICE_EVENT        (WM_FREEMAP + 4)

#ifndef snprintf
#define snprintf _snprintf
#endif
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#define trunc(X) (X)

LPWSTR ConvertToWideChar(LPCSTR string, UINT nCodePage);
char* ConvertToMultiByte(const LPCWSTR s, UINT nCodePage);
const char *roadmap_main_get_virtual_serial (void);

time_t timegm(struct tm *_tm);

#endif /* _ROADMAP_WIN32_H_ */
