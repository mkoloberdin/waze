/* WazeCameraView.cpp
 *
 * LICENSE:
 *
 *   Copyright 2009 Alex Agranovich
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
 */

#ifdef __CAMERA_ENABLED__

// INCLUDE FILES
#include <coemain.h>
#include <eikdoc.h>
#include <eikenv.h>
#include "WazeCameraView.h"
#include "FreeMapApplication.h"
#include <e32keys.h>
#include "FreeMapAppUi.h"
#include <avkon.hrh>
extern "C" {
#include "roadmap.h"
}
/*
#include "FreeMapAppUi.h"

#include <stdlib.h>

extern "C" {
#include "roadmap_canvas.h"
#include "ssd_keyboard_dialog.h"
}
*/

CWazeCameraEngine* CWazeCameraView::iCameraEngine = NULL;

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CWazeCameraView::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CWazeCameraView* CWazeCameraView::NewL( const TRect& aRect )
{
	CWazeCameraView* self = CWazeCameraView::NewLC( aRect );
	CleanupStack::Pop( self );
	return self;
}

// -----------------------------------------------------------------------------
// CWazeCameraView::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CWazeCameraView* CWazeCameraView::NewLC( const TRect& aRect )
{
	CWazeCameraView* self = new ( ELeave ) CWazeCameraView();
	CleanupStack::PushL( self );
	self->ConstructL( aRect );
	return self;
}

// -----------------------------------------------------------------------------
// CWazeCameraView::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CWazeCameraView::ConstructL( const TRect& aRect )
{
	// Create a window for this application view
	CreateWindowL();

#ifdef TOUCH_SCREEN
    EnableDragEvents();

//    iTouchFeedBack = MTouchFeedback::Instance();
//    iTouchFeedBack->SetFeedbackEnabledForThisApp(ETrue);
#endif

	// Set the windows size
	SetRect( aRect );
   
	// Fullscreen mode
	SetExtentToWholeScreen();

	// Activate the window, which makes it ready to be drawn	
	ActivateL();

	// Register to capture the camera keys
	CaptureCameraKeys();
	
	iCameraEngine = CWazeCameraEngine::NewL( this );
		
	CreateCacheScreenBufferL();
}

// -----------------------------------------------------------------------------
// CWazeCameraView::CWazeCameraView()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CWazeCameraView::CWazeCameraView() : iCacheGcInitialized( EFalse ), iCacheGcDirty( ETrue ), iShutterKeyPressed( EFalse )
{
	// No implementation required
}


// -----------------------------------------------------------------------------
// CWazeCameraView::~CWazeCameraView()
// Destructor.
// -----------------------------------------------------------------------------
//
CWazeCameraView::~CWazeCameraView()
{
	delete iCacheGcScreenContext;
	iCacheGcScreenContext = NULL;
	delete iCacheGcScreenDevice;
	iCacheGcScreenDevice = NULL;
	delete iCacheGcScreenBuffer;
	iCacheGcScreenBuffer = NULL;
}


// -----------------------------------------------------------------------------
// CWazeCameraView::Draw()
// Draws the display.
// -----------------------------------------------------------------------------
//
void CWazeCameraView::Draw( const TRect& /*aRect*/ ) const
{

    // Get the standard graphics context
	CWindowGc& gc = SystemGc();

	if ( iCacheGcInitialized && iCacheGcDirty )
	{
		// Draw the buffered frame
		gc.BitBlt( TPoint( 0, 0 ), iCacheGcScreenBuffer );
	}
}


// -----------------------------------------------------------------------------
// CWazeCameraView::DrawImage()
// Draws the frame to the buffer.
// -----------------------------------------------------------------------------
//
void CWazeCameraView::DrawImage( const CFbsBitmap& aFrame )
{
	if ( iCacheGcInitialized )
	{		
		iCacheGcScreenContext->BitBlt( TPoint( 0, 0 ), &aFrame );
		iCacheGcDirty = ETrue;
		DrawNow();
	}
}


