
#ifndef __QWERTY_LAYOUT_DEFS_H__
#define __QWERTY_LAYOUT_DEFS_H__
#include <stdlib.h>

// Automatically generated file by 'AutoLayout.exe' application
#ifndef IMAGE_LAYOUT_TYPES_DEFINED
#define IMAGE_LAYOUT_TYPES_DEFINED

#define BUTTON_FLAG_ENABLED         (0x01)
#define BUTTON_FLAG_VISIBLE         (0x02)
#define BUTTON_FLAG_BITMAP          (0x04)
#define BUTTON_FLAG_COMMAND         (0x08)
#define BUTTON_FLAG_NAME_IS_LABEL   (0x10)
#define BUTTON_FLAG_VALUE_IS_LABEL  (0x20)
#define BUTTON_FLAG_SELECTED        (0x40)

typedef struct tag_rect_corner_info
{
   const double   relative_x;
   const double   relative_y;

   int            cached_x;
   int            cached_y;

}  rect_corner_info;

typedef enum tag_rect_corner_type
{
   top_left,
   bottom_right,

   rect_corners_count

}  rect_corner_type;

typedef struct tag_button_info
{
   const char*       name;
   const char*       values;
   rect_corner_info  rect_corner_infos[ rect_corners_count];
   unsigned long     flags;
   void*             item_data;

}  button_info;

typedef struct tag_screen_area
{
   int   x;
   int   y;
   int   width;
   int   height;

}  screen_area;

typedef struct tag_buttons_layout_info
{
   button_info*   buttons;
   const int      buttons_count;
   const double   image_relation; // (width/height)
   screen_area    cache_ref;
   screen_area    area_used;
   const int      max_values_count;
   int            value_index;
   void*          item_data;

}  buttons_layout_info;

#endif // IMAGE_LAYOUT_TYPES_DEFINED

button_info QWERTY_BUTTONS[] = 
{
   {
      "1",
      "1",  
      {
         { 0.009091, 0.011194, -1, -1},
         { 0.090909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "2",
      "2",  
      {
         { 0.109091, 0.011194, -1, -1},
         { 0.190909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "3",
      "3",  
      {
         { 0.209091, 0.011194, -1, -1},
         { 0.290909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "4",
      "4",  
      {
         { 0.309091, 0.011194, -1, -1},
         { 0.390909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "5",
      "5",  
      {
         { 0.409091, 0.011194, -1, -1},
         { 0.490909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "6",
      "6",  
      {
         { 0.509091, 0.011194, -1, -1},
         { 0.590909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "7",
      "7",  
      {
         { 0.609091, 0.011194, -1, -1},
         { 0.690909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "8",
      "8",  
      {
         { 0.709091, 0.011194, -1, -1},
         { 0.790909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "9",
      "9",  
      {
         { 0.809091, 0.011194, -1, -1},
         { 0.890909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "0",
      "0",  
      {
         { 0.909091, 0.011194, -1, -1},
         { 0.990909, 0.182836, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Q",
      "Qqך1",
      {
         { 0.009091, 0.205224, -1, -1},
         { 0.090909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "W",
      "Wwף2",
      {
         { 0.109091, 0.205224, -1, -1},
         { 0.190909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "E",
      "Eeק3",
      {
         { 0.209091, 0.205224, -1, -1},
         { 0.290909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "R",
      "Rrר4",
      {
         { 0.309091, 0.205224, -1, -1},
         { 0.390909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "T",
      "Ttא5",
      {
         { 0.409091, 0.205224, -1, -1},
         { 0.490909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Y",
      "Yyט6",
      {
         { 0.509091, 0.205224, -1, -1},
         { 0.590909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "U",
      "Uuו7",
      {
         { 0.609091, 0.205224, -1, -1},
         { 0.690909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "I",
      "Iiן8",
      {
         { 0.709091, 0.205224, -1, -1},
         { 0.790909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "O",
      "Ooם9",
      {
         { 0.809091, 0.205224, -1, -1},
         { 0.890909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "P",
      "Ppפ0",
      {
         { 0.909091, 0.205224, -1, -1},
         { 0.990909, 0.376866, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "A",
      "Aaש-",
      {
         { 0.060606, 0.399254, -1, -1},
         { 0.142424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "S",
      "Ssד/",
      {
         { 0.160606, 0.399254, -1, -1},
         { 0.242424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "D",
      "Ddג:",
      {
         { 0.260606, 0.399254, -1, -1},
         { 0.342424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "F",
      "Ffכ;",
      {
         { 0.360606, 0.399254, -1, -1},
         { 0.442424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "G",
      "Ggע(",
      {
         { 0.460606, 0.399254, -1, -1},
         { 0.542424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "H",
      "Hhי)",
      {
         { 0.560606, 0.399254, -1, -1},
         { 0.642424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "J",
      "Jjח$",
      {
         { 0.660606, 0.399254, -1, -1},
         { 0.742424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "K",
      "Kkל&",
      {
         { 0.760606, 0.399254, -1, -1},
         { 0.842424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "L",
      "Llת@",
      {
         { 0.860606, 0.399254, -1, -1},
         { 0.942424, 0.570896, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Z",
      "Zzז.",
      {
         { 0.109091, 0.593284, -1, -1},
         { 0.190909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "X",
      "Xxס#",
      {
         { 0.209091, 0.593284, -1, -1},
         { 0.290909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Shift",
      "kb_shift",
      {
         { 0.109091, 0.813433, -1, -1},
         { 0.190909, 0.985075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "C",
      "Ccב+",
      {
         { 0.309091, 0.593284, -1, -1},
         { 0.390909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "V",
      "Vvה,",
      {
         { 0.409091, 0.593284, -1, -1},
         { 0.490909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "B",
      "Bbנ?",
      {
         { 0.509091, 0.593284, -1, -1},
         { 0.590909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "N",
      "Nnמ!",
      {
         { 0.609091, 0.593284, -1, -1},
         { 0.690909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "M",
      "Mmצ_",
      {
         { 0.709091, 0.593284, -1, -1},
         { 0.790909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Backspace", // 
      "kb_backspace",
      {
         { 0.860606, 0.593284, -1, -1},
         { 0.993939, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP,
      NULL
   },

   {
      "Space",
      " ",
      {
         { 0.409091, 0.813433, -1, -1},
         { 0.690909, 0.985075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_NAME_IS_LABEL,
      NULL
   },

   {
      ".123",
      "kb_123",
      {
         { 0.209091, 0.813433, -1, -1},
         { 0.390909, 0.985075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "Done",
      "Done",
      {
         { 0.709091, 0.813433, -1, -1},
         { 0.990909, 0.985075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_COMMAND|BUTTON_FLAG_VALUE_IS_LABEL,
      NULL
   },

   {
      "Grid",
      "kb_grid",
      {
         { 0.009091, 0.813433, -1, -1},
         { 0.090909, 0.985075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "Hebrew",
      "",  
      {
         { 0.009091, 0.593284, -1, -1},
         { 0.090909, 0.764925, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   }
};

buttons_layout_info QWERTY_BUTTONS_LAYOUT = 
{
   QWERTY_BUTTONS,
   sizeof(QWERTY_BUTTONS)/sizeof(button_info),
   1.231343,
   { -1, -1, -1, -1},
   { -1, -1, -1, -1},
   4,
   0,
   NULL
};

#endif // __QWERTY_LAYOUT_DEFS_H__
