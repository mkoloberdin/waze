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

import android.app.ProgressDialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.util.AttributeSet;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class WazeWebView extends WebView
{
    
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    public WazeWebView( Context aContext, AttributeSet aAttrSet )
    {
        super( aContext, aAttrSet );
    }
    public WazeWebView( Context aContext )
    {
        super( aContext );
        Init( aContext );        
    }

    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */

    private void Init( Context aContext )
    {
        mContext = aContext; 
        
        WebSettings webSettings = getSettings();
        webSettings.setSavePassword(false);
        webSettings.setSaveFormData(false);
        webSettings.setJavaScriptEnabled(true);
        webSettings.setSupportZoom(false);
        setWebViewClient( new WazeWebViewClient() );
        addJavascriptInterface( new WazeJavaScriptInterface(), "demo" );
    }
    
    final class WazeJavaScriptInterface {

    	WazeJavaScriptInterface() {
        }

        /**
         * This is not called on the UI thread. Post a runnable to invoke
         * loadUrl on the UI thread.
         */
        public void clickOnAndroid() {
            mHandler.post(new Runnable() {
                public void run() {
                    loadUrl("javascript:wave()"); 
                }
            });
        }
    }
    
    private class WazeWebViewClient extends WebViewClient {
        @Override
        public boolean shouldOverrideUrlLoading(WebView view, String url) {
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
        	
        	mProgressDlg = new ProgressDialog( mContext, android.R.style.Theme_Translucent_NoTitleBar );
        	mProgressDlg.setProgressStyle( ProgressDialog.STYLE_SPINNER );
        	mProgressDlg.setIndeterminate( true );
        	mProgressDlg.setCancelable( true );
        	mProgressDlg.setMessage( new String( "Loading...") ); 
        	mProgressDlg.show();
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
        }
    }
    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
    private ProgressDialog mProgressDlg = null;
    
    private Handler mHandler = new Handler();

    private Context mContext = null;
}
