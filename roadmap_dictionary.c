/* roadmap_dictionary.c - Access a Map dictionary table & index.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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
 * See roadmap_dictionary.h.
 *
 * These functions are used to access a dictionary of strings from
 * a RoadMap database.
 *
 * Every string in the dictionary can be retrieved using a character
 * search tree:
 * - each node of the tree represents one character in the string.
 * - each node is a list of references to the next level, one
 *   reference for each possible character in the referenced strings.
 * - the references are indexes into an indirection table (to keep
 *   the references small), either the string table (leaf)
 *   or the node table (node).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_dictionary.h"

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_dictionary.h"


struct dictionary_volume {

   const char *name;
   struct dictionary_volume *next;

   struct roadmap_dictionary_tree      *tree;
   int tree_count;

   struct roadmap_dictionary_reference *reference;
   int reference_count;

   unsigned int *string_index;
   int string_count;

   char *data;
   int   size;

   unsigned short *subtrees;
   unsigned short subtrees_count;
};

#define ROADMAP_DICTIONARY_MAX   16

static struct dictionary_volume *DictionaryVolume = NULL;


struct dictionary_cursor {

   struct dictionary_volume *volume;
   int tree_index;
   int complete;
   int completion;
   int position;
};

struct roadmap_dictionary_mask {
   int size;
   unsigned char relevant[1];
   // The array will have an actual size according to the amount of references
};


static char *roadmap_dictionary_get_string
                (struct dictionary_volume *dictionary, int index) {

   return dictionary->data
             + dictionary->string_index[dictionary->reference[index].index];
}

#ifndef J2ME
static void roadmap_dictionary_print_subtree
               (struct dictionary_volume *dictionary, int tree_index) {

   int index;
   struct roadmap_dictionary_tree *tree = dictionary->tree + tree_index;


   fprintf (stdout, "%*.*s --- start of tree %d (count = %d)\n",
                    tree->position,
                    tree->position,
                    "",
                    (int)(tree - dictionary->tree),
                    tree->count);

   for (index = tree->first;
        index < tree->first + tree->count;
        index++) {

      fprintf (stdout, "%*.*s '%c' [%d]",
                       tree->position,
                       tree->position,
                       "",
                       dictionary->reference[index].character,
                       tree->position);

      switch (dictionary->reference[index].type & 0x0f) {

      case ROADMAP_DICTIONARY_STRING:

         fprintf (stdout, " --> [%d] \"%s\"\n",
                          dictionary->reference[index].index,
                          roadmap_dictionary_get_string (dictionary, index));

         break;

      case ROADMAP_DICTIONARY_TREE:

         fprintf (stdout, "\n");
         roadmap_dictionary_print_subtree
            (dictionary, dictionary->reference[index].index);

         break;

      default:
         fprintf (stdout, "\n");
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      dictionary->reference[index].type & 0x0f);
      }
   }
}
#endif

static void roadmap_dictionary_walk
               (struct dictionary_volume *dictionary, int tree_index,
                RoadMapDictionaryMask mask,
                RoadMapDictionaryCB callback, void *data) {

   int index;
   struct roadmap_dictionary_tree *tree = dictionary->tree + tree_index;
   RoadMapString string;

   for (index = tree->first;
        index < tree->first + tree->count;
        index++) {

      if (mask && !mask->relevant[index]) continue;

      switch (dictionary->reference[index].type & 0x0f) {

      case ROADMAP_DICTIONARY_STRING:

         string = dictionary->reference[index].index;
         if (!(*callback)
               (string, roadmap_dictionary_get (dictionary, string), data)) {
            return;
         }
         break;

      case ROADMAP_DICTIONARY_TREE:

         roadmap_dictionary_walk
                  (dictionary, dictionary->reference[index].index,
                   mask, callback, data);
         break;

      default:
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      dictionary->reference[index].type & 0x0f);
      }
   }
}


static int roadmap_dictionary_get_reference
              (struct dictionary_volume *dictionary,
               struct roadmap_dictionary_tree *tree, char c) {

   int index;
   int end = tree->first + tree->count;
   struct roadmap_dictionary_reference *reference = dictionary->reference;

   c = tolower(c);

   for (index = tree->first; index < end; index++) {

      if (reference[index].character == c) {
         return index;
      }
   }

   return -1;
}


static int
roadmap_dictionary_search
   (struct dictionary_volume *dictionary, const char *string, int length) {

   int   i;
   int   index;
   char  character;
   const char *cursor;
   char *stored;
   struct roadmap_dictionary_tree *tree;


   tree = dictionary->tree;

   for (i= 0, cursor = string; i <= length; cursor++, i++) {

        if (tree->position > i) continue;

        if (i < length) {
           character = *cursor;
        } else {
           character = 0;
        }
        index = roadmap_dictionary_get_reference (dictionary, tree, character);

        if (index < 0) {
           return -1;
        }

        switch (dictionary->reference[index].type) {

        case ROADMAP_DICTIONARY_STRING:

           stored = roadmap_dictionary_get_string (dictionary, index);

           if ((stored[length] != 0) ||
               (strncasecmp (string, stored, length) != 0)) {
              return -1;
           }
           return dictionary->reference[index].index;

        case ROADMAP_DICTIONARY_TREE:

           if (*cursor == 0) {
              roadmap_log (ROADMAP_FATAL,
                           "found a subtree after end of string");
           }

           tree = dictionary->tree + dictionary->reference[index].index;
           break;

        default:
           roadmap_log (ROADMAP_FATAL,
                        "corrupted node when searching for %s", string);
        }
   }

   return -1;
}


static struct dictionary_volume *
         roadmap_dictionary_initialize_one (const roadmap_db_data_file *file,
         											  unsigned int id,
         											  const char *name,
         											  struct dictionary_volume *first) {

   struct dictionary_volume *dictionary;
   void *data;
   int size;
   
   if (!roadmap_db_get_data (file,
   								  id,
   								  sizeof (char),
   								  &data,
   								  &size)) {
   
   	roadmap_log (ROADMAP_ERROR, "invalid dictionary structure");
   	return first;								  	
   }
   
   if (!data) return first;


   /* Retrieve all the database sections: */

   dictionary = malloc (sizeof(struct dictionary_volume));

   roadmap_check_allocated(dictionary);

   dictionary->name = name;

   dictionary->data = data;
   dictionary->size = size;

   dictionary->tree = NULL;
   dictionary->tree_count = 0;

   dictionary->reference = NULL;
   dictionary->reference_count = 0;

   dictionary->string_index = NULL;
   dictionary->string_count = 0;

   dictionary->subtrees = NULL;
   dictionary->subtrees_count = 0;

	dictionary->next = first;
	
   return dictionary;
}


