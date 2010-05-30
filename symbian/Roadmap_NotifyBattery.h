/* Roadmap_NotifyBattery.h - battery status listener for Roadmap
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

#ifndef _ROADMAP_NOTIFY_BATTERY__H_
#define _ROADMAP_NOTIFY_BATTERY__H_

#include <e32base.h>

class CTelephony;

class CRoadmapNotifyBattery : public CActive
{
public:
  CRoadmapNotifyBattery();
  virtual ~CRoadmapNotifyBattery();
  
  static CRoadmapNotifyBattery* NewL(void* apBatteryCallback);
  
private: 
  void ConstructL(void* apBatteryCallback);
  void RunL();
  void DoCancel();
  
  void InitBatteryInfo();
  
  CTelephony* m_Telephony;
  CTelephony::TBatteryInfoV1 m_CurrStatus;
  TPckg<CTelephony::TBatteryInfoV1> m_CurrStatusPckg;
  void* m_pBatteryCallback;
  bool m_bGettingFirstState;  //  for getting first time (and Canceling properly)s
};

#endif  //  #ifndef _ROADMAP_NOTIFY_BATTERY__H_
