/* editor_main.h - main plugin file
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

#ifndef INCLUDE__EDITOR_MAIN__H
#define INCLUDE__EDITOR_MAIN__H

#define EDITOR_ALLOW_LINE_DELETION	0

extern int EditorPluginID;

int editor_is_enabled (void);
void editor_main_initialize (void);
void editor_main_shutdown (void);
void editor_main_check_map (void);
void editor_main_set (int status);
const char *editor_main_get_version (void);

#endif /* INCLUDE__EDITOR_MAIN__H */
