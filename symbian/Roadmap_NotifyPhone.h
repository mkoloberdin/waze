/* Roadmap_NotifyPhone.h - phone status listener for Roadmap
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

#ifndef _ROADMAP_NOTIFY_PHONE__H_
#define _ROADMAP_NOTIFY_PHONE__H_

#include <e32base.h>

class CTelephony;

class CRoadmapNotifyPhone : public CActive
{
public:
  CRoadmapNotifyPhone();
  virtual ~CRoadmapNotifyPhone();
  
  static CRoadmapNotifyPhone* NewL(void* apSmsReceivedCallback);
  
private: 
  void ConstructL(void* apCallAnsweredCallback);
  void RunL();
  void DoCancel();
  
  void StartListening();
  
  CTelephony* m_Telephony;
  CTelephony::TCallStatusV1 m_CurrStatus;
  CTelephony::TCallStatusV1Pckg m_CurrStatusPckg;
  void* m_pCallAnsweredCallback;
};

#endif  //  #ifndef _ROADMAP_NOTIFY_PHONE__H_
