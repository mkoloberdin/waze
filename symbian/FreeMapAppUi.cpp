/* FreeMapAppUi.cpp
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
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <f32file.h>
#include <s32file.h>
#include <hlplch.h>
#include <EscapeUtils.h> 
#include <math.h>
#include <stdlib.h>
#include <FreeMap_0x2001EB29.rsg>
#include <hal.h>
#include <hal_data.h>
#include "GSConvert.h"
//@@ #include "FreeMap_0x2001EB29.hlp.hrh"
#include "FreeMap.hrh"
#include "FreeMap.pan"
#include "FreeMapApplication.h"
#include "FreeMapAppUi.h"
#include "FreeMapAppView.h"
#include "WazeCameraView.h"
#include "WazeBrowserView.h"
#include <PtiEngine.h>
#include <PtiKeyMappings.h>
#include <ptidefs.h>
#include <featdiscovery.h>
#include <featureinfo.h>
#include <utf.h>

#include <remconcoreapitarget.h>            
#include <remconinterfaceselector.h>        

#include <string.h>
extern "C" {
#include "roadmap_start.h"
#include "roadmap_lang.h"
#include "editor/editor_main.h"
#include "roadmap_device_events.h"
#include "roadmap_urlscheme.h"
#include "roadmap_browser.h"
extern TKeyResponse roadmap_main_process_key( const TKeyEvent& aKeyEvent, TEventCode aType );
}

extern roadmap_input_type roadmap_input_type_get_mode( void );
extern void roadmap_canvas_new (RWindow& aWindow, int initWidth, int initHeight);
int global_FreeMapLock();
int global_FreeMapUnlock();
static RMutex* pSyncMutex = NULL; 
static int FM_ref = 0;

#define BACKLIGHT_DISABLE_INTERVAL	2000		// Miliseconds 


// ============================ MEMBER FUNCTIONS ===============================

int global_FreeMapLock()
{
  if ( pSyncMutex == NULL )
  {
    pSyncMutex = new RMutex();
    if ( pSyncMutex == NULL )
    {
      roadmap_log(ROADMAP_ERROR, "Mutex not created!");
      return KErrGeneral;
    }
    pSyncMutex->CreateLocal();
  }
  
  if (FM_ref > 0) {
    return 1;
  }
  FM_ref++;
  pSyncMutex->Wait();
  return KErrNone;
}

int global_FreeMapUnlock()
{
  if ( pSyncMutex == NULL )
  {
    //ERROR!
    roadmap_log(ROADMAP_ERROR, "Mutex not initialized!");
  }
  FM_ref--;
  pSyncMutex->Signal();
  return KErrNone;
}


/*************************************************************************************************
 * void roadmap_browser_launcher( RMBrowserContext* context )
 * Shows the symbian browser view
 *
 */
static void roadmap_browser_launcher( const RMBrowserContext* context )
{
	CFreeMapAppUi* pAppUi = static_cast<CFreeMapAppUi*>( CEikonEnv::Static()->EikAppUi() );
	TRect rect( 0, context->top_margin, 	// Top left
					pAppUi->ApplicationRect().Width() - 1,   context->top_margin + context->height // Bottom right
				); 

	TBuf<256> url;
	GSConvert::CharPtrToTDes16( context->url, url );
	pAppUi->ShowBrowserView( rect, url );
}

/*************************************************************************************************
 * void roadmap_browser_close( void )
 * Closes the symbian browser view
 *
 */
static void roadmap_browser_close( void )
{
	CFreeMapAppUi* pAppUi = static_cast<CFreeMapAppUi*>( CEikonEnv::Static()->EikAppUi() );		
	pAppUi->ShowAppView();
}

static void roadmap_start_event (int event) {
   switch (event) 
   {
	   case ROADMAP_START_INIT:
	   {
		  editor_main_check_map ();

		  roadmap_browser_register_launcher( roadmap_browser_launcher );
		  roadmap_browser_register_close( roadmap_browser_close );

		  break;
	   }   
   }
}





CStartTimer::CStartTimer(TInt aPriority)
  : CTimer(aPriority)
{
  m_pWaiter = NULL;
  m_State = EContinue;
}

