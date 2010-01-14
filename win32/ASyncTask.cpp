/* AsyncTask.cpp
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


#include "roadmap_win32.h"
#include "ASyncTask.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" HANDLE eventShuttingDown;

DWORD WINAPI ASyncTaskThread( void* pParams)
{
   LPThreadInfo   pTI      = (LPThreadInfo)pParams;
   HANDLE         events[] = {eventShuttingDown, pTI->m_eventPleaseQuit, pTI->m_eventNewTask};
   int            count    = sizeof(events)/sizeof(HANDLE);
   DWORD          res;
   
   pTI->InitThread();
   
   do
   {
      res = ::WaitForMultipleObjects( count, events, FALSE, INFINITE);
      if( ((WAIT_OBJECT_0 + 0) == res) || ((WAIT_OBJECT_0 + 1) == res))
         break;   // Was asked to quit
      
      if( (WAIT_OBJECT_0 + 2) != res)
         continue;
         
      pTI->FlushQueue();
      
   }  while(1);

#ifdef   EMPTY_QUEUE_ON_EXIT
   // Empty queue before leaving...
   pTI->FlushQueue();
#endif   // EMPTY_QUEUE_ON_EXIT

   return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
