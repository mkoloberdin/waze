/* roadmap_dictionary.h - the API for accessing RoadMap dictionary volumes.
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

#ifndef _ROADMAP_DICTIONARY__H_
#define _ROADMAP_DICTIONARY__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"

typedef struct dictionary_volume *RoadMapDictionary;
typedef struct roadmap_dictionary_mask *RoadMapDictionaryMask;
typedef int (*RoadMapDictionaryCB)
   (RoadMapString index, const char *string, void *data);

RoadMapDictionary roadmap_dictionary_open (char *name);

char *roadmap_dictionary_get (RoadMapDictionary d, RoadMapString index);
int   roadmap_dictionary_count (RoadMapDictionary d);


typedef struct dictionary_cursor *RoadMapDictionaryCursor;

RoadMapDictionaryCursor roadmap_dictionary_new_cursor (RoadMapDictionary d);
int  roadmap_dictionary_move_cursor (RoadMapDictionaryCursor c, char input,
                                     RoadMapDictionaryMask mask);
int  roadmap_dictionary_completable (RoadMapDictionaryCursor c);
void roadmap_dictionary_completion  (RoadMapDictionaryCursor c, char *data);
void roadmap_dictionary_get_next    (RoadMapDictionaryCursor c, char *set);
int  roadmap_dictionary_get_result  (RoadMapDictionaryCursor c);
void roadmap_dictionary_get_all_results (RoadMapDictionaryCursor c,
                                         RoadMapDictionaryMask mask,
                                         RoadMapDictionaryCB callback,
                                         void *data);
void roadmap_dictionary_free_cursor (RoadMapDictionaryCursor c);

int  roadmap_dictionary_search_all
            (RoadMapDictionary dictionary, RoadMapDictionaryMask mask,
             const char *str,
             int word_search,
             RoadMapDictionaryCB callback,
             void *data);

RoadMapString roadmap_dictionary_locate (RoadMapDictionary d,
                                         const char *string);
void          roadmap_dictionary_dump   (void);
void          roadmap_dictionary_dump_volume (char *name);

RoadMapDictionaryMask roadmap_dictionary_mask_new
                                  (RoadMapDictionary dictionary);
void roadmap_dictionary_mask_free (RoadMapDictionaryMask mask);
void roadmap_dictionary_mask_set  (RoadMapDictionary d,
                                   RoadMapString index,
                                   RoadMapDictionaryMask mask);

extern roadmap_db_handler RoadMapDictionaryHandler;

#endif // _ROADMAP_DICTIONARY__H_

