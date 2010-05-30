/**  
 * WazeLayoutManager - Initializes the main layout and 
 *           provides interface for management of the views on the screen. 
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 201     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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

import java.io.File;
import android.content.Context;
import android.view.View;
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
	
	public FreeMapAppView getAppView()
	{
		return mAppView;
	}
	
	public WazeWebView getWebView()
	{
		return mWebView;
	}

	public void ShowWebView( int aHeight, int aTopMargin, String aUrl )
	{
		final int FILL_PARENT = RelativeLayout.LayoutParams.FILL_PARENT;
		RelativeLayout.LayoutParams webParams = new RelativeLayout.LayoutParams( FILL_PARENT, aHeight );
		webParams.topMargin = aTopMargin;
		mWebView.setLayoutParams( webParams );
		mWebView.setVisibility( View.VISIBLE );
		mMainLayout.requestLayout();
		mWebView.loadUrl( aUrl );
	}
	

	public void HideWebView()
	{
		mWebView.setVisibility( View.GONE );
		mMainLayout.requestLayout();
	}
	
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	private void Init()
	{	
		final int FILL_PARENT = RelativeLayout.LayoutParams.FILL_PARENT;
		final int WRAP_CONTENT = RelativeLayout.LayoutParams.WRAP_CONTENT;
		
		mMainLayout = new RelativeLayout( mContext );
		
		// --------- Create the application views -----------
		mAppView = new FreeMapAppView( mContext );
		mWebView = new WazeWebView( mContext );
		
		// --------- Add the application view to the main layout -----------
		
		RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams( FILL_PARENT, FILL_PARENT );
		mMainLayout.addView( mAppView, WAZE_LAYOUT_INDEX_MAIN, params );
		mMainLayout.bringChildToFront( mAppView );
		
		// --------- Add the web view to the main layout -----------
		RelativeLayout.LayoutParams webParams = new RelativeLayout.LayoutParams( FILL_PARENT, WRAP_CONTENT );
		mMainLayout.addView( mWebView, WAZE_LAYOUT_INDEX_WEB, webParams );
		mWebView.setVisibility( View.GONE );
		
				
	}
	
	
	
	
	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
	RelativeLayout mMainLayout = null;
	FreeMapAppView mAppView = null;			// Main Application view
	WazeWebView	   mWebView = null;
    Context 	   mContext;
    
    public static final int WAZE_LAYOUT_INDEX_MAIN = 0;
    public static final int WAZE_LAYOUT_INDEX_EDIT = 2;
    public static final int WAZE_LAYOUT_INDEX_WEB = 1;

}
