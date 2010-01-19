/* Roadmap_NotifyBattery.cpp - battery status listener implementation for Roadmap
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
#include "Roadmap_NotifyBattery.h"


CRoadmapNotifyBattery::CRoadmapNotifyBattery()
  : CActive(EPriorityStandard), m_CurrStatusPckg(m_CurrStatus)
{
  m_pBatteryCallback = NULL;
}


CRoadmapNotifyBattery::~CRoadmapNotifyBattery()
{
  Cancel();
  delete m_Telephony;
}

CRoadmapNotifyBattery* CRoadmapNotifyBattery::NewL(void* apBatteryCallback)
{
  if ( apBatteryCallback == NULL )
  {// do not allow creating a notifier without a callback
   // can also use User::LeaveIfNull(apBatteryCallback) instead of all this
    //  return NULL;
    User::Leave(KErrGeneral);
  }
  CRoadmapNotifyBattery* self= new (ELeave) CRoadmapNotifyBattery();
  CleanupStack::PushL(self);
  self->ConstructL(apBatteryCallback);
  CleanupStack::Pop(self);
  return self;
}

void CRoadmapNotifyBattery::ConstructL(void* apBatteryCallback)
{
  CActiveScheduler::Add(this);
  m_pBatteryCallback = apBatteryCallback;
  m_Telephony = CTelephony::NewL();
  //SetActive();
  InitBatteryInfo();
}

void CRoadmapNotifyBattery::RunL()
{
  //TODO call m_pBatteryCallback with :
  //  m_CurrStatus.iChargeLevel
  //  m_CurrStatus.iStatus
  //  also use an "init" state for initial values (to detect chage etc)

  if(iStatus != KErrCancel)
  {
    m_bGettingFirstState = false;
    m_Telephony->NotifyChange(iStatus, CTelephony::EBatteryInfoChange, m_CurrStatusPckg); 
    SetActive();
  }
}
  
void CRoadmapNotifyBattery::DoCancel()
{
  if ( m_bGettingFirstState == true )
  {
    m_Telephony->CancelAsync(CTelephony::EGetBatteryInfoCancel);
  }
  else
  {
    m_Telephony->CancelAsync(CTelephony::EBatteryInfoChangeCancel);
  }
}

void CRoadmapNotifyBattery::InitBatteryInfo()
{
  m_bGettingFirstState = true;
  m_Telephony->GetBatteryInfo(iStatus, m_CurrStatusPckg);
  SetActive();
}
