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


#ifndef   __SOCKET_ASYNC_RECEIVE_H__
#define   __SOCKET_ASYNC_RECEIVE_H__

/* socket_async_receive

   Abstract:   Wrapper for asynchronious socket-read operations
   
   More:       roadmap layer for asynchronious socket-read operations wrapps
               the socket inside an abstract-union structure 'RoadMapIO'.
               Before any read operation can be done there is a need to call
               'roadmap_main_set_input()' method once. When task is done and
               no more reads are needed the caller must call 
               'roadmap_main_remove_input()'.
               The layer here wrapps these activities and expose two methods:
               o  socket_async_receive()     Called once for each aync-read 
               o  socket_async_receive_end() Called once when no more reads 
                                             are needed
*/

#include "../roadmap.h"
#include "../roadmap_net.h"

// A-syncronious receive:
typedef void(*CB_OnDataReceive)( void* data, int size, void* context);

BOOL socket_async_receive( RoadMapSocket     s, 
                           void*             data,
                           int               size,
                           CB_OnDataReceive  cbOnDataReceive,
                           void*             context);

void socket_async_receive_end(RoadMapSocket  s);




#endif   //   __SOCKET_ASYNC_RECEIVE_H__
