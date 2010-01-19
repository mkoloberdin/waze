/* WazeCameraEngine.cpp - Waze camera engine implementation
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

#ifdef __CAMERA_ENABLED__
#include <FreeMap_0x2001EB29.rsg>
#include "WazeCameraEngine.h"
#include "Roadmap_NativeSound.h"
#include <aknappui.h>
#include <bitmaptransforms.h>
#include <GSConvert.h>
#include <stdlib.h>
#include <imageconversion.h>
#include <aknsoundsystem.h> 


extern "C"
{
#include "roadmap.h"
}

const TInt KImageSizePreviewIndex = 2;
const TInt KImageSizeCaptureIndex = 1;
const TInt KSnapSoundId = 2;

_LIT8( KMimeType, "image/jpeg" );
/***********************************************************
 * Constructor (1st-phase)
 */
CWazeCameraEngine* CWazeCameraEngine::NewL( MWazeCameraViewer* aViewer )
{
	CWazeCameraEngine* self = new (ELeave) CWazeCameraEngine();

	self->ConstructL( aViewer );

	return self;
}


/***********************************************************
 * Destructor
 */
CWazeCameraEngine::~CWazeCameraEngine()
{
    delete iCamera;
    delete iPreviewBitmap;
    delete iSaveBitmap;
    delete iDecodeBitmap;
    
    // Cancel any outstanding request
    Cancel();
}

/***********************************************************
 * Constructor (2nd-phase)
 */
void CWazeCameraEngine::ConstructL( MWazeCameraViewer* aViewer )
{
	if ( !CCamera::CamerasAvailable() )
    {
        HandleError( KErrHardwareNotAvailable );
        return;
    }
	
	iEnv = CEikonEnv::Static();
	
	iCameraViewer = aViewer;

	iCameraSound =    static_cast<CAknAppUi*>(iEnv->AppUi())->KeySounds();
	
	if ( iCameraSound )  
	{
	  TRAPD( error, iCameraSound->AddAppSoundInfoListL( R_CAMERA_SNAP_SOUND ) );         
	  if ( ( error != KErrAlreadyExists ) && ( error != KErrNone ) )             
	  {             
		  roadmap_log( ROADMAP_ERROR, "Failed initializing the sound. Error: %d", error );             
	  }
	}
    // iImgProcessing = CCamera::CCameraImageProcessing::NewL( *iCamera );
	
	// Save bitmap creation
	iSaveBitmap = new (ELeave) CFbsBitmap();
	// Decode bitmap creation
	iDecodeBitmap = new (ELeave) CFbsBitmap();
	
	
	CActiveScheduler::Add( this );
	
    iPreviewBitmap = new (ELeave) CFbsBitmap();
    
    iWaiter = new (ELeave) CActiveSchedulerWait();
}


/***********************************************************
 * Resets the engine
 */
void CWazeCameraEngine::ResetEngine()
{
	/*
	 * TODO:: State checking ????? 
	 */
	if( iState != EEngineStatePrepare )
	{
		StopEngine();
		
		StartEngine();		
	} 
}

/***********************************************************
 * Resets the engine
 */
void CWazeCameraEngine::StartEngine()
{
	// Initilizations
	iImageData = NULL;

	if ( iState != EEngineStateNotReady )
	{
		roadmap_log( ROADMAP_ERROR, "Illegal state: %d", iState );
		return;		
	}
	
	iCamera = CCamera::New2L( *this, 0, 0 );
	
	if ( iCamera == NULL )
	{
		roadmap_log( ROADMAP_ERROR, "Error initializating Symbian objects for camera" );
		// TODO:: Add Leave
		return;
	}
#ifdef __ECAM_ADV_SETT__	
	iAdvSettings = CCamera::CCameraAdvancedSettings::NewL( *iCamera );
	if ( iAdvSettings == NULL )
	{
		roadmap_log( ROADMAP_WARNING, "Advanced camera settings are not supported" );
		return;
	}
#endif //__ECAM_ADV_SETT__
	
	/********************/	    
	iState = EEngineStatePrepare;
	iCamera->Reserve();
}

	
/***********************************************************
 * Stops the engine
 */
