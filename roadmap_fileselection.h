/* roadmap_fileselection.h - manage the Widget used in roadmap dialogs.
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
 */

#ifndef INCLUDED__ROADMAP_FILESELECTION__H
#define INCLUDED__ROADMAP_FILESELECTION__H

typedef void (*RoadMapFileCallback) (const char *filename, const char *mode);

void roadmap_fileselection_new (const char *title,
                                const char *filter,
                                const char *path,
                                const char *mode,
                                RoadMapFileCallback callback);

#endif // INCLUDED__ROADMAP_FILESELECTION__H

