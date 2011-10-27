/* ssd_widget_tab_order.c
 *
 * LICENSE:
 *
 *   Copyright 2009  PazO
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "ssd_widget.h"

#ifdef WIN32_DEBUG
//   #define DEBUG_GUI_TAB_ORDER
#endif   // WIN32_DEBUG

#ifdef   DEBUG_GUI_TAB_ORDER
   #define  LOG_ON_MISSING_POINTER(_side_)         \
         char dbgmsg[128];                         \
         sprintf( dbgmsg,                          \
                  "%s '%s' - Move ssd_widget focus %s - pointer is missing\r\n",    \
                  w->_typeid, w->name, _side_);    \
         OutputDebugStringA( dbgmsg);

#define  LOG_ON_OBJECT_POSITION(_label_,_var_)                    \
   {                                                              \
      sprintf( dbgmsg,                                            \
               "\t%s: '%s' (x:%d,y:%d)(l:%d;r:%d;t:%d;b:%d)\r\n", \
               _label_,                                           \
               _var_->name,                                       \
               _var_->position.x,_var_->position.y,               \
               get_widget_pos_left   (_var_),                     \
               get_widget_pos_right  (_var_),                     \
               get_widget_pos_top    (_var_),                     \
               get_widget_pos_bottom (_var_));                    \
      OutputDebugStringA(dbgmsg);                                 \
   }
#else
   #define  LOG_ON_MISSING_POINTER(_side_)
   #define  LOG_ON_OBJECT_POSITION(_label_,_var_)

#endif   // DEBUG_GUI_TAB_ORDER

// On what side is our neighbour?
typedef enum tag_neighbour_direction
{
   nd_none,

   nd_right,
   nd_left,
   nd_above,
   nd_below,

}  neighbour_direction;

// Phisical relation between the two neighbours - L and R
typedef enum tag_neighbour_location_type
{
   nlt_invalid,

   nlt_parallel,
   /*    +--+........+--+
         |L |        |R |
         +--+........+--+     */

   nlt_contained,
   /*    +--+
         |L |........+--+
         |  |        |R |
         +--+........+--+     */

   nlt_overlapping,
   /*    +--+
         |L |........+--+
         +--+........|R |
                     +--+     */
   nlt_detached
   /*    +--+
         |L |
         +--+
         ................
                     +--+
                     |R |
                     +--+     */

}  neighbour_location_type;

typedef struct tag_relative_position
{
   neighbour_direction     dir;        // Left, right..
   neighbour_location_type type;       // Parallel, detached..
   int                     distance;   // vertical distance + horizontal distance (c2c)

}     relative_position;
void  relative_position_init     ( relative_position* this);
BOOL  relative_position_keep_this( relative_position* this, relative_position* other);

void  relative_position_init( relative_position* this)
{ memset( this, 0, sizeof(relative_position));}

static BOOL is_horizontal_neighbour( neighbour_direction dir)
{
   switch( dir)
   {
      case nd_right: return TRUE;
      case nd_left : return TRUE;
      case nd_above: return FALSE;
      case nd_below: return FALSE;
      default: return FALSE;
   }

   assert(0);
   return FALSE;
}

// Should we keep 'this' or prefare 'other'?
//
// Return:
//    TRUE:    Keep 'this'
//    FALSE:   Select 'other'
BOOL relative_position_keep_this( relative_position* this, relative_position* other)
{
   // We handle 'detached' type differently from all other location types
   BOOL this_detached   = (nlt_detached == this->type);
   BOOL other_detached  = (nlt_detached == other->type);

   assert( this->dir   == other->dir);
   assert( nlt_invalid != this->type);
   assert( nlt_invalid != other->type);

   if( this_detached == other_detached)
      return (this->distance < other->distance);

   if( other_detached)
      return TRUE;   // Other detached, we are overlapping or contained or identical
                     // Prefare current (this)

   return FALSE;
}

#ifdef DEBUG_GUI_TAB_ORDER
const char* get_neighbour_location_type_string( neighbour_location_type type)
{
   switch( type)
   {
      case nlt_parallel:    return "parallel";
      case nlt_contained:   return "contained";
      case nlt_overlapping: return "overlapping";
      case nlt_detached:    return "detached";
   }

   return "<invalid>";
}

