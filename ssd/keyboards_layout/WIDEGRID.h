
#ifndef __WIDEGRID_LAYOUT_DEFS_H__
#define __WIDEGRID_LAYOUT_DEFS_H__
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

button_info WIDEGRID_BUTTONS[] = 
{
   {
      "A",
      "Aaט1",
      {
         { 0.009259, 0.018405, -1, -1},
         { 0.101852, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "B",
      "Bbח2",
      {
         { 0.120370, 0.018405, -1, -1},
         { 0.212963, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "C",
      "Ccז3",
      {
         { 0.231481, 0.018405, -1, -1},
         { 0.324074, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "D",
      "Ddו$",
      {
         { 0.342593, 0.018405, -1, -1},
         { 0.435185, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "E",
      "Eeה(",
      {
         { 0.453704, 0.018405, -1, -1},
         { 0.546296, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "F",
      "Ffד)",
      {
         { 0.564815, 0.018405, -1, -1},
         { 0.657407, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "G",
      "Ggג#",
      {
         { 0.675926, 0.018405, -1, -1},
         { 0.768519, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "H",
      "Hhב-",
      {
         { 0.787037, 0.018405, -1, -1},
         { 0.879630, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "I",
      "Iiא+",
      {
         { 0.898148, 0.018405, -1, -1},
         { 0.990741, 0.220859, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "J",
      "Jjצ4",
      {
         { 0.009259, 0.257669, -1, -1},
         { 0.101852, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "K",
      "Kkפ5",
      {
         { 0.120370, 0.257669, -1, -1},
         { 0.212963, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "L",
      "Llע6",
      {
         { 0.231481, 0.257669, -1, -1},
         { 0.324074, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "M",
      "Mmס/",
      {
         { 0.342593, 0.257669, -1, -1},
         { 0.435185, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "N",
      "Nnנ:",
      {
         { 0.453704, 0.257669, -1, -1},
         { 0.546296, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "O",
      "Ooמ;",
      {
         { 0.564815, 0.257669, -1, -1},
         { 0.657407, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "P",
      "Ppל&",
      {
         { 0.675926, 0.257669, -1, -1},
         { 0.768519, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Q",
      "Qqכ@",
      {
         { 0.787037, 0.257669, -1, -1},
         { 0.879630, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "R",
      "Rrי\"",
      {
         { 0.898148, 0.257669, -1, -1},
         { 0.990741, 0.460123, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "S",
      "Ssף7",
      {
         { 0.009259, 0.496933, -1, -1},
         { 0.101852, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "T",
      "Ttן8",
      {
         { 0.120370, 0.496933, -1, -1},
         { 0.212963, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "U",
      "Uuם9",
      {
         { 0.231481, 0.496933, -1, -1},
         { 0.324074, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Shift",
      "kb_shift",
      {
         { 0.120370, 0.766871, -1, -1},
         { 0.212963, 0.969325, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "V",
      "Vvך_",
      {
         { 0.342593, 0.496933, -1, -1},
         { 0.435185, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "W",
      "Wwת?",
      {
         { 0.453704, 0.496933, -1, -1},
         { 0.546296, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "X",
      "Xxש.",
      {
         { 0.564815, 0.496933, -1, -1},
         { 0.657407, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Y",
      "Yyר,",
      {
         { 0.675926, 0.496933, -1, -1},
         { 0.768519, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Z",
      "Zzק!",
      {
         { 0.787037, 0.496933, -1, -1},
         { 0.879630, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Backspace", // 
      "kb_backspace",
      {
         { 0.898148, 0.496933, -1, -1},
         { 0.990741, 0.699387, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP,
      NULL
   },

   {
      "Space",
      " ",
      {
         { 0.453704, 0.766871, -1, -1},
         { 0.768519, 0.969325, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_NAME_IS_LABEL,
      NULL
   },

   {
      ".123",
      "kb_123",
      {
         { 0.231481, 0.766871, -1, -1},
         { 0.435185, 0.969325, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "Done", // 
      "Done",
      {
         { 0.787037, 0.766871, -1, -1},
         { 0.990741, 0.969325, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_COMMAND|BUTTON_FLAG_VALUE_IS_LABEL,
      NULL
   },

   {
      "Grid",
      "kb_qwerty",
      {
         { 0.009259, 0.766871, -1, -1},
         { 0.101852, 0.969325, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   }
};

buttons_layout_info WIDEGRID_BUTTONS_LAYOUT = 
{
   WIDEGRID_BUTTONS,
   sizeof(WIDEGRID_BUTTONS)/sizeof(button_info),
   1.987730,
   { -1, -1, -1, -1},
   { -1, -1, -1, -1},
   4,
   0,
   NULL
};

#endif // __WIDEGRID_LAYOUT_DEFS_H__
