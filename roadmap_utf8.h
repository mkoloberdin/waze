/* roadmap_utf8.h - UTF8 utilities
 *
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */

#ifndef _ROADAMP_UTF8_H__
#define _ROADAMP_UTF8_H__

int         utf8_strlen             (const char* utf8);
void        utf8_remove_last_char   (char* utf8);
const char *utf8_get_next_char      (const char *s, char *c, int size);
const char *utf8_get_next_wchar     (const char *s, unsigned int *ch);

char*       utf8_char_from_utf16    ( unsigned short utf16);
char*       utf8_string_from_utf16  ( const unsigned short* utf16);

// Break utf8 string into an array of utf8 characters
//    Return value: 
//       Array of utf8 characters (strings). 
//       Size of array is 'size'.
char**      utf8_to_char_array      ( /* IN */ const char* utf8, /* OUT */ int* size);
void        utf8_free_char_array    ( char** array, int size);

#endif // _ROADAMP_UTF8_H__

