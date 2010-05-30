/* WazeCameraView.h
 *
 * LICENSE:
 *
 *   Copyright 2009, Alex Agranovich
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

#ifndef __WAZE_CAMERA_VIEW_H__
#define __WAZE_CAMERA_VIEW_H__

// INCLUDES
#include <coecntrl.h>
#include <vwsdef.h>
#include "WazeCameraEngine.h"
#include <coeview.h>

/*
#ifdef TOUCH_SCREEN
#include <touchfeedback.h>
#include <touchlogicalfeedback.h>
#endif
*/
const TInt KStdKeyCameraFocus   = 0xe2;
const TInt KStdKeyCameraFocus2  = 0xeb;     

const TInt KKeyCameraShutter    = 0xf883;
const TInt KKeyCameraShutter2   = 0xf849;   
const TInt EKeyCameraNseries2   = 0xf88c;   

const TUid KWazeCameraViewId = { 3 };

// CLASS DECLARATION
class CWazeCameraView : public CCoeControl, public MCoeView, public MWazeCameraViewer
	{
	public: // New methods

		/**
		* NewL.
		* Two-phased constructor.
		* @param aRect The rectangle this view will be drawn to.
		* @return a pointer to the created instance of CWazeCameraView.
		*/
		static CWazeCameraView* NewL( const TRect& aRect );

		/**
		* NewLC.
		* Two-phased constructor.
		* @param aRect Rectangle this view will be drawn to.
		* @return A pointer to the created instance of CWazeCameraView.
		*/
		static CWazeCameraView* NewLC( const TRect& aRect );

		
		/**
		* Engine
		* @return a const reference to the camera engine 
		*/
		inline CWazeCameraEngine& Engine() 
		{		
			return *iCameraEngine; 
		}
		
		/**
		* ~CWazeCameraView
		* Virtual Destructor.
		*/
		virtual ~CWazeCameraView();

	public:  // Functions from base classes

		/**
		* From HandleKeyEvent
		* Handles key event propogated from the view server
		*
		* @return Key response
		*/
		TKeyResponse HandleKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );
		/**
		* From MWazeCameraViewer
		*
		* @param aFrame - frame to be drawn on the screen
		*/
		void DrawImage( const CFbsBitmap& aFrame );

		/**
		* From MWazeCameraViewer
		*
		* @param void
		* @return Current view rectangle
		*/
		TRect Rect() const;

		
		/**
		* From MWazeCameraViewer
		*
		* @return Display mode
		*/
		TDisplayMode DisplayMode() const;

				/**
		* From MCoeView
		*
		* @return Camera View id 
		*/
		virtual TVwsViewId ViewId() const;
		/**
		* From CCoeControl, Draw
		* Draw this CWazeCameraView to the screen.
		* @param aRect the rectangle of this view that needs updating
		*/
		void Draw( const TRect& aRect ) const;

		/**
		* From CoeControl, SizeChanged.
		* Called by framework when the view size is changed.
		*/
		virtual void SizeChanged();


		//  Get the Window (temporary)
		//  This exposes a protected method...
		inline RWindow& GetWindow() {  return Window(); }

		CCoeControl* ComponentControl(TInt /*aIndex*/) const;

		TInt CountComponentControls() const;

		/**
		* Handle command from the view server
		* Called by framework when the view size is changed.
		*/
		void HandleCommandL( TInt aCommand );
        /**
         * From CCoeControl.
         * Handles changes to the control's resources.
         *
         * @param aType A message UID value        
         */
        virtual void HandleResourceChange( TInt aType );

        /**
         *  Handles going to background. The event comes from UI
         *
         * @param aType A message UID value        
         */
        // void HandleLosingForeground();
        /**
         *  Handles going to foreground. The event comes from UI
         *
         * @param aType A message UID value        
         */
        // void HandleGainingForeground();
		
private: // Constructors

		/**
		* ConstructL
		* 2nd phase constructor.
		* @param aRect The rectangle this view will be drawn to.
		*/
		void ConstructL(const TRect& aRect);

		/**
		* CWazeCameraView
		* C++ default constructor.
		*/
		CWazeCameraView();

		/**
		* CreateCacheScreenBufferL
		* Screen cache buffer constrctor 
		*/
		void CreateCacheScreenBufferL();
		
		/**
		* ReleaseCacheScreenBuffer
		*  
		*/
		void ReleaseCacheScreenBuffer();

		/*
		 * Camera keys capturing registration
		 */
		void CaptureCameraKeys();
		
		/**
		* Clear Gc
		*  
		*/
//		void ClearCacheScreenBuffer();

		
		
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

		
//#ifdef TOUCH_SCREEN
//			void HandlePointerEventL( const TPointerEvent& aPointerEvent );
//			MTouchFeedback*      iTouchFeedBack;
//#endif

		CWsBitmap* 					iCacheGcScreenBuffer;
		CFbsBitmapDevice* 			iCacheGcScreenDevice;
		CBitmapContext* 			iCacheGcScreenContext;

		static CWazeCameraEngine* 			iCameraEngine;
		TBool						iCacheGcInitialized;
		TBool 						iCacheGcDirty;
		TBool 						iShutterKeyPressed;
};

#endif // __WAZE_CAMERA_VIEW_H__

// End of File