void CWazeCameraEngine::StopEngine()
{
	if ( iState != EEngineStateNotReady )
	{
		if ( iCamera->ViewFinderActive() )
		{
			iCamera->StopViewFinder();
		}
        iCamera->PowerOff();
        iCamera->Release();
    	/* Reinstantiate the camera object to resolve rotation problems */
    	delete iAdvSettings;
    	iAdvSettings = NULL;
    	delete iCamera;		
    	iCamera = NULL;	
	}

//	Cancel();
    iState = EEngineStateNotReady;
}
/***********************************************************
 * Error handling
 */
TInt CWazeCameraEngine::HandleError( TInt aError )
{
    switch( aError )
	{
        case KErrNone:
        {
            break;
        }
        case KErrNoMemory:
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, Out of memory!" );
        	iEnv->HandleError( aError );
            break;
        }
        case KErrInUse:
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, Resource is in use!" );
            break;
        }
        case KErrHardwareNotAvailable:
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, Resource us not available!" );
            iState = EEngineStateError;		// Reset is necessary
            break;
        }
        case KErrExtensionNotSupported:     // AF not supported
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, AF is not supported!" );
        	break;
        }
        case KErrTimedOut:
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, Operation timed out!" );
        	iState = EEngineStateError;
            break;
        }
        default:
        {
        	roadmap_log( ROADMAP_WARNING, "Waze Camera Engine, General error" );
            iEnv->HandleError( aError );
        }
	}
    return aError;
}

/***********************************************************
 * Asynchronous callback for the decoder 
 */
void CWazeCameraEngine::DecodeHandler()
{
	switch ( iStatus.Int() )
	{
		case KErrNone:
		{
			roadmap_log( ROADMAP_DEBUG, "The frame decode is completed successfully" );
			
			delete iImgDecoder;
			
			if ( iThumbnailRequest )
			{
				iSaveBitmap->Create( iThumbnailSize, EColor16MA );
				// Clip the data
				Clip( iThumbnailSize, *iDecodeBitmap, *iSaveBitmap );
				// Store the thumbnail in buffer and finalize the engine work
				iThumbnailBuf->Copy( (TUint8*) iSaveBitmap->DataAddress(), iThumbnailBuf->Size() );
				Finalize();				
			}
			else
			{
				TRect rect = iCameraViewer->Rect();				
				if (  iTargetSize.iHeight > iTargetSize.iWidth ) // Portrait mode
				{
					iSaveBitmap->Create( iTargetSize, EColor16MA );
					Clip( iTargetSize, *iDecodeBitmap, *iSaveBitmap );					
				}
				else
				{
					iSaveBitmap->Duplicate( iDecodeBitmap->Handle() );
				}
				SaveBitmapAsync( iSaveBitmap, iFileName );
			}
			break;
		}
		case KErrUnderflow:
		{
			roadmap_log( ROADMAP_INFO, "The frame decoder underflow - continue decoding" );
			if ( iImgDecoder )
			{
				iImgDecoder->ContinueConvert( &iStatus );
			}
			SetActive();
			break;
		}
		default:
		{
			break;
		}
	
	}
}
/***********************************************************
 * Asynchronous callback for the encoder 
 */
void CWazeCameraEngine::EncodeHandler()
{
	switch ( iStatus.Int() )
	{
		case KErrNone:
		{
			roadmap_log( ROADMAP_DEBUG, "The encoding process is completed successfully" );
			delete iImgEncoder;
			/*
			 * The process is accomplished successfully
			 */
			Finalize();
			break;
		}
		default:
		{
			roadmap_log( ROADMAP_WARNING, "Encoder handler. Unhandled status: %d", iStatus.Int() );
			break;
		}
	
	}
}


/***********************************************************
 * Asynchronous callback
 */
