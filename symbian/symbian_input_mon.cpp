/* symbian_input_mon.cpp - AO to invoke callback on changes in input. 
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   roadmap_main.h
 */

#include <stdlib.h>

#include "symbian_input_mon.h"


CIOMonitor::CIOMonitor() 
  : CActive(EPriorityNormal)
{
  
}

CIOMonitor::~CIOMonitor()
{

}

void CIOMonitor::ConstructL(RoadMapIO* apIO, void* apParam) 
{
  m_pData = (roadmap_main_io*)apParam;
  m_pIO = m_pData->io;

  switch (apIO->subsystem)
  {//TODO perhaps do a factory here instead... 
  case ROADMAP_IO_SERIAL:
  case ROADMAP_IO_NET:
  case ROADMAP_IO_FILE:
    break;
  }
  
//TODO DEBUG  CActiveScheduler::Add(this);
//TODO DEBUG  SetActive();
}

CIOMonitor* CIOMonitor::StartL(RoadMapIO* apIO, void* apParam)
{
  CIOMonitor* self = new (ELeave) CIOMonitor();
  CleanupStack::PushL(self);
  self->ConstructL(apIO, apParam);
  CleanupStack::Pop(self);
  return self;
}

void CIOMonitor::RunL()
{
  if(m_pData->is_valid && (m_pIO->subsystem != ROADMAP_IO_INVALID))
  {
    // Send a message to main window so it can read. 
    //  Use a TRequestStatus or a TCallback
    //SendMessage(RoadMapMainWindow, WM_USER_READ, (WPARAM)data, 1);
  }
  
  if (m_pData->is_valid) 
  {
    m_pData->is_valid = 0;
  } 
  else 
  {
    free (m_pData);
    m_pData = NULL;
  }

  if ( m_pData != NULL )
  {
    SetActive();
  }
}

void CIOMonitor::DoCancel()
{

}


