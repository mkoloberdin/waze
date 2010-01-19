/**  
 * WazeEditText.java - Native android edit box customized for waze needs 
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2009     @author Alex Agranovich
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
 *   @see 
 */



package com.waze;

import android.content.Context;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.TextView;

public class WazeEditText extends EditText
{
    
    /*************************************************************************************************
     *================================= Public interface section =================================
     */

    public WazeEditText( Context aContext )
    {
        super( aContext );
        mContext = ( FreeMapAppActivity ) aContext;
        FreeMapAppView appView = FreeMapAppService.getAppView();
        if ( appView != null )
        {
        	this.setWidth( appView.getMeasuredWidth() );
        	this.setHeight( ViewGroup.LayoutParams.WRAP_CONTENT );
        }
        setSingleLine();
        setPadding( 0, 0, 0, 0 );
    }
    public WazeEditText( Context aContext, int aLineCount )
    {
        super( aContext );
//        mContext = ( FreeMapAppActivity ) aContext;
        setSingleLine();
    }

    @Override public boolean onKeyDown( int aKeyCode, KeyEvent aEvent )
    {
        if ( aKeyCode == KeyEvent.KEYCODE_BACK )
        { 
            HideSoftInput();
            mContext.getAnimator().showPrevious();
            return true;
        }
        return super.onKeyDown( aKeyCode, aEvent ); 
    }
    
    private void HideSoftInput()
    {
        InputMethodManager mgr;
        mgr = ( InputMethodManager ) this.getContext().getSystemService( Context.INPUT_METHOD_SERVICE );
        mgr.hideSoftInputFromWindow(  getWindowToken(), 0 );
    }    

    @Override public boolean  dispatchKeyEventPreIme( KeyEvent aEvent )
    {
        // Consume the key to prevent the keyboard from hiding
        return onKeyDown( aEvent.getKeyCode(), aEvent );
    }

/*    
    public boolean  onCheckIsTextEditor  ()
    {
        return true;
    }
   
    public InputConnection onCreateInputConnection( EditorInfo aAttrs )
    {
//        aAttrs.imeOptions |= mImeAction;
        return new EditTextInputConnection( this );
    }
    
    public void setImeAction( int aAction )
    {
        mImeAction = aAction;
    }

    private class EditTextInputConnection extends BaseInputConnection
    {
        EditTextInputConnection( View aView )
        {            
            super( aView, true );
        }        
    }
*/    
    protected void onSizeChanged( int w, int h, int oldw, int oldh )
    {
        super.onSizeChanged(w, h, oldw, oldh);
//        InputMethodManager mgr;
//        mgr = ( InputMethodManager ) this.getContext().getSystemService( Context.INPUT_METHOD_SERVICE );
//        mgr.restartInput( this );
//        mgr.showSoftInput( this, InputMethodManager.SHOW_FORCED );
//        mgr.
    }
    
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
    private FreeMapAppActivity mContext = null;
//    private int mImeAction = EditorInfo.IME_ACTION_UNSPECIFIED;     // Customized action button in the soft keyboard
   // private EditTextInputConnection mInputConnection = null;
}