// -----------------------------------------------------------------------------
// CWazeCameraView::Rect()
// Returns the view rectangle
// -----------------------------------------------------------------------------
//
TRect CWazeCameraView::Rect() const
{
	return CCoeControl::Rect();
}


// -----------------------------------------------------------------------------
// CWazeCameraView::SizeChanged()
// Called by framework when the view size is changed.
// -----------------------------------------------------------------------------
//
void CWazeCameraView::SizeChanged()
{
	if ( iCacheGcInitialized )
	{
		// Gets the control's extent
		TRect newRect( Rect() );
		// Clears the screen
		TSize newSize = newRect.Size();
		roadmap_log( ROADMAP_WARNING, "AGA DEBUG. New size: %d, %d", newSize.iWidth, newSize.iHeight );
		
//		iCacheGcScreenBuffer->Resize( newSize );
//		iCacheGcScreenDevice->Resize( newSize );
//		iCameraEngine->SizeChanged();
	}
	
}

CCoeControl* CWazeCameraView::ComponentControl(TInt aIndex) const
{
  return NULL;
}

TInt CWazeCameraView::CountComponentControls() const
{
  TInt num = 0;
  return num;
}

/*
#ifdef TOUCH_SCREEN
void roadmap_canvas_button_pressed(const TPoint *data);
void roadmap_canvas_button_released(const TPoint *data);
void roadmap_canvas_mouse_moved(const TPoint *data);

void CWazeCameraView::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{

    if (aPointerEvent.iType == TPointerEvent::EButton1Down)
	{
	// Give feedback to user (vibration)
	iTouchFeedBack->InstantFeedback(ETouchFeedbackBasic);

	}
    else if (aPointerEvent.iType == TPointerEvent::EDrag)
	{
	// Dragging...

	}
    else if (aPointerEvent.iType == TPointerEvent::EButton1Up)
	{
	// Check CBA buttons
	//if (iOptionsRect.Contains(aPointerEvent.iPosition))

	}
}
#endif
*/

/*
-------------------------------------------------------------------------------
Creates the data members cache screen buffer
-------------------------------------------------------------------------------
*/
void CWazeCameraView::CreateCacheScreenBufferL()
{
    iCacheGcScreenBuffer = new (ELeave) CWsBitmap( iCoeEnv->WsSession() );
    TSize size = Rect().Size();

    TInt error = iCacheGcScreenBuffer->Create( size, DisplayMode() );
    User::LeaveIfError( error );

    iCacheGcScreenDevice = CFbsBitmapDevice::NewL( iCacheGcScreenBuffer );

    error = iCacheGcScreenDevice->CreateBitmapContext( iCacheGcScreenContext );
    User::LeaveIfError( error );
    
	iCacheGcScreenContext->SetBrushColor( KRgbBlack );
	iCacheGcScreenContext->Clear();

	iCacheGcDirty = EFalse;

	iCacheGcInitialized = ETrue;
}

/*
-------------------------------------------------------------------------------
Releases the data members cache screen buffer
-------------------------------------------------------------------------------
*/
void CWazeCameraView::ReleaseCacheScreenBuffer()
{
	iCacheGcInitialized = EFalse;
    delete iCacheGcScreenBuffer;
    delete iCacheGcScreenDevice;
    delete iCacheGcScreenContext;
}

/***********************************************************
 *  Display mode 
 */
TDisplayMode CWazeCameraView::DisplayMode() const
{
    TInt color;
    TInt gray;
    CEikonEnv *pEnv = CEikonEnv::Static();
    TDisplayMode displayMode = pEnv->WsSession().GetDefModeMaxNumColors( color, gray );
    return displayMode;
}


/***********************************************************
 *  View id 
 */
TVwsViewId CWazeCameraView::ViewId() const
{
	return TVwsViewId( KUidFreeMapApp, KWazeCameraViewId );
}

/***********************************************************
 *  Make this view visible 
 */
void CWazeCameraView::ViewActivatedL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage )
{
	Window().SetOrdinalPosition( 0 );
	MakeVisible( ETrue );

}

/***********************************************************
 *  Make this view hidden 
 */