void CWazeCameraEngine::RunL()
{	
   roadmap_log( ROADMAP_WARNING, "AGA DEBUG RunL, Status: %d. State: %d", iStatus.Int(), iState );
   
   switch ( iState )
   {
	   case EEngineStateDecode:
	   {			  
		   DecodeHandler();
		   break;
	   }
	   case EEngineStateEncode:
	   {
		   EncodeHandler();
		   break;
	   }

	   default: 
	   {
		   break;
	   }
   }
}

/***********************************************************
 * Start engine
 */
void CWazeCameraEngine::TakePictureL( TJpegQualityFactor aQuality, const TFileName& aFilePath, const TSize& aSize )
{
	// AGA DEBUG - Reduce level
	roadmap_log( ROADMAP_WARNING, "Starting the camera engine!" );

	if ( iState == EEngineStateNotReady )
	{
		StartEngine();
	}
	else
	{
		ResetEngine();
	}
	iJpegQuality = aQuality;
	iFileName = aFilePath;
	iState = EEngineStatePrepare;
	iTargetSize = aSize;
	
	roadmap_log( ROADMAP_DEBUG, "Camera reservation requested. The call blocked" );
	
	iWaiter->Start();
	
	StopEngine();
	
	roadmap_log( ROADMAP_WARNING, "Finalized taking the picture" );
}


/***********************************************************
 * Camera event handler
 */
void CWazeCameraEngine::HandleEvent(const TECAMEvent &aEvent )
{
	// AGA DEBUG : Remove this
	roadmap_log( ROADMAP_WARNING, "AGA DEBUG Engine event: [%x, %d]", aEvent.iEventType, aEvent.iErrorCode );
	
	if( aEvent.iEventType == KUidECamEventReserveComplete )
	{
		ReserveComplete( aEvent.iErrorCode );
		return;
	}

	if( aEvent.iEventType == KUidECamEventPowerOnComplete )
    {
       PowerOnComplete( aEvent.iErrorCode );
       return;
    }
	
#ifdef __ECAM_ADV_SETT__
   // AUTOFOCUS RELATED EVENTS
	switch( aEvent.iEventType.iUid )
	{
		// Set some basic settings automatically (AF Area, Focus Range)
		// after receiving a call to SetFocusMode()
		case KUidECamEventCameraSettingFocusModeUidValue:
		{
		  if ( KErrNone == aEvent.iErrorCode )
		  {
				iAdvSettings->SetAutoFocusArea(CCamera::CCameraAdvancedSettings::EAutoFocusTypeAuto);
		  }
		  else
		  {
				roadmap_log( ROADMAP_INFO, "Set focus mode failed (%d)", aEvent.iErrorCode );
		  }
		  break;
		}
		
		case KUidECamEventCameraSettingAutoFocusAreaUidValue:
		{
		  if ( !aEvent.iErrorCode )
		  {
			  iAdvSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeNormal);
		  }
		  else
		  {
			  roadmap_log( ROADMAP_INFO, "Set AF area failed (%d)", aEvent.iErrorCode );
		  }
		  break;
		}
		/*
		case KUidECamEventCameraSettingFocusRange2UidValue:
		{
		  if ( !aEvent.iErrorCode )
		  {
			  // Do nothing after AF Range is set.
			  // AF is activated by calling SetAutoFocusType()
		  }
		  else
		  {
			  __LOGSTR_TOFILE1("set focus range failed (%d)", aEvent.iErrorCode);
			  iRange = ECameraExFRUnknown;
		  }
		  // reset capture mode so the new focus range will be shown in UI
		//          iController->SetCaptureModeL( iCaptureSize,
		//                                       (TInt)iFormat,
		//                                       IsAutoFocusSupported() );
		  break;
		}
		
		case KUidECamEventCameraSettingAutoFocusType2UidValue:
		{
		  if ( !aEvent.iErrorCode )
			  {
			  __LOGSTR_TOFILE("AF ok");
			  }
		  else
			  {
			  __LOGSTR_TOFILE1("set af type (%d)", aEvent.iErrorCode);
			  }
		  break;
		  }
		
		// Optimal focus has been attained
		case KUidECamEventCameraSettingsOptimalFocusUidValue:
		{
		  OptimisedFocusComplete( aEvent.iErrorCode );
		  break;
		}
		
		case KUidECamEventCameraSettingFocusRangeUidValue:
		{
		// case KUidECamEventCameraSettingAutoFocusTypeUidValue:
		  // ---> These are deprecated - safe to ignore
		  break;
		}
		*/
		// Simply log UIDs of other events, there are events for exposure,
		// digital zoom etc coming in.. see ecamadvancedsettings.h
		default:
		{
//		  roadmap_log( ROADMAP_INFO, "[%x, %d]", aEvent.iEventType, aEvent.iErrorCode );
		  break;
		}
	}
	#endif // __ECAM_ADV_SETT__	
}