static void *roadmap_dictionary_map (const roadmap_db_data_file *file) {

   struct dictionary_volume *first = NULL;


	first = roadmap_dictionary_initialize_one (file, model__tile_string_prefix, "prefix", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_street, "street", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_text2speech, "text2speech", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_type, "type", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_suffix, "suffix", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_city, "city", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_landmark, "landmark", first);
	first = roadmap_dictionary_initialize_one (file, model__tile_string_attributes, "attributes", first);

   return first;
}

static void roadmap_dictionary_activate (void *context) {

   DictionaryVolume = (struct dictionary_volume *) context;
}

static void roadmap_dictionary_unmap (void *context) {

   struct dictionary_volume *this = (struct dictionary_volume *) context;

   if (this == DictionaryVolume) {
      DictionaryVolume = NULL;
   }

   while (this != NULL) {

      struct dictionary_volume *next = this->next;

      if (this->subtrees) free (this->subtrees);

      free (this);
      this = next;
   }
}

roadmap_db_handler RoadMapDictionaryHandler = {
   "dictionary",
   roadmap_dictionary_map,
   roadmap_dictionary_activate,
   roadmap_dictionary_unmap
};


static void roadmap_dictionary_find_words
               (struct dictionary_volume *dictionary, int tree_index) {

   int index;
   struct roadmap_dictionary_tree *tree = dictionary->tree + tree_index;

   for (index = tree->first;
        index < tree->first + tree->count;
        index++) {

      switch (dictionary->reference[index].type & 0x0f) {

      case ROADMAP_DICTIONARY_STRING:
         break;

      case ROADMAP_DICTIONARY_TREE:

         if (dictionary->reference[index].character == ' ') {
            dictionary->subtrees[dictionary->subtrees_count] = tree_index;
            dictionary->subtrees_count++;
         }

         roadmap_dictionary_find_words
               (dictionary, dictionary->reference[index].index);
         break;

      default:
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      dictionary->reference[index].type & 0x0f);
      }
   }
}


