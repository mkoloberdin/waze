/* RoadmapNativeNet.cpp - Symbian network implementation for Roadmap
 *  This is an abstract interface to the network functionality. 
 * 
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
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
 *   See roadmap_net.h
 */

extern "C"{
#include "roadmap_net_mon.h"
#include "roadmap.h"  //  for log
}

#include <stdlib.h>
#include <string.h>
#include <commdb.h>
#include <CommDbConnPref.h>
#include <apsettingshandlerui.h>
#include <activeapdb.h> 
#include <centralrepository.h>
#include <profileenginesdkcrkeys.h>
#include <aknwaitdialog.h> 
#include <rconnmon.h>
#include <utf.h>
#include <aknlists.h> 
#include <aknpopup.h> 
#include <aknpopuplayout.h> 

#include "Roadmap_NativeNet.h"

#include <FreeMap_0x2001EB29.rsg>

RSocketServ* CRoadMapNativeNet::m_pSocketServer = NULL;
RConnection* CRoadMapNativeNet::m_pConnection = NULL;
bool CRoadMapNativeNet::m_isConnectionOpen = false;
TUint32 CRoadMapNativeNet::m_ChosenAP = 0;

//#define CLEANUP_STACK_PUSH(x, obj)  CleanupStack::PushL(obj); ++count;
//#define CLEANUP_STACK_POP(x)        CleanupStack::Pop(x);     count-=x; 

CDialogTimer::CDialogTimer(TInt aPriority)
  : CTimer(aPriority)
{
  m_pWaiter = NULL;
  m_State = EDismissAP;
}

CDialogTimer::~CDialogTimer()
{
  Cancel();
  Deque();
}

CDialogTimer* CDialogTimer::NewL()
{
  CDialogTimer* self = new (ELeave) CDialogTimer(EPriorityLess);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();
  return self;
}

void CDialogTimer::ConstructL()
{
  m_pWaiter = new (ELeave) CActiveSchedulerWait();
  CTimer::ConstructL();
  CActiveScheduler::Add(this);
}

void CDialogTimer::DoCancel()
{
  CTimer::DoCancel();
}

void CDialogTimer::RunL()
{
  if (m_pWaiter->IsStarted()) 
      m_pWaiter->AsyncStop();

  if ( m_State == EDismissAP )
  {// for now we do NOT do it again, man
    m_State = EShowWait;
    Start();
  }
}

void CDialogTimer::Start()
{
  TTimeIntervalMicroSeconds32 delay(100);
  After(delay);
  m_pWaiter->Start();
}


/////////////////////////////////////////////////////////////////////


CRoadMapNativeNet::CRoadMapNativeNet(const char *apHostname, int aPort)
{
  m_hostname = strdup(apHostname);
  m_port = aPort;
  
  m_pConnectCallback = NULL;
  m_context = NULL;
}

CRoadMapNativeNet::~CRoadMapNativeNet()
{
  delete m_pRepository;
}

void CRoadMapNativeNet::ConstructL(RoadMapNetConnectCallback apCallback, void* apContext)
{
  m_pConnectCallback = apCallback;
  m_context = apContext;  
  if ( m_pSocketServer == NULL )
  {
    m_pSocketServer = new RSocketServ();
  }
  if ( m_pConnection == NULL )
  {
    m_pConnection = new RConnection();
    m_isConnectionOpen = false;
  }  
  m_pRepository = CRepository::NewL( KCRUidProfileEngine );
  m_pDialogTimer = NULL;
}

bool CRoadMapNativeNet::OfflineProfileSwitch()
{
  bool retVal = false;
  /*
  //TODO include this
  //  Not sure we need this at this stage
  
  //  Check chosen profile (necessary for the selection options in Offline mode)
  TInt currentProfileId;
  m_pRepository->Get(KProEngActiveProfile, currentProfileId);
  
  // Close the connection only if
  // a) this is not the first time and
  // b) the profile has changed and
  // c) either the previous or the current profile is Offline (5 = Offline)
  if (m_LastProfileId != -1 && m_LastProfileId != currentProfileId &&
     (m_LastProfileId == 5 || currentProfileId == 5))
  {// Close and uninitialize
    retVal = true;
  }
  m_LastProfileId = currentProfileId;
  */
  return retVal;
}