CStartTimer::~CStartTimer()
{
  Cancel();
  Deque();
}

CStartTimer* CStartTimer::NewL()
{
  CStartTimer* self = new (ELeave) CStartTimer(EPriorityLess);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();
  return self;
}

void CStartTimer::ConstructL()
{
  m_pWaiter = new (ELeave) CActiveSchedulerWait();
  CTimer::ConstructL();
  CActiveScheduler::Add(this);
}

void CStartTimer::DoCancel()
{
  CTimer::DoCancel();
}

void CStartTimer::RunL()
{
  m_pWaiter->AsyncStop();
  if ( m_State == EContinue )
  {// for now we do NOT do it again, man
    m_State = EDismiss;
    //Start();
  }
}

void CStartTimer::Start()
{
  TTimeIntervalMicroSeconds32 delay(1000);
  After(delay);
  m_pWaiter->Start();
}



// -----------------------------------------------------------------------------
// CFreeMapAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::ConstructL()
	{
	// Initialise app UI with standard value.
	BaseConstructL(CAknAppUi::EAknEnableSkin);

	// Init rotation params
	HandleRotationChange(true);
	 
	
	// Check If qwerty is supported by the device
    if ( !CFeatureDiscovery::IsFeatureSupportedL( KFeatureIdQwertyInput ) )
	{
		m_pQwertyKeyMappings = NULL;
		m_pPtiEngine = NULL;
		USING_PHONE_KEYPAD = 1;
	}
	else
	{
		USING_PHONE_KEYPAD = 0;
	}
	   
	// View is fullscreen but we also want to hide the pane and softkeys
	//StatusPane()->MakeVisible(EFalse);
	//Cba()->MakeVisible(EFalse);
	// Create view object
	iAppView = CFreeMapAppView::NewL( ApplicationRect() );
    RegisterViewL( *iAppView );
	AddToStackL( iAppView );


	// Registration of the browser view
	iBrowserView = CWazeBrowserView::NewL( ApplicationRect() );
    RegisterViewL( *iBrowserView );
	AddToStackL( iBrowserView );
	
	
#ifdef __CAMERA_ENABLED__
	iCameraView = CWazeCameraView::NewL( ApplicationRect() );
	RegisterViewL( *iCameraView );
	AddToStackL( iCameraView );
#endif
	
	
	SetDefaultViewL( *iAppView );
    ShowAppView();

	// Init and start the backlight timer active object
	if ( m_pBLTimer == NULL ) 
	{
		m_pBLTimer = CBackLightTimer::NewL( ETrue );
	}
	m_pBLTimer->Start();
    
	// Return control to the system before showing anything onscreen
	if ( m_pStartTimer == NULL )
	{
	  m_pStartTimer = CStartTimer::NewL();
	}
	
	// Profiler timer
	m_pProfilerTimer = CProfilerTimer::NewL();
	m_pProfilerTimer->Start( 20000 /* 20 seconds */ );
  
  
	m_pStartTimer->Start();
  
	/* Utilizing media keys */
	iInterfaceSelector = CRemConInterfaceSelector::NewL();
	iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
	iInterfaceSelector->OpenTargetL();
	
	global_FreeMapLock();
    roadmap_start_subscribe (roadmap_start_event);
	roadmap_start(0, NULL);
	global_FreeMapUnlock();
	}
// -----------------------------------------------------------------------------
// CFreeMapAppUi::CFreeMapAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CFreeMapAppUi::CFreeMapAppUi()   
{
  	m_pStartTimer = NULL;
  	m_pBLTimer = NULL;
  	m_InputCapabilities = TCoeInputCapabilities::ENavigation | TCoeInputCapabilities::EAllText; 
  	// No implementation required
}

