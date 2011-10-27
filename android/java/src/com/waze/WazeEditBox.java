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
import android.text.Editable;
import android.text.InputType;
import android.util.AttributeSet;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

public class WazeEditBox extends EditText
{
    
    /*************************************************************************************************
     *================================= Public interface section =================================
     */

    public WazeEditBox( Context aContext )
    {
        super( aContext );
        
        Init( aContext );
    }
    
    public WazeEditBox( Context aContext, AttributeSet aAttrs )
    {
        super( aContext, aAttrs );
        Init( aContext );
    }
    
    public WazeEditBox( Context aContext, int aImeAction, boolean aCloseOnAction,
    		String aValue, WazeEditBoxCallback aCallback )
    {
        super( aContext );

        Init( aContext );
        
        setEditBoxAction( aImeAction );
        setEditBoxStayOnAction( aCloseOnAction );
        setEditBoxValue( aValue );
        setEditBoxCallback( aCallback );
        
    }
    
    /*************************************************************************************************
     * onCheckIsTextEditor - Indicates to the framework that the view is editor 
     * @param none
     */
    @Override
    public boolean onCheckIsTextEditor()
    {
    	return true;
    }
    
    /*************************************************************************************************
     * setEditBoxAction - Action type for native input done,search,next etc... 
     * @param aImeAction - action id
     */
    public void setEditBoxAction( int aImeAction )
    {
    	mImeAction = aImeAction;
    	// Set the soft keyboard options
        int imeOptions = getImeOptions();
        setImeOptions( imeOptions|mImeAction );
    }
    
    /*************************************************************************************************
     * setEditBoxFlags - Custom flags setting - for scalability purposes  
     * @param aFlags - custom flags to pass from the core
     */
    public void setEditBoxFlags( int aFlags )
    {
    	mFlags = aFlags;
    	/*
    	 * To disable prediction: InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE
    	 * To enable correction: InputType.TYPE_TEXT_FLAG_AUTO_CORRECT 
    	 */
    	int inputType = InputType.TYPE_CLASS_TEXT;
    	if ( ( mFlags & WAZE_EDITBOX_FLAG_PASSWORD ) > 0 )
    	{
        	 inputType = inputType | InputType.TYPE_TEXT_VARIATION_PASSWORD;
    	}
    	if ( ( mFlags & WAZE_EDITBOX_FLAG_SPEECHTT ) > 0 )
    	{
    		PrepareSpeechTTHandler();
    	}
    	this.setInputType( inputType );
    }

    /*************************************************************************************************
     * setEditBoxStayOnAction - Defines if editbox should be closed on action event  
     * @param aStayOnAction - True - the editbox stays on screen 
     */
    public void setEditBoxStayOnAction( boolean aStayOnAction )
    {
    	mStayOnAction = aStayOnAction;
    }
    /*************************************************************************************************
     * setEditBoxValue - Sets the initial text to be shown in the editbox  
     * @param aValue - Text string 
     */
    public void setEditBoxValue( String aValue )
    {
    	mValue = aValue;
    	setText( mValue );
    }
    /*************************************************************************************************
     * setEditBoxCallback - Sets the callback object implementing WazeEditBoxCallback interface class.
     *                         Called upon action/enter event
     * @param aCallback - WazeEditBoxCallback object 
     */
    public void setEditBoxCallback( WazeEditBoxCallback aCallback )
    {
    	mCallback = aCallback;
    }
    
    /*************************************************************************************************
     * ShowSoftInput - Shows the on-screen keyboard  
     * @param None 
     */
    public void ShowSoftInput()
    {    	
        getInputMethodManager().restartInput( this );
        getInputMethodManager().showSoftInput( this, InputMethodManager.SHOW_FORCED );
    }
    
    
    /*************************************************************************************************
     * HideSoftInput - Hides the on-screen keyboard  
     * @param None 
     */
    public void HideSoftInput()
    {
        getInputMethodManager().hideSoftInputFromWindow(  getWindowToken(), 0 );
    }    

    /*************************************************************************************************
     * onKeyDown - Overrides "ENTER"  
     * @param  
     */
    @Override public boolean onKeyDown( int aKeyCode, KeyEvent aEvent )
    {
        // Consume the key to prevent the keyboard from hiding
    	switch ( aEvent.getKeyCode() )
    	{
    		case KeyEvent.KEYCODE_ENTER:
    		{    	
    			ActionHandler();
    			return true;
    		}
    	}    	
    	
    	return super.onKeyDown( aKeyCode, aEvent );
    }

    /*************************************************************************************************
     * dispatchKeyEventPreIme - Overrides "ENTER" and "BACK" events 
     * @param  
     */
    @Override public boolean  dispatchKeyEventPreIme( KeyEvent aEvent )
    {
    	// If not action down - return
        if ( ( aEvent.getAction() != KeyEvent.ACTION_DOWN ) && ( aEvent.getAction() != KeyEvent.ACTION_MULTIPLE ) )
        {
            return false;
        }
        
        // Consume the key to prevent the keyboard from hiding
    	switch ( aEvent.getKeyCode() )
    	{
    		case KeyEvent.KEYCODE_ENTER:
    		{    	
    			ActionHandler();
    			return true;
    		}
    		case KeyEvent.KEYCODE_BACK:
    		{
    			mCallback.Post( false, null );
				WazeLayoutManager layoutMgr = mContext.getLayoutMgr();
				layoutMgr.HideEditBox();
//    			super.dispatchKeyEvent( aEvent );
				return true;
    		}
    	}
    	
    	return false;
    }
    