/***********************************************************
 * Reserve completion handler. Power on after successful registration
 */
void CWazeCameraEngine::ReserveComplete( TInt aError )
{
	roadmap_log( ROADMAP_INFO, "Camera reservation request complete. Status: %d", aError );

    HandleError( aError );
    if ( aError == KErrNone )
    {
        iCamera->PowerOn();
    }
}

/***********************************************************
 * Power on completed
 */
void CWazeCameraEngine::PowerOnComplete( TInt aError )
{
	roadmap_log( ROADMAP_INFO, "Power on request complete. Status: %d", aError );
	
    HandleError( aError );
    if ( aError == KErrNone )
	{
        // Start viewfinder
	    StartViewFinderL();
	}
}

/***********************************************************
 *  View finder launcher entry point
 */
void CWazeCameraEngine::StartViewFinderL()
{
	TCameraInfo info;
	TSize previewSize = PreviewSize();
	iCamera->CameraInfo( info );
	
    // Auto exposure
    TRAP_IGNORE( iCamera->SetExposureL( CCamera::EExposureAuto ) );
    // Auto whitebalance
	TRAP_IGNORE( iCamera->SetWhiteBalanceL( CCamera::EWBAuto ) );
    // Auto brightness
	TRAP_IGNORE( iCamera->SetBrightnessL( CCamera::EBrightnessAuto ) );
	// Auto contrast
	TRAP_IGNORE( iCamera->SetContrastL( CCamera::EContrastAuto ) );
    // Digital zoom	
    TRAP_IGNORE( iCamera->SetDigitalZoomFactorL( 0 ) );
    // Optical zoom
    TRAP_IGNORE( iCamera->SetZoomFactorL( info.iMinZoom ) );
    
#ifdef __ECAM_ADV_SETT__    
    SetFocusMode( CCamera::CCameraAdvancedSettings::EFocusModeAuto );
#endif __ECAM_ADV_SETT__    
    
    // Start to show view finder frames
    TRAPD( err, iCamera->StartViewFinderBitmapsL( previewSize ) );
    HandleError( err );
    if ( err != KErrNone )
	{
		roadmap_log( ROADMAP_ERROR, "Error starting view finder. Error: %d", err );
		// TODO :: Check if the engine has to be stopped here
		return;
	}
    
	iFormat = GetBestHWBmpFormat( info );
	iCamera->EnumerateCaptureSizes( iCaptureSize, KImageSizeCaptureIndex, iFormat );
	
    if ( info.iOptionsSupported & TCameraInfo::EImageCaptureSupported )
	{
		TRAPD( err, iCamera->PrepareImageCaptureL( iFormat, KImageSizeCaptureIndex ) );
		HandleError( err );
	}
    else	// TODO :: Check if handling of capture without viewfinder is relevant
	{
        User::Leave( KErrNotSupported );
    }
    HandleError( err );
    iState = EEngineStatePreview;    
}

/***********************************************************
 *  Engine is notified that there is buffer ready for the view finder
 */
