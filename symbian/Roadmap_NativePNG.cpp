/* Roadmap_NativePNG.cpp - native PNG implementation for Roadmap
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
 *   See roadmap_canvas_agg.h
 */

#include "Roadmap_NativePNG.h"

CRoadmapNativePNG::CRoadmapNativePNG(CImageDecoder* apDecoder, CFbsBitmap* apFrame, CFbsBitmap* apFrameMask)
: CActive(EPriorityStandard)
{
  m_pDecoder = apDecoder;
  m_pFrame = apFrame;
  m_pFrameMask = apFrameMask;
}


CRoadmapNativePNG::~CRoadmapNativePNG()
{

}

CRoadmapNativePNG* CRoadmapNativePNG::NewL(CImageDecoder* apDecoder, CFbsBitmap* apFrame, CFbsBitmap* apFrameMask)
{
  CRoadmapNativePNG* self= new (ELeave) CRoadmapNativePNG(apDecoder, apFrame, apFrameMask);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop(self);
  return self;
}

void CRoadmapNativePNG::ConstructL()
{
  CActiveScheduler::Add(this);
}

void CRoadmapNativePNG::Decode()
{
  m_ConvertStatus = KRequestPending;
  if ( m_pFrameMask == NULL )
  {
    m_pDecoder->Convert(&m_ConvertStatus, *m_pFrame, 0);
  }
  else
  {
    m_pDecoder->Convert(&m_ConvertStatus, *m_pFrame, *m_pFrameMask, 0);
  }
 // SetActive();
 // CActiveScheduler::Start();
  User::WaitForRequest(m_ConvertStatus);
}

void CRoadmapNativePNG::RunL()
{
  if ( iStatus == KErrNone )
  {
//    roadmap_log (ROADMAP_ERROR, "\r\nin RunL\r\n");
//    CActiveScheduler::Stop();
  }  
  else
  {
//    roadmap_log (ROADMAP_ERROR, "error in NativePNG %d", iStatus);
//    CActiveScheduler::Stop();
  }
}
  
void CRoadmapNativePNG::DoCancel()
{
//   CActiveScheduler::Stop();
}
