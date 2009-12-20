
/* roadmap_cyclic_array.h
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

#ifndef	__ROADMAP_CYCLIC_ARRAY_H__
#define	__ROADMAP_CYCLIC_ARRAY_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#include "roadmap.h"
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*init_array_item)(void* item);
typedef void(*free_array_item)(void* item);
typedef void(*copy_array_item)(void* copy_dest, void* copy_source);
typedef BOOL(*are_same_items) (void* item_a, void* item_b);

#include "roadmap_cyclic_array_context.h"
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
// struct   cyclic_array_context;
// typedef  cyclic_array_context*   cyclic_array_context_ptr;

void  cyclic_array_init       ( cyclic_array_context_ptr this, 
                                void*                    items_buffer,
                                int                      sizeof_item,
                                int                      max_items_count,
                                const char*              module_name,
                                init_array_item          init_item,
                                free_array_item          free_item,
                                copy_array_item          copy_item,
                                are_same_items           items_are_same);
void  cyclic_array_free       ( cyclic_array_context_ptr this);
BOOL  cyclic_array_push_first ( cyclic_array_context_ptr this, void*   item);   // copy
BOOL  cyclic_array_push_last  ( cyclic_array_context_ptr this, void*   item);   // copy
BOOL  cyclic_array_pop_first  ( cyclic_array_context_ptr this, void*   item);   // copy, free
BOOL  cyclic_array_pop_last   ( cyclic_array_context_ptr this, void*   item);   // copy, free
int   cyclic_array_size       ( cyclic_array_context_ptr this);
BOOL  cyclic_array_is_empty   ( cyclic_array_context_ptr this);
BOOL  cyclic_array_is_full    ( cyclic_array_context_ptr this);
void  cyclic_array_clear      ( cyclic_array_context_ptr this);

void* cyclic_array_get_item   ( cyclic_array_context_ptr this, int     index);  // get pointer
void* cyclic_array_get_same_item   
                              ( cyclic_array_context_ptr this, void*   item);   // compare, get pointer

BOOL  cyclic_array_remove_item( cyclic_array_context_ptr this, int     index);  // free
BOOL  cyclic_array_remove_same_item
                              ( cyclic_array_context_ptr this, void*   item);   // compare, free
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__ROADMAP_CYCLIC_ARRAY_H__