// -----------------------------------------------------------------------------
// CFreeMapAppUi::~CFreeMapAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CFreeMapAppUi::~CFreeMapAppUi()
{
	if ( iAppView )
	{
		RemoveFromStack(iAppView);
		delete iAppView;
		iAppView = NULL;
	}
#ifdef __CAMERA_ENABLED__	
	if ( iCameraView )
	{
		RemoveFromStack(iCameraView);
		delete iCameraView;
		iCameraView = NULL;
	}
#endif
	if ( iBrowserView )
	{
		RemoveFromStack(iBrowserView);
		delete iBrowserView;
		iBrowserView = NULL;
	}
	if (m_pStartTimer != NULL)
	{
		delete m_pStartTimer;
		m_pStartTimer = NULL;
	}
	if ( m_pBLTimer != NULL )
	{
		delete m_pBLTimer;
		m_pBLTimer = NULL;
	}	
	if( m_pPtiEngine != NULL )
	{
		delete m_pPtiEngine;
		m_pQwertyKeyMappings = NULL;				
	}
}

// -----------------------------------------------------------------------------
// CFreeMapAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::HandleCommandL( TInt aCommand )
	{
	if ( iCurrentView == KWazeAppViewId || iCurrentView == KWazeBrowserViewId )
	{
	switch( aCommand )
		{
		case EEikCmdExit:
		case EAknSoftkeyExit:
			roadmap_main_exit();
			break;
		default:
			Panic( EFreeMapUi );
			break;
		}
	}
#ifdef __CAMERA_ENABLED__
	if ( iCurrentView == KWazeCameraViewId )
	{
		iCameraView->HandleCommandL( aCommand );
	}
#endif // __CAMERA_ENABLED__
}

void CFreeMapAppUi::MrccatoCommand( TRemConCoreApiOperationId aOperationId, 
	TRemConCoreApiButtonAction aButtonAct )
{
#ifdef __CAMERA_ENABLED__
	if ( ( iCurrentView == KWazeCameraViewId ) && ( aButtonAct == ERemConCoreApiButtonClick ) )
	{
		switch( aOperationId )
		{
			case ERemConCoreApiVolumeUp:
			{
				TKeyEvent event = { 0, EStdKeyUpArrow, 0, 0 };

				iCameraView->HandleKeyEventL( event, EEventKeyDown );
				break;
			}
			case ERemConCoreApiVolumeDown:
			{
				TKeyEvent event = { 0, EStdKeyDownArrow, 0, 0 };
				iCameraView->HandleKeyEventL( event, EEventKeyDown );
				break;
			}
		}
	}
#endif //__CAMERA_ENABLED__	
	MRemConCoreApiTargetObserver::MrccatoCommand( aOperationId, aButtonAct );
}


TKeyResponse CFreeMapAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	TKeyResponse res = EKeyWasConsumed;
	if( iCurrentView == KWazeBrowserViewId )
	{
		if ( iBrowserView->HandleKeyEventL( aKeyEvent, aType ) == EKeyWasConsumed )
			return EKeyWasConsumed;
	}
	if ( iCurrentView == KWazeAppViewId || iCurrentView == KWazeBrowserViewId )
	{
		if (global_FreeMapLock() != 0) return res;
		// Process the data
		//	roadmap_log( ROADMAP_ERROR, "Key Data : %d, %d, %d, %d, %d", aKeyEvent.iCode, aKeyEvent.iModifiers, aKeyEvent.iRepeats, aKeyEvent.iScanCode, aType );
		res = roadmap_main_process_key( aKeyEvent, aType );
		global_FreeMapUnlock();
	}
#ifdef __CAMERA_ENABLED__
	if ( iCurrentView == KWazeCameraViewId )
	{
		iCameraView->HandleKeyEventL( aKeyEvent, aType );
	}
#endif //__CAMERA_ENABLED__
	return res;
}

// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::HandleStatusPaneSizeChange()
{
	/* Application view */
	iAppView->SetRect( ApplicationRect() );
	iAppView->SetExtentToWholeScreen();
#ifdef __CAMERA_ENABLED__	
	/* Camera view */
	iCameraView->SetRect( ApplicationRect() );
	iCameraView->SetExtentToWholeScreen();
#endif //__CAMERA_ENABLED__	
}