void CWazeCameraEngine::ViewFinderReady( MCameraBuffer &aCameraBuffer, TInt aError )
{
	TInt err = KErrNone;
	CFbsBitmap* pBmp;
	TRAP_IGNORE( pBmp = &aCameraBuffer.BitmapL(0) );
	
	HandleError( aError );
	
	if ( aError == KErrNone && iState == EEngineStatePreview )
	{
	  // If on bitmap - nothing to do
	  if ( !pBmp )
	  {
		  roadmap_log( ROADMAP_ERROR, "Error receiving view finder bitmap from the camera!" );
		  return;
	  }
  	  iPreviewBitmap->Reset();	// Access count decreased
	  err = iPreviewBitmap->Duplicate( pBmp->Handle() ); // Access count increased

	  // Draw the buffer
	  DrawViewFinderBuffer();
	}
	aCameraBuffer.Release();
}


/***********************************************************
 *  Draws the buffer on the screen
 */
void CWazeCameraEngine::DrawViewFinderBuffer()
{
	iCameraViewer->DrawImage( *iPreviewBitmap );	
}

/***********************************************************
 *  Captures the image upon request
 */
void CWazeCameraEngine::CaptureL()
{
    if ( iState == EEngineStatePreview )
    {        
        iCamera->StopViewFinder();
        iCameraSound->PlaySound( KSnapSoundId );
        iCamera->CaptureImage();
        iState = EEngineStateCapture;
    }
    else
    {
    	roadmap_log( ROADMAP_WARNING, "Cannot take capture invalid engine state: %d", iState );
        User::Leave( KErrNotReady );
    }
}

void CWazeCameraEngine::ImageBufferReady( MCameraBuffer &aCameraBuffer, TInt aError )
{
  CFbsBitmap* bitmap = 0;
  TSize decodeSize = iTargetSize;
  TRAP_IGNORE( bitmap = &aCameraBuffer.BitmapL(0) );

  TRAP_IGNORE( iImageData = aCameraBuffer.DataL(0) );

  HandleError( aError );

  // If No bitmap - nothing to do
  if ( !bitmap && !iImageData)
  {
	  roadmap_log( ROADMAP_ERROR, "Error receiving capture data from the camera!" );
	  return;
  }

  // If no error
  if ( aError == KErrNone )
  {
	  TInt err = KErrNone;
	  if ( bitmap )
	  {
		  iPreviewBitmap->Reset();
		  err = iPreviewBitmap->Duplicate( bitmap->Handle() );
	  }
	  
	  if ( HandleError( err ) == KErrNone )
	  {
		  iState = EEngineStateSave;
		  DrawViewFinderBuffer();
	  }
	  /*
	   *  The image always landscape !!!
	   *  Therefore decode size should be landscape to preserve the ratio
	   */
	  if ( iTargetSize.iHeight > iTargetSize.iWidth )
	  {
		  decodeSize.SetSize( (4*iTargetSize.iHeight)/3, iTargetSize.iHeight );
	  }
	  DecodeImageAsync( decodeSize );
  }
}

/***********************************************************
 *  Ctor
 */
CWazeCameraEngine::CWazeCameraEngine() : 
					CActive( EPriorityStandard ), 
					iEnv( NULL ),
					iCamera( NULL ),
					iState( EEngineStateNotReady ),
					iReserved( FALSE ),
					iCaptureSize( TSize::EUninitialized ),
					iViewFinderSize( TSize::EUninitialized ),
					iPreviewBitmap( NULL ),
					iJpegQuality( JpegQualityLow ),
					iCameraViewer( NULL ),
					iWaiter( NULL ),
					iFormat( CCamera::EFormatExif ),
					iAdvSettings( NULL ),
					iZoomFactor( 0 ),
					iImageData( NULL ),
					iSaveBitmap( NULL ),
					iDecodeBitmap( NULL ),
					iThumbnailRequest( EFalse ),
					iThumbnailSize( TSize::EUninitialized )
{
}


/***********************************************************
 *  Handle converting request
 */
