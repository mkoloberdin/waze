
#ifndef __ROADMAP_INPUT_TYPE_H__
#define __ROADMAP_INPUT_TYPE_H__

typedef enum tag_roadmap_input_type
{
   inputtype_none          = 0,
   inputtype_alphabetic    = (0x01),
   inputtype_numeric       = (0x02),
   inputtype_alphanumeric  = (inputtype_alphabetic|inputtype_numeric),
   inputtype_white_spaces  = (0x04),
   inputtype_alpha_spaces  = (inputtype_alphabetic|inputtype_white_spaces),
   inputtype_punctuation   = (0x08),
   inputtype_simple_text   = (inputtype_alphanumeric|inputtype_white_spaces|inputtype_punctuation),
   inputtype_symbols       = (0x10),
   inputtype_free_text     = (inputtype_simple_text|inputtype_symbols),
   inputtype_binary        = (0xff),

}  roadmap_input_type;

void roadmap_input_type_set_mode( roadmap_input_type mode );

#endif // __ROADMAP_INPUT_TYPE_H__

