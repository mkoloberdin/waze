/* WazeBrowserView.cpp
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
// INCLUDE FILES
#include <coemain.h>
#include <eikdoc.h>
#include <eikenv.h>
#include "WazeBrowserView.h"
#include "FreeMapApplication.h"
#include "Roadmap_NativeNet.h"
#include <e32keys.h>
#include <avkon.hrh>
#include "GSConvert.h"
#include <FreeMap_0x2001EB29.rsg>
#include "FreeMapAppUi.h"
#include "FreeMapAppView.h"
extern "C" {
#include "roadmap.h"
#include "roadmap_messagebox.h"
}

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CWazeBrowserView::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CWazeBrowserView* CWazeBrowserView::NewL( const TRect& aRect )
{
   CWazeBrowserView* self = CWazeBrowserView::NewLC( aRect );
   CleanupStack::Pop( self );
   return self;
}

// -----------------------------------------------------------------------------
// CWazeBrowserView::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CWazeBrowserView* CWazeBrowserView::NewLC( const TRect& aRect )
{
   CWazeBrowserView* self = new ( ELeave ) CWazeBrowserView();
   CleanupStack::PushL( self );
   self->ConstructL( aRect );
   return self;
}

// -----------------------------------------------------------------------------
// CWazeBrowserView::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CWazeBrowserView::ConstructL( const TRect& aRect )
{
   // Create a window for this application view
   CreateWindowL();

   // Set the windows size
   SetRect( aRect );

   // Activate the window, which makes it ready to be drawn
   ActivateL();
}

// -----------------------------------------------------------------------------
// CWazeBrowserView::CWazeBrowserView()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CWazeBrowserView::CWazeBrowserView() : iBrCtlInterface( NULL ),
      iBrLoadEventObserver( NULL ), iBrSpecialLoadObserver( NULL )
{
   // No implementation required
}


// -----------------------------------------------------------------------------
// CWazeBrowserView::~CWazeBrowserView()
// Destructor.
// -----------------------------------------------------------------------------
//
CWazeBrowserView::~CWazeBrowserView()
{
   if ( iBrCtlInterface != NULL )
   {
      iBrCtlInterface->RemoveLoadEventObserver( iBrLoadEventObserver );
      delete iBrCtlInterface;
      delete iBrLoadEventObserver;
      delete iBrSpecialLoadObserver;
   }
}

// -----------------------------------------------------------------------------
// CWazeBrowserView::~CWazeBrowserView()
// Destructor.
// -----------------------------------------------------------------------------
//
void CWazeBrowserView::SetUrl( const TDes16& aUrl )
{
	if ( aUrl.Size() > iUrl.MaxSize() )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot set the url of length: %d. Maximum length: %d", aUrl.Length(), iUrl.MaxLength() );
		return;
	}
    iUrl.Copy( aUrl );
}

/***********************************************************
 *  View id 
 */
TVwsViewId CWazeBrowserView::ViewId() const
{ 
   return TVwsViewId( KUidFreeMapApp, KWazeBrowserViewId );
}

/***********************************************************
 *  Make this view visible 
 */
void CWazeBrowserView::ViewActivatedL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage )
{
   Window().SetOrdinalPosition( 0 );
   MakeVisible( ETrue );

   TUint iCommandBase = TBrCtlDefs::ECommandIdBase;
   TUint iBrCtlCapabilities = TBrCtlDefs::ECapabilityDisplayScrollBar |
         TBrCtlDefs::ECapabilityLoadHttpFw |TBrCtlDefs::ECapabilityClientNotifyURL |
         TBrCtlDefs::ECapabilityAutoFormFill|TBrCtlDefs::ECapabilityCursorNavigation|
         TBrCtlDefs::ECapabilityGraphicalPage|TBrCtlDefs::ECapabilityFavicon|TBrCtlDefs::ECapabilityToolBar|
         TBrCtlDefs::ECapabilityWebKitLite;
         
   if ( iBrCtlInterface == NULL )
   {
      iBrSpecialLoadObserver = new CWazeBrowserSpecialLoadObserver();
      iBrLinkResolver = CWazeBrowserLinkResolver::NewL();
      iBrCtlInterface = CreateBrowserControlL( this,
            this->Rect(),
            iBrCtlCapabilities,
            iCommandBase,
            NULL,
            iBrLinkResolver,
            iBrSpecialLoadObserver,
            NULL,
            NULL );

      if( iBrCtlInterface != NULL )
      {
         //			iBrCtlInterface->SetBrowserSettingL( TBrCtlDefs::ESettingsApId, CRoadMapNativeNet::GetChosenAP() );
		  /*
		   * Disable security content warning
		   */
      
         iBrCtlInterface->SetBrowserSettingL( TBrCtlDefs::ESettingsSecurityWarnings, 0 );
         iBrCtlInterface->SetBrowserSettingL( TBrCtlDefs::ESettingsCookiesEnabled, 1 );
         iBrLoadEventObserver = new CWazeBrowserLoadEventObserver( iBrCtlInterface );
         iBrCtlInterface->AddLoadEventObserverL( iBrLoadEventObserver );
      }
      else
      {
         roadmap_log( ROADMAP_ERROR, "Cannot create symbian browser interface" );
         return;
      }
   }

   iBrCtlInterface->SetRect( this->Rect() );
   //	iBrCtlInterface->ActivateL();   
   
   iBrCtlInterface->LoadUrlL( iUrl );
}

