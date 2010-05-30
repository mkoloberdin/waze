/* AsyncTask.h
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


#ifndef	__ASYNCTASK_H__
#define	__ASYNCTASK_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  EMPTY_QUEUE_ON_EXIT

#include <assert.h>

#pragma warning(disable:4355)

DWORD WINAPI ASyncTaskThread( void* pParams);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagThreadInfo
{
   HWND     m_hWnd;
   HANDLE   m_eventPleaseQuit;
   HANDLE   m_eventNewTask;
   
   virtual void HandleNewTask()  = 0;
   virtual void FlushQueue()     = 0;
   virtual void InitThread()     = 0;
 
}  ThreadInfo, *LPThreadInfo;

template <class DataType, int TASK_QUEUE_SIZE>
class CASyncTaskMgr  :
   public ThreadInfo
{
protected:
   virtual  void  InitializeData ( DataType&       rData) = 0;
   virtual  BOOL  DataIsValid    ( const DataType& rData) = 0;
   virtual  BOOL  CellIsOccupied ( const DataType& rData) = 0;
   virtual  void  HandleTask     ( DataType&       rData) = 0;

public:
   HANDLE            m_hThread;
   CRITICAL_SECTION  m_cs;
   DataType          m_Queue[TASK_QUEUE_SIZE];

private:
   CASyncTaskMgr( const CASyncTaskMgr&){}
   CASyncTaskMgr& operator=( const CASyncTaskMgr&){ return *this;}
   
public:
   CASyncTaskMgr()
   { ::InitializeCriticalSection( &m_cs);}
   
   ~CASyncTaskMgr()
   { 
      Destroy( true /* bCalledFromDestructor */);
      ::DeleteCriticalSection( &m_cs);
   }
   
   void Init()
   {
      for( int i=0; i<TASK_QUEUE_SIZE; i++)
         InitializeData( m_Queue[i]);
   }
   
   void HandleNewTask()
   { FlushQueue();}
   
   virtual void FlushQueue()
   {         
      DataType data;
      do
      {
         if( FAILED( Pop( data)))
            return;
            
         HandleTask( data);
      
      }  while( 1);
   }
   
   virtual void InitThread()
   {}

   HRESULT Create()
   {
      DWORD dw;
      
      if( m_hThread || m_eventPleaseQuit || m_eventNewTask)
         return E_ACCESSDENIED;
      
      Init();
         
      try
      {
         m_eventPleaseQuit = ::CreateEvent( NULL, TRUE, FALSE, NULL);
         if( !m_eventPleaseQuit)
            throw E_FAIL;

         m_eventNewTask = ::CreateEvent( NULL, FALSE, FALSE, NULL);
         if( !m_eventNewTask)
            throw E_FAIL;
         
         m_hThread = ::CreateThread( NULL, 0, ASyncTaskThread, static_cast<LPThreadInfo>(this), 0, &dw);
         if( !m_hThread)
            throw E_FAIL;
      }
      catch( HRESULT hrErr)
      {
         Destroy();
         return hrErr;
      }
     
      return S_OK;
   }
   
   void Destroy( bool bCalledFromDestructor = false)
   {
      BOOL gracefull_shutdown = TRUE;
   
      if( m_hThread)
      {
         if( m_eventPleaseQuit)
            ::SetEvent( m_eventPleaseQuit);
            
         if( WAIT_OBJECT_0 != ::WaitForSingleObject( m_hThread, 3000))
         {
            assert(0);
            ::TerminateThread( m_hThread, -1);
            gracefull_shutdown = FALSE;
         }
         
         ::CloseHandle( m_hThread);
         m_hThread = NULL;
      }
      
#ifdef   EMPTY_QUEUE_ON_EXIT
      if( !bCalledFromDestructor && gracefull_shutdown)
         FlushQueue();
#endif   // EMPTY_QUEUE_ON_EXIT

      if( m_eventPleaseQuit)
      {
         ::CloseHandle( m_eventPleaseQuit);
         m_eventPleaseQuit = NULL;
      }

      if( m_eventNewTask)
      {
         ::CloseHandle( m_eventNewTask);
         m_eventNewTask = NULL;
      }
   }

   HRESULT  QueueNewTaskRequest( DataType& rData)
   { return Push( rData);}
   
   inline   void SetWindow( HWND hWnd)
   { m_hWnd = hWnd;}

protected:
   HRESULT Push( const DataType& rCI)
   {
      int i;
      
      if( !DataIsValid( rCI))
         return E_INVALIDARG;

      if( !m_hThread || !m_eventNewTask)
         return E_UNEXPECTED;

      ::EnterCriticalSection( &m_cs);
      for( i=0; i<TASK_QUEUE_SIZE; i++)
      {
         if( !CellIsOccupied( m_Queue[i]))
         {
            m_Queue[i] = rCI;
            break;
         }
      }
      ::LeaveCriticalSection( &m_cs);
      
      if( i<TASK_QUEUE_SIZE)
      {
         ::SetEvent( m_eventNewTask);
         return S_OK;
      }
      
      return E_OUTOFMEMORY;
   }
   
   HRESULT Pop( DataType& rCI)
   {
      BOOL  bFound = FALSE;
      
      InitializeData( rCI);

      ::EnterCriticalSection( &m_cs);
      if( CellIsOccupied( m_Queue[0]))
      {
         int i = 0;
         rCI   = m_Queue[0];
         bFound= TRUE;

         for( i=0; ((i < (TASK_QUEUE_SIZE-1)) && CellIsOccupied( m_Queue[i+1])); i++)
            m_Queue[i] = m_Queue[i+1];

         InitializeData( m_Queue[i]);
      }
      ::LeaveCriticalSection( &m_cs);
      
      return bFound? S_OK: E_FAIL;
   }
};
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__ASYNCTASK_H__
