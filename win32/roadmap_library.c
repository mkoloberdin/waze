/* roadmap_library.c - a low level module to manage plugins for RoadMap.
 * This is a dummy file as plugins are currently not implmemented under wince.
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
 *   See roadmap_library.h
 */

#include "../roadmap.h"
#include "../roadmap_library.h"
#include "../roadmap_file.h"

RoadMapLibrary roadmap_library_load (const char *pathname)
{
	return NULL;
}


int roadmap_library_exists (const char *pathname)
{
	return 0;
}


void *roadmap_library_symbol (RoadMapLibrary library, const char *symbol)
{
	return NULL;
}
