/**  
 * WazeWebView.java - android auxiliary webview. Used as embedded view in the main application view   
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

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public final class WazeWebView extends WebView
{
    
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    public WazeWebView( Context aContext, AttributeSet aAttrSet )
    {
        super( aContext, aAttrSet );
        Init( aContext, null, 0 );
    }
    public WazeWebView( Context aContext )
    {
	   super( aContext.getApplicationContext() );
	   Init( aContext, null, 0 );
    }
    public WazeWebView( Context aContext, int aFlags )
    {
        super( aContext.getApplicationContext() );
        Init( aContext, null, aFlags );                
    }

    public boolean dispatchKeyEventPreIme ( KeyEvent aEvent )
    {
    	// Temporary solution
    	// TODO:: Replace by interface callback for baack key
    	if ( aEvent.getKeyCode() == KeyEvent.KEYCODE_BACK )
    	{
    		 if ( mBackCallback.onBackEvent( aEvent ) )
    			 return true;
    	}
    	return super.dispatchKeyEventPreIme( aEvent );
    }
    
    public void setBackCallback( WazeWebViewBackCallback aCallback )
    {
    	mBackCallback = aCallback;
    }
    
    public void setFlags( int aFlags )
    {
    	mFlags = aFlags;
    }
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */

    private void Init( Context aContext, WazeWebViewBackCallback aBackCallback, int aFlags )
    {
        mContext = aContext; 
        mFlags = aFlags;
        
        WebSettings webSettings = getSettings();
        webSettings.setSavePassword(false);
        webSettings.setSaveFormData(false);
        webSettings.setJavaScriptEnabled(true);
        webSettings.setSupportZoom(false);
        webSettings.setSaveFormData(true);    
        if ( ( mFlags & BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL ) >  0 )
        {
        	setHorizontalScrollBarEnabled( false );
        	setVerticalScrollBarEnabled( false );
        	setScrollContainer( false );
        }
        if ( ( mFlags & BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT ) > 0 )
        {
        	setBackgroundColor( Color.TRANSPARENT );
        }
        
        setBackCallback( aBackCallback );
        setClickable( true );
        setFocusableInTouchMode( true );
        setWebViewClient( new WazeWebViewClient() );        
    }

    public static interface WazeWebViewBackCallback
    {
    	public boolean onBackEvent( KeyEvent aEvent );
    }
    
    private final class WazeWebViewClient extends WebViewClient {
        @Override
        public boolean shouldOverrideUrlLoading(WebView view, String url) {

        	// Opens the dialer for the "tel:*" links
        	if ( url.startsWith( "tel:" ) ) { 
                final Intent intent = new Intent( Intent.ACTION_DIAL, Uri.parse( url ) ); 
                final Activity activity = FreeMapAppService.getMainActivity();
                activity.runOnUiThread( new Runnable() {
					public void run() {
						activity.startActivity( intent );
					} } );                
                return true;
        	}
        	
        	FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
        	// First try to handle internally
        	if ( !mgr.UrlHandler( url ) )
        	{
        		view.loadUrl(url);
        	}
            return true;
        }
        @Override
        public void onPageStarted (WebView view, String url, Bitmap  favicon )
        {
        	if ( mProgressDlg == null || !mProgressDlg.isShowing() )
        	{
        		InitProgressDlg();
        		mProgressDlg.show();
        	}
        	super.onPageStarted( view, url, favicon );
        	
        }
        @Override
        public void onPageFinished ( WebView view, String url )
        {
        	super.onPageFinished( view, url );
        	if ( mProgressDlg != null )
        	{
        		mProgressDlg.dismiss();
        		mProgressDlg = null;
        	}
        	// Remove from memory. Leave on the disk
        	clearCache( false );
        }
        
        private void InitProgressDlg()
        {
        	mProgressDlg = new ProgressDialog( mContext, android.R.style.Theme_Translucent_NoTitleBar );
        	mProgressDlg.setProgressStyle( ProgressDialog.STYLE_SPINNER );
        	mProgressDlg.setIndeterminate( true );
        	mProgressDlg.setCancelable( true );
        	mProgressDlg.setMessage( new String( "Loading...") ); 
        }
    }
    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
    private WazeWebViewBackCallback mBackCallback = null;
    
    private ProgressDialog mProgressDlg = null;
    private int mFlags;
    private Context mContext = null;
//    private Handler mHandler = new Handler();

	/*************************************************************************************************
     *================================= Constants section =================================
     */
    private static final int BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT = 0x00000020;
    private static final int BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL = 0x00000040; 

}
