/**  
 * FreeMapAppView.java
 * This class represents the android layer view. It responsible for displaying
 * the supplied buffer to the device screen. The object has to be thread safe  
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
 *   @see FreeMapNativeCanvas.java
 */

package com.waze;

import java.util.Arrays;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.text.InputType;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import com.waze.FreeMapNativeManager.FreeMapUIEvent;

public final class FreeMapAppView extends SurfaceView implements SurfaceHolder.Callback 
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */
    public FreeMapAppView(Context aContext )
    {
        super(aContext);
        Init();
    }

    public FreeMapAppView(Context aContext, AttributeSet aParams )
    {
    	super( aContext, aParams );
    	Init();
     }
    /**
     * This method is part of the SurfaceHolder.Callback interface
     */
    public void surfaceCreated( SurfaceHolder holder ) 
    {    	     	
    	Log.i( "WAZE", "Surface created" );
    }

    /**
     * This method is part of the SurfaceHolder.
     */
    public void surfaceDestroyed(SurfaceHolder holder) 
    {
    	// AGA DEBUG
    	Log.i( "WAZE", "Surface destroyed" );    	
    	FreeMapNativeManager lNativeMgr = FreeMapAppService.getNativeManager();
    	WazeScreenManager lScreenMgr = FreeMapAppService.getScreenManager();
    	if ( lNativeMgr != null )
    	{
    		lScreenMgr.onSurfaceDestroyed();
    	}
    	mIsReady = false;
    }

    /**
     * This method is part of the SurfaceHolder.Callback interface 
     */
    public void surfaceChanged( SurfaceHolder holder, int format, int w, int h ) 
    {
    	// AGA DEBUG
    	Log.i( "WAZE", "Surface changed" );
    	
    	FreeMapAppActivity lActivity = FreeMapAppService.getAppActivity();
    	FreeMapNativeManager lNativeMgr = FreeMapAppService.getNativeManager();
    	WazeScreenManager lScreenMgr = FreeMapAppService.getScreenManager();
    	
    	
    	mIsReady = true;
    	
    	if ( mIsSplashShown /* already shown  and surface changed */ )
    	{
	    	if ( ( lNativeMgr != null ) && lNativeMgr.getInitializedStatus() )
	    	{	    		
	    		lScreenMgr.onSurfaceChanged();
	    	}
    	}
    }
    
    /**
     * True if the view is ready for drawing and querying
     */
    public boolean IsReady()
    {
    	return mIsReady;
    }

    
    /*************************************************************************************************
     *================================= Private interface section =================================
     */
    /*************************************************************************************************
     * Callback for the screen update Runs the application draw state machine
     * step Consequent calls update the canvas
     */
    @Override
    protected void onDraw( Canvas aCanvas )
    {
        FreeMapNativeManager lNativeManager = FreeMapAppService.getNativeManager();
        // Show screenshot
        if ( mScreenShot != null )
            DrawScreenShot( aCanvas );
    }
    
    
    
    /*************************************************************************************************
     * Surface initialization function
     */
    private void Init() 
    {
        SurfaceHolder holder = getHolder();
        holder.addCallback( this );
        holder.setType( SurfaceHolder.SURFACE_TYPE_GPU );
        setFocusableInTouchMode(true);        
        Arrays.sort( mUnhandledKeys );  // Sort ascending 

//        holder.setFormat( PixelFormat.RGBA_4444 );
    }

    /*************************************************************************************************
     * Draws the splash to the just created surface
     */
    public void DrawSplashOnHolder()
    {
        SurfaceHolder holder = this.getHolder();
    	Bitmap bmpSplash = FreeMapNativeCanvas.GetSplashBmp( FreeMapAppService.getAppActivity(), 
				this );        
        try
        {
        	if ( bmpSplash != null )
        	{        		
        		Canvas canvas = holder.lockCanvas( holder.getSurfaceFrame() );    		
        		canvas.drawBitmap( bmpSplash, 0, 0, null );
        		bmpSplash.recycle();
        		holder.unlockCanvasAndPost( canvas );        		
        	}
        }
        catch( Exception ex )
        {
        	Log.e( "WAZE", "Splash drawing problems." );
        	ex.printStackTrace();
        }
        finally
        {
        	mIsSplashShown = true;
        }
    }
    /*************************************************************************************************
     * Draws the splash on background
     */    
    public void DrawSplashBackground()
    {
    	try
    	{
	    	Bitmap bmpSplash = FreeMapNativeCanvas.GetSplashBmp( FreeMapAppService.getAppActivity(), 
					this );
	    	this.setBackgroundDrawable( new BitmapDrawable( bmpSplash ) );
	    	mSplashBitmap = bmpSplash;	    	
		}
		catch( Exception ex )
		{
			Log.e( "WAZE", "Splash drawing problems." );
			ex.printStackTrace();
		}
		finally
		{
			mIsSplashShown = true;
		}
	}
    /*************************************************************************************************
     * Remove the background splash if shown
     * Blocks the calling thread until the request has been executed on the UI thread   
     */
    public void PostRemoveSplash()
    {
    	if ( !mIsSplashDestroyed )
    	{
			// The background drawing must be done on the UI thread
	    	post( new Runnable() 
			{
					public void run()
					{						
						setBackgroundDrawable( null );
						mIsSplashDestroyed = true;
						mSplashBitmap.recycle();
						mSplashBitmap = null;
						
					}
			} );
	    	while( !mIsSplashDestroyed );
    	}
    }
    
    /*************************************************************************************************
     * Callback for the screen measure if the canvas is not ready - force new
     * canvas creation ( at this stage the dimensions of the view are known )
     */
    @Override
    protected void onMeasure( int aWidthMeasureSpec, int aHeightMeasureSpec )
    {
        super.onMeasure(aWidthMeasureSpec, aHeightMeasureSpec);
        /*
         * Draw the splash when the width and height are known         
         */
        if ( mIsSplashShown == false )
        {
        	DrawSplashBackground();
        }
    }

    /*************************************************************************************************
     * Callback for the touch screen event Posts the message to the native
     * thread and passes the event object to the manager
     */
    @Override
    public boolean onTouchEvent( MotionEvent aEvent )
    {
        if ( aEvent.getAction() == MotionEvent.ACTION_MOVE )
        {
            if ( mLastHandledEvent.getAction() == MotionEvent.ACTION_MOVE &&
                    Math.abs( mLastHandledEvent.getX() - aEvent.getX() ) <  MOTION_RESOLUTION_VAL &&
                    Math.abs( mLastHandledEvent.getY() - aEvent.getY() ) <  MOTION_RESOLUTION_VAL )                
            return true;
        }
            
        FreeMapNativeManager lNativeManager = FreeMapAppService.getNativeManager();
//           " X: " + String.valueOf( (int) aEvent.getX() ) +
//         " Y: " + String.valueOf( (int) aEvent.getY() ) );
        mLastHandledEvent = MotionEvent.obtain( aEvent );
        if ( lNativeManager != null)
        {
            lNativeManager.setTouchEvent( mLastHandledEvent );
            lNativeManager.PostUIMessage(FreeMapUIEvent.UI_EVENT_TOUCH);
        }
        return true;
    }
    @Override public boolean  dispatchKeyEventPreIme( KeyEvent aEvent )
    {
        // Consume the key to prevent the keyboard from hiding
        return onKeyDown( aEvent.getKeyCode(), aEvent );
    }

    public InputConnection onCreateInputConnection( EditorInfo aAttrs )
    {
        aAttrs.imeOptions = mImeAction|EditorInfo.IME_FLAG_NO_EXTRACT_UI;
        aAttrs.inputType = InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE;
        return new AppViewInputConnection( this );
    }
    
    public void ShowSoftInput()
    {
        getInputMethodManager().restartInput( this );
        getInputMethodManager().showSoftInput( this, InputMethodManager.SHOW_FORCED );     
    }
    
    public void HideSoftInput()
    {
        getInputMethodManager().hideSoftInputFromWindow(  getWindowToken(), 0 );
    }    
    
    public void setImeAction( int aAction )
    {
        mImeAction = aAction;
    }
    public void setImeCloseOnAction( boolean aCloseOnAction )
    {
        mImeCloseOnAction = aCloseOnAction;
    }
    
    public InputMethodManager getInputMethodManager()
    {
        InputMethodManager mgr;
        mgr = ( InputMethodManager ) this.getContext().getSystemService( Context.INPUT_METHOD_SERVICE );
        return mgr;
    }    

    /*************************************************************************************************
     * Callback for the key screen event Posts the message to the native thread
     * and passes the event object to the manager
     */
    @Override
    public boolean onKeyDown( int aKeyCode, KeyEvent aEvent )
    {
        int metaState = aEvent.getMetaState();
        Context ctx = this.getContext();
        Configuration cfg = ctx.getResources().getConfiguration();
        
        // If exceptional key - return unhandled. Array must be sorted in ascending order before
        if (  Arrays.binarySearch( mUnhandledKeys, aKeyCode ) >= 0 )
        {
            return false;
        }

        if ( aKeyCode == KeyEvent.KEYCODE_MENU )
        {
        	FreeMapNativeManager lNativeManager = FreeMapAppService.getNativeManager();        	
        	return ( lNativeManager.IsMenuEnabled() == false );   
        }

        // If not action down - return
        if ( ( aEvent.getAction() != KeyEvent.ACTION_DOWN ) && ( aEvent.getAction() != KeyEvent.ACTION_MULTIPLE ) )
        {
            return true;
        }
        // Handle end call event
        if ( aKeyCode == KeyEvent.KEYCODE_ENDCALL )
        {
            return true;
        }
        
//        if ( aKeyCode == KeyEvent.KEYCODE_DPAD_CENTER )
//        {
//        	Dialog dl = new Dialog( this.getContext() );
//        	dl.requestWindowFeature( Window.FEATURE_NO_TITLE );
//        	dl.setCancelable( true );
//        	View view = new WazeEditText( this.getContext() );
//        	dl.setContentView( view, new ViewGroup.LayoutParams( ViewGroup.LayoutParams.WRAP_CONTENT, 
//					ViewGroup.LayoutParams.WRAP_CONTENT ) );      	
//        	dl.show();
//        	return true;
//        }
        
        // Handle shift key event for physical (hard) input
        if ( cfg.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO )
        {
	        if ( aEvent.isShiftPressed() )
	        {
	            mShiftState = ( mShiftState + 1 ) % ( SHIFT_STATE_DOUBLE_CLICK + 1 );
	        }
	        else
	        {
	            // Set the meta state for the shift
	            if ( mShiftState > SHIFT_STATE_NONE )       
	                metaState = KeyEvent.META_SHIFT_ON;
	            // Reset the shift if is not locked by double click
	            if ( mShiftState == SHIFT_STATE_CLICK )
	                mShiftState = SHIFT_STATE_NONE;
	        }
        }
        
        if ( aKeyCode == KeyEvent.KEYCODE_CAMERA )
        {
            buildDrawingCache();
            mScreenShot = Bitmap.createBitmap( getDrawingCache() );
            ( new FreeMapScreenShot( mScreenShot ) ).start();
            destroyDrawingCache();        
            invalidate();
        }

        // Otherwise (not call button) pass it to the native layer
        FreeMapNativeManager lNativeManager = FreeMapAppService.getNativeManager();
        if ( lNativeManager != null )
        {
        	KeyEvent event = aEvent;
        	if ( aEvent.getAction() == KeyEvent.ACTION_DOWN )
        	{
	            event = new KeyEvent( aEvent.getEventTime(), aEvent.getEventTime(), 
	                    aEvent.getAction(), aEvent.getKeyCode(), aEvent.getRepeatCount(), metaState );
        	}
        	if ( aEvent.getAction() == KeyEvent.ACTION_MULTIPLE )
        	{
        		event = new KeyEvent( aEvent.getEventTime(), aEvent.getCharacters(), aEvent.getDeviceId(), aEvent.getFlags() );
        	}            
            lNativeManager.setKeyEvent( event );
            lNativeManager.PostUIMessage( FreeMapUIEvent.UI_EVENT_KEY_DOWN );
        }
        return true;
    }
    
    @Override public boolean  onKeyMultiple( int aKeyCode, int aRepeatCount, KeyEvent aEvent )
    {
    	return onKeyDown( aKeyCode, aEvent );
    }
    
    /*************************************************************************************************
     * Draws the screen shot to indicate that the capture is taken
     * 
     */
    private void DrawScreenShot( Canvas aCanvas )
    {
        aCanvas.drawBitmap( mScreenShot, 0, 0, null );
        aCanvas.scale( ( float ) 0.75, ( float ) 0.75, aCanvas.getWidth()/2, aCanvas.getHeight()/2 );
        aCanvas.drawColor( Color.DKGRAY );
        mScreenShot = null;
        // Invalidate after 0.75 sec
        postInvalidateDelayed( 750L );
    }

    private class AppViewInputConnection extends BaseInputConnection
    {
        AppViewInputConnection( View aView )
        {            
            super( aView, false );
        }        
        @Override public boolean performEditorAction( int editorAction )
        {
            if ( editorAction == mImeAction )
            {
                onKeyDown( KeyEvent.KEYCODE_ENTER, new KeyEvent( KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER ) );
                if ( mImeCloseOnAction )
                {
                    HideSoftInput();
                }
                return true;
            }            
            return false;            
        }   
    }
    
    
    /*************************************************************************************************
     *================================= Data members section =================================
     */
    
    private final static int SHIFT_STATE_NONE = 0;
    private final static int SHIFT_STATE_CLICK = 1;
    private final static int SHIFT_STATE_DOUBLE_CLICK = 2;
    private int mShiftState = SHIFT_STATE_NONE;
    
    private Bitmap mScreenShot = null;
    private MotionEvent mLastHandledEvent;
    private final static int MOTION_RESOLUTION_VAL = 3;
    private int mImeAction = EditorInfo.IME_ACTION_UNSPECIFIED;     // Customized action button in the soft keyboard
    private boolean mImeCloseOnAction = false;                      // Close keyboard when action button iks pressed
    private int[] mUnhandledKeys = { KeyEvent.KEYCODE_VOLUME_UP, KeyEvent.KEYCODE_VOLUME_DOWN };

    public boolean mIsSplashShown = false;							// Indicates if the splash was already shown
    public volatile boolean mIsSplashDestroyed = false;				// Indicates if the splash was already destroyed
    
    public Bitmap	mSplashBitmap = null;
    private volatile boolean mIsReady = false;
}
