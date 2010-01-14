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
#include "../roadmap_main.h"

#include "socket_async_receive.h"

#define  RECEIVER_QUEUE_SIZE     (20)

typedef struct tag_sar_info
{
   RoadMapIO         IO;
   void*             pData;
   int               iSize;
   CB_OnDataReceive  cbOnDataReceive;
   void*             pContext;

}  sar_info, *sar_info_ptr;

inline void SAR_ReceiveInfo_Init( sar_info_ptr this)
{ memset( this, 0, sizeof(sar_info));}

static   sar_info   AsyncJobs[RECEIVER_QUEUE_SIZE];

static void on_socket_has_data( RoadMapIO* io)
{
   int            i  = 0;
   sar_info_ptr   pDI= NULL;
   
   assert(io);
   if( !io)
      return;

   for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      if(roadmap_io_same(&(AsyncJobs[i].IO), io))
      {
         pDI = &(AsyncJobs[i]);
         break;
      }
   
   if( !pDI)
   {
      // Socket must be found...
      assert( 0 && "socket_async_receive::on_socket_has_data() - UNEXPECTED");
      return;
   }
  
   pDI->iSize = roadmap_net_receive( io->os.socket, pDI->pData, pDI->iSize);
   pDI->cbOnDataReceive( pDI->pData, pDI->iSize, pDI->pContext);
}

sar_info_ptr find_receive_info( const RoadMapSocket s)
{
   int i;
   
   if( !s || (ROADMAP_INVALID_SOCKET == s))
      return NULL;
   
   for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      if( s == AsyncJobs[i].IO.os.socket)
         return &(AsyncJobs[i]);
   
   return NULL;
}

void socket_async_receive_end( RoadMapSocket s)
{
   sar_info_ptr  pDI = find_receive_info( s);
   
   if( pDI)
   {
      roadmap_main_remove_input( &(pDI->IO));
      SAR_ReceiveInfo_Init( pDI);
   }
}

BOOL socket_async_receive(  RoadMapSocket     s, 
                                 void*             data,
                                 int               size,
                                 CB_OnDataReceive  on_data_received,
                                 void*             context)
{
   sar_info_ptr   pDI      = find_receive_info( s);
   BOOL           set_input= (NULL == pDI)? TRUE: FALSE;
   
   if( !s || (ROADMAP_INVALID_SOCKET==s) || !data || !size || !on_data_received)
      return FALSE;

   if( !pDI)   // First time round?
   {
      int i; 
      
      for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      {
         if( (NULL == AsyncJobs[i].cbOnDataReceive) && (0 == AsyncJobs[i].IO.os.socket))
         {
            pDI = &(AsyncJobs[i]);
            break;
         }
      }
      
      if( !pDI)
      {
         assert( 0 && "socket_async_receive::socket_async_receive() - QUEUE IS FULL");
         return FALSE;
      }
   }
  
   pDI->IO.os.socket    = s;
   pDI->IO.subsystem    = ROADMAP_IO_NET;
   pDI->pData           = data;
   pDI->iSize           = size;
   pDI->cbOnDataReceive = on_data_received;
   pDI->pContext        = context;

   if( set_input)   
      roadmap_main_set_input( &(pDI->IO), on_socket_has_data);

   return TRUE;
}                                 




