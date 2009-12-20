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
#include <stdlib.h>

#include "websvc_trans_defs.h"

void wstq_item_init( wstq_item_ptr this)
{ memset( this, 0, sizeof(wstq_item));}

void wstq_item_release( wstq_item_ptr this)
{
   if( this->packet)
   {
      free( this->packet);
      this->packet = NULL;
   }
   
   wstq_item_init( this);
}

void wstq_init( wst_queue_ptr this)
{ memset( this, 0, sizeof(wst_queue));}

// Loose all items
void wstq_clear( wst_queue_ptr this) 
{
   int i;
   for( i=0; i<this->size; i++)
      wstq_item_release( this->queue + i);
   this->size = 0;
}

int wstq_size( wst_queue_ptr this)
{ return this->size;}

BOOL wstq_is_empty( wst_queue_ptr this)
{ return (0 == this->size);}

BOOL wstq_enqueue( wst_queue_ptr this, wstq_item_ptr item)
{
   if(!this                || 
      !item                || 
      !item->action        || 
      !item->action[0]     ||
      !item->packet        || 
      !item->packet[0]     ||
      !item->cbOnCompleted ||
      !item->parsers       ||
      !item->parsers_count)
   {
      roadmap_log( ROADMAP_ERROR, "wstq_enqueue() - Invalid argument");
      return FALSE;  // Invalid argument
   }

   if( TRANSACTION_QUEUE_SIZE == this->size)
   {
      roadmap_log( ROADMAP_ERROR, "wstq_enqueue() - queue is full");
      return FALSE;  // queue is full
   }
   
   this->queue[this->size++] = *item;
	return TRUE;
}

BOOL wstq_dequeue( wst_queue_ptr this, wstq_item_ptr item)
{
   if( item)
      wstq_item_init( item);

   if(!this || !item)
   {
      roadmap_log( ROADMAP_ERROR, "wstq_dequeue() - Invalid argument");
      return FALSE;  // Invalid argument
   }

   if( !this->size)
   {
      roadmap_log( ROADMAP_DEBUG, "wstq_enqueue() - queue is empty");
      return FALSE;
   }
   
   (*item) = this->queue[0];
   this->size--;
   
   if( 0 == this->size)
      wstq_item_init( this->queue);
   else
   {
      void* dest     = &(this->queue[0]);
      void* src      = &(this->queue[1]);
      int   count    =(this->size * sizeof(wstq_item));
      memmove( dest, src, count);
      wstq_item_init( this->queue + this->size);
   }
   
   return TRUE;
}
