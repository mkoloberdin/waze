
/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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

#include "roadmap.h"
#include <assert.h>

#include "roadmap_keyboard_text.h"

const unsigned char keyboard_symbol_keys[0xFF] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
   0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
   1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 1, 0, 1, 1, 0, 1, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0
};

const unsigned char keyboard_whitespaces_keys[0xFF] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
   1, 0, 0, 1, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0
};

const unsigned char keyboard_punctuation_keys[0xFF] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 1, 0, 1, 0, 0, 0, 0, 1,
   1, 1, 0, 0, 1, 1, 1, 1, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 1, 1, 1, 0, 1, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0
};

BOOL is_symbol( unsigned char key)
{ return keyboard_symbol_keys[key];}

BOOL is_numeric( unsigned char key)
{ return (('0'<=key)&&(key<='9'));}

BOOL is_white_space( unsigned char key)
{
   switch( key)
   {
      case ' ':  return TRUE;
      case '\t': return TRUE;
      case '\r': return TRUE;
      case '\n': return TRUE;
   }
   
   return FALSE;
}

BOOL is_punctuation( unsigned char key)
{ return keyboard_punctuation_keys[key];}

BOOL is_alphabetic( unsigned char key)
{
   return ( (('a'<=key) && (key <= 'z'))  ||
            (('A'<=key) && (key <= 'Z')));
}            

BOOL is_valid_nonalphabetic_char( char ch, unsigned short input_type)
{
   if( (inputtype_numeric     & input_type) && is_numeric( ch))
      return TRUE;

   if( (inputtype_white_spaces& input_type) && is_white_space( ch))
      return TRUE;

   if( (inputtype_punctuation & input_type) && is_punctuation( ch))
      return TRUE;
      
   if( (inputtype_symbols     & input_type) && is_symbol( ch))
      return TRUE;
      
   return FALSE;
}      

BOOL is_valid_key( const char* utf8char, uint16_t input_type)
{
   BOOL latin_alphabethic;
   
   if( !utf8char || !(*utf8char))
   {
      assert(0);
      return FALSE;
   }
   
   latin_alphabethic = ('\0' != utf8char[1]);

   if( inputtype_binary == input_type)
      return TRUE;
      
   // Searching for alpha-bet?
   if( (inputtype_alphabetic & input_type) && (latin_alphabethic || is_alphabetic( *utf8char)))
      return TRUE;
   
   if( latin_alphabethic)
      return FALSE;  // All other tests should fail...
      
   return is_valid_nonalphabetic_char( utf8char[0], input_type);
}      