const char* get_neighbour_direction_string( neighbour_direction dir)
{
   switch( dir)
   {
      case nd_right: return "right";
      case nd_left : return "left";
      case nd_above: return "above";
      case nd_below: return "below";
   }

   return "<invalid>";
}
#endif   // DEBUG_GUI_TAB_ORDER

static void widget_get_next_focus( SsdWidget this, SsdWidget* pp)
{
   SsdWidget p = this;

   if( (*pp) && (*pp)->default_widget)
      return;

   while( p)
   {
      if( p->tab_stop)
      {
         if( p->default_widget)
         {
            (*pp) = p;
            return;
         }

         if( !(*pp))
            (*pp) = p;
      }

      widget_get_next_focus( p->children, pp);

      p = p->next;
   }
}

SsdWidget ssd_widget_get_next_focus( SsdWidget w)
{
   SsdWidget found = NULL;

   widget_get_next_focus( w, &found);

   return found;
}

roadmap_input_type ssd_widget_get_input_type( SsdWidget this)
{
   roadmap_input_type t = inputtype_none;

   if( this->get_input_type)
      t = this->get_input_type( this);

   if( inputtype_none == t)
   {
      SsdWidget next = this->children;
      while( (inputtype_none == t) && next)
      {
         t     = ssd_widget_get_input_type( next);
         next  = next->next;
      }
   }

   return t;
}

BOOL ssd_widget_set_focus( SsdWidget w)
{

   if( w->tab_stop)
   {
      roadmap_input_type input_type;

      w->in_focus = TRUE;

      input_type = ssd_widget_get_input_type( w );
      if ( input_type != inputtype_none )
      {
		  roadmap_input_type_set_mode( input_type );
      }
   }

   w->background_focus = FALSE;

   return w->in_focus;
}

void ssd_widget_loose_focus( SsdWidget w)
{
   w->in_focus          = FALSE;
   w->background_focus  = FALSE;
}

static SsdWidget ssd_widget_focus_backward( SsdWidget w)
{

   if( !w->prev_tabstop || (w->prev_tabstop == w))
      return w;   // Only one tab-stop in w dialog. Nothing changed.

   return w->prev_tabstop;
}

static SsdWidget ssd_widget_focus_forward(SsdWidget w)
{
   if(!w->next_tabstop || (w->next_tabstop == w))
      return w;   // Only one tab-stop in w dialog. Nothing changed.

   return w->next_tabstop;
}

static SsdWidget ssd_widget_focus_left( SsdWidget w) {

   if( !w->tabstop_left)
   {
      LOG_ON_MISSING_POINTER("LEFT")
      return ssd_widget_focus_backward( w);
   }

   return w->tabstop_left;
}

static SsdWidget ssd_widget_focus_right( SsdWidget w) {

   if( !w->tabstop_right)
   {
      LOG_ON_MISSING_POINTER("RIGHT")
      return ssd_widget_focus_forward( w);
   }

   return w->tabstop_right;
}

static SsdWidget ssd_widget_focus_up( SsdWidget w) {

   if( !w->tabstop_above)
   {
      LOG_ON_MISSING_POINTER("UP")
      return ssd_widget_focus_backward( w);
   }

   return w->tabstop_above;
}

static SsdWidget ssd_widget_focus_down( SsdWidget w)
{
   if( !w->tabstop_below)
   {
      LOG_ON_MISSING_POINTER("DOWN")
      return ssd_widget_focus_forward( w);
   }

   return w->tabstop_below;
}

SsdWidget ssd_widget_move_focus (SsdWidget w, SSD_FOCUS_DIRECTION direction) {
   SsdWidget next_w;
   if (!w) return NULL;

   switch (direction) {
      case FOCUS_BACK:     next_w = ssd_widget_focus_backward(w); break;
      case FOCUS_FORWARD:  next_w = ssd_widget_focus_forward(w); break;
      case FOCUS_UP:       next_w = ssd_widget_focus_up(w); break;
      case FOCUS_DOWN:     next_w = ssd_widget_focus_down(w); break;
      case FOCUS_LEFT:     next_w = ssd_widget_focus_left(w); break;
      case FOCUS_RIGHT:    next_w = ssd_widget_focus_right(w); break;
      default:             next_w = w; break;
   }

   if( w == next_w)
      return w;   // Only one tab-stop in w dialog. Nothing changed.

   ssd_widget_loose_focus  (w);
   ssd_widget_set_focus    (next_w);

   return next_w;
}

