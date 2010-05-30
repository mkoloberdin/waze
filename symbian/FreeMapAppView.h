/* FreeMapAppView.h
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

#ifndef __FREEMAPAPPVIEW_h__
#define __FREEMAPAPPVIEW_h__

// INCLUDES
#include <coecntrl.h>

#ifdef TOUCH_SCREEN
#include <touchfeedback.h>
#include <touchlogicalfeedback.h>
#endif

const TUid KWazeAppViewId = { 1 };

// CLASS DECLARATION
class CFreeMapAppView : public CCoeControl, public MCoeView
	{
	public: // New methods

		/**
		* NewL.
		* Two-phased constructor.
		* Create a CFreeMapAppView object, which will draw itself to aRect.
		* @param aRect The rectangle this view will be drawn to.
		* @return a pointer to the created instance of CFreeMapAppView.
		*/
		static CFreeMapAppView* NewL( const TRect& aRect );

		/**
		* NewLC.
		* Two-phased constructor.
		* Create a CFreeMapAppView object, which will draw itself
		* to aRect.
		* @param aRect Rectangle this view will be drawn to.
		* @return A pointer to the created instance of CFreeMapAppView.
		*/
		static CFreeMapAppView* NewLC( const TRect& aRect );

		/**
		* ~CFreeMapAppView
		* Virtual Destructor.
		*/
		virtual ~CFreeMapAppView();

	public:  // Functions from base classes

		/**
		* From CCoeControl, Draw
		* Draw this CFreeMapAppView to the screen.
		* @param aRect the rectangle of this view that needs updating
		*/
		void Draw( const TRect& aRect ) const;

		/**
		* From CoeControl, SizeChanged.
		* Called by framework when the view size is changed.
		*/
		virtual void SizeChanged();
		
		/**
		* From MCoeView
		* Returns the id of the view
		*/
		virtual TVwsViewId ViewId() const;
		
		//  Get the Window (temporary)
		//  This exposes a protected method...
		inline RWindow& GetWindow() {  return Window(); }

		CCoeControl* ComponentControl(TInt /*aIndex*/) const;	
		TInt CountComponentControls() const;
		
		/**
		* Returns the stay background flag
		*/
		inline TBool GetStayBackground();
		/**
		* Sets the stay background flag
		*/
		void SetStayBackground( TBool aValue );
		
private: // Constructors

		/**
		* ConstructL
		* 2nd phase constructor.
		* Perform the second phase construction of a
		* CFreeMapAppView object.
		* @param aRect The rectangle this view will be drawn to.
		*/
		void ConstructL(const TRect& aRect);

		/**
		* CFreeMapAppView.
		* C++ default constructor.
		*/
		CFreeMapAppView();
		
		/**
		* ViewActivatedL
		*	From MCoeView
		* @return  
		*/
		virtual void ViewActivatedL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage );
		/**
		* From MCoeView
		* Make this view not visible
		* @return  
		*/
		virtual void ViewDeactivated();

#ifdef TOUCH_SCREEN
			void HandlePointerEventL(const TPointerEvent& aPointerEvent);        
			MTouchFeedback*      iTouchFeedBack;
#endif
		
		CWsBitmap* iOffScreenBitmap;
		CFbsBitmapDevice* iOffScreenDevice;
		CBitmapContext* iOffScreenGc;		
		
		TBool		iStayBackground;
	};

#endif // __FREEMAPAPPVIEW_h__

// End of File