/***********************************************************
 *  Make this view hidden 
 */
void CWazeBrowserView::ViewDeactivated()
{	
   if ( iBrCtlInterface != NULL )
   {
      iBrCtlInterface->HandleCommandL( TBrCtlDefs::ECommandCancelFetch );
      iBrCtlInterface->HandleCommandL( TBrCtlDefs::ECommandDisconnect );
   }
   MakeVisible( EFalse );
}

TKeyResponse CWazeBrowserView::HandleKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType )
{
	TKeyResponse res = EKeyWasNotConsumed;
	TBrCtlDefs::TBrCtlElementType elem_type = iBrCtlInterface->FocusedElementType();
	
	if ( iBrCtlInterface != NULL )
    {
	    res = iBrCtlInterface->OfferKeyEventL( aKeyEvent, aType );	    
    }
	
    /*
     * Consume all events except non handled softkeys. Only softkeys codes in the input box should be ignored   
     */
	if ( res == EKeyWasConsumed )
		return res;
	
	if ( !( elem_type != TBrCtlDefs::EElementActivatedInputBox &&  
			  ( ( aKeyEvent.iScanCode == EStdKeyDevice0 ) || ( aKeyEvent.iScanCode == EStdKeyDevice1 ) ) ) ) 			
	{		
		res = EKeyWasConsumed;
	}
   
   return res;
}

// ---------------------------------------------------------
// CBrCtlSampleAppContainer::CountComponentControls() const
// ---------------------------------------------------------
//
TInt CWazeBrowserView::CountComponentControls() const
{
   if (iBrCtlInterface)
      return 1;
   return 0;
}

// ---------------------------------------------------------
// CBrCtlSampleAppContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CWazeBrowserView::ComponentControl(TInt aIndex) const
{
   switch ( aIndex )
   {
   case 0:
      return iBrCtlInterface; // Could be NULL
   default:
      return NULL;
   }
}

void CWazeBrowserSpecialLoadObserver::NetworkConnectionNeededL( TInt* aConnectionPtr, TInt* aSockSvrHandle,
      TBool* aNewConn, TApBearerType* aBearerType )
{
   *aBearerType = EApBearerTypeAllBearers;

   if(iFirstTime) 
   {
      //New connection is established only once
      User::LeaveIfError(iSocketServer.Connect(KESockDefaultMessageSlots));
      User::LeaveIfError(iConnection.Open(iSocketServer, KConnectionTypeDefault));
      TCommDbConnPref prefs;
      prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
      prefs.SetDirection(ECommDbConnectionDirectionOutgoing);
      prefs.SetIapId( CRoadMapNativeNet::GetChosenAP() );	//preferred IAP
      User::LeaveIfError(iConnection.Start(prefs));
      *aNewConn = ETrue;
      iFirstTime = EFalse;
   }
   else
   {
      *aNewConn = EFalse;
   }

   *aConnectionPtr = reinterpret_cast<TInt>(&iConnection);
   *aSockSvrHandle = iSocketServer.Handle();
   return;
}

/****************************************************************************************************************
 * ================================== CWazeBrowserSpecialLoadObserver ===========================================
 */
TBool CWazeBrowserSpecialLoadObserver::HandleRequestL( RArray<TUint>* aTypeArray, CDesCArrayFlat* aDesArray )
{ return EFalse; }

TBool CWazeBrowserSpecialLoadObserver::HandleDownloadL( RArray<TUint>* aTypeArray, CDesCArrayFlat* aDesArray )
{ return EFalse; }


CWazeBrowserSpecialLoadObserver::~CWazeBrowserSpecialLoadObserver()
{}


