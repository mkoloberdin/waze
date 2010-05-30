/* Roadmap_NotifySMS.h - native SMS event listener for Roadmap
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
 *   See roadmap.h
 */

#ifndef _ROADMAP_NOTIFY_SMS__H_
#define _ROADMAP_NOTIFY_SMS__H_

#include <e32base.h>
#include <msvapi.h> 

class CRoadmapNotifySMS : public MMsvSessionObserver //, public CActive
{
public:
  CRoadmapNotifySMS();
  virtual ~CRoadmapNotifySMS();
  
  static CRoadmapNotifySMS* NewL(void* apSmsReceivedCallback);
  
  virtual void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);
  
private: 
  void ConstructL(void* apSmsReceivedCallback);
  //void RunL();
  //void DoCancel();
  
  // A session to the messaging server
  CMsvSession* m_pMsvSession;  
  CMsvEntry* m_pMsvEntry;
  TMsvId m_NewMessageId;
  
  void* m_pSmsReceivedCallback;
};

#endif  //  #ifndef _ROADMAP_NOTIFY_SMS__H_
