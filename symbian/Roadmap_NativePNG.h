/* Roadmap_NativePNG.h - native png decoder for Roadmap
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
 *   See roadmap_canvas_agg.h
 */

#ifndef _ROADMAP_NATIVE_PNG__H_
#define _ROADMAP_NATIVE_PNG__H_

#include <e32base.h>
#include <ImageConversion.h> 
#include <fbs.h>

//  This class is a wrapper for CImageDecoder

class CRoadmapNativePNG : public CActive
{
public:
  CRoadmapNativePNG(CImageDecoder* apDecoder, CFbsBitmap* apFrame, CFbsBitmap* apFrameMask);
  virtual ~CRoadmapNativePNG();
  
  //  Main method called to decode a PNG file
  static CRoadmapNativePNG* NewL(CImageDecoder* apDecoder, CFbsBitmap* apFrame, CFbsBitmap* apFrameMask);
  void Decode();
  
private: 
  void ConstructL();
  void RunL();
  void DoCancel();
  
  CImageDecoder* m_pDecoder;
  CFbsBitmap* m_pFrame;
  CFbsBitmap* m_pFrameMask;
  
  CActiveSchedulerWait* m_pAsw;
  TRequestStatus m_ConvertStatus;
};

#endif  //  #ifndef _ROADMAP_NATIVE_PNG__H_
