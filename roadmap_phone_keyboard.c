
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

#include <string.h>

#include "roadmap_phone_keyboard.h"
#include "roadmap_utf8.h"
#include "roadmap_lang.h"
#include "roadmap_time.h"
#include "roadmap_keyboard_text.h"
#include "roadmap_config.h"
#include "roadmap_phone_keyboard_defs.h"

void multiple_key_info_init( multiple_key_info_ptr this)
{ memset( this, 0, sizeof(multiple_key_info));}

void multiple_key_info_free( multiple_key_info_ptr this)
{ utf8_free_char_array( this->values, this->count);}

phone_keys phone_keys_from_string( const char* key)
{
   if( !key || !key[0] || key[1])
      return phonekey__invalid;

   switch(*key)
   {
      case '0': return phonekey_0;
      case '1': return phonekey_1;
      case '2': return phonekey_2;
      case '3': return phonekey_3;
      case '4': return phonekey_4;
      case '5': return phonekey_5;
      case '6': return phonekey_6;
      case '7': return phonekey_7;
      case '8': return phonekey_8;
      case '9': return phonekey_9;
      case '*': return phonekey_star;
      case '#': return phonekey_ladder;
      default:  return phonekey__invalid;
   }

   return phonekey__invalid;
}

const char* phone_keys_tag_name( phone_keys key)
{
   switch( key)
   {
      case phonekey_0:     return "CombinationKeyboardKey_0";
      case phonekey_1:     return "CombinationKeyboardKey_1";
      case phonekey_2:     return "CombinationKeyboardKey_2";
      case phonekey_3:     return "CombinationKeyboardKey_3";
      case phonekey_4:     return "CombinationKeyboardKey_4";
      case phonekey_5:     return "CombinationKeyboardKey_5";
      case phonekey_6:     return "CombinationKeyboardKey_6";
      case phonekey_7:     return "CombinationKeyboardKey_7";
      case phonekey_8:     return "CombinationKeyboardKey_8";
      case phonekey_9:     return "CombinationKeyboardKey_9";
      case phonekey_star:  return "CombinationKeyboardKey_*";
      case phonekey_ladder:return "CombinationKeyboardKey_#";

      default:             return NULL;
   }
}

void phone_keyboard_init( phone_keyboard_ptr this)
{
   int i;

   for( i=0; i<phonekey__count; i++)
      multiple_key_info_init( this->keys + i);

   this->state.key= phonekey__invalid;
   this->state.seq= 0;
   this->state.ms = 0;
}

void phone_keyboard_free( phone_keyboard_ptr this)
{
   int i;
   for( i=0; i<phonekey__count; i++)
      multiple_key_info_free( this->keys + i);

   this->state.key= phonekey__invalid;
   this->state.seq= 0;
   this->state.ms = 0;
}

int multiple_key_info_get_next_valid_key(
                                 multiple_key_info_ptr   this,
                                 int                     start_from,
                                 uint16_t                input_type,
                                 BOOL*                   single_option)
{
   int   found = -1;
   int   i     = start_from;

   (*single_option) = FALSE;

   do
   {
      if( -1 != found)
         return found;

      if( is_valid_key( this->values[i], input_type))
         found = i;

      i++;
      if(i == this->count)
         i =  0;

   }  while( i != start_from);

   (*single_option) = TRUE;

   return found;
}

#if(0)
const char* roadmap_lang_get( const char* tag_name)
{
   if( !tag_name || !(*tag_name))
   {
      assert(0);
      return NULL;
   }

   if( !strcmp( tag_name, "CombinationKeyboardKey_0"))
      return "0 ";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_1"))
      return "1.,'?!";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_2"))
      return "2דהוabc";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_3"))
      return "3אבגdef";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_4"))
      return "4מםנןghi";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_5"))
      return "5יכךלjkl";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_6"))
      return "6זחטmno";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_7"))
      return "7רשתpqrs";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_8"))
      return "8צץקtuv";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_9"))
      return "9סעפףwxyz";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_*"))
      return "*=/+-";
   else if( !strcmp( tag_name, "CombinationKeyboardKey_#"))
      return "#";

   assert(0);
   return NULL;
}
#endif   // (0)