void CFreeMapAppUi::HandleRotationChange(bool bInitOnly)
{
  TPixelsTwipsAndRotation sizeAndRot;
  CWsScreenDevice* pScreenDevice = CEikonEnv::Static()->ScreenDevice();
  pScreenDevice->GetScreenModeSizeAndRotation(pScreenDevice->CurrentScreenMode(),
                                              sizeAndRot);
  if ( !bInitOnly && (sizeAndRot.iPixelSize != m_ScreenSize) )
  {// rotation changed
    roadmap_canvas_new (iAppView->GetWindow(), 
                        sizeAndRot.iPixelSize.iWidth,
                        sizeAndRot.iPixelSize.iHeight);
//    iAppView->Draw(ApplicationRect());  //  or just use roadmap_canvas_refresh();

    roadmap_device_event_notification( device_event_window_orientation_changed );
  }
  //  keep for next time
  //m_Orientation = sizeAndRot.iRotation;
  m_ScreenSize = sizeAndRot.iPixelSize;
}

void CFreeMapAppUi::HandleResourceChangeL(TInt aType)
{
  CAknAppUi::HandleResourceChangeL(aType);
  iAppView->HandleResourceChange(aType);
#ifdef __CAMERA_ENABLED__  
  iCameraView->HandleResourceChange( aType );  
#endif //__CAMERA_ENABLED__
  HandleRotationChange(false);
}

CArrayFix<TCoeHelpContext>* CFreeMapAppUi::HelpContextL() const
	{
	#warning "Please see comment about help and UID3..."
	// Note: help will not work if the application uid3 is not in the
	// protected range.  The default uid3 range for projects created
	// from this template (0xE0000000 - 0xEFFFFFFF) are not in the protected range so they
	// can be self signed and installed on the device during testing.
	// Once you get your official uid3 from Symbian Ltd. and find/replace
	// all occurrences of uid3 in your project, the context help will
	// work.
	CArrayFixFlat<TCoeHelpContext>* array = new(ELeave)CArrayFixFlat<TCoeHelpContext>(1);
	CleanupStack::PushL(array);
//@@	array->AppendL(TCoeHelpContext(KUidFreeMapApp, KGeneral_Information));
	CleanupStack::Pop(array);
	return array;
	}

void CFreeMapAppUi::InitQwertyMappingsL()
{
	/* Currently hebrew only !!! */
	/* TODO :: initialize the set of the languages */
   TLanguage lang = ELangEnglish;
   CPtiCoreLanguage* pCoreLanguage;    
   TInt err = KErrNone; 	
   
    // Instantiate the engine - no default user dictionary
    m_pPtiEngine = CPtiEngine::NewL( EFalse );

    if ( !USING_PHONE_KEYPAD )
	{
        /*
         * Why upper ??
         * m_pPtiEngine->SetCase(EPtiCaseUpper);
         */
    	m_pPtiEngine->SetCase( EPtiCaseLower );
	}	
    
    if ( strcasecmp( roadmap_lang_get( "lang" ), "Hebrew" ) == 0 )
	{
		lang = ELangHebrew;
	}
    if ( strcasecmp( roadmap_lang_get( "lang" ), "Spanish" ) == 0 )
	{
		lang = ELangSpanish;
	}
    
    err = m_pPtiEngine->ActivateLanguageL( lang, EPtiEngineQwerty );
    if ( err != KErrNone )
	{
		roadmap_log( ROADMAP_WARNING, "Error activating language. Error code: %d", err );
	}
    
    // Get a pointer to the core language object
    pCoreLanguage = static_cast<CPtiCoreLanguage*> ( m_pPtiEngine->GetLanguage( lang ) );
    if ( pCoreLanguage == NULL )
    {
		roadmap_log( ROADMAP_ERROR, "Failed to obtain the language PTI object for the language: %d", lang );
    }
	// Get the keyboard mappings for the language
    else
    {
    	m_pQwertyKeyMappings = static_cast<CPtiQwertyKeyMappings*>( pCoreLanguage->GetQwertyKeymappings());
    }
}