static int get_widget_width( SsdWidget w)
{
   if( 0 < w->size.width)
      return w->size.width;

   if( 0 < w->cached_size.width)
      return w->cached_size.width;

   assert(0);  // Maybe this method was called on a wrong time.
               // Maybe before widget where assigned with sizes.
               // Consider calling 'ssd_dialog_invalidate_tab_order()'
               // instead of 'ssd_dialog_resort_tab_order();'
   return 0;
}

static int get_widget_height( SsdWidget w)
{
   if( 0 < w->size.height)
      return w->size.height;

   if( 0 < w->cached_size.height)
      return w->cached_size.height;

   assert(0);  // Maybe this method was called on a wrong time.
               // Maybe before widget where assigned with sizes.
               // Consider calling 'ssd_dialog_invalidate_tab_order()'
               // instead of 'ssd_dialog_resort_tab_order();'
   return 0;
}

static BOOL valid_gui_element( SsdWidget w)
{
#ifdef DEBUG_GUI_TAB_ORDER
   char dbgmsg[512];

   if(   ((0 < w->size.width)        && (0 < w->size.height))
               ||
         ((0 < w->cached_size.width) && (0 < w->cached_size.height)))
      return TRUE;

   sprintf( dbgmsg, "Invalid GUI element: '%s' (%s)\r\n", w->name, w->_typeid);
   OutputDebugStringA( dbgmsg);


   return FALSE;
#else
   return (((0 < w->size.width)        && (0 < w->size.height))
               ||
           ((0 < w->cached_size.width) && (0 < w->cached_size.height)));
#endif   // DEBUG_GUI_TAB_ORDER
}

static int get_widget_hcenter( SsdWidget w)
{
   int   w_width     = get_widget_width( w);
   int   half_width  = w_width/2;

   return w->position.x + half_width;
}

static int get_widget_vcenter( SsdWidget w)
{
   int   w_height     = get_widget_height( w);
   int   half_height  = w_height/2;

   return w->position.y + half_height;
}

static int get_widget_hdistance( SsdWidget a, SsdWidget b)
{
   int ha = get_widget_hcenter( a);
   int hb = get_widget_hcenter( b);
   if( ha < hb)
      return hb-ha;
   return ha-hb;
}

static int get_widget_vdistance( SsdWidget a, SsdWidget b)
{
   int va = get_widget_vcenter( a);
   int vb = get_widget_vcenter( b);
   if( va < vb)
      return vb-va;
   return va-vb;
}

static int get_widget_distance( SsdWidget a, SsdWidget b)
{ return get_widget_hdistance( a, b) + get_widget_vdistance( a, b);}

static int get_widget_pos_left( SsdWidget w)
{ return w->position.x;}

static int get_widget_pos_right( SsdWidget w)
{ return w->position.x + get_widget_width( w);}

static int get_widget_pos_top( SsdWidget w)
{ return w->position.y;}

static int get_widget_pos_bottom( SsdWidget w)
{ return w->position.y + get_widget_height( w);}

static void get_widget_oposite_range( SsdWidget w, int* from, int* to, BOOL horizontal)
{
   if( horizontal)
   {
      (*from)  = get_widget_pos_top    ( w);
      (*to)    = get_widget_pos_bottom ( w);
   }
   else
   {
      (*from)  = get_widget_pos_left   ( w);
      (*to)    = get_widget_pos_right  ( w);
   }
}

neighbour_location_type get_neighbour_location_type( SsdWidget this, SsdWidget other, neighbour_direction dir)
{
   BOOL  horizontal = is_horizontal_neighbour( dir);
   int   this_from;
   int   this_to;
   int   other_from;
   int   other_to;

   get_widget_oposite_range( this, &this_from, &this_to, horizontal);
   get_widget_oposite_range( other,&other_from,&other_to,horizontal);

   assert( this_from < this_to);
   assert( other_from< other_to);

   if( (this_from==other_from) && (this_to==other_to))
      return nlt_parallel;

   if( (this_to <= other_from) || (other_to <= this_from))
      return nlt_detached;

   if(((this_from <= other_from)&& (other_to<=this_to))
      ||
      ((other_from<= this_from) && (this_to <=other_to)))
      return nlt_contained;

   return nlt_overlapping;
}

