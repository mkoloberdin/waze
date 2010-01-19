/* Roadmap_NotifySMS.cpp - native SMS listener implementation for Roadmap
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

#include <smut.h>
#include "Roadmap_NotifySMS.h"

#ifdef __WINS__
const TMsvId KObservedFolderId = KMsvDraftEntryId;
#else
const TMsvId KObservedFolderId =  KMsvGlobalInBoxIndexEntryId;
#endif

const TMsvId KInbox = KMsvGlobalInBoxIndexEntryId;


CRoadmapNotifySMS::CRoadmapNotifySMS()
//: CActive(EPriorityStandard)
{
  m_pSmsReceivedCallback = NULL;
}


CRoadmapNotifySMS::~CRoadmapNotifySMS()
{
  delete m_pMsvEntry;
  m_pMsvEntry = NULL;

  delete m_pMsvSession;
  m_pMsvSession = NULL;
}

CRoadmapNotifySMS* CRoadmapNotifySMS::NewL(void* apSmsReceivedCallback)
{
  if ( apSmsReceivedCallback == NULL )
  {// do not allow creating a notifier without a callback
   // can also use User::LeaveIfNull(apSmsReceivedCallback) instead of all this
    //  return NULL;
    User::Leave(KErrGeneral);
  }
  CRoadmapNotifySMS* self= new (ELeave) CRoadmapNotifySMS();
  CleanupStack::PushL(self);
  self->ConstructL(apSmsReceivedCallback);
  CleanupStack::Pop(self);
  return self;
}

void CRoadmapNotifySMS::ConstructL(void* apSmsReceivedCallback)
{
//  CActiveScheduler::Add(this);
  m_pSmsReceivedCallback = apSmsReceivedCallback;
  m_pMsvSession = CMsvSession::OpenSyncL(*this);
  m_pMsvEntry = CMsvEntry::NewL(*m_pMsvSession, KInbox, TMsvSelectionOrdering());
}

void CRoadmapNotifySMS::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3)
{
  switch (aEvent)
  {
  case EMsvServerReady:
    //  We use OpenSyncL so do nothing here,
    //  otherwise intialize m_pMsvEntry here if NULL
    break;
  case EMsvEntriesCreated:
    // Only look for changes in the Inbox
    if (aArg2 &&  *(static_cast<TMsvId*>(aArg2)) == KObservedFolderId)
    {
      CMsvEntrySelection* entries =
        static_cast<CMsvEntrySelection*>(aArg1);
        if( entries->Count() >= 1 )
        {
          m_NewMessageId = entries->At(0);
        }
        else
        {
          roadmap_log(ROADMAP_ERROR, "SMS Engine Internal error");//Panic(ESmsEngineInternal);
        }
    }
    break;

  case EMsvEntriesChanged:
    //Look for changes. When using the emulator observed folder is
    //drafts, otherwise inbox.
    if (aArg2 &&  *(static_cast<TMsvId*>(aArg2)) == KObservedFolderId)
    {
      CMsvEntrySelection* entries =
        static_cast<CMsvEntrySelection*>(aArg1);

        if( entries->Count() < 1 )
        {
          roadmap_log(ROADMAP_ERROR, "SMS Engine Internal error");  //Panic(ESmsEngineInternal);
        }
        else if (m_NewMessageId == entries->At(0))
        {
          /*
           * it's initialized after OpenSyncL; use code below only if async
          if( !m_pMsvEntry )
          {
            roadmap_log(ROADMAP_ERROR, "SMS Engine not initialized");  //Panic(ESmsEngineNotInitialized);
            return;
          }
          */

          // Set entry context to the new message
          m_pMsvEntry->SetEntryL(m_NewMessageId);

          // Check the type of the arrived message and that the
          // message is complete.
          // only SMS's are our consern.
          if ( m_pMsvEntry->Entry().iMtm != KUidMsgTypeSMS ||
              !m_pMsvEntry->Entry().Complete() )
          {
            return;
          }

          //TODO call m_pSmsReceivedCallback here

        }
    }
    break;

  default:
    break;
  }

}

//void CRoadmapNativeMsgs::RunL()
//{
//}
//  
//void CRoadmapNativeMsgs::DoCancel()
//{
//}