void phone_keyboard_load( phone_keyboard_ptr this)
{
   int i;

   for( i=0; i<phonekey__count; i++)
   {
      multiple_key_info_ptr   mki      = this->keys + i;
      const char*             tag_name = phone_keys_tag_name( i);
      const char*             value    = roadmap_lang_get( tag_name);

      if (value[0] == '^')
         value++;
      mki->values = utf8_to_char_array( value, &mki->count);

      assert(mki->values);
      assert(mki->count);
   }
}

void phone_keyboard_state_init( phone_keyboard_state_ptr this)
{
   memset( this, 0, sizeof(phone_keyboard_state));
   this->key = phonekey__invalid;
}

static phone_keyboard ctx;


extern RoadMapConfigDescriptor RoadMapConfigUseNativeKeyboard;

void roadmap_phone_keyboard_init()
{
   phone_keyboard_init( &ctx);
   phone_keyboard_load( &ctx);
   /*
    * Added by AGA.
    * TODO:: Find more appropriate place for this declaration
    */
#ifdef ANDROID   
   if ( roadmap_lang_rtl() )
   {
	   roadmap_config_declare_enumeration
			("user", &RoadMapConfigUseNativeKeyboard, NULL, "no", "yes", NULL);
   }
   else
   {
	   roadmap_config_declare_enumeration
			("user", &RoadMapConfigUseNativeKeyboard, NULL, "yes", "no", NULL);
   }
#else
   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigUseNativeKeyboard, NULL, "yes", "no", NULL);
#endif   
}

void roadmap_phone_keyboard_term()
{  phone_keyboard_free( &ctx);}

const char* roadmap_phone_keyboard_get_multiple_key_value(
               void*       caller,
               const char* key_str,
               uint16_t      input_type,
               BOOL*       value_was_replaced)
{
   phone_keys              key = phone_keys_from_string( key_str);
   uint32_t                now = roadmap_time_get_millis();
   uint32_t                last_ms;
   multiple_key_info_ptr   pi;
   int                     valid_index;
   BOOL                    single_option;

   // If this is not the same caller - restart counting
   if(ctx.caller != caller)
   {
      ctx.caller  = caller;
      phone_keyboard_state_init( &(ctx.state));
   }

   (*value_was_replaced) = FALSE;

   // This should not happen (unless this is not a phone keyboard)
   if( phonekey__invalid == key)
   {
      // Key pressed is not from group (0..9,*,#)
      phone_keyboard_state_init( &(ctx.state));
      return NULL;
   }

   pi          = ctx.keys + key; // Pointer to key-info structure
   last_ms     = ctx.state.ms;
   ctx.state.ms= now;            // Reset 'last ms'

   // Different key?
   if(ctx.state.key != key)
   {
      ctx.state.key = key;
      ctx.state.seq = 0;   // Reset counter
   }
   else
   {
      // Same key

      // Multiple presses?
      uint32_t ms_passsed = now - last_ms;
      if( TIME_BETWEEN_MULTIPLE_KEYBOARD_PRESSES < ms_passsed)
         // Same key, but not a 'multiple presses' case,
         //    Do not replace key value, just reset sequence:
         ctx.state.seq = 0;
      else
      {
         // Multiple presses - change value

         ctx.state.seq++;
         if( pi->count <= ctx.state.seq)
            ctx.state.seq  = 0;

         (*value_was_replaced) = TRUE;
      }
   }

   // Now we have index ('seq') to the appropriate key from the array ('values')
   // However, still need to verify that the selected value 'values[seq]' is a
   // valid choice considering the requested 'input_type'
   //
   //    [For more info on 'input_type' see file roadmap_input_type.h]

   valid_index = multiple_key_info_get_next_valid_key(
                  pi, ctx.state.seq, input_type, &single_option);

   if( -1 == valid_index)
      return NULL;

   if( single_option)
      (*value_was_replaced) = FALSE;

   ctx.state.seq = valid_index;
   return pi->values[valid_index];
}