// Method:  get_relative_position()
//
// Abstract:
//    This method will compare all area of 'this' and 'other'
//    and will decide:
//    1. On which side of 'this' is 'other' located
//    2. What is the location type (parallel, overlapping...)
//    2. How far is it (h-distance + v-distance)
static relative_position get_relative_position(
         SsdWidget this, SsdWidget other)
{
   relative_position    pos;

   neighbour_direction  hdir        = nd_none;
   neighbour_direction  vdir        = nd_none;
   BOOL                 horizontal  = FALSE;

   // This
   //    Rect corners
   int this_pos_left    = get_widget_pos_left   (this);
   int this_pos_right   = get_widget_pos_right  (this);
   int this_pos_top     = get_widget_pos_top    (this);
   int this_pos_bottom  = get_widget_pos_bottom (this);
   //    Center point
   int this_h_center    = get_widget_hcenter    (this);
   int this_v_center    = get_widget_vcenter    (this);

   // Other
   //    Rect corners
   int other_pos_left   = get_widget_pos_left   (other);
   int other_pos_right  = get_widget_pos_right  (other);
   int other_pos_top    = get_widget_pos_top    (other);
   int other_pos_bottom = get_widget_pos_bottom (other);
   //    Center point
   int other_h_center   = get_widget_hcenter    (other);
   int other_v_center   = get_widget_vcenter    (other);

   // Distance:
   int hdistance        = abs(this_h_center-other_h_center);
   int vdistance        = abs(this_v_center-other_v_center);

   assert( 0 <= hdistance);
   assert( 0 <= vdistance);

   relative_position_init( &pos);

   if( this == other)
   {
      assert(0);
      return pos;
   }

   if( other_pos_right <= this_pos_left)
      hdir = nd_left;
   else if ( this_pos_right <= other_pos_left)
      hdir = nd_right;

   if( this_pos_bottom <= other_pos_top)
      vdir = nd_below;
   else if ( other_pos_bottom <= this_pos_top)
      vdir = nd_above;

   if ((this->position.x == 0) && (this->position.y == 0) && (other->position.x == 0) && (other->position.y == 0)){
      pos.type    = nd_none;
      pos.distance= 0;
      return pos;
   }


//  assert( (nd_none != hdir) || (nd_none != vdir));

   if( nd_none != hdir)
   {
      if( (nd_none != vdir) && (hdistance < vdistance))
         pos.dir = vdir;   // Bigger distance
      else
      {
         horizontal  = TRUE;
         pos.dir     = hdir;
      }
   }
   else
      pos.dir = vdir;

   pos.type    = get_neighbour_location_type( this, other, pos.dir);
   pos.distance= (hdistance + vdistance);

   return pos;
}

static SsdWidget* get_dir_pointer( SsdWidget w, neighbour_direction dir)
{
   switch( dir)
   {
      case nd_right: return &(w->tabstop_right);
      case nd_left:  return &(w->tabstop_left);
      case nd_above: return &(w->tabstop_above);
      case nd_below: return &(w->tabstop_below);
      default: assert(0); return NULL;
   }
}

static SsdWidget* get_oposite_dir_pointer( SsdWidget w, neighbour_direction dir)
{
   switch( dir)
   {
      case nd_right: return &(w->tabstop_left);
      case nd_left:  return &(w->tabstop_right);
      case nd_above: return &(w->tabstop_below);
      case nd_below: return &(w->tabstop_above);
      default: assert(0); return NULL;
   }
}