void CWazeCameraEngine::SaveBitmapAsync( CFbsBitmap* aBmp, TFileName &aFileName )
{
	TFileName fileName;
	char path[256];
	char *pPath = &path[0];
	GSConvert::TDes16ToCharPtr( aFileName, &pPath, EFalse );
	// AGA DEBUG - reduce level
	roadmap_log( ROADMAP_WARNING, "Saving the capture to the %s. Size: (%d, %d)", path, aBmp->SizeInPixels().iWidth, aBmp->SizeInPixels().iHeight );
	
	// AGA DEBUG - remove this
	SaveRawImage();
	
	iImgEncoder = CImageEncoder::FileNewL( iEnv->FsSession(), aFileName, KMimeType );
	iImgEncoder->Convert( &iStatus, *aBmp );
	
	iState = EEngineStateEncode;
	SetActive();
}

/***********************************************************
 *  Save the image as received from the camera - DEBUG purposes
 */
void CWazeCameraEngine::SaveRawImage()
{
	RFile fileImage;
	_LIT( KRawFile, "C:\\Data\\Others\\maps\\capture_temp_raw.jpg");
	TFileName filePath( KRawFile );
	
	if( iFormat == CCamera::EFormatExif )
	{
		TInt fErr = fileImage.Replace( iEnv->FsSession(), filePath, EFileWrite );
		if ( fErr == KErrNone ) 
		{
			fileImage.Write( *iImageData );
			roadmap_log( ROADMAP_DEBUG, "The image is written successfully" );
		}
		else
		{
			roadmap_log( ROADMAP_WARNING, "Error saving image. Error code: %d", fErr );
		}
		fileImage.Close();
	}
}

/***********************************************************
 *  Override CActive
 */
void CWazeCameraEngine::DoCancel()
{
}

/***********************************************************
 *  SetFocusMode 
 *  
 */
TFocusMode CWazeCameraEngine::SetFocusMode( TFocusMode aMode )
{
#ifdef __ECAM_ADV_SETT__
	TFocusMode mode = CCamera::CCameraAdvancedSettings::EFocusModeUnknown;
	if ( iAdvSettings )
	{
		iAdvSettings->SetFocusMode( aMode );
		mode = iAdvSettings->FocusMode(); 
	}
	return mode;	
#endif //__ECAM_ADV_SETT__	
}

/***********************************************************
 *  GetBestHWBmpFormat( const TCameraInfo& info ) const
 *  
 */
CCamera::TFormat CWazeCameraEngine::GetBestHWBmpFormat( const TCameraInfo& aInfo ) const
{
	if ( aInfo.iImageFormatsSupported & CCamera::EFormatExif )
    {
		return CCamera::EFormatExif;
    }
	if ( aInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor16M )
    {
		return CCamera::EFormatFbsBitmapColor16M;
    }
	else if ( aInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor64K )
    {
		return CCamera::EFormatFbsBitmapColor64K;
    }
	else
    {
		return CCamera::EFormatFbsBitmapColor4K;
    }
}


/***********************************************************
 *  Starts the auto focus
 */
void CWazeCameraEngine::StartFocusL()
{
#ifdef __ECAM_ADV_SETT__
    if ( iAdvSettings != NULL ) 
	{        
		iAdvSettings->SetAutoFocusType(CCamera::CCameraAdvancedSettings::EAutoFocusTypeSingle);
	}
#endif //__ECAM_ADV_SETT__    
}

/***********************************************************
 *  Stops the auto focus
 */
void CWazeCameraEngine::StopFocusL()
{
#ifdef __ECAM_ADV_SETT__
	if ( iAdvSettings != NULL )
	{
		iAdvSettings->SetAutoFocusType( CCamera::CCameraAdvancedSettings::EAutoFocusTypeOff );
	}
#endif //__ECAM_ADV_SETT__	
}


/***********************************************************
 *  Rectangle dimensions
 */
