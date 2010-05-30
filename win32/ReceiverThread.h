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


#ifndef	__RECEIVERTHREAD_H__
#define	__RECEIVERTHREAD_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "ASyncTask.h"

extern "C" {
#include "../roadmap_net.h"
};
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RECEIVER_QUEUE_SIZE           (20)

typedef struct tagDataInfo
{
   RoadMapSocket        socket;
   void*                pData;
   int                  iSize;
   PFN_ON_DATA_RECEIVED pfnOnDataReceived;
   void*                pContext;

}  DataInfo, *LPDataInfo;

inline void DataInfo_Init( LPDataInfo pThis)
{
   memset( pThis, 0, sizeof(DataInfo)); 
   pThis->socket = -1;
   pThis->iSize  = -1;
}

inline BOOL DataInfo_IsValid( const DataInfo* pThis)
{ return (-1 != pThis->socket) && (-1 != pThis->iSize) && pThis->pfnOnDataReceived && pThis->pData;}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
class CReceiverThreadMgr :
   public CASyncTaskMgr<DataInfo,RECEIVER_QUEUE_SIZE>
{
public:
   virtual  void  InitializeData( DataInfo& r)
   { DataInfo_Init( &r);}
   
   virtual  BOOL  DataIsValid( const DataInfo& r)
   { return DataInfo_IsValid( &r);}
   
   virtual  BOOL  CellIsOccupied( const DataInfo& r)
   { return DataIsValid(r);}
   
   virtual  void  HandleTask( DataInfo& r);

public:
   inline HRESULT  QueueReceiveRequest( LPDataInfo pCI)
   {
      if( !pCI)
         return E_INVALIDARG;
      return QueueNewTaskRequest( *pCI);
   }
};
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__RECEIVERTHREAD_H__