TBool  CFreeMapAppUi::GetUnicodeForScanCodeL( const TKeyEvent& aKeyEvent, TUint16 &aUnicodeOut )
{
	// Scan code of the key you are interested in
    TBuf<20> iResUnicode;
    TBuf8<20> iResUtf8;
    TBool iRes = EFalse;
    
    aUnicodeOut = 0x0;
    
    // The first call to the qwerty translator - initialize the engine
    if ( m_pPtiEngine == NULL )
	{
		InitQwertyMappingsL();
	}
    
	// Exceptional cases handling 
    switch ( aKeyEvent.iScanCode )
	{
    	case EPtiKeyQwertySpace:	// SPACE - return the utf8 0x20 
		{
			aUnicodeOut = 0x20;
			return ETrue;
		}
    	case EStdKeyEnter:
		{
			aUnicodeOut = 0x0D;
			return ETrue;
		}
    	default: break;
	}
    
    // If numeric - no translation is necessary.
    // If success - return, Failure try to translate anyway
    if ( roadmap_input_type_get_mode() == inputtype_numeric )
	{
		if ( GetUnicodeForScanCodeNumericL( aKeyEvent, aUnicodeOut ) )
			return ETrue;
	}
    // If no qwerty return false
    if ( m_pQwertyKeyMappings )
    {
    	TPtiTextCase textCase = ( aKeyEvent.iModifiers & EModifierShift ) ? EPtiCaseUpper : EPtiCaseLower;
	    m_pQwertyKeyMappings->GetDataForKey( (TPtiKey) aKeyEvent.iScanCode, iResUnicode, textCase );
	    // If the mapping fail return false
	    if ( iResUnicode.Length() )
		{
			HBufC8* pTmpBuf;
			TUint8 *ptr;
			TInt len2copy;
			
			// Convert to UTF8
			CnvUtfConverter::ConvertFromUnicodeToUtf8( iResUtf8, iResUnicode );
			pTmpBuf = iResUtf8.AllocLC(); 
			ptr = reinterpret_cast<TUint8*>( &aUnicodeOut );
			// 2 bytes maximum for the english/hebrew and most ther languages
			len2copy = ( ( (*pTmpBuf)[0] & 0xC0 ) == 0xC0 ) ? 2 : 1;
			Mem::Copy( ptr, pTmpBuf->Ptr(), len2copy );			
			CleanupStack::PopAndDestroy( pTmpBuf );
	    	iRes = ETrue;
		}
	    else
    	{
    		roadmap_log( ROADMAP_INFO, "GetUnicodeForScanCodeL(..) - No qwerty mapping for the ScanCode %d", aKeyEvent.iScanCode );
    	}
    }
    return ( iRes ); 
}


TBool  CFreeMapAppUi::GetUnicodeForScanCodeNumericL( const TKeyEvent& aKeyEvent, TUint16 &aUnicodeOut )
{
	TPtiKey key = ( TPtiKey ) aKeyEvent.iScanCode;
    TBool iRes = EFalse;
    aUnicodeOut = 0x0;    
    
	// Exceptional cases handling 
	switch ( key )
	{

		case EPtiKey0:
		case EPtiKey1:
		case EPtiKey2:
		case EPtiKey3:
		case EPtiKey4:
		case EPtiKey5:
		case EPtiKey6:
		case EPtiKey7:    		
		case EPtiKey8:
		case EPtiKey9:		
		case EPtiKeyStar:
		case EPtiKeyHash:
		{			
			aUnicodeOut = key;
			break;
		}
		case EPtiKeyQwertyM:
		{
			aUnicodeOut = '0';
			break;
		}
		case EPtiKeyQwertyR:
		{
			aUnicodeOut = '1';
			break;
		}
		case EPtiKeyQwertyT:
		{
			aUnicodeOut = '2';
			break;
		}
		case EPtiKeyQwertyY:
		{
			aUnicodeOut = '3';
			break;
		}
		case EPtiKeyQwertyF:
		{
			aUnicodeOut = '4';
			break;
		}
		case EPtiKeyQwertyG:
		{
			aUnicodeOut = '5';
			break;
		}
		case EPtiKeyQwertyH:
		{
			aUnicodeOut = '6';
			break;
		}
		case EPtiKeyQwertyV:
		{
			aUnicodeOut = '7';
			break;
		}
		case EPtiKeyQwertyB:
		{
			aUnicodeOut = '8';
			break;
		}
		case EPtiKeyQwertyN:
		{
			aUnicodeOut = '9';
			break;
		}
		case EStdKeyNkpAsterisk:
		case EPtiKeyQwertyU:
		{
			aUnicodeOut = '*';
			break;
		}
		case EStdKeyHash:
		case EPtiKeyQwertyJ:
		{
			aUnicodeOut = '#';
			iRes = ETrue;
			break;
		}
		default:
		{
			break;
		}
	}	
	iRes = ( aUnicodeOut != 0x0 );
	return iRes;	
}

