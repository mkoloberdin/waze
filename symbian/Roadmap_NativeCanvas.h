/* Roadmap_NativeCanvas.h - native canvas for Roadmap using DSA
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Copyright 2008 Ehud Shabtai
 *    Based off Nokia wiki example code at 
 *    http://wiki.forum.nokia.com/index.php/How_to_draw_image_to_screen_directly
 *    http://wiki.forum.nokia.com/index.php/Anti-tearing_with_CDirectScreenBitmap
 * 
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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

#ifndef _ROADMAP_NATIVE_CANVAS__H_
#define _ROADMAP_NATIVE_CANVAS__H_

#include <w32std.h>
//#include <cdsb.h>   //  for CDirectScreenBitmap

#define DEFAULT_BPP 4     //  bgra = (32/8) 
//  This class is a wrapper for CDirectScreenAccess

class CRoadmapNativeCanvas : public MDirectScreenAccess
{
public:
  static CRoadmapNativeCanvas* NewL(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow);
  static CRoadmapNativeCanvas* NewLC(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow);
  virtual ~CRoadmapNativeCanvas();
 
  inline CWsScreenDevice& GetScreenDevice(){return m_ScreenDevice;}
  inline RWindow& GetWindow(){return m_Window;}
  inline CFbsBitGc* GetScreenGc(){return m_pGc;}
 
  inline void SetBuffer(const unsigned char* pBuf, int bufSize) { m_pBuffer = pBuf; m_BufSize = bufSize;  }
  
  int Start();
  void Stop();
  
  //  This is called on every refresh
  void UpdateScreen();
  
private:
  //ctor (invoked by NewL/C)
  CRoadmapNativeCanvas(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow);
  void ConstructL();
  void Restart(RDirectScreenAccess::TTerminationReasons aReason);
  void AbortNow(RDirectScreenAccess::TTerminationReasons aReason);
 
  //  These are internal methods called by UpdateScreen() 
//  void BeginUpdate();
//  void EndUpdate();
 
private:
  CWsScreenDevice&      m_ScreenDevice;
  RWsSession&           m_WsSession;
  RWindow&              m_Window;
 
  CDirectScreenAccess*  m_pDirectScreenAccess;
  RRegion*              m_pRegion;
  CFbsBitGc*            m_pGc;
  CFbsBitmap*           m_pBitmap;
  
  //  Buffer to copy to the bitmap
  const unsigned char*  m_pBuffer;
  int                   m_BufSize;
  
//  CDirectScreenBitmap*  m_pDSBitmap;
//  TUint8*               m_pScreenAddress;
  
  TBool                 m_bDrawing;
  TRequestStatus        m_bUpdateComplete;
};

#endif  //  #ifndef _ROADMAP_NATIVE_CANVAS__H_