static void roadmap_dictionary_build_substrees
                  (struct dictionary_volume *dictionary) {

   if (dictionary->subtrees) return;

   dictionary->subtrees =
      malloc(sizeof(unsigned short) * dictionary->tree_count);

   roadmap_dictionary_find_words (dictionary, 0);

   if (dictionary->subtrees_count > dictionary->tree_count) {
      roadmap_log (ROADMAP_FATAL,
                  "roadmap_dictionary_build_substrees: too many subtrees!!");
   }

   if (dictionary->subtrees_count < dictionary->tree_count) {
      dictionary->subtrees =
         realloc(dictionary->subtrees,
                  sizeof(unsigned short) * dictionary->subtrees_count);
   }
}


RoadMapDictionary roadmap_dictionary_open (char *name) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (volume->name, name) == 0) {
         return volume;
      }
   }

   return NULL;
}


char *roadmap_dictionary_get (RoadMapDictionary d, RoadMapString index) {
/*
   if (index >= d->string_count) {
      return NULL;
   }
*/
   return d->data + index; //d->string_index[index];
}


int  roadmap_dictionary_count (RoadMapDictionary d) {

   return d->string_count;
}


RoadMapDictionaryCursor roadmap_dictionary_new_cursor (RoadMapDictionary d) {

   RoadMapDictionaryCursor cursor = malloc (sizeof(struct dictionary_cursor));

   if (cursor != NULL) {
      cursor->volume = d;
      cursor->tree_index = 0;
      cursor->complete   = 0;
      cursor->completion = 0;
      cursor->position = 0;
   }

   return cursor;
}


int roadmap_dictionary_move_cursor (RoadMapDictionaryCursor c, char input,
                                    RoadMapDictionaryMask mask) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;


   if (c->complete) {
      return 0;
   }

   tree = c->volume->tree + c->tree_index;
   reference = c->volume->reference;
   end = tree->first + tree->count;

   if (tree->position > c->position) {
      c->position++;
      return 1;
   }

   input = tolower(input);
   c->position++;

   for (index = tree->first; index < end; index++) {

      if (mask && !mask->relevant[index]) continue;

      if (reference[index].character == input) {

         switch (reference[index].type & 0x0f) {

         case ROADMAP_DICTIONARY_STRING:

            c->complete = 1;
            c->completion = index;

            break;

         case ROADMAP_DICTIONARY_TREE:

            c->complete = 0;
            c->tree_index = reference[index].index;

            break;

         default:

            roadmap_log (ROADMAP_FATAL,
                         "corrupted node type %d",
                         reference[index].type & 0x0f);
         }

         return 1;
      }
   }

   return 0;
}


int  roadmap_dictionary_completable (RoadMapDictionaryCursor c) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   if (c->complete) {
      return 1;
   }

   tree = c->volume->tree + c->tree_index;
   end = tree->first + tree->count;
   reference = c->volume->reference;

   if (tree->position == 0) {
      return 0;
   }

   for (index = tree->first; index < end; index++) {

      if (reference[index].character == 0) {
         c->completion = index;
         return 1;
      }
   }

   return 0;
}


void roadmap_dictionary_completion (RoadMapDictionaryCursor c, char *data) {

   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   tree = c->volume->tree;

   if (c->completion <= 0) {

      reference = c->volume->reference;

      /* Let run tree into the subtree, looking for any of the possible
       * solutions: by truncating the string at the current tree position,
       * we will get our string completion string.
       */

      for (index = tree[c->tree_index].first;
           (reference[index].type & 0x0f) == ROADMAP_DICTIONARY_TREE;
           index = tree[reference[index].index].first) ;

      if ((reference[index].type & 0x0f) != ROADMAP_DICTIONARY_STRING) {
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      reference[index].type & 0x0f);
      }

      c->completion = index;
   }

   strcpy (data, roadmap_dictionary_get_string (c->volume, c->completion));
   if (!c->complete) {
      data[tree[c->tree_index].position] = 0;
   }
}


