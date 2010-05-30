/* FreeMapAppView.cpp
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


// INCLUDE FILES
#include <coemain.h>
#include <eikenv.h>
#include <BARSREAD.H>
#include <eikedwin.h> 
#include <akninfrm.h> 
#include <UTF.H>

#include "GSConvert.h"
#include "FreeMapApplication.h"
#include "FreeMapAppView.h"
#include "FreeMapAppUi.h"
#include "Roadmap_QueryDialog.h"

#include <stdlib.h>

extern "C" {
#include "roadmap_canvas.h"
#include "ssd_keyboard_dialog.h"
}

#include <FreeMap_0x2001EB29.rsg>

static CB_OnKeyboardDone kbd_callback = NULL;
static void *kbd_context = NULL;
static CRoadMapQueryDialog* pAddNoteDialog = NULL;

#define EDITBOX_HEIGHT 20

extern void roadmap_canvas_start( void );
extern void roadmap_canvas_stop( void );

void ShowEditbox(const char* aTitleUtf8, const char* aTextUtf8, CB_OnKeyboardDone callback, void *context, int aBoxType )
{
	kbd_callback = callback;
	kbd_context = context;
	TCoeInputCapabilities curCapabilities;
	TUint newCapabilities = TCoeInputCapabilities::ENavigation | TCoeInputCapabilities::EAllText;
	CFreeMapAppUi* pAppUi;

	// Update the UI object
	pAppUi = static_cast<CFreeMapAppUi*>( CEikonEnv::Static()->EikAppUi() );
	curCapabilities = pAppUi->InputCapabilities();
    // If the custom input type is requested  
    if ( aBoxType & EEditBoxNumeric ) // Numeric
    {
		newCapabilities = TCoeInputCapabilities::ENavigation | 
							TCoeInputCapabilities::EWesternNumericIntegerPositive;
    }
    if ( aBoxType & EEditBoxAlphabetic ) // Alphabetic
    {
		// Use the default capabilities
		newCapabilities = TCoeInputCapabilities::ENavigation | TCoeInputCapabilities::EAllText;
    }
    if ( aBoxType & EEditBoxStandard ) // Standard
    {
		// Use the default capabilities
		newCapabilities = TCoeInputCapabilities::ENavigation | TCoeInputCapabilities::EAllText;
    }
    if ( aBoxType & EEditBoxAlphaNumeric ) // Alphanumeric
    {
		// Use the default capabilities
    newCapabilities = TCoeInputCapabilities::ENavigation | TCoeInputCapabilities::EAllText;
    }
    // Set the new input capabilities
    // pAppUi->SetInputCapabilities( newCapabilities );
        
  if ( pAddNoteDialog == NULL )
  {
    TBuf<128> enterText;
    TBuf<128> title;
    TPtrC8 ptr( ( const TUint8* ) aTitleUtf8, User::StringLength( ( const TUint8* ) aTitleUtf8 ) );
    CnvUtfConverter::ConvertToUnicodeFromUtf8( title, ptr );
    
    ptr.Set( (const TUint8* ) aTextUtf8, User::StringLength( ( const TUint8* ) aTextUtf8 ) );
    CnvUtfConverter::ConvertToUnicodeFromUtf8( enterText, ptr );
    
    pAddNoteDialog = CRoadMapQueryDialog::NewL( enterText, CAknQueryDialog::ENoTone );
    pAddNoteDialog->SetInputCapabilities( newCapabilities );
    // For the password box - another resource
    if ( aBoxType & EEditBoxPassword )
	{
		// For password use the secured edit box
		pAddNoteDialog->PrepareLC( R_ADD_NOTE_QUERY );
	}
    else
	{
    	pAddNoteDialog->PrepareLC( R_ADD_NOTE_QUERY );
    	pAddNoteDialog->SetPredictiveTextInputPermitted(ETrue);
	}
    
    // Set prompt 
    pAddNoteDialog->SetPromptL(title);
    // Set the soft left key visible (in case of empty string ) 
    // 				for all types except the EEditBoxEmptyForbidden  
    pAddNoteDialog->SetLeftSoftKeyVisible( !( aBoxType & EEditBoxEmptyForbidden ) );
    

    TInt answer = pAddNoteDialog->RunLD();
    if(answer == EEikBidOk) 
    {
      int bits = (enterText.Length()+1)* 4;
      unsigned char* textBuf = (unsigned char*)calloc(bits+1,1);
      TPtr8 textBufPtr(textBuf, bits);
      CnvUtfConverter::ConvertFromUnicodeToUtf8(textBufPtr, enterText);
      kbd_callback(dec_ok, (const char*) textBuf, kbd_context);
      free(textBuf);
    }
    else
    {
      //Do nothing
    }
    
    // Restore the old values
    pAppUi->SetInputCapabilities( curCapabilities.Capabilities() );
    pAddNoteDialog = NULL;
    
  }
}

extern "C" void roadmap_main_configure(CBitmapContext *, CFreeMapAppView *);

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CFreeMapAppView::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CFreeMapAppView* CFreeMapAppView::NewL( const TRect& aRect )
	{
	CFreeMapAppView* self = CFreeMapAppView::NewLC( aRect );
	CleanupStack::Pop( self );
	return self;
	}

// -----------------------------------------------------------------------------
// CFreeMapAppView::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CFreeMapAppView* CFreeMapAppView::NewLC( const TRect& aRect )
	{
	CFreeMapAppView* self = new ( ELeave ) CFreeMapAppView;
	CleanupStack::PushL( self );
	self->ConstructL( aRect );
	return self;
	}

// -----------------------------------------------------------------------------
// CFreeMapAppView::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CFreeMapAppView::ConstructL( const TRect& aRect )
	{
	// Create a window for this application view
	CreateWindowL();

#ifdef TOUCH_SCREEN
    EnableDragEvents();
    
    iTouchFeedBack = MTouchFeedback::Instance();
    iTouchFeedBack->SetFeedbackEnabledForThisApp(ETrue);
#endif
    
	// Set the windows size
	SetRect( aRect );

	// Fullscreen mode
	SetExtentToWholeScreen();
	
	// Activate the window, which makes it ready to be drawn
	ActivateL();
	
	iOffScreenBitmap = new CWsBitmap(iEikonEnv->WsSession());
	if (!iOffScreenBitmap) return;

	TInt error = iOffScreenBitmap->Create(Rect().Size(), iEikonEnv->ScreenDevice()->DisplayMode());
	if (error != KErrNone)
	{
		delete iOffScreenBitmap;
		iOffScreenBitmap = NULL;
		return;
	}		

	TRAP(error,iOffScreenDevice = CFbsBitmapDevice::NewL(iOffScreenBitmap));
	if (error != KErrNone)
	{
		delete iOffScreenDevice;
		iOffScreenDevice = NULL;
		delete iOffScreenBitmap;
		iOffScreenBitmap = NULL;
		return;
	}

	error = iOffScreenDevice->CreateBitmapContext(iOffScreenGc);
	if (error != KErrNone)
	{
		delete iOffScreenGc;
		iOffScreenGc = NULL;
		delete iOffScreenDevice;
		iOffScreenDevice = NULL;
		delete iOffScreenBitmap;
		iOffScreenBitmap = NULL;
		return;
	}

	roadmap_main_configure(iOffScreenGc, this);
	}

// -----------------------------------------------------------------------------
// CFreeMapAppView::CFreeMapAppView()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CFreeMapAppView::CFreeMapAppView() : iStayBackground( EFalse )
	{
	// No implementation required
	}


// -----------------------------------------------------------------------------
// CFreeMapAppView::~CFreeMapAppView()
// Destructor.
// -----------------------------------------------------------------------------
//
CFreeMapAppView::~CFreeMapAppView()
	{
  delete iOffScreenGc;
  iOffScreenGc = NULL;
  delete iOffScreenDevice;
  iOffScreenDevice = NULL;
  delete iOffScreenBitmap;
  iOffScreenBitmap = NULL;
	}


// -----------------------------------------------------------------------------
// CFreeMapAppView::Draw()
// Draws the display.
// -----------------------------------------------------------------------------
//
void CFreeMapAppView::Draw( const TRect& /*aRect*/ ) const
	{
#ifdef _USE_GC
  // Get the standard graphics context
	CWindowGc& gc = SystemGc();

	if (iOffScreenBitmap) {
		gc.BitBlt(TPoint(0, 0), iOffScreenBitmap);
	} else {
		// Gets the control's extent
		TRect drawRect( Rect());
	
		// Clears the screen
		gc.Clear( drawRect );	
	}
#else
	roadmap_canvas_refresh();
#endif
	}

