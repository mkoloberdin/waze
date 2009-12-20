/* editor_dictionary.c - Build a Map dictionary table & index for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "roadmap.h"

#include "editor_db.h"
#include "editor_dictionary.h"

static editor_db_section *ActiveDictionary;

static void editor_dictionary_activate (editor_db_section *section) {
   ActiveDictionary = section;
}

editor_db_handler EditorDictionaryHandler = {
   EDITOR_DB_DICTIONARY,
   1,
   0,
   editor_dictionary_activate
};

int editor_dictionary_locate (const char *string) {

	int i;
	
	if (!string) return -1;
	
	for (i = editor_db_get_item_count(ActiveDictionary) - 1; i >= 0; i--) {
		if (!strcmp (string, editor_db_get_item (ActiveDictionary, i, 0, NULL))) {
			return i;
		}
	}	
	
	return -1;
}


int editor_dictionary_add
                 (const char *string) {

   int string_id;
   char *db_string;
   int length = strlen (string);

	if (!string) return -1;
	
	string_id = editor_dictionary_locate (string);
	if (string_id >= 0) return string_id;
	
   string_id =
      editor_db_allocate_items(ActiveDictionary, length+1);

   if (string_id == -1) {
      roadmap_log (ROADMAP_ERROR, "dictionary data full");
      return -1;
   }

   db_string = (char *) editor_db_get_item (
      ActiveDictionary, string_id, 0, NULL);

   strncpy (db_string, string, length);
   db_string[length] = 0;

   editor_db_write_item(ActiveDictionary, string_id, length + 1);

   return string_id;
}


char *editor_dictionary_get (int index) {

   if (index >= editor_db_get_item_count(ActiveDictionary) || index < 0) {
      return NULL;
   }

   return (char *) editor_db_get_item(ActiveDictionary, index, 0, NULL);
}