static BOOL select_nearest_neighbour(
                  SsdWidget            this,
                  SsdWidget            new,
                  relative_position    pos_new)
{
#ifdef DEBUG_GUI_TAB_ORDER
   char        dbgmsg[512];
#endif   // DEBUG_GUI_TAB_ORDER

   SsdWidget*  p_my_dir    = get_dir_pointer( this, pos_new.dir);
   SsdWidget   my_dir      = (*p_my_dir);

   relative_position pos_cur;

   if( my_dir == new)
      return FALSE;

   assert(this != new);
   assert(this != my_dir);

#ifdef DEBUG_GUI_TAB_ORDER
   sprintf( dbgmsg, "Direction: '%s'\r\n",
            get_neighbour_direction_string( pos_new.dir));
   OutputDebugStringA( dbgmsg);
   LOG_ON_OBJECT_POSITION("Mua",this)
   if( my_dir)
      LOG_ON_OBJECT_POSITION("Cur",my_dir)
   else
      OutputDebugStringA("\tCur: NONE\r\n");
   LOG_ON_OBJECT_POSITION("New",new)
#endif   // DEBUG_GUI_TAB_ORDER

   if( my_dir)
   {
      pos_cur = get_relative_position( this, my_dir);

      if( relative_position_keep_this( &pos_cur, &pos_new))
      {
#ifdef DEBUG_GUI_TAB_ORDER
         OutputDebugStringA("\tStay with current\r\n");
#endif   // DEBUG_GUI_TAB_ORDER
         return FALSE;  // Current position is better
      }

      // Else,
      //    Selecting [New], even if distance is more far
   }

#ifdef DEBUG_GUI_TAB_ORDER
   OutputDebugStringA("\t!!! Selected new !!!\r\n");
#endif   // DEBUG_GUI_TAB_ORDER

   (*p_my_dir) = new;

   return TRUE;
}

static void consider_a_neighbour( SsdWidget this, SsdWidget other)
{
   relative_position pos = get_relative_position( this, other);

   if( nd_none == pos.dir)
   {
      return;
   }

#ifdef DEBUG_GUI_TAB_ORDER
   if( select_nearest_neighbour( this, other, pos))
   {
      char dbgstr[112];
      neighbour_location_type nt = get_neighbour_location_type( this, other, pos.dir);

      sprintf( dbgstr, "\tNeighbour type: %s\r\n\r\n", get_neighbour_location_type_string( nt));
      OutputDebugStringA( dbgstr);
   }
#else
   select_nearest_neighbour( this, other, pos);
#endif   // DEBUG_GUI_TAB_ORDER
}

// Helper for 'ssd_widget_sort_gui_tab_order__fix_orphan_pointers()' below
void ssd_widget_sort_gui_tab_order__fix_corners( SsdWidget w, neighbour_direction dir)
{
   SsdWidget*  my_dir_ptr  =  get_dir_pointer( w, dir);
   SsdWidget   my_dir      = (*my_dir_ptr);
   SsdWidget   my_oposite  = *get_oposite_dir_pointer( w, dir);

   SsdWidget   p           = w;
   SsdWidget   last        = w;

   SsdWidget   far_widget  = NULL;  // This will be used for cases of loops
   int         far_distance= 0;

   SsdWidget   loop_watch  = w;     // Loop watch-dog
   BOOL        inc_watch   = FALSE; // Loop watch-dog
   int         distance;

   if( my_dir || !my_oposite || !valid_gui_element(w))
      return;

   while( my_oposite)
   {
      last  = p;
      p     = my_oposite;

      if( p == w)
      {
         p = last;
         break;     // Looped back to 'w'
      }

      // Keep track on widget, which is the most-far (farest?) from us:
      // The 'far_widget' will be used as a match for cases of internal loop,
      // discovered by the watchdog:
      distance = get_widget_distance( w, p);
      if( far_distance < distance)
      {
         far_distance= distance;
         far_widget  = p;
      }

      // Watch-dog:
      if( loop_watch == p)
      {
         if( !far_widget || (far_widget==w))
         {
            assert(0);
            return;     // Internal closed loop
         }

         p = far_widget;
         break;
      }

      // Increment watchdog
      if( inc_watch)
         loop_watch = *get_oposite_dir_pointer( loop_watch, dir);
      inc_watch = !inc_watch;

      my_oposite = *get_oposite_dir_pointer( p, dir);
   }

   assert(p);
   assert(p!=w);

   // Set 'next' pointer:
   (*my_dir_ptr) = p;
}

