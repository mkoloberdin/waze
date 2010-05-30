/* WazeCameraEngine.h - Asynchronus camera engine. Implemented as active object
 *			* Provides interface for camera control
 *			* Handles camera events
 *			*
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd, AGA
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

#ifndef __WAZECAMERAENGINE_h__
#define __WAZECAMERAENGINE_h__

#include <ecam.h>
#include <gdi.h>
#include <f32file.h>
#include <e32std.h>
#include <eikenv.h>
#include "WazeCameraViewer.h"


#ifdef __ECAM_ADV_SETT__
#include <ecamadvsettings.h>
typedef enum CCamera::CCameraAdvancedSettings::TFocusMode TFocusMode;
#else
typedef int TFocusMode;
#endif

typedef enum
{
	JpegQualityLow = 0x0,
	JpegQualityMedium,
	JpegQualityHigh
}TJpegQualityFactor;

class CWsBitmap;
class CBitmapScaler;
class CImageDecoder;
class CImageEncoder;
class CAknKeySoundSystem;
class CWazeCameraEngine : public MCameraObserver2 , public CActive
{
    public:

        static CWazeCameraEngine* NewL( MWazeCameraViewer* aViewer );

        virtual ~CWazeCameraEngine();

    public: // Types

    	typedef enum
    	{
			EEngineStateNotReady = 0,
    		EEngineStateIdle,
    		EEngineStatePrepare,
    		EEngineStatePreview,
    		EEngineStateCapture,
    		EEngineStateDecode,
    		EEngineStateEncode,
    		EEngineStateSave,
    		EEngineStateMakeThumbnail,
    		EEngineStateCompleted,
    		EEngineStateError,
    	} TEngineState;


    	typedef enum
    	{
    		EEngingErrorNone = 0,
    		EEngingErrorPreview,
    		EEngineErrorCapturing,
    		EEngineErrorSaving
    	} TEngineStatus;
    public:	// Engine Interface

    	/**
    	 * TakePictureL()
    	 * 1. Shows preview to the user
    	 * 2. Compress the captured image  according to the provided quality factor
    	 * 3. Saves the image to the file of the provided name
    	 * 4. The image buffer can be accessed
    	 */
    	void TakePictureL( TJpegQualityFactor aQuality, const TFileName& aFilePath, const TSize& aSize );
		/**
		 * Captures the image if the preview is ready		 
		 */
		void CaptureL();

		/**
		 * GetThumbnail - builds the thumbnail and copies it to the bufOut
		 * Synchronous  		 
		 */
		TBool GetThumbnail( TDes8* bufOut, const TFileName& aCaptureFilePath, const TSize& aThumbSize );
		
        /**
         * Starts the engine
         */
        void StartEngine();
        /**
         * Stops the engine
         */
        void StopEngine();

        /**
         * Resets the engine
         */
        void ResetEngine();

		/**
		 * Sets the focus mode for the camera
		 * @param aMode - the requesed mode
		 * @return current focus mode
		 * 
		 */
		TFocusMode SetFocusMode( TFocusMode aMode );
        /**
         * Returns the current display mode
         */
        TDisplayMode DisplayMode();

        /**
         * Returns the engine state.
         */
        TEngineState GetEngineState();
		/**
		 * Starts the focus engine
		 * 
		 */
		 void StartFocusL( void );

		 /**
		 * Stops the focus engine
		 * 
		 */
		 void StopFocusL( void );

		 /**
		 * Sets the zoom in by one step
		 * 
		 */
		 void ZoomStepIn( void );
		 /**
		 * Sets the zoom in by one step
		 * 
		 */
		 void ZoomStepOut( void );
		 /**
		 * 
		 * Indicates to the engine that the size is changed
		 */
		 void SizeChanged();
		 /**
		 * Active view finder indicator
		 */
		 TBool ViewFinderActive( void );
		 /**
		 * Setups the noise reduction
		 */
		 void SetNoiseReduction();
		 
    private:

    	CWazeCameraEngine();
        /**
         * Calculates the appropriate rectangle for the portrait/landscape mode 
         */
    	TSize PreviewSize();
    	
        /**
         * Draws view finder buffer on the screeen 
         */
    	void DrawViewFinderBuffer();
        /**
         * Starts the asyncronous process of encoding and saving of the image to the target file 
         */
    	void SaveBitmapAsync( CFbsBitmap* aBmp, TFileName &aFileName );

        /**
         * Symbian OS constructor.
         */
       void ConstructL( MWazeCameraViewer* aViewer );


    	/**
		 * From MCameraObserver2
		 * Camera event handler
		 */
		virtual void HandleEvent(const TECAMEvent &aEvent);

		/**
		 * From MCameraObserver2
		 * Notifies the client of new viewfinder data
		 */
		virtual void ViewFinderReady(MCameraBuffer &aCameraBuffer, TInt aError);

		/**
		 * From MCameraObserver2
		 * Notifies the client of a new captured image
		 */
		 virtual void ImageBufferReady(MCameraBuffer &aCameraBuffer, TInt aError);
		
		 
		/**
		 * From MCameraObserver2
		 * Not implemented
		 */
		virtual void VideoBufferReady(MCameraBuffer& /*aCameraBuffer*/, TInt /*aError*/) {}

		/**
		 * Handles the errors
		 *
		 */
		TInt virtual HandleError( TInt aError );

		/**
		 * Handles the camera reserve event
		 * Called by HandleEvent
		 */
		virtual void ReserveComplete( TInt aError );

		/**
		 * Handles the camera power on
		 * Called by HandleEvent
		 */
		virtual void PowerOnComplete( TInt aError );

		/**
		 * Starts the viewfinder.
		 * Called by the event handler when the camera is ready
		 */
		virtual void StartViewFinderL();


		/**
		 * Returns the best bitmap format supported by the camera device
		 * Called by HandleEvent
		 */
		CCamera::TFormat GetBestHWBmpFormat( const TCameraInfo& info ) const;
		/**
		 * Sets the zoom in/out
		 * @param aZoomIn : ETrue - zoom in step, EFalse - zoom out step
		 */
		void ZoomStep( TBool aZoomIn );
		/*
		 * Handles the decoding process after capturing
		 */
		void DecodeHandler();
		/*
		 * Clips the image to the given size
		 */
		void Clip( const TSize& aSize, const CFbsBitmap& aBmpIn, CFbsBitmap& aBmpOut );
		/*
		 * Handles the encoding process after capturing
		 */
		void EncodeHandler();
		/**
		 * Starts the asynchronus process of the image decoding from the jpeg resizing 
		 * Called internally
		 */
		void DecodeImageAsync( const TSize& aNewSize );
		/**
		 * Saves the raw jpeg as received from the camera engine.  
		 * DEBUG purposes
		 */
		void SaveRawImage();
		/**
		 * Finilizes the engine work  
		 * 
		 */
		void Finalize();
		/**
		 * Thumbnail size to be scaled to before clipping  
		 * 
		 */
		void GetThumbnailDecodeSize( const TSize& aThumbSize, TSize& aThumbDecodeSize ) const;
    private: // Functions from base CActive classes
        /**
         * From CActive.
         * Cancels the Active object. Empty.
         */
        void DoCancel();

        /**
         * From CActive.
         * Called when an asynchronous request has completed.
         */
        void RunL();
        
        
        

    private: //data

    	CEikonEnv*				  iEnv;					
        CCamera*                  iCamera;
        TEngineState              iState;
        CBitmapScaler*			  iScaler;				// Bitmap scaling operations engine	  
        TInt		              iZoomFactor;
        CImageDecoder*			  iImgDecoder;			// Image decoder for the image decoding,
														// buffering and scaling
        CImageEncoder*			  iImgEncoder;			// Image encoder
        
        TSize					  iTargetSize;			// The target image size
        TBool					  iReserved;			// TODO:: Check if not included in states
        TBool					  iThumbnailRequest;	// True if there is a thumbnail request
        TSize					  iCaptureSize;
        TSize					  iViewFinderSize;
        CFbsBitmap*               iPreviewBitmap;		// The bitmap containing the preview
        CFbsBitmap*               iDecodeBitmap;		// The decoded bitmap after capture 
        CFbsBitmap*               iSaveBitmap;			// The bitmap to be saved after processing
        TDesC8*					  iImageData;			// The captured image data 
        TDes8*					  iThumbnailBuf;		// The thumbnail buffer 
        TSize					  iThumbnailSize;		// The thumbnail size
        TJpegQualityFactor		  iJpegQuality;			// Jpeg quality for the image to save
        CCamera::TFormat          iFormat;				// Capture format
        TFileName				  iFileName;			// File name to save the image (Not owned)
        CActiveSchedulerWait*	  iWaiter;
        CCamera::CCameraAdvancedSettings* iAdvSettings;	// Advanced settings for this device
        CCamera::CCameraImageProcessing*  iImgProcessing;  // Image processing engine for the camera 
        // TODO:: Make own waiter class
        MWazeCameraViewer*  	iCameraViewer;		// Camera viewer - draws the viewfinder frames on the screen
        static const TInt		  iCaptureTimeout = 30000;		// Timeout for taking the picture in milliseconds
        CAknKeySoundSystem* 	  iCameraSound;			// Snap sound
        
};

#endif // __WAZECAMERAENGINE_h__

// End of File
