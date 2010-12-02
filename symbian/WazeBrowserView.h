/* WazeBrowserView.h
 *
 * LICENSE:
 *
 *
 *
 *   Copyright 2010, Alex Agranovich (AGA), Waze Mobile Ltd
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

#ifndef __WAZE_BROWSER_VIEW_H__
#define __WAZE_BROWSER_VIEW_H__

// INCLUDES
#include <coecntrl.h>
#include <vwsdef.h>
#include <coeview.h>
#include <brctlinterface.h>
#include <aknwaitdialog.h> 
extern "C" {
#include "roadmap_browser.h"
}
const TUid KWazeBrowserViewId = { 2 };

// FORWARD DECLARATIONS
class CWazeBrowserSpecialLoadObserver;
class CWazeBrowserLoadEventObserver;
class CWazeBrowserLinkResolver;
// CLASS DECLARATION
class CWazeBrowserView : public CCoeControl, public MCoeView
{
public: // New methods

   /**
    * NewL.
    * Two-phased constructor.
    * @param aRect The rectangle this view will be drawn to.
    * @return a pointer to the created instance of CWazeBrowserView.
    */
   static CWazeBrowserView* NewL( const TRect& aRect );

   /**
    * NewLC.
    * Two-phased constructor.
    * @param aRect Rectangle this view will be drawn to.
    * @return A pointer to the created instance of CWazeBrowserView.
    */
   static CWazeBrowserView* NewLC( const TRect& aRect );

   /**
    * From CoeControl,CountComponentControls.
    */
   TInt CountComponentControls() const;

   /**
    * From CCoeControl,ComponentControl.
    */
   CCoeControl* ComponentControl(TInt aIndex) const;


   /**
    * ~CWazeBrowserView
    * Virtual Destructor.
    */
   ~CWazeBrowserView();

public:  // Functions from base classes

   void SetUrl( const TDes16& aUrl );
   
   void SetFlags(TInt aFlags );
   /**
    * From HandleKeyEvent
    * Handles key event propogated from the view server
    *
    * @return Key response
    */
   TKeyResponse HandleKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );

   /**
    * From MCoeView
    *
    * @return Browser View id
    */
   virtual TVwsViewId ViewId() const;


private: // Constructors

   /**
    * ConstructL
    * 2nd phase constructor.
    * @param aRect The rectangle this view will be drawn to.
    */
   void ConstructL(const TRect& aRect);

   /**
    * CWazeBrowserView
    * C++ default constructor.
    */
   CWazeBrowserView();

   /**
    * From MCoeView
    * Make this view visible
    * @return
    */
   virtual void ViewActivatedL( const TVwsViewId& aPrevViewId,TUid aCustomMessageId,const TDesC8& aCustomMessage );

   /**
    * From MCoeView
    * Make this view not visible
    * @return
    */
   virtual void ViewDeactivated();

   CBrCtlInterface *iBrCtlInterface;
   TBuf<WEB_VIEW_URL_MAXSIZE+1>  iUrl;
   CWazeBrowserLoadEventObserver *iBrLoadEventObserver;
   CWazeBrowserSpecialLoadObserver *iBrSpecialLoadObserver;
   CWazeBrowserLinkResolver *iBrLinkResolver;
   TInt iFlags;
};

// CLASS DECLARATION
/*
 * Special class overload to overcome the bug in the FP1 where the AP dialog is always displayed 
 */
class CWazeBrowserSpecialLoadObserver : public MBrCtlSpecialLoadObserver 
{
public:
   void NetworkConnectionNeededL( TInt* aConnectionPtr, TInt* aSockSvrHandle,
         TBool* aNewConn, TApBearerType* aBearerType );

   TBool HandleRequestL(RArray<TUint>* aTypeArray, CDesCArrayFlat* aDesArray);
   TBool HandleDownloadL(RArray<TUint>* aTypeArray, CDesCArrayFlat* aDesArray);
   virtual ~CWazeBrowserSpecialLoadObserver();
private:
   RSocketServ  iSocketServer;
   RConnection  iConnection;
   TBool        iFirstTime;  //Initial value should be ETrue
};

// CLASS DECLARATION
/*
 * Event observer for progress dialog 
 */
class CWazeBrowserLoadEventObserver : public MBrCtlLoadEventObserver
{
public:
   CWazeBrowserLoadEventObserver( CBrCtlInterface* aBrCtlInterface );
   void HandleBrowserLoadEventL( TBrCtlDefs::TBrCtlLoadEvent aLoadEvent, TUint aSize, TUint16 aTransactionId );
   virtual ~CWazeBrowserLoadEventObserver();
private:
   void CreateWaitDialog();
private:
   CAknWaitDialog  *iWaitDialog;
   CBrCtlInterface* iBrCtlInterface;
   
};

// CLASS DECLARATION
/*
 * Event observer for links 
 */
class CWazeBrowserLinkResolver : public MBrCtlLinkResolver
{
public:
        /**                                                                                                                                                         
        * Two-phased constructor.                                                                                                                                   
        */                                                                                                                                                          
       static CWazeBrowserLinkResolver* NewL();

	TBool ResolveEmbeddedLinkL( const TDesC& aEmbeddedUrl, const TDesC& aCurrentUrl,
	                                         TBrCtlLoadContentType aLoadContentType, MBrCtlLinkContent& aEmbeddedLinkContent );
	TBool ResolveLinkL  ( const TDesC& aUrl, const TDesC &aCurrentUrl, MBrCtlLinkContent &aBrCtlLinkContent ); 

	void CancelAll();
	
	virtual ~CWazeBrowserLinkResolver();
private:
        
        void ConstructL();
        
        CWazeBrowserLinkResolver();

};

#endif // __WAZE_BROWSER_VIEW_H__

// End of File
