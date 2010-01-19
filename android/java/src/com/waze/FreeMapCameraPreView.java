/**  
 * FreeMapCameraPreView.java
 * This class represents the android layer activity wrapper for the SurfaceView.  
 * Responsible for the displaying of the camera preview to the screen and processing of the 
 * shots. 
 * 
 * LICENSE:
 *
 *   Copyright 2008 	@author Alex Agranovich
 *   
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
 *   @see FreeMapCameraView.java
 */

package com.waze;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Bitmap.CompressFormat;
import android.hardware.Camera;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FreeMapCameraPreView extends SurfaceView implements
        SurfaceHolder.Callback
{

    static
    {
        // Create the static capture parameters object
        // Can be initialized before CameraPreview is created
        FreeMapCameraPreView.mCaptureParams = new FreeMapCameraPreView.CaptureParams();
        FreeMapCameraPreView.mBufOS = new ByteArrayOutputStream();
    }

    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */

    public FreeMapCameraPreView(Context aContext)
    {
        super(aContext);
        setFocusableInTouchMode(true);
        // This class implements the SurfaceHolder.Callback to be notified on
        // changes in the underlying
        // surface
        mHolder = getHolder();
        mHolder.addCallback(this);
        // We have no own buffer
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    /*************************************************************************************************
     * surfaceCreated
     * 
     * Update the camera with the surface to draw the preview on it
     */
    public void surfaceCreated( SurfaceHolder aHolder )
    {
        try
        {
            System.gc();
            mCamera = Camera.open();
            mCamera.setPreviewDisplay( aHolder );
            mPreviewStatus = CAMERA_PREVIEW_STATUS_PREPARED;
        }
        catch( Exception ex )
        {
            WazeLog.e( "Error in creating the surface", ex );
            // Return to the application
            mPreviewStatus = CAMERA_PREVIEW_STATUS_NOT_READY;
            mCaptureStatus = CAMERA_CAPTURE_STATUS_NONE;
            // This will call the surfaceDestroy
            FreeMapNativeManager.Notify(0);
        }
    }

    /*************************************************************************************************
     * surfaceDestroyed Stop the camera when the surface is destroyed
     */
    public void surfaceDestroyed( SurfaceHolder aHolder )
    {    
        if ( mCamera != null )
        {
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;                        
        }
        mPreviewStatus = CAMERA_PREVIEW_STATUS_NOT_READY;
        // In case of manager waiting - release to continue
        FreeMapNativeManager.Notify(0);        
    }

    /*************************************************************************************************
     * surfaceChanged Set up the camera parameters and begin the preview
     */
    public void surfaceChanged( SurfaceHolder aHolder, int aFormat, int aWidth,
            int aHeight )
    {
        if ( mPreviewStatus == CAMERA_PREVIEW_STATUS_PREPARED )
        {
            if ( (mCaptureParams.mImageWidth >= 0)
                    || (mCaptureParams.mImageHeight >= 0))
            {
                Camera.Parameters parameters = mCamera.getParameters();
                parameters.setPreviewSize(aWidth, aHeight);
                parameters.setPictureFormat(PixelFormat.RGB_565);
                mCamera.setParameters(parameters);
                mCaptureStatus = CAMERA_CAPTURE_STATUS_NONE;
                mPreviewStatus = CAMERA_PREVIEW_STATUS_ACTIVE;
                mCamera.startPreview();
            }
            else
            {
                WazeLog.w( "Requested image dimensions are invalid. Width: "
                        + mCaptureParams.mImageWidth + ". Height: "
                        + mCaptureParams.mImageHeight );
            }
        }
        else
        {
            WazeLog.w( "Camera preivew is not ready!" );
        }
    }

    /*************************************************************************************************
     * onKeyDown Trigger the capture process
     */
    @Override
    public boolean onKeyDown( int aKeyCode, KeyEvent aEvent )
    {
        try
        {            
            if ( ( (aKeyCode == KeyEvent.KEYCODE_CAMERA)
                    || (aKeyCode == KeyEvent.KEYCODE_DPAD_CENTER) ) && ( mCaptureStatus == CAMERA_CAPTURE_STATUS_NONE ) )
            {
                
                mCamera.takePicture(null, null, new CaptureCallback());
                mCaptureStatus = CAMERA_CAPTURE_STATUS_IN_PROCESS;
                return true;
            }
            else
            {
                return super.onKeyDown(aKeyCode, aEvent);
            }
        }
        catch( Exception ex )
        {
            WazeLog.e( "Error in creating the surface", ex );
            // Return to the application
            mPreviewStatus = CAMERA_PREVIEW_STATUS_NOT_READY;
            mCaptureStatus = CAMERA_CAPTURE_STATUS_NONE;
            // This will call the surfaceDestroy
            FreeMapNativeManager.Notify(0);
            return true;
        }
    };

    /*************************************************************************************************
     * getPreviewActive Inspects if the preview is active
     * 
     */
    public static boolean getPreviewActive()
    {
        return ( mPreviewStatus == CAMERA_PREVIEW_STATUS_ACTIVE );
    }

    /*************************************************************************************************
     * getCaptureStatus Returns if the capturing was successful
     * 
     */
    public static boolean getCaptureStatus()
    {
        return ( mCaptureStatus == CAMERA_CAPTURE_STATUS_SUCCESS );
    }

    /*************************************************************************************************
     * This class represents the API parameters for the capture
     * 
     * @author Alex Agranovich
     */
    public static class CaptureParams
    {
        public void setImageWidth( int aVal )
        {
            mImageWidth = aVal;
        }

        public void setImageHeight( int aVal )
        {
            mImageHeight = aVal;
        }

        public void setImageQuality( int aVal )
        {
            mImageQuality = aVal;
        }

        public void setImageFolder( String aVal )
        {
            mImageFolder = aVal;
        }

        public void setImageFile( String aVal )
        {
            mImageFile = aVal;
        }

        private int    mImageWidth   = 256;
        private int    mImageHeight  = 256;
        private int    mImageQuality = 50;
        private String mImageFolder  = "./";
        private String mImageFile    = "temp.jpg";
    }

    /*************************************************************************************************
     * CaptureConfig - Capture parameters configurator
     * 
     * @param aImageWidth
     *            - target image width
     * @param aImageHeight
     *            -target image height
     * @param aImageQuality
     *            - the compression quality
     * @param aImageFolder
     *            - the full folder path for the target file
     * @param aImageFile
     *            - the file name for the target file
     */
    public static void CaptureConfig( int aImageWidth, int aImageHeight,
            int aImageQuality, String aImageFolder, String aImageFile )
    {
//        Log.d(LOG_TAG, "CaptureConfig. Width: " + aImageWidth + ". Height: "
//                + aImageHeight);
        mCaptureParams.setImageWidth(aImageWidth);
        mCaptureParams.setImageHeight(aImageHeight);
        mCaptureParams.setImageQuality(aImageQuality);
        mCaptureParams.setImageFolder(aImageFolder);
        mCaptureParams.setImageFile(aImageFile);
    }

    /*************************************************************************************************
     *================================= Private interface section
     * =================================
     */
    /*************************************************************************************************
     * CaptureCallback Main logic of taking the picture from the hardware and
     * save it to the sdcard
     */
    private final class CaptureCallback implements Camera.PictureCallback
    {
        public void onPictureTaken( byte[] aData, Camera aCamera )
        {
            // Decode the JPEG result to the bitmap
            Bitmap imageBitmap = BitmapFactory.decodeByteArray(aData, 0,
                    aData.length);
            // Scale the bitmap to the destination size
            // Here the image in the landscape => horizontal scaling according
            // to height
            Bitmap scaledBitmap = Bitmap.createScaledBitmap(imageBitmap,
                    mCaptureParams.mImageHeight, mCaptureParams.mImageWidth,
                    true);
            // Force GC
            imageBitmap.recycle();
            // Rotate the image to be portrait
            Canvas canvas = new Canvas(scaledBitmap);
            canvas.rotate(90);
            canvas.save();
            scaledBitmap = Bitmap.createBitmap(scaledBitmap, 0, 0, scaledBitmap
                    .getWidth(), scaledBitmap.getHeight(), canvas.getMatrix(),
                    true);

            // Saving the bitmap to the file
            CompressToBuffer(scaledBitmap, CompressFormat.JPEG);
            mBitmapOut = scaledBitmap;
            // Notify the manager (Assumption is that the callback have taken
            // some time. So the delay is a bit smaller )
            mCaptureStatus = CAMERA_CAPTURE_STATUS_SUCCESS;
            // Release the camera (surface destroyed) and return to the main
            // activity
            FreeMapNativeManager.Notify(MainScreenShowDelay);
        }
    }

    /*************************************************************************************************
     * CompressToBuffer Save bitmap to the buffer using the requested format
     * 
     * @param aBitmap
     *            - the bitmap to save
     * @param aFormat
     *            - the compression format
     */
    private void CompressToBuffer( Bitmap aBitmap, CompressFormat aFormat )
    {
        synchronized ( mBufOS )
        {
            // Reset the stream
            mBufOS.reset();
            // Compress to the stream
            aBitmap.compress( aFormat, mCaptureParams.mImageQuality, mBufOS );
        }
    }

    /*************************************************************************************************
     * SaveToFile Saves the buffer to the file using the path given in the
     * configuration Thread safe
     */
    public static void SaveToFile()
    {
        // No data - return
        if (mBufOS.size() <= 0)
            return;

        try
        {
            // Build the path
            String fullPath = new String(mCaptureParams.mImageFolder);
            String imageFileName = new String(mCaptureParams.mImageFile);
            fullPath += String.valueOf("/");
            fullPath += imageFileName;

            // Check the directories
            File imageFile = new File(fullPath);
            if (imageFile.getParentFile() != null)
            {
                imageFile.getParentFile().mkdirs();
            }

            // Open the stream
            FileOutputStream fileOS = new FileOutputStream(imageFile);
            // Write the file
            FileChannel fileChannel = fileOS.getChannel();
            synchronized (mBufOS)
            {
                fileChannel.write(ByteBuffer.wrap(mBufOS.toByteArray()));
            }
            fileOS.close();
        }
        catch (Exception ex)
        {
            WazeLog.e("Error in writing the file to the disk. ", ex );
            ex.printStackTrace();
        }

    }

    /*************************************************************************************************
     * ReleaseBuf - releases the buffer memory
     */
    public static void ReleaseBuf()
    {
        synchronized (mBufOS)
        {
            try
            {
                mBufOS.close();
            }
            catch (Exception ex)
            {
                WazeLog.e( "Cannot release the buffer. ", ex );
                ex.printStackTrace();
            }
        }
    }

    /*************************************************************************************************
     * GetBufSize Returns the size of the current capture in bytes Thread safe
     */
    public static int GetBufSize()
    {
        int size;
        synchronized (mBufOS)
        {
            size = mBufOS.size();
        }
        return size;
    }

    /*************************************************************************************************
     * GetCaptureBuffer Returns the buffer containing the image data Thread safe
     */
    public static byte[] GetCaptureBuffer()
    {
        byte[] byteArray;
        synchronized (mBufOS)
        {
            byteArray = mBufOS.toByteArray();
        }
        return byteArray;
    }

    /*************************************************************************************************
     * GetThumbnail Returns the buffer containing the thumbnail data Thread safe
     */
    public static int[] GetThumbnail( int aThumbWidth, int aThumbHeight )
    {
        Bitmap bmScaled;
        int destWidth, destHeight;
        Paint paint = new Paint();
        
        if ( mBitmapOut == null )
            return null;
        
        if ( mPreserveWholeImage )
        {
            // Get the destination dimensions preserving the
            if (aThumbWidth / mCaptureParams.mImageWidth < aThumbHeight / mCaptureParams.mImageHeight)
            {
                destWidth = aThumbWidth;
                destHeight = (aThumbHeight * aThumbWidth)
                        / mCaptureParams.mImageWidth;
            }
            else
            {
                destHeight = aThumbHeight;
                destWidth = (aThumbHeight * aThumbWidth)
                        / mCaptureParams.mImageHeight;
            }
        }
        else
        {
            destWidth = aThumbWidth;
            destHeight = (mCaptureParams.mImageHeight * aThumbWidth)
            / mCaptureParams.mImageWidth;
        }
        // Scaling the bitmap
        synchronized (mBufOS)
        {
            bmScaled = Bitmap.createScaledBitmap(mBitmapOut, destWidth,
                    destHeight, true);
        }

        if ( destHeight > aThumbHeight )
        {
            int margin = ( destHeight - aThumbHeight )/2;
            ( new Canvas( bmScaled ) ).clipRect( 0, margin-1, bmScaled.getWidth()-1, bmScaled.getHeight()-margin );
        }
        
        // Create the bitmap
        Bitmap bmRes = Bitmap.createBitmap(aThumbWidth, aThumbHeight,
                Bitmap.Config.ARGB_8888);

        // Find the center
        float left = ((aThumbWidth - destWidth) / 2);
        float top = ((aThumbHeight - destHeight) / 2);

        // Color filter - replace blue and red to met bgra format in the agg
        ColorMatrix cm = new ColorMatrix();
        cm.set(new float[] { 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                0, 1, 0, });

        // Attach the bitmap to the canvas
        Canvas canvas = new Canvas(bmRes);
        // Fill the bitmap
        canvas.drawARGB(0, 0, 0, 0);
        // Set the filter
        paint.setColorFilter(new ColorMatrixColorFilter(cm));
        // Draw
        canvas.drawBitmap(bmScaled, left, top, paint);
        // Extract the array of pixels
        int[] pixBufInt = new int[aThumbHeight * aThumbWidth];
        bmRes.getPixels(pixBufInt, 0, bmRes.getWidth(), 0, 0, bmRes.getWidth(),
                bmRes.getHeight());
        // Force GC
        bmScaled.recycle();
        bmRes.recycle();
        return pixBufInt;
    }

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     */

    private static final int CAMERA_PREVIEW_STATUS_NOT_READY = 0;
    private static final int CAMERA_PREVIEW_STATUS_PREPARED = 1;
    private static final int CAMERA_PREVIEW_STATUS_ACTIVE = 2;
    
    private static final int CAMERA_CAPTURE_STATUS_NONE = 0;
    private static final int CAMERA_CAPTURE_STATUS_IN_PROCESS = 1;
    private static final int CAMERA_CAPTURE_STATUS_SUCCESS = 2;
//    private static final int CAMERA_CAPTURE_STATUS_FAILURE = 3;
    
	SurfaceHolder mHolder; // The customized holder of the surface
	Camera mCamera; // The camera object
	private static final int MainScreenShowDelay = 1 * 100; // 0.5 second delay
//    private static final String          LOG_TAG             = "FreeMapCameraPreView";
    private static int mPreviewStatus = CAMERA_PREVIEW_STATUS_NOT_READY; // True = success
	private static int mCaptureStatus = CAMERA_CAPTURE_STATUS_NONE; // 
	private static boolean mPreserveWholeImage = false; 
    private static ByteArrayOutputStream mBufOS;
    private static Bitmap                mBitmapOut;

    
    // Capture params
    private static CaptureParams         mCaptureParams;
}
