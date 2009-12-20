/* editor_dictionary.h - database layer
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
 */

#ifndef INCLUDE__EDITOR_DICTIONARY__H
#define INCLUDE__EDITOR_DICTIONARY__H

#include "editor/db/editor_db.h"

typedef int EditorString;

int editor_dictionary_locate (const char *string);
int editor_dictionary_add (const char *string);

char *editor_dictionary_get (int index);

extern editor_db_handler EditorDictionaryHandler;

#endif // INCLUDE__EDITOR_DICTIONARY__H

