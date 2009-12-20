/* roadmap_utf8.c - UTF8 utilities
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

#include "roadmap.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap_utf8.h"

// UTF-8 format
// ------------
//
// Char. number range  |        UTF-8 octet sequence
//    (hexadecimal)    |              (binary)
// --------------------+---------------------------------------------
// 0000 0000-0000 007F | 0xxxxxxx
// 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
// 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
// 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
////////////////////////////////////////////////////////////////////////////////

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const unsigned int offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL,
               0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

////////////////////////////////////////////////////////////////////////////////

int utf8_strlen (const char* utf8) {
   int   len = 0;
   int   sizeof_current_char;

   if( !utf8) {
      return 0;
   }

   while (*utf8) {

      if ((unsigned)(*utf8) < 0x80)
         sizeof_current_char = 1;         // binary:  0xxxxxxx
      else if (0 == (0x20 & (*utf8)))
         sizeof_current_char = 2;         // binary:  110xxxxx
      else if (0 == (0x10 & (*utf8)))
         sizeof_current_char = 3;         // binary:  1110xxxx
      else
         sizeof_current_char = 4;         // binary:  11110xxx

#ifdef   _DEBUG
      if ((1 < sizeof_current_char) && (0 == (0x40 & (*utf8))))
         assert(0);  // not a valid case in utf8
#endif   // _DEBUG

      len++;
      utf8 += sizeof_current_char;     
   }

   return len;
}

void utf8_remove_last_char (char* utf8) {
   char*          str_end;
   unsigned char  char_erased;

   // Stop condition:
   int  erased_all_string;      // String was one character long
   int  single_byte_character;  // Need to erase only one byte
   int  first_byte_of_sequence; // Erased all bytes of character,
                                 //  including the first one

   if( !utf8 || !(*utf8))
      return;

   str_end = utf8 + strlen(utf8) - 1;

   do {
      char_erased = *str_end;
      (*str_end--)= '\0';

      // Make stop-condition more readable:
      erased_all_string       = (str_end     < utf8);
      single_byte_character   = (char_erased < 0x80);
      first_byte_of_sequence  = (char_erased & 0x40);

   }  while( !erased_all_string && !single_byte_character &&
             !first_byte_of_sequence);
}


// Extracts the first character as a utf8 string.
// Returns a pointer to the next character of the string.
const char *utf8_get_next_char (const char *s, char *c, int size) {

   int count = strlen(s);
   int skip = 1;

   if (!count) return s;

   if ((unsigned char)*s <= 0x7F) skip = 1;
   else if (((unsigned char)*s & 0xE0) == 0xC0) skip = 2;
   else if (((unsigned char)*s & 0xF0) == 0xE0) skip = 3;
   else if (((unsigned char)*s & 0xF8) == 0xF0) skip = 4;

   if (skip > count) skip = count;

   if (c) {
      size--;
      while (skip && size) {
         *c = *s;
         c++;
         s++;
         skip--;
         size--;
      }
      *c = '\0';

   } else {
      s += skip;
   }
   return s;
}


// Extracts the first character as a wchar value.
// Returns a pointer to the next character of the string.
const char *utf8_get_next_wchar (const char *s, unsigned int *ch) {

	char c[7]; // Max size of one utf character + null
	const unsigned char *source = (unsigned char *)c;
	const char *ret = utf8_get_next_char(s, c, sizeof(c));

	*ch = 0;

	switch (strlen(c)) {
	    case 6: *ch += *source++; *ch <<= 6;
	    case 5: *ch += *source++; *ch <<= 6;
	    case 4: *ch += *source++; *ch <<= 6;
	    case 3: *ch += *source++; *ch <<= 6;
	    case 2: *ch += *source++; *ch <<= 6;
	    case 1: *ch += *source++;
	}
	*ch -= offsetsFromUTF8[strlen(c) - 1];

	return ret;
}

#ifdef WIN32
char* utf8_from_utf16( const unsigned short* utf16, BOOL single_character)
{
   const unsigned short*   w; 
   unsigned short          buffer[2];
   const unsigned short*   input = utf16;
   unsigned char*          output= NULL;
   int                     i     = 0;
   int                     len   = 0;
   
   if( single_character)
   {
      buffer[0] = (short)utf16;
      buffer[1] = L'\0';
      
      input = buffer;
   }

   for ( w = input; *w; w++ )
   {
      if ( *w < 0x0080) 
         len++;
      else if ( *w < 0x0800 ) 
         len += 2;
      else 
         len += 3;
   }

   output = ( unsigned char* )malloc( len+1 );
   if( !output)
      return NULL;

   for( w = input; *w; w++)
   {
      if ( *w < 0x0080 )
         output[i++] = ( unsigned char ) *w;
      else if ( *w < 0x0800 )
      {
         output[i++] = 0xC0 | ((*w) >> 6 );
         output[i++] = 0x80 | ((*w) & 0x3F);
      }
      else
      {
         output[i++] = 0xE0 |  ((*w) >> 12 );
         output[i++] = 0x80 | (((*w) >> 6 ) & 0x3F);
         output[i++] = 0x80 |  ((*w) & 0x3F);
      }
   }

   output[i] = '\0';
   return (char*)output;
}

char* utf8_string_from_utf16( const unsigned short* utf16)
{ return utf8_from_utf16( utf16, FALSE /* single char? */);}

char* utf8_char_from_utf16( unsigned short utf16)
{ return utf8_from_utf16( (const unsigned short*)utf16, TRUE /* single char? */);}

#endif

void utf8_free_char_array( char** array, int size)
{
   int i;
   for( i=0; i<size; i++)
      FREE(array[i])
      
   FREE(array);
}

char** utf8_to_char_array( const char* utf8, int* count)
{
   int      sizeof_current_char;
   int      index;
   char**   array;
   
   (*count) = utf8_strlen(utf8);
   
   if( !(*count))
      return NULL;
      
   array = (char**)malloc( sizeof(char*) * (*count));
   if( !array)
   {
      assert(0);
      (*count) = 0;
      return NULL;
   }
   
   index = 0;
   while( *utf8)
   {
      sizeof_current_char = 0;
      
      if ((unsigned)(*utf8) < 0x80)
         sizeof_current_char = 1;         // binary:  0xxxxxxx
      else if (0 == (0x20 & (*utf8)))
         sizeof_current_char = 2;         // binary:  110xxxxx
      else if (0 == (0x10 & (*utf8)))
         sizeof_current_char = 3;         // binary:  1110xxxx
      else
         sizeof_current_char = 4;         // binary:  11110xxx

#ifdef   _DEBUG
      if ((1 < sizeof_current_char) && (0 == (0x40 & (*utf8))))
         assert(0);  // not a valid case in utf8
#endif   // _DEBUG

      if( !sizeof_current_char)
      {
         assert(0);
         (*count) = 0;
         return NULL;
      }
      
      array[index] = (char*)malloc(sizeof_current_char+1);
      assert(array[index]);
      strncpy( array[index], utf8, sizeof_current_char);
      array[index][sizeof_current_char] = '\0';
      
      index++;
      utf8 += sizeof_current_char;     
   }
   
   assert( index == (*count));
   
   return array;   
}

