
#ifndef IMAGE_LAYOUT_TYPES_DEFINED
#define IMAGE_LAYOUT_TYPES_DEFINED

#include <stdlib.h>

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

