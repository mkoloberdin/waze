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

                                               
#ifndef	__HTTPTRANSQUEUE_H__
#define	__HTTPTRANSQUEUE_H__

#define  TRANSACTION_QUEUE_SIZE              (5)

typedef struct tag_wstq_item
{
   const char*       action;        // (/<service_name>/)<ACTION>
   wst_parser_ptr    parsers;       // Array of 1..n data parsers
   int               parsers_count; // Parsers count
   CB_OnWSTCompleted cbOnCompleted; // Callback for transaction completion
   void*             context;       // Caller context
   char*             packet;        // Custom data for the HTTP request

}  wstq_item, *wstq_item_ptr;

void wstq_item_init   ( wstq_item_ptr this);
void wstq_item_release( wstq_item_ptr this);

typedef struct tag_wst_queue
{
   wst_handle  session;
   wstq_item   queue[TRANSACTION_QUEUE_SIZE];
   int         size;

}  wst_queue, *wst_queue_ptr;

void  wstq_init      ( wst_queue_ptr this);
void  wstq_clear     ( wst_queue_ptr this);  // Loose all items

int   wstq_size      ( wst_queue_ptr this);
BOOL  wstq_is_empty  ( wst_queue_ptr this);

// Shallow copy: Caller has to alloc/free packet buffer
BOOL  wstq_enqueue   ( wst_queue_ptr this, wstq_item_ptr item);
BOOL  wstq_dequeue   ( wst_queue_ptr this, wstq_item_ptr item);

#endif	//	__HTTPTRANSQUEUE_H__