// After left/right/above/below pointers were set, some corner widgets will not
// have anything to point at.
// Why?  For example - the top-left cornered widget does not have anything above it
//       or to the left of it.
// The next method will search for these 'orphan' pointers, and set them to point
// to other widgets:
void ssd_widget_sort_gui_tab_order__fix_orphan_pointers( SsdWidget first)
{
   SsdWidget p = first;

   do
   {
      ssd_widget_sort_gui_tab_order__fix_corners( p, nd_right);
      ssd_widget_sort_gui_tab_order__fix_corners( p, nd_left);
      ssd_widget_sort_gui_tab_order__fix_corners( p, nd_above);
      ssd_widget_sort_gui_tab_order__fix_corners( p, nd_below);

      p = p->next_tabstop;

   }  while(p && (p != first));
}

SsdWidget ssd_widget_sort_gui_tab_order( SsdWidget first)
{
   SsdWidget new_focus  = NULL;
   SsdWidget w          = NULL;

#ifdef DEBUG_GUI_TAB_ORDER
   char dbgmsg[0xFF];
#endif   // DEBUG_GUI_TAB_ORDER

   if( !first)
      return NULL;

   if( !first->next_tabstop)
   {
      if( valid_gui_element( first))
         return first;
      return NULL;
   }

   w = first;
   do
   {
      if( valid_gui_element( w))
      {
         SsdWidget next = w->next_tabstop;

         if(!new_focus)
             new_focus = w;
         if (next){
            do
            {
#if 0
               // Maybe neighbours?
               if( valid_gui_element( next))
                  consider_a_neighbour(w,next);
#endif
               next = next->next_tabstop;

            }  while (next && ( next != w));
         }
#ifdef DEBUG_GUI_TAB_ORDER
         sprintf( dbgmsg,
                  "Mapping of '%s':\r\n"
                  "   '%s'\r\n"
                  "'%s' [%s] '%s'\r\n"
                  "   '%s'\r\n\r\n",
                   w->name,
                  (w->tabstop_above? w->tabstop_above->name: "NULL"),
                  (w->tabstop_left?  w->tabstop_left->name: "NULL"),
                   w->name,
                  (w->tabstop_right? w->tabstop_right->name: "NULL"),
                  (w->tabstop_below? w->tabstop_below->name: "NULL"));
         OutputDebugStringA(dbgmsg);
#endif   // DEBUG_GUI_TAB_ORDER
      }


      w = w->next_tabstop;

   }  while( w && (w != first));

   assert(new_focus);

   // See remarks above the next method for more info:
   ssd_widget_sort_gui_tab_order__fix_orphan_pointers( first);

   return new_focus;
}

void ssd_widget_reset_tab_order_recursive( SsdWidget w)

{
   SsdWidget next;

   w->prev_tabstop   = NULL;
   w->next_tabstop   = NULL;
   w->tabstop_left   = NULL;
   w->tabstop_right  = NULL;
   w->tabstop_above  = NULL;
   w->tabstop_below  = NULL;
#ifndef TOUCH_SCREEN  // Avi
   w->in_focus       = FALSE;
#endif
   next = w->children;
   while( next)
   {
      ssd_widget_reset_tab_order_recursive( next);
      next = next->next;
   }

   if( (NULL == w->parent) && w->next)
      ssd_widget_reset_tab_order_recursive( w->next);
}

void ssd_widget_reset_tab_order( SsdWidget head)
{
   if( head)
      ssd_widget_reset_tab_order_recursive( head);
}