int roadmap_dictionary_get_result (RoadMapDictionaryCursor c) {

   if (c->completion <= 0) {
      return 0;
   }

   return c->volume->reference[c->completion].index;
}


void roadmap_dictionary_get_all_results (RoadMapDictionaryCursor c,
                                         RoadMapDictionaryMask mask,
                                         RoadMapDictionaryCB callback,
                                         void *data) {

   if (c->complete) {

      RoadMapString index = c->volume->reference[c->completion].index;
      (*callback) (index, roadmap_dictionary_get (c->volume, index), data);
      return;
   }

   roadmap_dictionary_walk (c->volume, c->tree_index, mask, callback, data);

   return;
}


void roadmap_dictionary_get_next (RoadMapDictionaryCursor c, char *set) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   if (! c->complete) {

      tree = c->volume->tree + c->tree_index;
      end  = tree->first + tree->count;

      reference = c->volume->reference;

      for (index = tree->first; index < end; index++) {

         if (reference[index].character != 0) {
            *(set++) = reference[index].character;
         }
      }
   }

   *set = 0;
}


void roadmap_dictionary_free_cursor (RoadMapDictionaryCursor c) {
   free (c);
}


RoadMapString roadmap_dictionary_locate (RoadMapDictionary d,
                                         const char *string) {

   int result;

   result = roadmap_dictionary_search (d, string, strlen(string));

   if (result < 0) {
      return ROADMAP_INVALID_STRING;
   }

   return (RoadMapString)result;
}

#ifndef J2ME
void roadmap_dictionary_dump (void) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      printf ("Dictionary %s:\n", volume->name);
      roadmap_dictionary_print_subtree (volume, 0);
   }
}


void roadmap_dictionary_dump_volume (char *name) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (volume->name, name) == 0) {
         printf ("Dictionary %s:\n", volume->name);
         roadmap_dictionary_print_subtree (volume, 0);
      }
   }
}

#endif

int roadmap_dictionary_search_all
            (RoadMapDictionary dictionary, RoadMapDictionaryMask mask,
             const char *str,
             int word_search,
             RoadMapDictionaryCB callback,
             void *data) {

   struct dictionary_cursor cursor;
   const char *itr;
   int subtree_index;
   int subtrees_count = 0;
   int match_count = 0;
   char subtree_str[500];

   cursor.volume = dictionary;

   if (word_search) {
      if (!dictionary->subtrees) {
            roadmap_dictionary_build_substrees (dictionary);
      }

      if (strlen(str)) {
         subtrees_count = dictionary->subtrees_count;
         subtree_str[0] = ' ';
         strncpy(subtree_str + 1, str, sizeof(subtree_str) - 2);
         subtree_str[sizeof(subtree_str)-1] = '\0';
      }
   }

   for (subtree_index = -1; subtree_index < subtrees_count; subtree_index++) {
      int status;
      int subtree_start_pos = 0;

      itr = str;
      if (subtree_index == -1) {
         cursor.tree_index = 0;
         cursor.position   = 0;
      } else {
         cursor.tree_index = dictionary->subtrees[subtree_index];
         cursor.position   = (dictionary->tree + cursor.tree_index)->position;
         subtree_start_pos = cursor.position;
      }

      cursor.complete   = 0;
      cursor.completion = 0;

      if (subtree_index != -1) {
         // A subtree search always starts with a space
         status = roadmap_dictionary_move_cursor (&cursor, ' ', mask);
      } else {
         status = 1;
      }

      while (*itr && status) {
         status = roadmap_dictionary_move_cursor (&cursor, *itr++, mask);
      }

      if (!status && !cursor.complete) continue; // Search the next sub tree

      if (cursor.complete) {
         const char *name;

         name = roadmap_dictionary_get_string
            (cursor.volume, cursor.completion);

         if (((subtree_index == -1) && !strncmp (str, name, strlen(str))) ||
             ((subtree_index != -1) &&
                !strncmp (name + subtree_start_pos, subtree_str,
                          strlen(subtree_str)))) {
            RoadMapString string;

            match_count += 1;

            if (!callback) continue;

            string = dictionary->reference[cursor.completion].index;
            (*callback)
               (string, roadmap_dictionary_get (dictionary, string), data);
         }

         continue;

      } else {

         /* Make sure that this path really leads to strings which match */
         const char *name;
         struct roadmap_dictionary_tree *tree =
                  dictionary->tree + cursor.tree_index;

         while ((dictionary->reference[tree->first].type & 0x0f) ==
                  ROADMAP_DICTIONARY_TREE) {
            tree = dictionary->tree + dictionary->reference[tree->first].index;
         }

         if  ((dictionary->reference[tree->first].type & 0x0f) !=
                     ROADMAP_DICTIONARY_STRING) {

            roadmap_log (ROADMAP_FATAL,
                  "corrupted node type %d",
                  dictionary->reference[tree->first].type & 0x0f);
         }

         name = roadmap_dictionary_get
                  (dictionary, dictionary->reference[tree->first].index);

         if (((subtree_index == -1) && strncmp (str, name, strlen(str))) ||
             ((subtree_index != -1) &&
                  strncmp(name + subtree_start_pos, subtree_str,
                          strlen(subtree_str)))) {
            continue;
         }
      }

      if (callback) {
         roadmap_dictionary_get_all_results (&cursor, mask, callback, data);
      }

      if (mask) {
         int index;
         int count = 0;
         struct roadmap_dictionary_tree *tree =
            cursor.volume->tree + cursor.tree_index;

         for (index = tree->first;
               index < tree->first + tree->count;
               index++) {

            count += mask->relevant[index];
         }

         match_count += count;

      } else {
         match_count += (cursor.volume->tree + cursor.tree_index)->num_strings;
      }
   }

   return match_count;
}


