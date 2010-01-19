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

import android.app.Dialog;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import com.waze.FreeMapNativeManager.FreeMapUIEvent;

public final class FreeMapAppView extends View
{

    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */

    public FreeMapAppView(Context aContext)
    {
        super(aContext);
        setFocusableInTouchMode(true);        
        Arrays.sort( mUnhandledKeys );  // Sort ascending
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
        
        if (lNativeManager != null)
        {
            lNativeManager.UpdateCanvas(aCanvas);
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
        FreeMapNativeManager lNativeManager = FreeMapAppService.getNativeManager();
        if (!lNativeManager.getCanvasBufReady())
            lNativeManager.ForceNewCanvas();
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
        if (lNativeManager != null)
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
}