// -----------------------------------------------------------------------------
// CFreeMapAppView::SizeChanged()
// Called by framework when the view size is changed.
// -----------------------------------------------------------------------------
//
void CFreeMapAppView::SizeChanged()
{
}

// -----------------------------------------------------------------------------
// ViewId
// Returns the id of the view inside the server
// -----------------------------------------------------------------------------
TVwsViewId CFreeMapAppView::ViewId() const
{
	return TVwsViewId( KUidFreeMapApp, KWazeAppViewId );
}


// -----------------------------------------------------------------------------
// ViewActivatedL
// Make this view visible
// -----------------------------------------------------------------------------
void CFreeMapAppView::ViewActivatedL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage )
{
	Window().SetOrdinalPosition( 0 );
	MakeVisible( ETrue );
	roadmap_canvas_start();
}

// -----------------------------------------------------------------------------
// ViewDeActivatedL
// Make this view hidden
// -----------------------------------------------------------------------------
void CFreeMapAppView::ViewDeactivated()
{
	/*
	 * AGA NOTE:: Workaround
	 * This is used because of unexplainable call to this function while switching 
	 * views without deactivation. Should be removed when the problem is resolved
	 */
	if ( !iStayBackground )
	{
		roadmap_canvas_stop();
		MakeVisible( EFalse );
	}
}
TBool CFreeMapAppView::GetStayBackground()
{
	return iStayBackground;
}
/**
* Sets the stay background flag
*/
void CFreeMapAppView::SetStayBackground( TBool aValue )
{
	iStayBackground = aValue;
	// Prevent from fading to ensure normal appearance of the view if at the background
	this->Window().SetNonFading( aValue );
}
CCoeControl* CFreeMapAppView::ComponentControl(TInt aIndex) const
{
  return NULL;
}

TInt CFreeMapAppView::CountComponentControls() const
{
  TInt num = 0;
  return num;
}

#ifdef TOUCH_SCREEN
void roadmap_canvas_button_pressed(const TPoint *data);
void roadmap_canvas_button_released(const TPoint *data);
void roadmap_canvas_mouse_moved(const TPoint *data);

void CFreeMapAppView::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
  
    if (aPointerEvent.iType == TPointerEvent::EButton1Down)
        {
        // Give feedback to user (vibration)
        iTouchFeedBack->InstantFeedback(ETouchFeedbackBasic);
        roadmap_canvas_button_pressed(&aPointerEvent.iPosition);
        }
    else if (aPointerEvent.iType == TPointerEvent::EDrag)
        {
        // Dragging...
        roadmap_canvas_mouse_moved(&aPointerEvent.iPosition);
        }
    else if (aPointerEvent.iType == TPointerEvent::EButton1Up)
        {
        // Check CBA buttons
        //if (iOptionsRect.Contains(aPointerEvent.iPosition))
        roadmap_canvas_button_released(&aPointerEvent.iPosition);
        }
    }
#endif


// End of File