    /*************************************************************************************************
     * class WazeEditBoxCallback - Provides an interface for the callback to be called upon action/enter event arrival 
     * Called on the native thread
     */
    public static abstract class WazeEditBoxCallback implements Runnable    
    {   
    	WazeEditBoxCallback( long aCbContext )
    	{
    		mCbContext = aCbContext;
    	}
    	public void run()
    	{   
    		int res = mResult ? 1 : 0;
			CallbackDone( res, mText, mCbContext );
		}
    	protected void Post( boolean aResult, String aText )
    	{
    		mText = aText;
    		mResult = aResult;
    		FreeMapNativeManager lMgr = FreeMapAppService.getNativeManager();
    		lMgr.PostRunnable( this );
    	}
    	
    	public abstract void CallbackDone( int aRes, String aText, long aCbContext );
    	
    	private volatile String mText = null;
    	private volatile boolean mResult = false;
    	private volatile long mCbContext = 0;
    }
    
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    
    /*************************************************************************************************
     * Init - Initializes text box, on-screen keyboard and action listener 
     * @param aContext - Activity reference  
     */
    protected void Init( Context aContext )
    {
        mContext = ( FreeMapAppActivity ) aContext;        
        setSingleLine();        
        setFocusableInTouchMode( true );
    	int inputType = InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE;
    	this.setInputType( inputType ); 
    	this.setTag( WAZE_EDITBOX_TAG );
    	/*
    	 * Editor action listener
    	 */
    	OnEditorActionListener editorActionListener = new OnEditorActionListener()
    	{
    		public boolean onEditorAction( TextView aView, int aActionId, KeyEvent aEvent )
    		{
    			boolean res = false;
    	        if ( aActionId == mImeAction )
    	        {
    	            res = onKeyDown( KeyEvent.KEYCODE_ENTER, new KeyEvent( KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER ) );
    	            if ( !mStayOnAction )
    	            {
    	                HideSoftInput();
    	            }           
    	        } 
				return res;
    		}
    	};
    	setOnEditorActionListener( editorActionListener );
    	/*
    	 * Key listener 
    	 */
    	TextWatcher textWatcher = new TextWatcher() {
			public void onTextChanged( CharSequence aSeq, int aStart, int aBefore, int aCount ) {
				if ( aSeq.length() == 1 )
				{
					final FreeMapNativeManager lMgr = FreeMapAppService.getNativeManager();
					lMgr.EditBoxCheckTypingLock( WazeEditBox.this );
				}
			}
			
			public void beforeTextChanged( CharSequence arg0, int arg1, int arg2, int arg3 ) {
			}
			
			public void afterTextChanged( Editable arg0 ) {
			}
		};
		addTextChangedListener( textWatcher );
    }
    
    
    /*************************************************************************************************
     * ActionHandler - Auxiliary. Called upon enter/action event 
     * @param None  
     */
    protected void ActionHandler()
    {
		mCallback.Post( true, getText().toString() );
		if ( !mStayOnAction )
		{
			WazeLayoutManager layoutMgr = mContext.getLayoutMgr();
			layoutMgr.HideEditBox();    				
		}
    }
    /*************************************************************************************************
     * ActionHandler - Auxiliary. Prepares the handler for the voice recognition 
     * @param None  
     */
    protected void PrepareSpeechTTHandler()
    {
    	final WazeSpeechttManagerBase.Callback speechTTCb= new WazeSpeechttManagerBase.Callback() {
			@Override
			protected void onResults( long aCbContext, String aResult, int aStatus ) {
				if ( aStatus == WazeSpeechttManagerBase.SPEECHTT_RES_STATUS_SUCCESS )
					setText( aResult );
				else
					Log.e( WazeLog.TAG, "Got error result from the speech to text manager" );
				ShowSoftInput();				
			}
		};
    	final View.OnClickListener voiceBtnListener = new View.OnClickListener() {
			public void onClick( View aView ) {
				FreeMapNativeManager nativeMgr = FreeMapAppService.getNativeManager();
				WazeSpeechttManagerBase speechTTManager = nativeMgr.getSpeechttManager();
				HideSoftInput();
				speechTTManager.Start( speechTTCb, speechTTCb.CONTEXT_NULL, 5, null, null, 0 );
			}
		};
		View btn = this.getRootView().findViewById( R.id.VoiceButton );
		if ( btn != null )
		{
			btn.setOnClickListener( voiceBtnListener );
		}
    }
    
    
    private InputMethodManager getInputMethodManager()
    {
        InputMethodManager mgr;
        mgr = ( InputMethodManager ) this.getContext().getSystemService( Context.INPUT_METHOD_SERVICE );        
        return mgr;
    }
    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
	
    private FreeMapAppActivity mContext = null;						// Activity reference
    private int mImeAction = EditorInfo.IME_ACTION_UNSPECIFIED;     // Customized action button in the soft keyboard
    private boolean mStayOnAction = false;                          // Close keyboard when action button iks pressed
    private WazeEditBoxCallback mCallback = null;					// Callback object					
    private String mValue = null;									// Initial string value
    private int mFlags = 0;											// Custom flags
    
    public static String WAZE_EDITBOX_TAG = "@WazeEditBox";  
    
    /* Custom flags values */
    public static final int WAZE_EDITBOX_FLAG_PASSWORD = 0x00000001;
    public static final int WAZE_EDITBOX_FLAG_SPEECHTT = 0x00000002;
}