void ssd_widget_sort_tab_order_recursive(
      void*       parent_dialog,
      SsdWidget   currenlt_position,
      SsdWidget*  previous_tabstop,
      SsdWidget*  default_widget,
      SsdWidget*  first_tabstop,
      SsdWidget*  last_tabstop)
{
   SsdWidget next;

   assert(parent_dialog);
   currenlt_position->parent_dialog = parent_dialog;

   if( (currenlt_position->tab_stop) && !(SSD_WIDGET_HIDE & currenlt_position->flags))
   {
      // Set first and last:
      if( NULL == (*first_tabstop))
         (*first_tabstop)  = currenlt_position;
      (*last_tabstop) = currenlt_position;

      // Tie-up the previous and the current:
      if( NULL == (*previous_tabstop))
         (*previous_tabstop)= currenlt_position;
      else
      {
         (*previous_tabstop)->next_tabstop= currenlt_position;
         currenlt_position->prev_tabstop   = (*previous_tabstop);
         (*previous_tabstop)              = currenlt_position;
      }
   }

   if( currenlt_position->default_widget)
   {
      // assert( NULL == (*default_widget)); // More then one default-widget in a construct?
      (*default_widget) = currenlt_position;
   }

   next = currenlt_position->children;
   while( next)
   {
      ssd_widget_sort_tab_order_recursive(
            parent_dialog,
            next,
            previous_tabstop,
            default_widget,
            first_tabstop,
            last_tabstop);

      next = next->next;
   }

   // Only for top-level widgets we need to check brothers.
   // Other widgets brothers were already covered by the children-enumaration of their parent
   if( (NULL == currenlt_position->parent) && currenlt_position->next)
      ssd_widget_sort_tab_order_recursive(
            parent_dialog,
            currenlt_position->next,
            previous_tabstop,
            default_widget,
            first_tabstop,
            last_tabstop);
}

void switch_widgets_tab_order( SsdWidget a, SsdWidget b)
{
   struct ssd_widget a_data_with_b_pointers = *a;
   struct ssd_widget b_data_with_a_pointers = *b;

   a_data_with_b_pointers.prev_tabstop = b->prev_tabstop;
   a_data_with_b_pointers.next_tabstop = b->next_tabstop;
   b_data_with_a_pointers.prev_tabstop = a->prev_tabstop;
   b_data_with_a_pointers.next_tabstop = a->next_tabstop;

   *a = b_data_with_a_pointers;
   *b = a_data_with_b_pointers;
}

void fix_widget_tab_order_sequence(SsdWidget widget)
{
   SsdWidget w = widget->next_tabstop;

   while( w != widget)
   {
      if( w != w->next_tabstop)
      {
         int   my_seq      = w->tab_sequence;
         int   my_next_seq = w->next_tabstop->tab_sequence;
         int   w_seq       = w->tab_sequence;
         BOOL  wrong_order = FALSE;

         if( my_seq < my_next_seq)
         {
            if( (w_seq < my_next_seq) && (my_seq < w_seq))
               wrong_order = TRUE;
         }
         else
            if( w_seq < my_next_seq)
               wrong_order = TRUE;

         if( wrong_order)
         {
            assert(0 && "fix_tab_order_sequence() - unexpected order");
            switch_widgets_tab_order( w->next_tabstop, w);
         }
      }

      w = w->next_tabstop;
   }
}

void fix_tab_order_sequence( SsdWidget widget)
{
   SsdWidget w = widget->next_tabstop;

   while( w != widget)
   {
      fix_widget_tab_order_sequence( w);
      w = w->next_tabstop;
   }
}

SsdWidget ssd_widget_sort_tab_order(void* parent_dialog, SsdWidget head)
{
   SsdWidget   previous_tabstop  = NULL;
   SsdWidget   default_widget    = NULL;
   SsdWidget   first_tabstop     = NULL;
   SsdWidget   last_tabstop      = NULL;

   if( !head)
      return NULL;

   // Sort internal items:
   ssd_widget_sort_tab_order_recursive(parent_dialog,
         head,
         &previous_tabstop,
         &default_widget,
         &first_tabstop,
         &last_tabstop);

#ifdef _DEBUG
   // Check against bug in the code:
   if( (last_tabstop && !first_tabstop) || (!last_tabstop && first_tabstop))
   {
      assert( 0);
      return NULL;
   }
#endif   // _DEBUG

   // Found any?
   if(!last_tabstop)
      return NULL;   // No tab-stops found

#ifdef TOUCH_SCREEN
   if( first_tabstop != last_tabstop)
   {
      // Make 'last' and 'first' point at each other:
      last_tabstop->next_tabstop = first_tabstop;
      first_tabstop->prev_tabstop= last_tabstop;

      fix_tab_order_sequence( first_tabstop);
   }
#endif
   if( !default_widget)
	   default_widget = first_tabstop;

#ifndef TOUCH_SCREEN
   // Set first widget with focus:
   if(default_widget)
      ssd_widget_set_focus(default_widget);
   else
      ssd_widget_set_focus(first_tabstop);
#endif

   return default_widget;
}