void CFreeMapAppUi::SetBackLiteOn( TBool aValue )
{
	m_pBLTimer->SetBLEnable( aValue );
}


TCoeInputCapabilities CFreeMapAppUi::InputCapabilities() const
{		
/*
 * The input capabilities API is not working proeperly on all devices - not in 
 * use now
 */
//	TCoeInputCapabilities caps( CAknAppUi::InputCapabilities() );
//	caps.SetCapabilities( caps.Capabilities() | m_InputCapabilities );
	return CAknAppUi::InputCapabilities();
}

void CFreeMapAppUi::SetInputCapabilities( TUint aCapabilities )
{
	m_InputCapabilities = aCapabilities;
}

void CFreeMapAppUi::ShowAppView( void )
{
	iAppView->SetStayBackground( EFalse );
	DeactivateActiveViewL();
	iCurrentView = KWazeAppViewId;
	ActivateViewL( TVwsViewId( KUidFreeMapApp, iCurrentView ) );
}
void CFreeMapAppUi::ShowCameraView( void )
{
	DeactivateActiveViewL();
	iCurrentView = KWazeCameraViewId;
	ActivateViewL( TVwsViewId( KUidFreeMapApp, iCurrentView ) );
	
}

void CFreeMapAppUi::ShowBrowserView( const TRect& aRect, const TDes16& aUrl )
{
	iBrowserView->SetRect( aRect );
	iBrowserView->SetUrl( aUrl );

	iAppView->SetStayBackground( ETrue );
	iCurrentView = KWazeBrowserViewId;
	ActivateViewL( TVwsViewId( KUidFreeMapApp, iCurrentView ) );	
}

// -----------------------------------------------------------------------------
// CFreeMapAppUi::ProcessCommandParametersL()
// Handles waze parameters parsing and handling 
// -----------------------------------------------------------------------------
//
TBool CFreeMapAppUi::ProcessCommandParametersL( TApaCommand aCommand, TFileName &aDocumentName, const TDesC8 &aTail )
{
	char url[512] = {0};
	
	HBufC8* buf = EscapeUtils::EscapeDecodeL( aTail );
	
	strncpy_safe( url, (const char*) buf->Ptr(), buf->Length() + 1 );
		
	if ( !roadmap_urlscheme_valid( url ) )
	{
		roadmap_log( ROADMAP_ERROR, "URL is not valid: %s", url );
	}
	else
	{
		roadmap_urlscheme_remove_prefix( url, url );
		roadmap_urlscheme_init( url );
	}

	return ETrue;
}

TBool CFreeMapAppUi::IsQwertyEnabled( void )
{	
	return CFeatureDiscovery::IsFeatureSupportedL( KFeatureIdQwertyInput );
}

TBool CFreeMapAppUi::IsHwQwertyEnabled( void )
{	
	TInt val;
	TBool res = EFalse;
	if (  HAL::Get( HALData::EKeyboardDeviceKeys, val ) == KErrNone )
	{
		res = val & HALData::EKeyboard_Full;
	}
	
	return res;
}



#ifdef __CAMERA_ENABLED__
void CFreeMapAppUi::TakePicture( TJpegQualityFactor aQuality, const TFileName& aFilePath, const TSize& aSize )
{
	ShowCameraView();
	iCameraView->Engine().TakePictureL( aQuality, aFilePath, aSize );
	ShowAppView();
}

CWazeCameraEngine& CFreeMapAppUi::GetCameraEngine() 
{ 
	return iCameraView->Engine();
}

#endif __CAMERA_ENABLED__

