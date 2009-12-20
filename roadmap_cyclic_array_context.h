
/* roadmap_cyclic_array_context.h
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

#ifndef	__ROADMAP_CYCLIC_ARRAY_CONTEXT_H__
#define	__ROADMAP_CYCLIC_ARRAY_CONTEXT_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tag_cyclic_array_context
{
   int               sizeof_item;
   int               max_items_count;
   int               first_item;
   int               items_count;
   const char*       module_name;
   init_array_item   init_item;
   free_array_item   free_item;
   copy_array_item   copy_item;
   are_same_items    items_are_same;
   void*             items;

}  cyclic_array_context, *cyclic_array_context_ptr;
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__ROADMAP_CYCLIC_ARRAY_CONTEXT_H__
