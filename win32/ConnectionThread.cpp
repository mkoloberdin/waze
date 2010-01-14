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

#include "roadmap_win32.h"

#include "ConnectionThread.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void CConnectionThreadMgr::HandleTask( ConnectionInfo& r)
{
	RoadMapSocket Socket = roadmap_net_connect( r.Protocol, r.Name, r.tUpdate, r.iDefaultPort, &(r.rc));

   // The next method-call (SendMessage) is blocking.
   // Current thread will be halted until the main thread will complete its handling
   SendMessage( m_hWnd, WM_FREEMAP_SOCKET, (WPARAM)&r, (LPARAM)Socket);
}

#ifdef   UNDER_CE
extern "C" BOOL roadmap_net_is_connected();
#endif   // UNDER_CE   

void CConnectionThreadMgr::InitThread()
{
#ifdef   UNDER_CE
   roadmap_net_is_connected();
#endif   // UNDER_CE   
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
