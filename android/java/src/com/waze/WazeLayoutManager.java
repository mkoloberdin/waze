/**  
 * WazeLayoutManager - Initializes the main layout and 
 *           provides interface for management of the views on the screen. 
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2010     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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

import com.waze.WazeWebView.WazeWebViewBackCallback;
import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;

public final class WazeLayoutManager 
{
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	public WazeLayoutManager( Context aContext )
	{
		mContext = aContext;
		Init(); 
	}
	
	public RelativeLayout getMainLayout()
	{	
		return mMainLayout;
	}
	
	public WazeMainView getAppView()
	{
		return mAppView;
	}
	
	public WazeWebView getWebView()
	{
		return mWebView;
	}

	public WazeEditBox getEditBox()
	{
		WazeEditBox editBox = null;
		
		if ( mEditBoxView != null )
			editBox = (WazeEditBox) mEditBoxView.findViewWithTag( WazeEditBox.WAZE_EDITBOX_TAG );
		
		return editBox;
	}

	public WazeWebView CreateWebView( int aFlags )
	{
		mWebView = new WazeWebView( mContext, aFlags );
		final WazeWebViewBackCallback backCallback = new WazeWebViewBackCallback() {
			public boolean onBackEvent( KeyEvent aEvent )
			{
				return mAppView.onKeyDown( KeyEvent.KEYCODE_BACK, aEvent );
			}
		};
		mWebView.setBackCallback( backCallback );
		return mWebView;
	}
	
	public void ShowWebView( String aUrl, WazeRect aRect, int aFlags )
	{		
		if ( mWebView != null )
		{
			mMainLayout.removeView( mWebView );
			mWebView.destroy();
			mWebView = null;	
		}
		
		CreateWebView( aFlags );
		
		mWebView.clearView();
		
		// Webview back callback
		int width = aRect.maxx - aRect.minx + 1;
		int height = aRect.maxy - aRect.miny + 1;
		RelativeLayout.LayoutParams webParams = new RelativeLayout.LayoutParams( width, height );
		webParams.leftMargin = aRect.minx;
		webParams.topMargin = aRect.miny;
		
		mMainLayout.addView( mWebView, webParams );
//		mWebView.setLayoutParams( webParams );		
		
		mWebView.setVisibility( View.VISIBLE );
		mMainLayout.bringChildToFront( mWebView );
		mMainLayout.requestLayout();
		
		mWebView.loadUrl( aUrl );
		mWebView.requestFocus();	
	}
	
	public void ResizeWebView( WazeRect aRect )
	{		
		if ( mWebView == null )
		{
			return;
		}
		
		int width = aRect.maxx - aRect.minx + 1;
		int height = aRect.maxy - aRect.miny + 1;
		RelativeLayout.LayoutParams webParams = (RelativeLayout.LayoutParams) mWebView.getLayoutParams();
		webParams.width = width;
		webParams.height = height;
		webParams.leftMargin = aRect.minx;
		webParams.topMargin = aRect.miny;
		
		mWebView.setLayoutParams( webParams );
		mMainLayout.requestLayout();
	}

	public void HideWebView()
	{
		if ( mWebView != null )
		{
			HideSoftInput( mWebView );
			
			mWebView.setVisibility( View.GONE );
			
			mMainLayout.removeView( mWebView );
	 	
	// TODO:: Add backward compatibility wrapper		
	//		mWebView.freeMemory();
	
	// TODO :: Check this		
	//		try
	//		{
	//			Class.forName("android.webkit.WebView").getMethod("onPause", (Class[]) null).invoke( mWebView, (Object[]) null );
	//		}
	//		catch( Exception ex )
	//		{
	//			Log.w( "WAZE", "Problems in pausing webview" );
	//		}
			
			mWebView.destroy();
			mWebView = null;		
	
			mMainLayout.requestLayout();
	
			System.gc();
		}
	}
	
	public WazeEditBox CreateEditBox( int aType )
	{
		switch( aType )
		{
			case WAZE_LAYOUT_EDIT_TYPE_SIMPLE:
				mEditBoxView = new WazeEditBox( mContext );
				break;
			case WAZE_LAYOUT_EDIT_TYPE_VOICE:
				mEditBoxView = ( View ) View.inflate( mContext, R.layout.editbox_voice, null );
				break;
			default:
				mEditBoxView = new WazeEditBox( mContext );
		}		
		return getEditBox();
	}

	public void ShowEditBox( int aTopMargin, int aType )
	{		
		final float scale = mContext.getResources().getDisplayMetrics().density;

		if ( mEditBoxView == null )
			CreateEditBox( aType );

		RelativeLayout.LayoutParams textParams = new RelativeLayout.LayoutParams( FILL_PARENT, WRAP_CONTENT );
		textParams.leftMargin = (int) ( WAZE_LAYOUT_EDIT_SIDE_MARGIN * scale );
		textParams.rightMargin = textParams.leftMargin;
		textParams.topMargin = aTopMargin;

		mMainLayout.addView( mEditBoxView, textParams );		

		mEditBoxView.setVisibility( View.VISIBLE );
		mMainLayout.bringChildToFront( mEditBoxView );
		mMainLayout.requestLayout();
		
		mEditBoxView.requestFocus();
		
		final Runnable showSoftInput = new Runnable() {
			public void run() {
				final WazeEditBox editBox = getEditBox();
				if ( editBox != null )
					ShowSoftInput( editBox );		
			}
		};
		mEditBoxView.postDelayed( showSoftInput, 100L );
	}
	
	public void HideEditBox()
	{
		if ( mEditBoxView != null )
		{
			getEditBox().HideSoftInput();
			mMainLayout.removeView( mEditBoxView );			
			mMainLayout.requestLayout();			
			mEditBoxView = null;
		}
	}
	public void ShowProgressView()
	{	
		/*
		 * Destroy previous view if shown
		 */
		HideProgressView();
		
		mProgressView = ( View ) View.inflate( mContext, R.layout.progress_bar, null );
		
		if ( mProgressView == null )
		{
			Log.e( "WAZE", "Progress view is not available" );
			return;
		}
		
		/*
		 * Build layout params params
		 */
		RelativeLayout.LayoutParams viewParams = new RelativeLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT );
		viewParams.addRule( RelativeLayout.CENTER_IN_PARENT );

		/*
		 * Add to the layout and show 
		 */
		mMainLayout.addView( mProgressView, viewParams );		
		mMainLayout.bringChildToFront( mProgressView );
		mMainLayout.requestLayout();
		mProgressView.setVisibility( View.VISIBLE );		
		mProgressView.requestFocus();
	}
	
	public void HideProgressView()
	{
		if ( mProgressView != null )
		{
			mProgressView.setVisibility( View.GONE );
			mMainLayout.removeView( mWebView );
			mMainLayout.requestLayout();
		}
	}
	
	
	/*************************************************************************************************
	 * This class implements the rectangle coordinates bundle
	 * 
	 * @author Alex Agranovich
	 */
	public static class WazeRect
	{
		WazeRect( int aMinX, int aMinY, int aMaxX, int aMaxY )
		{
			minx = aMinX; maxx = aMaxX;
			miny = aMinY; maxy = aMaxY;
		}
		public int minx;
		public int miny;
		public int maxx;
		public int maxy;
	}
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	private void Init()
	{	
		// --------- Main layout object ----------- 
		mMainLayout = new RelativeLayout( mContext );
		
		// --------- Create the application views -----------
		mAppView = new WazeMainView( mContext );		
		
		// --------- Add the application view to the main layout -----------		
		RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams( FILL_PARENT, FILL_PARENT );
		mMainLayout.addView( mAppView, params );
		mMainLayout.bringChildToFront( mAppView );

	}
	
	
    private InputMethodManager getInputMethodManager()
    {
        InputMethodManager mgr;
        mgr = ( InputMethodManager ) mContext.getSystemService( Context.INPUT_METHOD_SERVICE );        
        return mgr;
    }

    /*************************************************************************************************
     * HideSoftInput - Hides the on-screen keyboard  
     * @param None 
     */
    public void HideSoftInput( View aView )
    {
        getInputMethodManager().hideSoftInputFromWindow(  aView.getWindowToken(), 0 );
    }    
    /*************************************************************************************************
     * ShowSoftInput - Shows the on-screen keyboard  
     * @param None 
     */
    public void ShowSoftInput( View aView )
    {
    	getInputMethodManager().restartInput( aView );
        getInputMethodManager().showSoftInput( aView, InputMethodManager.SHOW_FORCED );
    }    
	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private RelativeLayout mMainLayout = null;
    private WazeMainView mAppView = null;			// Main Application view
    private WazeWebView	   mWebView = null;
    private View	       mEditBoxView = null;
    private Context 	   mContext = null;
    private View 		   mProgressView = null;
    
	/*************************************************************************************************
     *================================= Constants section =================================
     */
    public static final int WAZE_LAYOUT_EDIT_TYPE_SIMPLE = 0; // Simple editbox
    public static final int WAZE_LAYOUT_EDIT_TYPE_VOICE = 1; // Voice to text edit box style
    public static final float WAZE_LAYOUT_EDIT_HEIGHT = 50; // Apporx 0.3 inch relative to the base 160dpi
    public static final float WAZE_LAYOUT_EDIT_SIDE_MARGIN = 2; // Apporx 1/80 inch relative to the base 160dpi
    
    private final int FILL_PARENT = RelativeLayout.LayoutParams.FILL_PARENT;
	private final int WRAP_CONTENT = RelativeLayout.LayoutParams.WRAP_CONTENT;
}