/****************************************************************************************************************
 * ================================== CWazeBrowserLoadEventObserver ===========================================
 */
void CWazeBrowserLoadEventObserver::HandleBrowserLoadEventL( TBrCtlDefs::TBrCtlLoadEvent aLoadEvent, TUint aSize, TUint16 aTransactionId )
{

//	roadmap_log( ROADMAP_WARNING, "Browser. Getting the event: %d.", aLoadEvent );
   
   switch ( aLoadEvent )
   {
   case TBrCtlDefs::EEventNewContentStart:
   case TBrCtlDefs::EEventUrlLoadingStart:
   case TBrCtlDefs::EEventNewUrlContentArrived:
   case TBrCtlDefs::EEventMoreUrlContentArrived:
   case TBrCtlDefs::EEventNewContentDisplayed:
   {
      if ( iWaitDialog == NULL )
      {
		 CreateWaitDialog();
		 iWaitDialog->ExecuteLD( R_WAIT_NOTE );
      }
      break;
   }
   case TBrCtlDefs::EEventContentFinished:
   case TBrCtlDefs::EEventLoadFinished:
   {
      if ( iWaitDialog != NULL )
      {
         iWaitDialog->ProcessFinishedL();
         iWaitDialog = NULL;
      }
      break;
   }
   default:
   {
      break;
   }
   };
}


inline void CWazeBrowserLoadEventObserver::CreateWaitDialog()
{
    iWaitDialog = new CAknWaitDialog( ( REINTERPRET_CAST( CEikDialog**, &iWaitDialog ) ) );
    iWaitDialog->SetTextL( _L( "Loading..." ) );
}

CWazeBrowserLoadEventObserver::CWazeBrowserLoadEventObserver( CBrCtlInterface* aBrCtlInterface ) 
				: iWaitDialog( NULL ), iBrCtlInterface( aBrCtlInterface )
{
   // Wait dialog
	CreateWaitDialog();
	iWaitDialog->ExecuteLD( R_WAIT_NOTE );
	
}

CWazeBrowserLoadEventObserver::~CWazeBrowserLoadEventObserver()
{
   if ( iWaitDialog != NULL )
   {
      delete( iWaitDialog );
   }
}





/****************************************************************************************************************
 * ================================== CWazeBrowserLinkResolver ===========================================
 */

CWazeBrowserLinkResolver* CWazeBrowserLinkResolver::NewL()                                                                     
{                                                                                                   
    CWazeBrowserLinkResolver* self = new (ELeave) CWazeBrowserLinkResolver();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

void CWazeBrowserLinkResolver::CancelAll()
{}

void CWazeBrowserLinkResolver::ConstructL()
{}

TBool CWazeBrowserLinkResolver::ResolveEmbeddedLinkL( const TDesC& aEmbeddedUrl, const TDesC& aCurrentUrl,
                                         TBrCtlLoadContentType aLoadContentType, MBrCtlLinkContent& aEmbeddedLinkContent )
{	
//	HBufC8 *pUrl = HBufC8::NewL( aEmbeddedUrl.Size() + 1 );	// The size in bytes is relevant
//	pUrl->Des().Copy( aEmbeddedUrl );
//	TPtr8 pDes = pUrl->Des();
	
	//GSConvert::TDes16ToCharPtr( aEmbeddedUrl, (char**) &url, false );
	//roadmap_log( ROADMAP_INFO, "Going to resolve embedded link: %s", (const char*) pDes.PtrZ() );
	
	return ResolveLinkL( aEmbeddedUrl, aCurrentUrl, aEmbeddedLinkContent );	
}

TBool CWazeBrowserLinkResolver::ResolveLinkL  ( const TDesC& aUrl, const TDesC &aCurrentUrl, MBrCtlLinkContent &aBrCtlLinkContent )
{
	TBool handled = EFalse;
	
	HBufC8 *pUrl = HBufC8::NewL( aUrl.Size() + 1 );	// The size in bytes is relevant
	pUrl->Des().Copy( aUrl );
	TPtr8 pDes = pUrl->Des();
	
	
	roadmap_log( ROADMAP_INFO, "Going to resolve link: %s", (const char*) pDes.PtrZ() );
	
	if ( roadmap_browser_url_handler( (const char*) pDes.PtrZ() ) )
	{
		roadmap_messagebox( "URL", (const char*) pDes.PtrZ() );
		handled = ETrue;
	}
	delete pUrl;
	return handled;
}

CWazeBrowserLinkResolver::CWazeBrowserLinkResolver() {}
CWazeBrowserLinkResolver::~CWazeBrowserLinkResolver() {}
