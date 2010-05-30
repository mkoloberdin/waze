/* Roadmap_NativeCanvas.cpp - native canvas implementation for Roadmap using DSA
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

#include "Roadmap_NativeCanvas.h"
#include "util/agg_color_conv_rgb8.h"

CRoadmapNativeCanvas* CRoadmapNativeCanvas::NewL(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow)
{
  CRoadmapNativeCanvas* self = CRoadmapNativeCanvas::NewLC(aWsSession, aScreenDevice, aWindow);
  CleanupStack::Pop(self);
  return self;
}
 
CRoadmapNativeCanvas* CRoadmapNativeCanvas::NewLC(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow)
{
  CRoadmapNativeCanvas* self = new(ELeave)CRoadmapNativeCanvas(aWsSession, aScreenDevice, aWindow);
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
}
 
CRoadmapNativeCanvas::CRoadmapNativeCanvas(RWsSession& aWsSession, CWsScreenDevice& aScreenDevice, RWindow& aWindow) 
: m_ScreenDevice(aScreenDevice),
  m_WsSession(aWsSession),
  m_Window(aWindow)
{
  m_pDirectScreenAccess = NULL;
  m_pBitmap = NULL;
//  m_pDSBitmap = NULL;
  m_pRegion = NULL;
  m_pGc = NULL;
  m_bDrawing = EFalse;
  
  m_pBuffer = NULL;
}
 
CRoadmapNativeCanvas::~CRoadmapNativeCanvas()
{
  delete m_pDirectScreenAccess;
  delete m_pBitmap;
//  delete m_pDSBitmap;
}
 
void CRoadmapNativeCanvas::ConstructL()
{
//  m_pDSBitmap = CDirectScreenBitmap::NewL();
  m_pBitmap = new CFbsBitmap();
  m_pBitmap->Create(m_ScreenDevice.SizeInPixels(), m_ScreenDevice.DisplayMode());
  m_pDirectScreenAccess = CDirectScreenAccess::NewL(m_WsSession, m_ScreenDevice, m_Window, *this);
  m_WsSession.Flush();
}
 
int CRoadmapNativeCanvas::Start()
{
  if( !m_bDrawing )
  {
    TRAPD(err1, m_pDirectScreenAccess->StartL());
    if( err1 != KErrNone )
    {
      return err1;
    }
    m_pGc = m_pDirectScreenAccess->Gc();
    m_pRegion = m_pDirectScreenAccess->DrawingRegion();
    /*
    if(Rect() != m_pRegion->BoundingRect())
    {
      return;
    }
    */
    m_pGc->SetClippingRegion(m_pRegion);
    m_bDrawing = ETrue;
/*    
    // It may happen that a device does not support double buffering.
    TRect theRect(0, 0, m_pRegion->BoundingRect().Width(), m_pRegion->BoundingRect().Height());
    TRAPD(err2, m_pDSBitmap->Create(theRect, CDirectScreenBitmap::EDoubleBuffer));
    if ( err2 != KErrNone )
    {
      //TODO do something here
      return err2;
    }
*/
  }
  
  return KErrNone;
}
 
void CRoadmapNativeCanvas::Stop()
{
	AbortNow( RDirectScreenAccess::ETerminateRegion );
}
 /*
void CRoadmapNativeCanvas::RunL()
{
  m_pDirectScreenAccess->ScreenDevice()->Update();
}
 */
void CRoadmapNativeCanvas::Restart(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
  Start();
}
 
void CRoadmapNativeCanvas::AbortNow(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
  m_pDirectScreenAccess->Cancel();
  m_bDrawing = EFalse;
//  m_pDSBitmap->Close();
}
/*
void CRoadmapNativeCanvas::BeginUpdate()
{
  // Obtain the screen address every time before drawing the frame, 
  // since the address always changes
  TAcceleratedBitmapInfo bitmapInfo;
  m_pDSBitmap->BeginUpdate(bitmapInfo);        
  m_pScreenAddress = bitmapInfo.iAddress;
}

void CRoadmapNativeCanvas::EndUpdate()
{
  m_pDSBitmap->EndUpdate(m_bUpdateComplete);//; or use Active Obj with (iStatus);
}

void CRoadmapNativeCanvas::UpdateScreen()
{
  // This method is responsible to render and draw a frame to the screen
  BeginUpdate();

  // Fill the frame using m_pScreenAddress
  //  We use the static char* buf from the agg implementation

  EndUpdate();
}
*/

void CRoadmapNativeCanvas::UpdateScreen() 
{
  //  We use the static char* buf from the agg implementation

  if ( !m_bDrawing )
	return;
	
#ifndef _USING_USERSVR
  m_pBitmap->LockHeap();
  TUint32* bmpAddr = m_pBitmap->DataAddress();
  int width = m_pBitmap->SizeInPixels().iWidth;
  int height = m_pBitmap->SizeInPixels().iHeight;
  int strideSrc = width * DEFAULT_BPP;  //  src is always bgra32
  int strideDst = m_pBitmap->DataStride();
  agg::rendering_buffer src_rbuf ((agg::int8u*)m_pBuffer, width, height, -strideSrc);
  agg::rendering_buffer dst_rbuf ((unsigned char*)bmpAddr, width, height, -strideDst);
  switch ( m_pBitmap->DisplayMode() )
  {
    case EColor64K:
      agg::color_conv(&dst_rbuf, &src_rbuf, agg::color_conv_bgra32_to_rgb565());
      break;
    case EColor16M:
      agg::color_conv(&dst_rbuf, &src_rbuf, agg::color_conv_bgra32_to_bgr24());
      break;
    case EColor16MU:
    case EColor16MA:
      memcpy(bmpAddr, m_pBuffer, m_BufSize); 
      break;
    default:
      break;
  }
  m_pBitmap->UnlockHeap();
  
  m_pGc->BitBlt(TPoint(0,0), m_pBitmap);
  m_pDirectScreenAccess->ScreenDevice()->Update();
#else
  TPckgBuf<TScreenInfoV01> infoPckg;
  TScreenInfoV01& screenInfo = infoPckg();
  UserSvr::ScreenInfo(infoPckg);

  TUint8* screenMemory = screenInfo.iScreenAddressValid ? (TUint8*) screenInfo.iScreenAddress + 32 : 0;
  User::LeaveIfNull(screenMemory);
  
  memcpy((void*)screenMemory, (const void*)m_pBuffer, m_BufSize);  //  32 to 8... say... huh?! what about alignment??? 

  CFbsScreenDevice* pMyScreenDev = NULL;
  TRAPD( err, pMyScreenDev = CFbsScreenDevice::NewL(0 ,m_ScreenDevice.DisplayMode())); 
  if ( err != KErrNone )
  {
    roadmap_log(ROADMAP_FATAL,"Could not create screen device!");
    return;
  }
  RRegion Myregion;
  Myregion.AddRect(TRect(0,0,240,320)); // the out of date rect region.
  pMyScreenDev->Update(Myregion);
  Myregion.Close();
#endif
}

