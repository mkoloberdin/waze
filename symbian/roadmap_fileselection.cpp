/* roadmap_fileselection.cpp - manage the Widget used in roadmap dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Base off code Copyright 2005 Ehud Shabtai
 *
 *   This file is part of RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_fileselection.h
 */


extern "C" {
#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_file.h"
#include "roadmap_fileselection.h"
}

void roadmap_fileselection_new (const char *title,
								const char *filter,
								const char *path,
								const char *mode,
								RoadMapFileCallback callback)
{
//TODO this stub - FIX
#ifndef __SYMBIAN32__
	WCHAR filename[MAX_PATH] = {0};
	WCHAR strFilter[MAX_PATH] = {0};
	LPWSTR fltr = NULL;
	LPWSTR title_unicode = ConvertToWideChar(title, CP_UTF8);
	BOOL res;

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);
	ofn.Flags = OFN_EXPLORER;
	ofn.lpstrTitle = title_unicode;
	if (filter != NULL) {
		fltr = ConvertToWideChar(filter, CP_UTF8);
		_snwprintf(strFilter, sizeof(strFilter)/sizeof(strFilter[0]),
						TEXT("*.%s|*.%s\0"), fltr, fltr);
      strFilter[wcslen(fltr)+2] = 0;
      ofn.lpstrDefExt = fltr;
		ofn.lpstrFilter = strFilter;
	} else {
		ofn.lpstrFilter = TEXT("*.*\0*.*\0");
	}

	if (strchr(mode, 'r') != NULL) {
		ofn.Flags |= OFN_FILEMUSTEXIST;
		res = GetOpenFileName(&ofn);

	} else {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		res = GetSaveFileName(&ofn);
	}

	free((char*)ofn.lpstrTitle);
   if (fltr) free(fltr);

	if (res) {
		char *name = ConvertToMultiByte(filename, CP_UTF8);
		(*callback)(name, mode);
		free(name);
	}
#endif
}