TSize CWazeCameraEngine::PreviewSize()
{
	TSize rect = iCameraViewer->Rect().Size();	
	if ( rect.iHeight > rect.iWidth )
	{ 
		TInt height = rect.iWidth*4/3;
		rect = TSize( height*4/3, height );	
	}
	return rect;
}
/***********************************************************
 *  Size change indication
 */
void CWazeCameraEngine::SizeChanged()
{
// 	StateAsync( EEngineStatePreviewRestart );
}

void CWazeCameraEngine::ZoomStepIn( void )
{
	this->ZoomStep( ETrue );
}

void CWazeCameraEngine::ZoomStepOut( void )
{
	this->ZoomStep( EFalse );
}


TBool CWazeCameraEngine::ViewFinderActive( void )
{
	TBool res;
	// The camera initialized and preview is started
	if ( iState > EEngineStateNotReady )
	{
		res = iCamera->ViewFinderActive();
	}
	return res;
}


void CWazeCameraEngine::ZoomStep( TBool aZoomIn )
{
	TCameraInfo info;
	iCamera->CameraInfo( info );
	// Both 0 and 1 indicate that zoom functionality is not supported
	///// Optical zoom
	if ( info.iMaxZoomFactor != 0 && info.iMaxZoomFactor != 1 )
    {
		if( aZoomIn && ( iZoomFactor < info.iMaxZoom ) )
        {
			iZoomFactor++;
        }
		if ( !aZoomIn && ( iZoomFactor > info.iMinZoom ) )
        {
			iZoomFactor--;
        }
		iCamera->SetZoomFactorL( iZoomFactor );		
    }
	///// Digital zoom
	else if ( info.iMaxDigitalZoomFactor != 0 && info.iMaxDigitalZoomFactor != 1 )
    {
		if ( aZoomIn && iZoomFactor < info.iMaxDigitalZoom )
        {
			iZoomFactor++;
        }
		if ( !aZoomIn && ( iZoomFactor > 0 ) )
        {
			iZoomFactor--;
        }
		iCamera->SetDigitalZoomFactorL( iZoomFactor );
    }
}

/***********************************************************
 *  Image decoding process launcher 
 *  
 */
void CWazeCameraEngine::DecodeImageAsync( const TSize& aNewSize )
{
	iDecodeBitmap->Create( aNewSize, EColor16MA );
	iImgDecoder = CImageDecoder::DataNewL( iEnv->FsSession(), *iImageData );
	
	iImgDecoder->Convert( &this->iStatus, *iDecodeBitmap, 0 );
	
	iState = EEngineStateDecode;
	
	SetActive();
}

/***********************************************************
 *  Finalize capturing process 
 *  
 */
void CWazeCameraEngine::Finalize()
{
	iState = EEngineStateIdle;
	iThumbnailRequest = EFalse;
	iThumbnailBuf = NULL;
	iSaveBitmap->Reset();
	iDecodeBitmap->Reset();	
	if ( iWaiter->IsStarted() )
	{
		iWaiter->AsyncStop();
	}
}

/***********************************************************
 *  Thumbnail request 
 *  
 */
TBool CWazeCameraEngine::GetThumbnail( TDes8* bufOut, const TFileName& aCaptureFilePath, const TSize& aThumbSize )
{
	RFile file;
	TBool res = EFalse;
	RBuf8  fileBuf;
	TInt  fileSize;
	CleanupClosePushL( fileBuf );
	if ( ( iState != EEngineStateIdle ) && ( iState != EEngineStateNotReady ) )
	{
		roadmap_log( ROADMAP_WARNING, "The engine is busy - cannot build thumbnail" );
		return res;
	}
	
	iThumbnailRequest = ETrue;
	iThumbnailBuf = bufOut;
	iThumbnailSize = aThumbSize;
	
	// Load the raw data from the file
	file.Open( iEnv->FsSession(), aCaptureFilePath, EFileRead );
	file.Size( fileSize );
	fileBuf.Create( fileSize );
	TInt fErr = file.Read( fileBuf );
	file.Close();
	if ( fErr != KErrNone )
	{
		roadmap_log( ROADMAP_WARNING, "Error reading the image for creating the thumbnail" );
		return res;
	}

	// Create the bitmap for the saving the encoded and resized image to
	iImageData = &fileBuf;
	
	// Define the destination size
	TSize decodeSize;
	GetThumbnailDecodeSize( aThumbSize, decodeSize );
	
	DecodeImageAsync( decodeSize );
	
	iWaiter->Start();
	
	iState = EEngineStateNotReady;
	
	// Clean up at the end ...
	CleanupStack::PopAndDestroy();
	
	return ETrue;
}