CCoeControl* CFreeMapAppUi::GetViewById( TUid aId ) const
{
	CCoeControl *view;
#ifdef __CAMERA_ENABLED__
	if ( iCurrentView == KWazeCameraViewId )
	{
		view = static_cast<CCoeControl*>( iCameraView );
	}
#endif __CAMERA_ENABLED__	
	
	if ( iCurrentView == KWazeAppViewId )
	{
		view =  static_cast<CCoeControl*>( iAppView );
	}
	return view;
}


// -----------------------------------------------------------------------------

CBackLightTimer::CBackLightTimer( TInt aPriority, TBool aEnable ) : 
	CTimer( aPriority ), iEnable( aEnable )
{
}

CBackLightTimer::~CBackLightTimer()
{
	Cancel();
	Deque();
}

CBackLightTimer* CBackLightTimer::NewL( TBool aEnable )
{
	// Create an object
	CBackLightTimer* self = new ( ELeave ) CBackLightTimer( EPriorityLess, aEnable );
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

void CBackLightTimer::ConstructL()
{
	// Enable the timer work
	iEnable = ETrue;
	SetBLDisableInterval( BACKLIGHT_DISABLE_INTERVAL );
	
	CTimer::ConstructL();
	// Add to active scheduler	
	CActiveScheduler::Add( this );
}

void CBackLightTimer::DoCancel()
{
  CTimer::DoCancel();
}

void CBackLightTimer::RunL()
{
	// If an error occurred (admittedly unlikely)
	// deal with the problem in RunError()
	User::LeaveIfError( iStatus.Int() );

	// Otherwise reset the inactivity and resubmit the timer
	if ( iEnable )	
	{
		User::ResetInactivityTime();
	}
	After( iInterval );		// SetActive inside	
}

void CBackLightTimer::Start()
{
	if ( iEnable )
	{
		// First reset
		User::ResetInactivityTime();
	}
	// Set the timer
	After( iInterval );		// SetActive inside
	// SetActive();
}

inline void CBackLightTimer::SetBLDisableInterval( TInt aValue )
{
	iInterval = aValue;
}
inline TInt CBackLightTimer::GetBLDisableInterval( void ) const
{
	return iInterval.Int();
}

inline void CBackLightTimer::SetBLEnable( TBool aValue )
{
	iEnable = aValue;
}
inline TBool CBackLightTimer::GetBLEnable( void ) const
{
	return iEnable;
}

TInt CBackLightTimer::RunError( TInt aError )
{
	roadmap_log( ROADMAP_ERROR, "BackLightTimer leaves with error: %d", aError );
	return (KErrNone);                // Error has been handled
}


// -----------------------------------------------------------------------------


CProfilerTimer* CProfilerTimer::NewL()
{
	CProfilerTimer *self = new (ELeave) CProfilerTimer( EPriorityLess );
	self->ConstructL();
	return self;
}
void CProfilerTimer::ConstructL()
{
	CTimer::ConstructL();
	// Add to active scheduler	
	CActiveScheduler::Add( this );
}
void CProfilerTimer::Start( TInt aInterval /* In milliseconds */ )	
{
	iInterval = 1000*aInterval;
	After( iInterval );
}


CProfilerTimer::~CProfilerTimer()
{
	Cancel();
	Deque();
}

CProfilerTimer::CProfilerTimer( TInt aPriority  ) : CTimer( aPriority ) 
{
}

void CProfilerTimer::RunL( void )
{
	TInt allocSize;
	TInt biggestBlock;
	
	// If an error occurred (admittedly unlikely)
	// deal with the problem in RunError()
	User::LeaveIfError( iStatus.Int() );

	
	User::AllocSize( allocSize );
	roadmap_log_raw_data_fmt( "WAZE PROFILER. Allocated: %d.  Available: %d\n", allocSize, User::Available( biggestBlock ) );
	
	After( iInterval );
}
void CProfilerTimer::DoCancel( void )
{
	CTimer::DoCancel();
}
TInt CProfilerTimer::RunError( TInt aError )
{
	roadmap_log( ROADMAP_ERROR, "ProfilerTimer leaves with error: %d", aError );
	return (KErrNone);                // Error has been handled
}

// End of File



