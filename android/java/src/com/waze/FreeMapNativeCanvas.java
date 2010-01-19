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

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;

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
        // Buffer allocation
        AllocateCanvasBuffer(mNativeManager.getAppView().getMeasuredWidth(),
                mNativeManager.getAppView().getMeasuredHeight());
        // Draw the splash
        DrawSplash2Buffer();
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
        ForceNewCanvasNTV();
        // Force refresh
        // mNativeManager.setCanvasBufReady( true );
        mNativeManager.setScreenRefresh( true );
    }

    /*************************************************************************************************
     * ConfigureNativeCanvas - Initialize the native layer
     * 
     */
    public void InitNativeCanvas()
    {
        InitCanvasNTV();
        // Initialize the canvas
        ConfigureNativeCanvas(mNativeManager.getAppView().getMeasuredWidth(),
                mNativeManager.getAppView().getMeasuredHeight());
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

        synchronized ( this )
        {
            AllocateCanvasBuffer(aCanvasWidth, aCanvasHeight);

            // NOTE :: At this time the native library should be loaded !!!
            CanvasConfigureNTV( mCanvasWidth, mCanvasHeight, lDisplay
                    .getPixelFormat(), mJCanvas );
        }
    }

    public int getPixelFormat()
    {
        return mDisplay.getPixelFormat();
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
        // Get the native data - the m_JCanvas has been already updated !!
    	if ( mDoubleBuffer )
    	{
	        synchronized (mCacheCanvas)
	        {
	            mCacheCanvas.setPixels(mJCanvas, 0, mCanvasWidth, 0, 0,
	                    mCanvasWidth, mCanvasHeight);
	        }
    	}
		mNativeManager.setScreenRefresh(true);
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
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void InitCanvasNTV();

    private native void CanvasConfigureNTV( int aWidth, int aHeight,
            int aConfig, int aJCanvas[] );

    private native void ForceNewCanvasNTV();

    private native void KeyDownHandlerNTV( int aKeyCode,
            boolean isSpecalButton, byte utf8Bytes[] );

    private native void MousePressedNTV( int aX, int aY );

    private native void MouseReleasedNTV( int aX, int aY );

    private native void MouseMovedNTV( int aX, int aY );

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */
    private FreeMapNativeManager mNativeManager;     // Main application
                                                      // activity

    private Display              mDisplay;
    private int                  mJCanvas[]   = null; // The Java side android
                                                      // canvas
    
    // with the drawing thread
    private int                  mCanvasWidth;       // Actual canvas width
    private int                  mCanvasHeight;      // Actual canvas height
    private Canvas               mCanvas      = null; // The display canvas
                                                      // reference
    
    private boolean 				 mDoubleBuffer = false;
    private Bitmap               	 mCacheCanvas = null; // The cache canvas for
    // the double buffer
}
