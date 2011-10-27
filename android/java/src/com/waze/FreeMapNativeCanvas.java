/**  
 * FreeMapNativeCanvas.java
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
 *   @see WazeMainView.java
 */

package com.waze;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;

import javax.microedition.khronos.opengles.GL10;

import com.waze.FreeMapNativeManager.FreeMapUIEvent;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap.Config;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

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
    }
    
    
    /*************************************************************************************************
     * 
     * 
     */
    public boolean PrepareCanvas( final SurfaceView aView )
    {
    	boolean res = true;
    	SurfaceView surfaceView = aView;
    	
    	if ( !mCanvasReady ) 
    	{
    		// Prepare the context - the view surface has to be ready here 
	        if ( mEglManager == null )
	        {
	        	WazeMainView lView = FreeMapAppService.getMainView();
	        	if ( surfaceView == null )
	        	{
	        		if ( lView == null )
	        		{
		        		WazeLog.ee( "Cannot initialize canvas - main view is not available" );
	     			    return false;
	        		}
	        		else
	        		{
	        			surfaceView = lView;
	        		}
	        	}
	        	
	        	mEglManager = new WazeEglManager( surfaceView );
	        	res = mEglManager.InitEgl();
	        }
	        else
	        {
	        	// Reset the surface
	        	res = mEglManager.CreateEglSurface();
	        }
	        
	        if ( res )
	        {	        	
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
     * Wrapper for the EGL manager getGL
     * 
     */
    public GL10 getGL()
    {
    	return mEglManager.getGL();
    }    
    /*************************************************************************************************
     * Wrapper for the EGL engine swap buffers procedure
     * Called from the native side  
     */
    public void SwapBuffersEgl()
    {
    	
    	WazeScreenManager screenMgr = FreeMapAppService.getScreenManager();
    	if ( mEglManager == null || screenMgr == null || !screenMgr.IsReady() || !mCanvasReady )
    	{
    		WazeLog.e( "Canvas is not ready for rendering!" );
    		return;
    	}
    	
    	if ( mFirstSwapBuffers )
    	{
    		mFirstSwapBuffers = false;
    		if ( FreeMapAppActivity.mIsSplashShown )
    		{
	    		final FreeMapAppActivity appActivity = FreeMapAppService.getMainActivity();
	    		if ( appActivity != null )
	    		{
	    			appActivity.runOnUiThread( new Runnable() {
						public void run() {
			    			appActivity.CancelSplash();				    		
						}
					} );
	    		}
    		}
    		mNativeManager.FirstSwapEvent();    		
    	}
    	mEglManager.SwapBuffersEgl();
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
    	Context ctx = FreeMapAppService.getAppContext();
    	WindowManager windowMgr = (WindowManager) ctx.getSystemService( Context.WINDOW_SERVICE );
    	int pixFmt = -1;
    	
        if ( windowMgr != null )
        {
        	pixFmt = windowMgr.getDefaultDisplay().getPixelFormat();
        }
        
        if ( mCanvasWidth != -1 )
        {
        	mIsOrientionUpdate = ( mCanvasWidth > mCanvasHeight )  ^ 
        									( aCanvasWidth > aCanvasHeight );
        }
        
        mCanvasWidth = aCanvasWidth;
        mCanvasHeight = aCanvasHeight;
        
        
//            AllocateCanvasBuffer(aCanvasWidth, aCanvasHeight);
        CanvasConfigureNTV( mCanvasWidth, mCanvasHeight, pixFmt );
    }
    
    /*************************************************************************************************
     * Destroys the OGL canvas
     * 
     * 
     */
    public void DestroyCanvas()
    {
    	if ( mEglManager != null )
    		mEglManager.DestroyEgl();
		mEglManager = null;
    	mCanvasReady = false;
    	mFirstSwapBuffers = true;
    }
    
    public void DestroyCanvasSurface()
    {
    	if ( mCanvasReady && mEglManager != null )
    	{
	    	mEglManager.DestroyEglSurface();
	    	mCanvasReady = false;
    	}
    }

    /*************************************************************************************************
     * Resize the OGL canvas
     * 
     */
    public void ResizeCanvas( SurfaceView aSurfaceView, int aWidth, int aHeight )
    {
    	CreateCanvas( aSurfaceView, aWidth, aHeight );
    	
    	CanvasOrientationUpdate();
    }
    /*************************************************************************************************
     * Resize the OGL canvas
     * 
     */
    public void CreateCanvas( SurfaceView aSurfaceView, int aWidth, int aHeight )
    {
		PrepareCanvas( aSurfaceView );
    	
    	// Initialize the canvas
    	ConfigureNativeCanvas( aWidth, aHeight );
    
    	// Create new canvas on the native layer side
    	CreateCanvasNTV();
    }
    
    /*************************************************************************************************
     * Redraw the OGL canvas
     * 
     */
    public void RedrawCanvas()
    {
    	CanvasRedrawNTV();
    }
    
    /*************************************************************************************************
     * Redraw the OGL canvas with delay
     * 
     */
    public void RedrawCanvas( long aDelay )
    {
    	final Runnable redrawEvent = new Runnable() {
			
			public void run() {
				CanvasRedrawNTV();		
			}
		};
    	mNativeManager.PostRunnable( redrawEvent, aDelay );
		
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
        // Log.i( "Sending Touch Event", "Event : " + String.valueOf( aEvent.getAction() ) +
        // " X: " + String.valueOf( (int) aEvent.getX() ) + " Y: " + String.valueOf( (int) aEvent.getY() ) );
    	
    	final int sdkBuildVersion = Integer.parseInt( android.os.Build.VERSION.SDK );
    	
    	int[] coordXY = null;
    	if ( sdkBuildVersion >= android.os.Build.VERSION_CODES.ECLAIR )
    	{            
    		/*
    		 * Fill the array with the points according to their index
    		 * Note: Check if should use PointerId 
    		 */
    		final int action = aEvent.getAction() & MotionEvent.ACTION_MASK;
    		final int pointerCount = CompatabilityWrapper.getPointerCount( aEvent );
            coordXY = new int[2*pointerCount];
            // Fill the array
            for ( int i = 0;i < pointerCount; ++i )
            {
            	if ( i == WAZE_CORDING_MAX_POINTS )
            		break;
            	
            	coordXY[2*i] = (int) CompatabilityWrapper.getX( aEvent, i ); 
            	coordXY[2*i+1] = (int) CompatabilityWrapper.getY( aEvent, i );
            }
            // Handle special events
    		if ( action == MotionEvent.ACTION_POINTER_DOWN )
    		{
    			MousePressedNTV( coordXY );
    			return true;
    		}
    		if ( action == MotionEvent.ACTION_POINTER_UP )
    		{
    			MouseReleasedNTV( coordXY );
    			return true;
    		}
    	}
    	else
    	{
    		coordXY = new int[2]; 
    		coordXY[0] = (int)aEvent.getX();
    		coordXY[1] = (int)aEvent.getY();
    	}

        switch (aEvent.getAction())
        {
            case MotionEvent.ACTION_DOWN:
            {
            	MousePressedNTV( coordXY );
                break;
            }
            case MotionEvent.ACTION_UP:
            {
            	MouseReleasedNTV( coordXY );
                break;
            }
            case MotionEvent.ACTION_MOVE:
            {
            	MouseMovedNTV( coordXY );
                break;
            }
        }
    	return true;
    }

    /*************************************************************************************************
     * Returns the splash buffer in bitmap or null if failure
     */
    public static Bitmap GetSplashBmp( View aView )
    {
        InputStream splashStream = null;
        Bitmap bmScaled = null;        
        // Select landscape or portrait
        try
        {
        	/*
        	 * There is no wide mode on splash for now
        	 */        	
        	splashStream = WazeResManager.LoadSkinStream( WazeResManager.GetSplashName( false ) );
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
     *================================= Private interface section     =================================
     */
    
    /*************************************************************************************************
     * Shows the alert message box for the GPU access error
     * Auxiliary
     */
    private void ShowCreateSurfaceError()
    {
		final Activity context = FreeMapAppService.getMainActivity();
		if ( context == null )
			return;
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

		context.runOnUiThread( errDlg );
    }
    
    /*************************************************************************************************
     * CompatabilityWrapper the compatability wrapper class used to overcome dalvik delayed  
     * class loading. Supposed to be loaded only for android 2.0+.  
     */
    private static final class CompatabilityWrapper
    {
    	public static int getPointerCount( MotionEvent aEvent )
    	{
    		return aEvent.getPointerCount();
    	}
    	public static float getX( MotionEvent aEvent, int aIndex )
    	{
    		return aEvent.getX( aIndex );
    	}
    	public static float getY( MotionEvent aEvent, int aIndex )
    	{
    		return aEvent.getY( aIndex );
    	}

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
    
    public native void CanvasRedrawNTV();
    
    private native void KeyDownHandlerNTV( int aKeyCode,
            boolean isSpecalButton, byte utf8Bytes[] );

    private native void MousePressedNTV( int aCoordXY[] );

    private native void MouseReleasedNTV( int aCoordXY[] );

    private native void MouseMovedNTV( int aCoordXY[] );

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private FreeMapNativeManager mNativeManager;     // Main application
                                                      // activity

    private Display              mDisplay;
    
    // with the drawing thread
    private int                  mCanvasWidth = -1;       // Actual canvas width
    private int                  mCanvasHeight = -1;      // Actual canvas height
                                                      // reference
    private volatile boolean				mCanvasReady	=	false;
    
    private boolean				mFirstSwapBuffers	= true;
    private boolean				mIsOrientionUpdate	= false;		// This flag indicates if orientation update is necessary
    
    private WazeEglManager 		mEglManager		= null;
    
    // the double buffer
    public final static long WAZE_OGL_SWAP_BUFFERS_DELAY = 10L;
    public final static long WAZE_OGL_RENDER_WAIT_TIMEOUT = 100L;
    public final static int WAZE_CORDING_MAX_POINTS = 8; 
}
