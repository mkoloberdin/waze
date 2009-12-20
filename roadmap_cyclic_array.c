
/* roadmap_cyclic_array.c
 *
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap_cyclic_array.h"

#define  INVALID_INDEX        (-1)
#define  SHALLOW_COPY
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_physical_index( cyclic_array_context_ptr this, int logical_index)
{
   int physical_index;
   
   if( (logical_index < 0) || (this->items_count <= logical_index))
      return INVALID_INDEX;
   
   physical_index = this->first_item + logical_index;
   if( this->max_items_count <= physical_index)
      physical_index -= this->max_items_count;
   
   return physical_index;   
}

static void* get_item_by_physical_index( cyclic_array_context_ptr this, int physical_index)
{
   long   pointer_base   = (long)(this->items);
   int    pointer_offset = (physical_index * this->sizeof_item);
   
   if( INVALID_INDEX == physical_index)
      return NULL;
   
   return (void*)(pointer_base + pointer_offset);
}

static void* get_item_by_logical_index( cyclic_array_context_ptr this, int logical_index)
{
   int physical_index = get_physical_index( this, logical_index);
   return get_item_by_physical_index( this, physical_index);
}

static BOOL insert_first_item( cyclic_array_context_ptr this, void* item)
{
   if( !cyclic_array_is_empty(this))
      return FALSE;

   this->first_item  = 0;
   this->items_count = 1;
   this->init_item( this->items);
   this->copy_item( this->items, item);
   
   return TRUE;
}

// ->>>>>
static void shift_one_item_up(cyclic_array_context_ptr   this, 
                              int                        range_begin, 
                              int                        range_end)
{
   int i;
   
   assert(0           <= range_begin);
   assert(range_end   <= this->items_count);
   assert(range_begin <= range_end);
   
   // From: 6
   // To:   9
   
   // From:       [8]   [7]   [6]
   // To:         [9]   [8]   [7]
   
   for( i=range_end; range_begin<i; i--)
   {
      void* item_to   = get_item_by_physical_index( this, i);
      void* item_from = get_item_by_physical_index( this, i-1);

#ifdef   SHALLOW_COPY
      memcpy( item_to, item_from, this->sizeof_item);
#else
      // Deep copy:
      this->copy_item( item_to, item_from); // Alloc new
      this->free_item( item_from);          // Free old
#endif   // SHALLOW_COPY

      this->init_item( item_from);
   }
}

// <<<<<-
static void shift_one_item_down( cyclic_array_context_ptr   this, 
                                 int                        range_begin,
                                 int                        range_end)
{
   int i;
   
   assert(0           <= range_begin);
   assert(range_end   <= this->items_count);
   assert(range_begin <= range_end);
   
   // From: 6
   // To:   9
   
   // From:       [7]   [8]   [9]
   // To:         [6]   [7]   [8]
   
   for( i=range_begin; i<range_end; i++)
   {
      void* item_to   = get_item_by_physical_index( this, i);
      void* item_from = get_item_by_physical_index( this, i+1);
      
#ifdef   SHALLOW_COPY
      memcpy( item_to, item_from, this->sizeof_item);
#else
      // Deep copy:
      this->copy_item( item_to, item_from); // Alloc new
      this->free_item( item_from);          // Free old
#endif   // SHALLOW_COPY

      this->init_item( item_from);
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
void cyclic_array_init( cyclic_array_context_ptr this, 
                        void*                    items_buffer,
                        int                      sizeof_item,
                        int                      max_items_count,
                        const char*              module_name,
                        init_array_item          init_item,
                        free_array_item          free_item,
                        copy_array_item          copy_item,
                        are_same_items           items_are_same)
{
   assert(this);
   assert(items_buffer);
   assert(sizeof_item);
   assert(max_items_count);
   assert(init_item);
   assert(free_item);
   assert(copy_item);
   assert(items_are_same);
   
   this->sizeof_item    = sizeof_item;
   this->max_items_count= max_items_count;
   this->first_item     = INVALID_INDEX;
   this->items_count    = 0;
   this->init_item      = init_item;
   this->free_item      = free_item;
   this->copy_item      = copy_item;
   this->items_are_same = items_are_same;
   this->items          = items_buffer;
   
   if( module_name)
      this->module_name = module_name;
   else
      this->module_name = "";
}
                                
void cyclic_array_free( cyclic_array_context_ptr this)
{
   int logical_index;

   for( logical_index=0; logical_index<cyclic_array_size(this); logical_index++)
   {
      void* item = get_item_by_logical_index( this, logical_index);
      
      this->free_item( item);
      this->init_item( item);
   }
   
   this->first_item  = INVALID_INDEX;
   this->items_count = 0;
}

BOOL cyclic_array_push_first( cyclic_array_context_ptr this, void* item)
{
   void* array_item;
   
   if( cyclic_array_is_full(this))
   {
      roadmap_log( ROADMAP_WARNING, "cyclic_array_push_first(%s) - Array is full", this->module_name);
      return FALSE;
   }
   
   if( cyclic_array_is_empty(this))
      return insert_first_item( this, item);
   
   if(this->first_item)
      this->first_item--;
   else
      this->first_item = (this->max_items_count - 1);
   this->items_count++;

   array_item = get_item_by_physical_index( this, this->first_item);
   this->init_item( array_item);
   this->copy_item( array_item, item);

   return TRUE;
}

BOOL cyclic_array_push_last( cyclic_array_context_ptr this, void* item)
{
   void* array_item;
   
   if( cyclic_array_is_full(this))
   {
      roadmap_log( ROADMAP_WARNING, "cyclic_array_push_last(%s) - Array is full", this->module_name);
      return FALSE;
   }
   
   if( cyclic_array_is_empty(this))
      return insert_first_item( this, item);
   
   array_item = get_item_by_logical_index( this, this->items_count++);
   this->init_item( array_item);
   this->copy_item( array_item, item);

   return TRUE;
}

BOOL cyclic_array_pop_first( cyclic_array_context_ptr this, void* item)
{
   void* array_item;
   
   this->init_item( item);

   if( cyclic_array_is_empty(this))
      return FALSE;

   array_item = get_item_by_physical_index( this, this->first_item);

#ifdef   SHALLOW_COPY
   memcpy( item, array_item, this->sizeof_item);
#else
   // Deep copy:
   this->copy_item( item, array_item);
   this->free_item( array_item);
#endif   // SHALLOW_COPY

   this->init_item( array_item);
   this->items_count--;
   
   if( !this->items_count)
      this->first_item = INVALID_INDEX;
   else
   {
      this->first_item++;
      if(this->first_item ==  this->max_items_count)
         this->first_item = 0;
   }
      
   return TRUE;
}

BOOL cyclic_array_pop_last( cyclic_array_context_ptr this, void* item)
{
   void* array_item;
   
   this->init_item( item);

   if( cyclic_array_is_empty(this))
      return FALSE;

   array_item = get_item_by_logical_index( this, (this->items_count - 1));

#ifdef   SHALLOW_COPY
   memcpy( item, array_item, this->sizeof_item);
#else
   // Deep copy:
   this->copy_item( item, array_item);
   this->free_item( array_item);
#endif   // SHALLOW_COPY

   this->init_item( array_item);
   this->items_count--;
   
   if( !this->items_count)
      this->first_item = INVALID_INDEX;
      
   return TRUE;
}

int cyclic_array_size( cyclic_array_context_ptr this)
{ return this->items_count;}

BOOL cyclic_array_is_empty( cyclic_array_context_ptr this)
{ return (0 == this->items_count);}

BOOL cyclic_array_is_full( cyclic_array_context_ptr this)
{ return (this->max_items_count == this->items_count);}

void cyclic_array_clear( cyclic_array_context_ptr this)
{ cyclic_array_free( this);}

void* cyclic_array_get_item( cyclic_array_context_ptr this, int logical_index)
{ return get_item_by_logical_index( this, logical_index);}

void* cyclic_array_get_same_item( cyclic_array_context_ptr this, void* item)
{
   int logical_index;
   
   assert(item);

   for( logical_index=0; logical_index<cyclic_array_size(this); logical_index++)
   {
      void* current = get_item_by_logical_index( this, logical_index);
      assert(current);
      if( this->items_are_same( item, current))
         return cyclic_array_get_item( this, logical_index);
   }
   
   return NULL;
}

BOOL cyclic_array_remove_item( cyclic_array_context_ptr this, int logical_index)
{
   int   first_item;
   int   last_item;
   int   physical_index;
   void* item_to_remove;
   
   if( cyclic_array_is_empty(this))
      return FALSE;

   if( (logical_index < 0) || (this->items_count <= logical_index))
   {
      assert(0);
      return FALSE;
   }

   first_item     = this->first_item;
   last_item      = get_physical_index( this, (this->items_count - 1));
   physical_index = get_physical_index( this, logical_index);
   item_to_remove = get_item_by_physical_index( this, physical_index);

   // Free item:
   this->free_item( item_to_remove);

   // Shift all other items:
   if( (first_item < last_item) || (first_item <= physical_index))
   {
      shift_one_item_up( this, first_item, physical_index);
      
      this->first_item++;
      if( this->max_items_count == this->first_item)
         this->first_item = 0;
   }
   else
      shift_one_item_down( this, physical_index, last_item);

   this->items_count--;
      
   return TRUE;
}

BOOL  cyclic_array_remove_same_item( cyclic_array_context_ptr this, void* item)
{
   int logical_index;
   
   assert(item);

   for( logical_index=0; logical_index<cyclic_array_size(this); logical_index++)
   {
      void* current = get_item_by_logical_index( this, logical_index);
      assert(current);
      if( this->items_are_same( item, current))
         return cyclic_array_remove_item( this, logical_index);
   }
   
   return FALSE;
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