void CRoadMapNativeNet::StartL()
{
  if ( OfflineProfileSwitch() )
  {
    m_pConnection->Close();
    m_pSocketServer->Close();
    m_isConnectionOpen = false;
  }
  
  StartConnectionL();
}

void CRoadMapNativeNet::StartConnectionL()
{
  if (m_isConnectionOpen && !FindExistingConnection())
  {
    m_isConnectionOpen = false;
  }

  if (m_isConnectionOpen == true)
  {// Connection setup is done
    OpenSession();
    ConnectWithParamsL();

    return;
  }

  // Make the settings handler go away
  if ( m_pDialogTimer == NULL )
  {
    m_pDialogTimer = CDialogTimer::NewL();
  }
  m_pDialogTimer->Start();
  InternalStartConnectionL();
}
void CRoadMapNativeNet::InternalStartConnectionL()
{
  
  roadmap_net_mon_start();
  User::LeaveIfError(m_pSocketServer->Connect());
  User::LeaveIfError(m_pConnection->Open(*m_pSocketServer));
  TInt err = KErrNone;
  if ( m_ChosenAP != 0 )
  {
    TCommDbConnPref connectPref;
    connectPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    connectPref.SetDirection(ECommDbConnectionDirectionOutgoing);
  //    connectPref.SetBearerSet(ECommDbBearerGPRS);
    connectPref.SetIapId(m_ChosenAP);
    err = ConnAsyncStart(connectPref);
    // NOTE :: err is always KErrNone, other wise the program halts. AGA 
    User::LeaveIfError( err );
  }
  else
  {
    //m_pWaitDialog =  new(ELeave)CAknWaitDialog(reinterpret_cast<CEikDialog**>(&m_pWaitDialog)); 
    //m_pWaitDialog->SetTone( CAknNoteDialog::EConfirmationTone ); 
    //m_pWaitDialog->ExecuteLD(R_WAIT_NOTE);
    err = ConnAsyncStart();
    // NOTE :: err is always KErrNone, other wise the program halts. AGA
    User::LeaveIfError(err);
    //m_pWaitDialog->ProcessFinishedL(); // !!! deletes the dialog
  }
  m_isConnectionOpen = true;

}

void CRoadMapNativeNet::UpdateIAPid()
{
  TPtrC ptrCommDbTabnameFieldname(_L("IAP\\Id"));
  m_pConnection->GetIntSetting(ptrCommDbTabnameFieldname, m_ChosenAP);
}

void CRoadMapNativeNet::Shutdown()
{
  delete m_pConnection; m_pConnection = NULL;
  delete m_pSocketServer; m_pSocketServer = NULL;
}

bool CRoadMapNativeNet::FindExistingConnection()
{
  TConnectionInfoBuf connInfoBuf;
  TUint count;
  TInt err;
  bool connected = false;
  if ( m_pConnection == NULL )
  {
    return false;
  }
  
  err = m_pConnection->EnumerateConnections(count);
  if (err == KErrNone)
  {
    for (TUint i=1; i<=count; i++)
    {// Note: GetConnectionInfo expects 1-based index.
      if (m_pConnection->GetConnectionInfo(i, connInfoBuf) == KErrNone)
      {// Check using AP id
        TUint32 apId = connInfoBuf().iIapId;
        if (apId == m_ChosenAP)
        {
          roadmap_log(ROADMAP_DEBUG, "found existing connection %d", m_ChosenAP);
          connected = true;
          break;
        }
      }
    }
  }
  else
  {
//    roadmap_log (ROADMAP_ERROR, "EnumerateConnections error=%d", err);    
  }
//  roadmap_log (ROADMAP_ERROR, "count=%d, connected=%d, m_ChosenIP=%d", count, connected, m_ChosenAP);
  return connected;
}

