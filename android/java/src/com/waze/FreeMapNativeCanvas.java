/**  
 * FreeMapNativeAggView.java
 * Main view class prepares the canvas for the displaying on the device. Interacting with the 
 * native layer
 *  
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
 *   @see FreeMapAppView.java
 */

package com.waze;

import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Field;
import java.nio.MappedByteBuffer;

import com.waze.FreeMapNativeManager.FreeMapUIEvent;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public final class FreeMapNativeCanvas
{

    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */
    public FreeMapNativeCanvas(FreeMapNativeManager aNativeManager)
    {
        mNativeManager = aNativeManager;
        // The library should be loaded
        InitCanvasNTV();
        
        mEglManager = new WazeEglManager( FreeMapAppService.getAppView() );
        
        // Buffer allocation
//        AllocateCanvasBuffer(mNativeManager.getAppView().getMeasuredWidth(),
//                mNativeManager.getAppView().getMeasuredHeight());
        
        // Draw the splash
//        DrawSplash2Buffer();
    }

    /*************************************************************************************************
     * ForceNewCanvas - force configuration and redrawing of the new canvas
     * 
     */
    public void ForceNewCanvas()
    {
        if ( !mNativeManager.getInitializedStatus() )
        {
            return;
        }
        FreeMapAppView lAppView = mNativeManager.getAppView();
        // Initialize the canvas
        ConfigureNativeCanvas( lAppView.getMeasuredWidth(), lAppView
                .getMeasuredHeight() );
        // Application start
//        Log.d("FreeMapAppActivity", "Force new native canvas creation!!! Width: " + lAppView.getMeasuredWidth()
//                                                                        + "Height: " + lAppView.getMeasuredHeight() );
        CreateCanvasNTV();
        // Force refresh
        // mNativeManager.setCanvasBufReady( true );
        //mNativeManager.setScreenRefresh( true );
    }

    public boolean CreateCanvas()
    {
    	return CreateCanvasInternal( false );
    }
    
    /*************************************************************************************************
     * 
     * 
     */
    private boolean CreateCanvasInternal( boolean aIsResize )
    {
    	boolean res = true;
    	if ( !mCanvasReady ) 
    	{
    		FreeMapAppActivity lActivity = FreeMapAppService.getAppActivity();
    		
	        WazeLog.i( "Initializing native canvas with " + mNativeManager.getAppView().getMeasuredWidth() + 
	        		", " + mNativeManager.getAppView().getMeasuredHeight() );
	        
	        View lView = lActivity.getCurrentView(); 
	        
	        // Prepare the context - the view surface has to be ready here 
	        if ( aIsResize )
	        	res = mEglManager.CreateEglSurface();
	        else
	        	res = mEglManager.InitEgl();
	        
	        if ( res )
	        {
	        	// Initialize the canvas
	        	ConfigureNativeCanvas( lView.getMeasuredWidth(), lView.getMeasuredHeight() );
	        
	        	// Create new canvas on the native layer side
	        	CreateCanvasNTV();
	        	mCanvasReady = true;
	        }
	        else
	        {
	        	mCanvasReady = false;
	        	WazeLog.e( "[???????] Fatal error: Egl surface initialization failed. Exiting the application!!!" );
	        	Log.e( "WAZE", "Egl surface initialization failed. Exiting the application!!!" );
	        	ShowCreateSurfaceError();		        
	        }
    	}
    	return res;
    }

    /*************************************************************************************************
     * Wrapper for the EGL engine swap buffers procedure
     * Called from the native side  
     */
    public void SwapBuffersEgl()
    {
    	if ( mFirstSwapBuffers )
    	{
        	final FreeMapAppView appView = FreeMapAppService.getAppView();
        	final FreeMapAppActivity appActivity = FreeMapAppService.getAppActivity();
    		appView.PostRemoveSplash();
    		mFirstSwapBuffers = false;
    		appActivity.setRequestedOrientation( ActivityInfo.SCREEN_ORIENTATION_USER );
    	}
    	mEglManager.SwapBuffersEgl();
    }
    
    /*************************************************************************************************
     * Returns the splash buffer in bitmap or null if failure
     */
    public Bitmap GetSplashBmp()
    {        
        FreeMapAppActivity lAppActivity = mNativeManager.getAppActivity();
        View lView = lAppActivity.getCurrentView();
        Bitmap bmScaled = GetSplashBmp( lAppActivity, lView );        

        return bmScaled;
    }
    /*************************************************************************************************
     * ConfigureNativeCanvas - Configures the native canvas with the parameters
     * obtained from the device
     * 
     * @param aCanvasHeight
     *            - actual canvas height
     * @param aCanvasWidth
     *            - actual canvas width
     * 
     */
    public void ConfigureNativeCanvas( int aCanvasWidth, int aCanvasHeight )
    {
        Display lDisplay = mNativeManager.getAppActivity().getWindowManager()
                .getDefaultDisplay();
        
        if ( mCanvasWidth != -1 )
        {
        	mIsOrientionUpdate = ( mCanvasWidth > mCanvasHeight )  ^ 
        									( aCanvasWidth > aCanvasHeight );
        }
        
        mCanvasWidth = aCanvasWidth;
        mCanvasHeight = aCanvasHeight;
        
        
//            AllocateCanvasBuffer(aCanvasWidth, aCanvasHeight);
        CanvasConfigureNTV( mCanvasWidth, mCanvasHeight, lDisplay.getPixelFormat() );
    }
    
    /*************************************************************************************************
     * Destroys the OGL canvas
     * 
     * 
     */
    public void DestroyCanvas()
    {
		mEglManager.DestroyEgl();
    	mCanvasReady = false;
    }   
    /*************************************************************************************************
     * Resize the OGL canvas
     * 
     * 
     */
    public void ResizeCanvas()
    {
    	mEglManager.DestroyEglSurface();
    	mCanvasReady = false;
    	CreateCanvasInternal( true );
    }   
 
    public int getPixelFormat()
    {
        return mDisplay.getPixelFormat();
    }
    public boolean getCanvasReady()
    {
        return mCanvasReady;
    }
    
    /*************************************************************************************************
     * Updates the canvas with the buffer
     * 
     * @param aCanvas
     *            - active canvas to be updated
     * 
     * 
     */
    public boolean UpdateCanvas( Canvas aCanvas )
    {
        Paint lPaint;
        // Rect clipRgn = aCanvas.getClipBounds();
       
        try
        {
            // //////////////////////////////////////////////////////////////
            // Create the paint 
            lPaint = new Paint();
            lPaint.setAntiAlias( false );

            // Draw the bitmap
            synchronized ( this )
            {
        		if ( mDoubleBuffer )
        		{
        			aCanvas.drawBitmap(mCacheCanvas, 0, 0, lPaint);
        		}
        		else
        		{
        			aCanvas.drawBitmap( mJCanvas, 0, mCanvasWidth, 0, 0, mCanvasWidth, mCanvasHeight, true, null );
        		}
            }

            // Update refresh to false - the canvas has been just refreshed
            mNativeManager.setScreenRefresh(false);
           return true;
        }
        catch (Exception ex)
        {
            WazeLog.w( "UpdateCanvas ", ex );
        }
        return false;
    }

    /*************************************************************************************************
     * Key event override - updates the native layer with the key event data
     */
    public boolean OnKeyDownHandler( KeyEvent aEvent )
    {
        int aKeyCode = aEvent.getKeyCode();
        // Trying to get the unicode character
        int unicodeChar = aEvent.getUnicodeChar();
        String chars = aEvent.getCharacters();
        byte utf8Bytes[] = new byte[64];
        boolean isSpecalButton;

        
        // Log.i( "onKeyDown", "Key Code : " + String.valueOf( aKeyCode ) );
        if ( unicodeChar != 0 || chars != null ) // True if the unicode character is mapped
        {
            String strUnicode;
            int codePoints[] = new int[1];
            isSpecalButton = false;
            
            if ( unicodeChar != 0 )
            {
	            codePoints[0] = unicodeChar;
	            strUnicode = new String(codePoints, 0, 1);
            }
            else
            {
            	strUnicode = chars;
            }
            try
            {
                byte tmpBuf[] = strUnicode.getBytes("UTF-8");
                for (int i = 0; i < tmpBuf.length && i < 64; ++i)
                {
                    utf8Bytes[i] = tmpBuf[i];
                }
            }
            catch (UnsupportedEncodingException ex)
            {
                WazeLog.e( "The conversion to the unicode cannot be performed for " + strUnicode, ex );
            }
        }
        else
        // The special button
        {
            isSpecalButton = true;
        }

        // Native layer will handle the event
        KeyDownHandlerNTV(aKeyCode, isSpecalButton, utf8Bytes);

        return true;
    }

    /*************************************************************************************************
     * Touch event override Updates the native layer with the motion (touch )
     * event data
     */
    public boolean OnTouchEventHandler( MotionEvent aEvent )
    {
        // Log.i( "Sending Touch Event", "Event : " + String.valueOf(
        // aEvent.getAction() ) +
        // " X: " + String.valueOf( (int) aEvent.getX() ) +
        // " Y: " + String.valueOf( (int) aEvent.getY() ) );
        switch (aEvent.getAction())
        {
            case MotionEvent.ACTION_DOWN:
            {
                MousePressedNTV((int) aEvent.getX(), (int) aEvent.getY());
                break;
            }
            case MotionEvent.ACTION_UP:
            {
                MouseReleasedNTV((int) aEvent.getX(), (int) aEvent.getY());
                break;
            }
            case MotionEvent.ACTION_MOVE:
            {
                MouseMovedNTV((int) aEvent.getX(), (int) aEvent.getY());
                break;
            }
        }
        return true;
    }

    /*************************************************************************************************
     * Indicator for the screen refresh. This routine updates the mJCanvas from
     * the native layer Thread safe. Implicit update of the mJCanvas is
     * performed on the native side
     */
    public void RefreshScreen()
    {
    	
    	mNativeManager.setScreenRefresh(true);
        try
        {
            synchronized (mNativeManager)
            {
            }
            
        }
        catch (Exception ex)
        {
            WazeLog.e( "Error waiting for the rendering. ", ex );
            ex.printStackTrace();
        }
    }

    /*************************************************************************************************
     * Canvas reference modifier
     */
    public void setCanvas( Canvas aCanvas )
    {
        mCanvas = aCanvas;
    }

    /*************************************************************************************************
     *================================= Private interface section     =================================
     */

    /*************************************************************************************************
     * Draws the splash to the java side buffer Note, that mJCanvas can be
     * accessed from different threads, however the splash is shown while canvas
     * object creation so the functions can not be called on it THe synchronized
     * is stated just to point out the issue.
     */
    private void DrawSplash2Buffer()
    {
        FreeMapAppView lAppView = mNativeManager.getAppView();
        FreeMapAppActivity lAppActivity = mNativeManager.getAppActivity();
        AssetManager assetMgr = lAppActivity.getAssets();        
        InputStream splashStream = null;
        // Select landscape or portrait
        try
        {
	        if ( lAppView.getMeasuredWidth() > lAppView.getMeasuredHeight() )
	        {

	            splashStream = assetMgr.open( FreeMapResources.GetSplashPath( true ) );
	        }
	        else
	        {
	        	splashStream = assetMgr.open( FreeMapResources.GetSplashPath( false ) );
	        }
        }
        catch( Exception ex )
        {
        	Log.w("WAZE", "Error loading splash screen! " + ex.getMessage() );
        	ex.printStackTrace();
        }
        if ( splashStream != null )
        {
	        // Load the resource
	        Bitmap bm = BitmapFactory.decodeStream( splashStream );
	
	        // Set the buffer
	        synchronized ( this )
	        {
	        	if  ( mDoubleBuffer )
	        	{
	        		mCacheCanvas = Bitmap.createScaledBitmap( bm, 
	        				lAppView.getMeasuredWidth(), lAppView.getMeasuredHeight(), false );
	        	}
	        	else
	        	{
		            Bitmap bmScaled = Bitmap.createScaledBitmap(bm, lAppView
		                    .getMeasuredWidth(), lAppView.getMeasuredHeight(), false);
		            bmScaled.getPixels( mJCanvas, 0, bmScaled.getWidth(), 0, 0,
		            					bmScaled.getWidth(), bmScaled.getHeight() );
	        	}
	        }
	        // Force refresh
	        mNativeManager.setScreenRefresh( true );
	        // mContext.getWindow().getWindowManager().getDefaultDisplay().getOrientation();
        }
    }

    /*************************************************************************************************
     * Returns the splash buffer in bitmap or null if failure
     */
    private int GetSurfaceId( SurfaceView aView )
    {
        int surfaceId = -1;        	
        try
        { 
            SurfaceHolder holder = aView.getHolder();
        	Surface surface = holder.getSurface();
        	Field surfaceIdFld = Surface.class.getDeclaredField( "mSurface" );
        	surfaceIdFld.setAccessible( true );
        	surfaceId = surfaceIdFld.getInt( surface );
        	surfaceIdFld.setAccessible( false );

        }
        catch( Exception ex )
        {
        	WazeLog.e( "Cannot get SurfaceId", ex );
        }
        return surfaceId;
    }
    
    /*************************************************************************************************
     * Returns the splash buffer in bitmap or null if failure
     */
    public static Bitmap GetSplashBmp( Activity aContext, View aView )
    {
        View lAppView = aView;
        Activity lAppActivity = aContext;
        AssetManager assetMgr = lAppActivity.getAssets();        
        InputStream splashStream = null;
        Bitmap bmScaled = null;        
        // Select landscape or portrait
        try
        {
	        if ( lAppView.getMeasuredWidth() > lAppView.getMeasuredHeight() )
	        {

	            splashStream = assetMgr.open( FreeMapResources.GetSplashPath( true ) );
	        }
	        else
	        {
	        	splashStream = assetMgr.open( FreeMapResources.GetSplashPath( false ) );
	        }
        }
        catch( Exception ex )
        {
        	Log.w("WAZE", "Error loading splash screen! " + ex.getMessage() );
        	ex.printStackTrace();
        }
        if ( splashStream != null )
        {
        	BitmapFactory.Options options = new BitmapFactory.Options();
        	options.inPreferredConfig = Config.ARGB_8888;
	        // Load the resource
	        bmScaled = BitmapFactory.decodeStream( splashStream, null, options );
//	        	bmScaled = Bitmap.createScaledBitmap(bm, lAppView
//		                    .getMeasuredWidth(), lAppView.getMeasuredHeight(), false);
//            bm.recycle();
        }
                
        return bmScaled;
    }
    
    /*************************************************************************************************
     * Returns the next power of two of the parameter
     */
    public static int nextPowerOf2( int aNum ) 
    {
    	   int outNum = aNum;

    	   --outNum;
    	   outNum |= outNum >> 1;
    	   outNum |= outNum >> 2;
    	   outNum |= outNum >> 4;
    	   outNum |= outNum >> 8;
    	   outNum |= outNum >> 16;
    	   outNum++;
    	   return outNum;
	};
    /*************************************************************************************************
     * Update snative layer with the orientation change
     * Resets the update flag
     */
	public void CanvasOrientationUpdate()
	{
		if ( mIsOrientionUpdate )
		{
			CanvasOrientationUpdateNTV();			
		}
		mIsOrientionUpdate = false;
	}
	
    /*************************************************************************************************
     * Allocation of the buffer to be filled by the native side. If allocation
     * was necessary returns true, if already allocated returns false Not thread
     * safe !
     */
    boolean AllocateCanvasBuffer( int aCanvasWidth, int aCanvasHeight )
    {
        boolean lAllocationPerformed = false;
        if (mCanvasWidth != aCanvasWidth || mCanvasHeight != aCanvasHeight)
        {
            mCanvasWidth = aCanvasWidth;
            mCanvasHeight = aCanvasHeight;

            // Reallocate the canvas buffer
            mJCanvas = new int[mCanvasWidth * mCanvasHeight];
            
            if ( mDoubleBuffer )
            {
	            if ( mCacheCanvas != null )
	                mCacheCanvas.recycle();
	            // Create the bitmap
	            mCacheCanvas = Bitmap.createBitmap(mCanvasWidth, mCanvasHeight,
	                    Bitmap.Config.ARGB_8888);
            }
            lAllocationPerformed = true;
        }
        return lAllocationPerformed;
    }

    

    /*************************************************************************************************
     * Shows the alert message box for the GPU access error
     * 
     */
    public void ShowCreateSurfaceError()
    {
    		final View view = FreeMapAppService.getAppView();
    		final Activity context = FreeMapAppService.getAppActivity();
    		Runnable errDlg = new Runnable() 
    		{
    			public void run()
    	    	{
				    AlertDialog dlg;
	        		AlertDialog.Builder dlgBuilder = new AlertDialog.Builder( context );
	        		FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
	        		
	        		dlg = dlgBuilder.create();
	        		
	        		final String btnLabel = new String( "Ok" );
	        		final String title = new String( "Unable to run waze" );
	        		final String msgText = new String( "Ooops, we've encountered a problem. \n" +  
	        											"We suggest you restart your phone to try again." );
	        		Message msg = mgr.GetUIMessage( FreeMapUIEvent.UI_EVENT_STARTUP_GPUERROR );
	    			
	    			dlg.setTitle( title );
	    			dlg.setButton( DialogInterface.BUTTON_POSITIVE, btnLabel, msg );
	    			dlg.setMessage( msgText );
	    			dlg.setIcon( android.R.drawable.ic_dialog_alert );
	    			dlg.show();	    			
    	    	}	        	
    		};
		view.post( errDlg );
    }

    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void InitCanvasNTV();

    private native void CanvasConfigureNTV( int aWidth, int aHeight, int aPixelFormat );

    private native void CreateCanvasNTV();
    
    public native void CanvasPrepareNTV();
    
    public native void CanvasOrientationUpdateNTV();
    
    public native void CanvasContextNTV();
    
    public native void CanvasRenderNTV();

    private native void KeyDownHandlerNTV( int aKeyCode,
            boolean isSpecalButton, byte utf8Bytes[] );

    private native void MousePressedNTV( int aX, int aY );

    private native void MouseReleasedNTV( int aX, int aY );

    private native void MouseMovedNTV( int aX, int aY );

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private FreeMapNativeManager mNativeManager;     // Main application
                                                      // activity

    private Display              mDisplay;
    private int                  mJCanvas[]   = null; // The Java side android
                                                      // canvas
    
    // with the drawing thread
    private int                  mCanvasWidth = -1;       // Actual canvas width
    private int                  mCanvasHeight = -1;      // Actual canvas height
    private Canvas               mCanvas      = null; // The display canvas
                                                      // reference
    
    private boolean 				 mDoubleBuffer = false;
    private Bitmap               	 mCacheCanvas = null; // The cache canvas for
    
    private volatile boolean				mCanvasReady	=	false;
    
    private boolean				mFirstSwapBuffers	= true;
    private boolean				mIsOrientionUpdate	= false;		// This flag indicates if orientation update is necessary
    
    private WazeEglManager 		mEglManager		= null;
    
    // the double buffer
    public final static long WAZE_OGL_SWAP_BUFFERS_DELAY = 10L;
    public final static long WAZE_OGL_RENDER_WAIT_TIMEOUT = 100L;
}
