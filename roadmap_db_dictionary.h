/* roadmap_db_dictionary.h - Name dictionary tables for RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   A RoadMap dictionary is made of the following tables:
 *
 *   string.*.data      a buffer that contains the string value.
 *   string.*.tree      for each node, the list of children nodes.
 *   string.*.node      the tree nodes, reference either a tree branch
 *                      or a leaf (i.e. the offset to the string data).
 *
 *   The tree structure speeds up the retrieval of a given string. It is also
 *   designed to help enter a string by predicting which characters are
 *   possible after the last entered. Each time one enter a new character
 *   of the string, the program can move one node deeper in the tree and
 *   look ahead at what is the possible next character.
 *
 *   There is a dictionary for each type of strings: street prefix, name,
 *   suffix and type, city name.
 */

#ifndef _ROADMAP_DB_DICTIONARY__H_
#define _ROADMAP_DB_DICTIONARY__H_


#include "roadmap_db.h"

#define ROADMAP_DICTIONARY_NULL   0
#define ROADMAP_DICTIONARY_TREE   1
#define ROADMAP_DICTIONARY_STRING 2

struct roadmap_dictionary_reference {  /* tables string.*.node */

   char           character;
   unsigned char  type;
   unsigned short index;
};

struct roadmap_dictionary_tree {  /* tables string.*.tree */

   unsigned short first;
   unsigned char  count;
   unsigned char  position;
   unsigned char  num_strings;
};

typedef struct roadmap_dictionary_reference RoadMapDictionaryReference;
typedef struct roadmap_dictionary_tree      RoadMapDictionaryTree;

#endif /* _ROADMAP_DB_DICTIONARY__H_ */

