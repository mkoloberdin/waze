
#ifndef __GRID_LAYOUT_DEFS_H__
#define __GRID_LAYOUT_DEFS_H__

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

button_info GRID_BUTTONS[] = 
{
   {
      "1",
      "1",  
      {
         { 0.011905, 0.000000, -1, -1},
         { 0.083333, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "2",
      "2",  
      {
         { 0.115079, 0.000000, -1, -1},
         { 0.186508, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "3",
      "3",  
      {
         { 0.214286, 0.000000, -1, -1},
         { 0.285714, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "4",
      "4",  
      {
         { 0.317460, 0.000000, -1, -1},
         { 0.388889, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "5",
      "5",  
      {
         { 0.416667, 0.000000, -1, -1},
         { 0.488095, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "6",
      "6",  
      {
         { 0.519841, 0.000000, -1, -1},
         { 0.591270, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "7",
      "7",  
      {
         { 0.615079, 0.000000, -1, -1},
         { 0.686508, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "8",
      "8",  
      {
         { 0.710317, 0.000000, -1, -1},
         { 0.781746, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "9",
      "9",  
      {
         { 0.817460, 0.000000, -1, -1},
         { 0.888889, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "0",
      "0",  
      {
         { 0.916667, 0.000000, -1, -1},
         { 0.988095, 0.138075, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "A",
      "Aaז1",
      {
         { 0.011905, 0.171548, -1, -1},
         { 0.130952, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "B",
      "Bbו2",
      {
         { 0.154762, 0.171548, -1, -1},
         { 0.273810, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "C",
      "Ccה3",
      {
         { 0.297619, 0.171548, -1, -1},
         { 0.416667, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "D",
      "Ddד$",
      {
         { 0.440476, 0.171548, -1, -1},
         { 0.559524, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "E",
      "Eeג(",
      {
         { 0.583333, 0.171548, -1, -1},
         { 0.702381, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "F",
      "Ffב)",
      {
         { 0.726190, 0.171548, -1, -1},
         { 0.845238, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "G",
      "Ggא#",
      {
         { 0.869048, 0.171548, -1, -1},
         { 0.988095, 0.309623, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "H",
      "Hhנ4",
      {
         { 0.011905, 0.334728, -1, -1},
         { 0.130952, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "I",
      "Iiמ5",
      {
         { 0.154762, 0.334728, -1, -1},
         { 0.273810, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "J",
      "Jjל6",
      {
         { 0.297619, 0.334728, -1, -1},
         { 0.416667, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "K",
      "Kkכ-",
      {
         { 0.440476, 0.334728, -1, -1},
         { 0.559524, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "L",
      "Llי/",
      {
         { 0.583333, 0.334728, -1, -1},
         { 0.702381, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "M",
      "Mmט:",
      {
         { 0.726190, 0.334728, -1, -1},
         { 0.845238, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "N",
      "Nnח;",
      {
         { 0.869048, 0.334728, -1, -1},
         { 0.988095, 0.472803, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "O",
      "Ooש7",
      {
         { 0.011905, 0.497908, -1, -1},
         { 0.130952, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "P",
      "Ppר8",
      {
         { 0.154762, 0.497908, -1, -1},
         { 0.273810, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Q",
      "Qqק9",
      {
         { 0.297619, 0.497908, -1, -1},
         { 0.416667, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "R",
      "Rrצ&",
      {
         { 0.440476, 0.497908, -1, -1},
         { 0.559524, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "S",
      "Ssפ@",
      {
         { 0.583333, 0.497908, -1, -1},
         { 0.702381, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "T",
      "Ttע\"",
      {
         { 0.726190, 0.497908, -1, -1},
         { 0.845238, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "U",
      "Uuס.",
      {
         { 0.869048, 0.497908, -1, -1},
         { 0.988095, 0.635983, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Shift",
      "kb_shift",
      {
         { 0.011905, 0.661088, -1, -1},
         { 0.130952, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "V",
      "Vvף0",
      {
         { 0.154762, 0.661088, -1, -1},
         { 0.273810, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "W",
      "Wwן_",
      {
         { 0.297619, 0.661088, -1, -1},
         { 0.416667, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "X",
      "Xxם?",
      {
         { 0.440476, 0.661088, -1, -1},
         { 0.559524, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Y",
      "Yyך!",
      {
         { 0.583333, 0.661088, -1, -1},
         { 0.702381, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Z",
      "Zzת,",
      {
         { 0.726190, 0.661088, -1, -1},
         { 0.845238, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE,
      NULL
   },

   {
      "Backspace", // 
      "kb_backspace",
      {
         { 0.869048, 0.661088, -1, -1},
         { 0.988095, 0.799163, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP,
      NULL
   },

   {
      "Grid", // 
      "kb_qwerty",
      {
         { 0.011905, 0.845188, -1, -1},
         { 0.130952, 0.983264, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      ".123",
      "kb_123",
      {
         { 0.154762, 0.845188, -1, -1},
         { 0.416667, 0.983264, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_BITMAP|BUTTON_FLAG_COMMAND,
      NULL
   },

   {
      "Space",
      " ",
      {
         { 0.440476, 0.845188, -1, -1},
         { 0.702381, 0.983264, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_NAME_IS_LABEL,
      NULL
   },

   {
      "Done",
      "Done",
      {
         { 0.726190, 0.845188, -1, -1},
         { 0.988095, 0.983264, -1, -1},
      },
      BUTTON_FLAG_ENABLED|BUTTON_FLAG_VISIBLE|
         BUTTON_FLAG_COMMAND|BUTTON_FLAG_VALUE_IS_LABEL,
      NULL
   }
};

buttons_layout_info GRID_BUTTONS_LAYOUT = 
{
   GRID_BUTTONS,
   sizeof(GRID_BUTTONS)/sizeof(button_info),
   1.054393,
   { -1, -1, -1, -1},
   { -1, -1, -1, -1},
   4,
   0,
   NULL
};

#endif // __GRID_LAYOUT_DEFS_H__