/***********************************************************
 *  Noise reduction - May not be supported in the previous SDK-s. 
 *  *** FFU ***
 */
void CWazeCameraEngine::SetNoiseReduction()
{
#ifdef __ECAM_ADV_SETT__
	RArray<TUid> suppTransforms; // array of supported transformations
	CleanupClosePushL( suppTransforms );
	
	iImgProcessing->GetSupportedTransformationsL( suppTransforms );
	if ( suppTransforms.Find( KUidECamEventImageProcessingNoiseReduction ) != KErrNotFound )
	{
		iImgProcessing->SetTransformationValue( KUidECamEventImageProcessingNoiseReduction, 
				CCamera::CCameraImageProcessing::ENoiseReductionBasic );		
	}
	CleanupStack::PopAndDestroy();
#endif // __ECAM_ADV_SETT__	
}

/***********************************************************
 *   
 *  
 */
void CWazeCameraEngine::Clip( const TSize& aSize, const CFbsBitmap& aBmpIn, CFbsBitmap& aBmpOut )
{
	TSize frameSize = aBmpIn.SizeInPixels();
	
	if ( aSize.iWidth > frameSize.iWidth || aSize.iHeight > frameSize.iHeight )
	{
		roadmap_log( ROADMAP_ERROR, "Cannot clip to the larger size. Current size = (%d,%d). Requested size = (%d,%d)",
				frameSize.iWidth, frameSize.iHeight, aSize.iWidth, aSize.iHeight );
		return;
	}
	
	TInt x1 = ( frameSize.iWidth-aSize.iWidth )/2;
	TInt x2 = x1+aSize.iWidth;
	TInt y1 = ( frameSize.iHeight - aSize.iHeight )/2;
	TInt y2 = y1 + aSize.iHeight;
	
	CFbsBitGc* fbsBitGc = CFbsBitGc::NewL(); //graphic context
	CleanupStack::PushL( fbsBitGc );
	CFbsBitmapDevice* imageDevice = CFbsBitmapDevice::NewL( &aBmpOut );
	CleanupStack::PushL( imageDevice );
	fbsBitGc->Activate( imageDevice );
	fbsBitGc->SetBrushColor( KRgbBlack );
	fbsBitGc->Clear();
	fbsBitGc->BitBlt( TPoint(0,0), &aBmpIn, TRect( x1, y1, x2, y2 ) );
	
	CleanupStack::PopAndDestroy( 2 );	// 	fbsBitGc, imageDevice
}


/***********************************************************
 *  Thumbnail decode size according to the requested thumbnail size and given 
 *  capture image
 *  
 */
void CWazeCameraEngine::GetThumbnailDecodeSize( const TSize& aThumbSize, TSize& aThumbDecodeSize ) const
{
	float xFactor = (float) iTargetSize.iWidth / (float) aThumbSize.iWidth;
	float yFactor = (float) iTargetSize.iHeight / (float) aThumbSize.iHeight;
	// Apply the smallest
	float xyFactor = xFactor < yFactor ? xFactor : yFactor;
	
	int decodeWidth = (float) iTargetSize.iWidth / xyFactor;
	int decodeHeight = (float) iTargetSize.iHeight / xyFactor;
	
	aThumbDecodeSize.SetSize( decodeWidth, decodeHeight );
	
}

#endif // __CAMERA_ENABLED__
