/* Roadmap_NotifyPhone.cpp - phone status listener implementation for Roadmap
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Copyright 2008 Ehud Shabtai

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
 *   See roadmap.h
 */

extern "C"{
#include "roadmap.h"
}

#include <Etel3rdParty.h>
#include "Roadmap_NotifyPhone.h"


CRoadmapNotifyPhone::CRoadmapNotifyPhone()
  : CActive(EPriorityStandard), m_CurrStatusPckg(m_CurrStatus)
{
  m_pCallAnsweredCallback = NULL;
}


CRoadmapNotifyPhone::~CRoadmapNotifyPhone()
{
  Cancel();
  delete m_Telephony;
}

CRoadmapNotifyPhone* CRoadmapNotifyPhone::NewL(void* apCallAnsweredCallback)
{
  if ( apCallAnsweredCallback == NULL )
  {// do not allow creating a notifier without a callback
   // can also use User::LeaveIfNull(apSmsReceivedCallback) instead of all this
    //  return NULL;
    User::Leave(KErrGeneral);
  }
  CRoadmapNotifyPhone* self= new (ELeave) CRoadmapNotifyPhone();
  CleanupStack::PushL(self);
  self->ConstructL(apCallAnsweredCallback);
  CleanupStack::Pop(self);
  return self;
}

void CRoadmapNotifyPhone::ConstructL(void* apCallAnsweredCallback)
{
  CActiveScheduler::Add(this);
  m_pCallAnsweredCallback = apCallAnsweredCallback;
  m_Telephony = CTelephony::NewL();
  //SetActive();
  StartListening();
}

void CRoadmapNotifyPhone::RunL()
{
  if ( iStatus == KErrNone )
  {
    if ( m_CurrStatus.iStatus == CTelephony::EStatusConnected )
    {
      //TODO call m_pCallAnsweredCallback
            
    }
  }
  
  if(iStatus != KErrCancel)
  {// go on
    StartListening(); 
  }
}
  
void CRoadmapNotifyPhone::DoCancel()
{
  m_Telephony->CancelAsync(CTelephony::EVoiceLineStatusChangeCancel);
}

void CRoadmapNotifyPhone::StartListening()
{
  Cancel();
  m_CurrStatus.iStatus = CTelephony::EStatusUnknown;  //  reset status although not strictly necessary
  
  m_Telephony->NotifyChange(iStatus, CTelephony::EVoiceLineStatusChange, m_CurrStatusPckg);
  SetActive();
}
