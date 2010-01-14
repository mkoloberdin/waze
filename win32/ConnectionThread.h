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


#ifndef	__CONNECTIONTHREAD_H__
#define	__CONNECTIONTHREAD_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "ASyncTask.h"

extern "C" {
#include "../roadmap_net.h"
};
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  CI_STRING_MAX_SIZE            (127)
#define  CONNECTION_QUEUE_SIZE         (20)

typedef struct tagConnectionInfo
{
   char                       Protocol [CI_STRING_MAX_SIZE+1];
   char                       Name     [CI_STRING_MAX_SIZE+1];
	time_t							tUpdate;
   int                        iDefaultPort;
   RoadMapNetConnectCallback  pfnOnNewSocket;
   void*                      pContext;
   roadmap_result             rc;

}  ConnectionInfo, *LPConnectionInfo;

inline void ConnectionInfo_Init( LPConnectionInfo pThis)
{ memset( pThis, 0, sizeof(ConnectionInfo));}

inline BOOL ConnectionInfo_IsValid( const ConnectionInfo* pThis)
{ return pThis->Protocol[0] && pThis->pfnOnNewSocket;}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
class CConnectionThreadMgr :
   public CASyncTaskMgr<ConnectionInfo,CONNECTION_QUEUE_SIZE>
{
public:
   virtual  void  InitializeData( ConnectionInfo& r)
   { ConnectionInfo_Init( &r);}
   
   virtual  BOOL  DataIsValid( const ConnectionInfo& r)
   { return ConnectionInfo_IsValid( &r);}
   
   virtual  BOOL  CellIsOccupied( const ConnectionInfo& r)
   { return DataIsValid(r);}
   
   virtual  void  HandleTask( ConnectionInfo& r);
   
   virtual  void  InitThread();

public:
   inline HRESULT  QueueConnectionRequest( LPConnectionInfo pCI)
   {
      if( !pCI)
         return E_INVALIDARG;
      return QueueNewTaskRequest( *pCI);
   }
};
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__CONNECTIONTHREAD_H__