RoadMapDictionaryMask roadmap_dictionary_mask_new (RoadMapDictionary dictionary) {
   RoadMapDictionaryMask mask =
            (RoadMapDictionaryMask)
               calloc(sizeof(struct roadmap_dictionary_mask) + sizeof(unsigned char) * dictionary->reference_count, 1);

   mask->size = dictionary->reference_count;

   return mask;
}

   
void roadmap_dictionary_mask_free (RoadMapDictionaryMask mask) {

   free (mask);
}


static int
roadmap_dictionary_set_mask (struct dictionary_volume *dictionary,
                             struct roadmap_dictionary_tree *tree,
                             const char *string,
                             int length,
                             RoadMapDictionaryMask mask,
                             int *pos) {

   int   index;
   char  character;
   char *stored;

   while (tree->position > *pos) (*pos)++;

   if (*pos > length) return 0;

   character = string[*pos];

   index = roadmap_dictionary_get_reference (dictionary, tree, character);

   if (index < 0) {
      return 0;
   }

   switch (dictionary->reference[index].type) {

      case ROADMAP_DICTIONARY_STRING:

         stored = roadmap_dictionary_get_string (dictionary, index);

         if ((stored[length] != 0) ||
               (strncasecmp (string, stored, length) != 0)) {
            return 0;
         }

         if (index >= mask->size) {
            roadmap_log (ROADMAP_FATAL,
                  "roadmap_dictionary_set_mask: invalid index %d", index);
         }
         // Check if this string is already set
         if (mask->relevant[index]) return 0;

         mask->relevant[index] = 1;
         return 1;

      case ROADMAP_DICTIONARY_TREE:

         tree = dictionary->tree + dictionary->reference[index].index;
         (*pos)++;
         if (roadmap_dictionary_set_mask
                     (dictionary, tree, string, length, mask, pos)) {

            if (index >= mask->size) {
               roadmap_log (ROADMAP_FATAL,
                     "roadmap_dictionary_set_mask: invalid index %d", index);
            }
            if (mask->relevant[index] < 255) mask->relevant[index]++;
            return 1;
         }
         break;

      default:
         roadmap_log (ROADMAP_FATAL,
               "corrupted node when searching for %s", string);
   }

   return 0;
}


void roadmap_dictionary_mask_set (RoadMapDictionary d,
                                  RoadMapString index,
                                  RoadMapDictionaryMask mask) {

   const char *str;
   int pos = 0;

   if (index >= d->string_count) {
      roadmap_log (ROADMAP_FATAL,
                   "roadmap_dictionary_mask_set: invalid index %d", index);
      return;
   }

   str = d->data + d->string_index[index];
   roadmap_dictionary_set_mask (d, d->tree, str, strlen(str), mask, &pos);
}

