/* roadmap_messagebox.c - message box implementation
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
 * SYNOPSYS:
 *
 *   See roadmap_messagebox.h
 */

#include <windows.h>
#include "../roadmap.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../roadmap_messagebox.h"


void roadmap_messagebox (const char *title, const char *text)
{
#ifdef _UNICODE
	LPWSTR u_title, u_text;
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);
	u_title = ConvertToWideChar(title, CP_UTF8);
	u_text = ConvertToWideChar(text, CP_UTF8);
	MessageBox(GetActiveWindow(), u_text, u_title, MB_OK);
	free(u_title);
	free(u_text);
#else
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);
	MessageBox(GetActiveWindow(), text, title, MB_OK);
#endif
}