void CWazeCameraView::ViewDeactivated()
{
	MakeVisible( EFalse );
}

/***********************************************************
 *  Handle key events 
 */
TKeyResponse CWazeCameraView::HandleKeyEventL(  const TKeyEvent& aKeyEvent, TEventCode aType  )
{
	TKeyResponse res = EKeyWasNotConsumed;
	TInt code = aKeyEvent.iScanCode;
	// Autofocus requests
	if ( code == KStdKeyCameraFocus || code == KStdKeyCameraFocus2 )
    {
		switch ( aType )
        {
			case EEventKeyDown:
			{
				iCameraEngine->StartFocusL();
				break;
			}
			case EEventKeyUp:
			{
				iCameraEngine->StopFocusL();				
				break;
			}
			default:
				break;
        }
		res = EKeyWasConsumed;
    }
	// Zoom adjustment request
	if ( code == EStdKeyDownArrow )
	{
		iCameraEngine->ZoomStepOut();
		res = EKeyWasConsumed;
	}
	if ( code == EStdKeyUpArrow )
	{
		iCameraEngine->ZoomStepIn();
		res = EKeyWasConsumed;
	}
	
    switch ( aKeyEvent.iCode )
	{
        case EKeyUpArrow:
		{
    		iCameraEngine->ZoomStepIn();
    		res = EKeyWasConsumed;
		}
        case EKeyDownArrow:
		{
            iCameraEngine->ZoomStepOut();
            res = EKeyWasConsumed;
		}
        case EKeyOK:
        case EKeyCamera:
        case KKeyCameraShutter:
        case EKeyCameraNseries2:
		{
			iShutterKeyPressed = ETrue;
			iCameraEngine->CaptureL();
			res = EKeyWasConsumed;
			break;
		}
        default:
            break;
	}
	return res;
}

void CWazeCameraView::HandleCommandL( TInt aCommand )
{
	switch( aCommand )
	{
		case EEikCmdExit:
		case EAknSoftkeyExit:
		{
			iCameraEngine->StopEngine();
// AGA DEBUG			
			CFreeMapAppUi* pAppUi = static_cast<CFreeMapAppUi*> ( CEikonEnv::Static()->EikAppUi() );
//			pAppUi->ShowAppView();
			pAppUi->Exit();
			break;
		}
		default:
			break;
	}
}

void CWazeCameraView::HandleResourceChange( TInt aType )
{
	if( aType == KEikDynamicLayoutVariantSwitch  && this->IsActivated() )
    {
		roadmap_log( ROADMAP_INFO, "Orientation changed!!!Rect: ( %d, %d )", Rect().Size().iWidth, Rect().Size().iHeight );
		
		iCameraEngine->StopEngine();
		// Clear context and reset the engine 
		// Reset the screen buffer
		ReleaseCacheScreenBuffer();
		// TODO :: Add skipping frames
		

		CreateCacheScreenBufferL();
		
		 iCameraEngine->StartEngine();
	}
	// Call base class implementation
    CCoeControl::HandleResourceChange( aType );	  
}

void CWazeCameraView::CaptureCameraKeys()
{
       RWindowGroup& grp= CEikonEnv::Static()->RootWin();
       TUint camera_keys[] = { EKeyOK, EKeyCamera, KKeyCameraShutter, KKeyCameraShutter2, EKeyCameraNseries2 };
       TUint i, key;
       TUint keys_num = sizeof( camera_keys ) / sizeof( camera_keys[0] );
       
       for ( i = 0; i < keys_num; ++i )
	   {
		   key = camera_keys[i];
		   grp.CaptureKey( key, 0, 0);
		   grp.CaptureKeyUpAndDowns( key, 0, 0);
		   grp.CaptureLongKey( key, key, 0, 0, EPrioritySupervisor, ELongCaptureRepeatEvents );
	   }
} 

#endif // __CAMERA_ENABLED__

// End of File
/*
 * WazeCameraView.cpp
 *
 *  Created on: Jul 2, 2009
 *      Author: alex
 */

