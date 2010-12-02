/**  
 * WazeScreenManager.java - responsible for the background and orientation changes. 
 * Manages GL/EGL context and creation/destruction of the canvas   
 * 
 * NOTE:: Some Logs are still at the higher log level for the upcoming verion (s). Should be reduced when stable
 * 
 * LICENSE:
 *
 *   Copyright 2010     @author Alex Agranovich (AGA)
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

import android.os.SystemClock;
import android.util.Log;


final public class WazeScreenManager 
{

	/*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	public WazeScreenManager( FreeMapNativeManager aNativeMgr )
	{	
		mNativeManager = aNativeMgr;
	}
	
    /*************************************************************************************************
     * Activity onResume event notification 
     */
    public void onResume()
    {
    	ForegroundRequest();
    }
    /*************************************************************************************************
     * Activity onPause event notification 
     */
    public void onPause()
    {
    	BackgroundRequest();
    }
    /*************************************************************************************************
     * View onSurfaceChanged event notification 
     */
    public void onSurfaceChanged()
    {
    	/*
    	 * If currently in foreground - post resize request
    	 * otherwise notify the native thread if blocked while waiting
    	 * for the surface view
    	 */
    	if ( mState == SCREENMGR_STATE_FOREGROUND )
    	{
    		ResizeRequest();
    	}
    	else
    	{    		
        	synchronized( this )
        	{
        		this.notify();
        	}
    	}
    }
    /*************************************************************************************************
     * View onSurfaceDestroyed event notification 
     */
    public void onSurfaceDestroyed()
    {
    	BackgroundRequest();
    }
    /*************************************************************************************************
     * View onSurfaceDestroyed event notification 
     */
    public void onSurfaceCreated()
    {
    	ForegroundRequest();
    }	
    /*************************************************************************************************
     * Application is shutting down 
     */
    public void onShutDown()
    {
    	mState = SCREENMGR_STATE_APP_SHUTDOWN;
    }

    /*************************************************************************************************
     * ForegroundRequest - checks state and posts the foreground request to be accomplished on the native thread  
     */
    public void ForegroundRequest()
    {
    	/*
    	 * If already in foreground or in foreground request handling - nothing to do
    	 */
    	if ( mState == SCREENMGR_STATE_FOREGROUND ||
    			mState == SCREENMGR_STATE_FOREGROUND_REQUEST || 
    			mState == SCREENMGR_STATE_APP_SHUTDOWN )
    		return;
    	
    	mState = SCREENMGR_STATE_FOREGROUND_REQUEST;
    	
    	if ( mNativeManager.IsNativeThread() )
    	{
    		ForegroundRequestHandler();
    	}
    	else
    	{
	    	/*
	    	 * Post the request
	    	 */
	    	Runnable request = new Runnable() {			
				public void run() {
					ForegroundRequestHandler();				
				}
	    	};
			mNativeManager.PostRunnable( request );
		}    	
    }
    
    /*************************************************************************************************
     * BackgroundRequest - checks state and posts the background request to be accomplished on the native thread  
     */
    public void BackgroundRequest()
    {
    	/*
    	 * If already in background or in background request handling or shutting down - nothing to do
    	 */
    	if ( mState == SCREENMGR_STATE_BACKGROUND ||
    			mState == SCREENMGR_STATE_BACKGROUND_REQUEST || 
    			mState == SCREENMGR_STATE_APP_SHUTDOWN )
    		return;
    	
    	mState = SCREENMGR_STATE_BACKGROUND_REQUEST;
    	
    	if ( mNativeManager.IsNativeThread() )
    	{
    		BackgroundRequestHandler( false );
    	}
    	else // UI Thread
    	{
	    	/*
	    	 * Post the request
	    	 */
	    	Runnable request = new Runnable() {			
				public void run() {
					BackgroundRequestHandler( true );				
				}
			};
			mNativeManager.PostRunnableAtFront( request );
	
	    	/*
	    	 * Wait for the background request to complete
	    	 */
	    	synchronized( this )
	    	{
		        try 
		        {
		        	long wait_start_time = SystemClock.currentThreadTimeMillis();
		        	while( mState != SCREENMGR_STATE_BACKGROUND )
		        	{	        		
		        		wait( SCREENMGR_WAIT_TIMEOUT);
		        	}
		        	if ( SystemClock.currentThreadTimeMillis() - wait_start_time >= SCREENMGR_WAIT_TIMEOUT )
		        	{
		        		WazeLog.w( "Too long time waiting in BackgroundRequest" );
		        	}
		        }
		        catch( Exception ex )
		        {
		        	Log.e( "WAZE", "Error while waiting for the background request to complete" );
		        	WazeLog.e( "Error while waiting for the background request to complete", ex );
		        }
	    	}
    	}
    }
    
    /*************************************************************************************************
     * ResizeRequest - checks state and posts the resize request to be accomplished on the native thread  
     */
    public void ResizeRequest()
    {
		if ( mState != SCREENMGR_STATE_FOREGROUND )
		{
			WazeLog.e( "WAZE ResizeRequest. Invalid state " + mState );
			return;
		}
		
		mState = SCREENMGR_STATE_RESIZE_REQUEST;
		
    	if ( mNativeManager.IsNativeThread() )
    	{
    		ResizeRequestHandler( false );
    	}
    	else
    	{
			/*
	    	 * Post the request
	    	 */
	    	Runnable request = new Runnable() {			
				public void run() {
					ResizeRequestHandler( true );				
				}
			};
			mNativeManager.PostRunnable( request );
			/*
	    	 * Wait for the resize request to complete
	    	 */
	    	synchronized( this )
	    	{
		        try 
		        {
		        	long wait_start_time = SystemClock.currentThreadTimeMillis();
		        	while( mState != SCREENMGR_STATE_FOREGROUND )
		        	{	        		
		        		wait( SCREENMGR_WAIT_TIMEOUT );
		        	}
		        	if ( SystemClock.currentThreadTimeMillis() - wait_start_time >= SCREENMGR_WAIT_TIMEOUT )
		        	{
		        		WazeLog.w( "Too long time waiting in ResizeRequest" );
		        	}
		        }
		        catch( Exception ex )
		        {
		        	Log.e( "WAZE", "Error while waiting for the resize request to complete" );
		        	WazeLog.e( "Error while waiting for the resize request to complete", ex );
		        }
	    	}
			
    	}
		    	
    }    
	/*************************************************************************************************
     *================================= Private interface section =================================
     */

    /*************************************************************************************************
     * ForegroundRequestHandler - if surface is ready creates egl context and informs the native layer on 
     * foreground run  
     */
    private void ForegroundRequestHandler()
    {
//    	Log.w( "WAZE", "ForegroundRequestHandler" );
    	// AGA NOTE:: Should be reduced when stable
		WazeLog.w( "WAZE ForegroundRequestHandler" );

		if ( mState != SCREENMGR_STATE_FOREGROUND_REQUEST )
		{
			WazeLog.e( "WAZE ForegroundRequestHandler. Invalid state " + mState );
			return;
		}
		
		/*
		 * Block the native thread until the surface is ready
		 */
    	synchronized( this )
    	{
	        try 
	        {
	        	long wait_start_time = SystemClock.currentThreadTimeMillis();
	        	WazeMainView appView = FreeMapAppService.getMainView();
	        	while( !appView.IsReady() )
	        	{
//	        		Log.w( "WAZE", "ForegroundRequestHandler: Waiting for the surface to be ready" );
	        		wait( SCREENMGR_WAIT_TIMEOUT );
	        	}
	        	if ( SystemClock.currentThreadTimeMillis() - wait_start_time >= SCREENMGR_WAIT_TIMEOUT )
	        	{
	        		WazeLog.w( "Too long time waiting in ForegroundRequestHandler" );
	        	}

	        }
	        catch( Exception ex )
	        {
	        	WazeLog.e( "Error while waiting for the view to be ready", ex );
	        }
    	}
    	// Create the canvas
		mNativeManager.getNativeCanvas().CreateCanvas();
		// Get it back
		mNativeManager.BackgroundRun( false );

		mState = SCREENMGR_STATE_FOREGROUND;    	
    	// AGA NOTE:: Should be reduced when stable		
    	WazeLog.w( "ForegroundRequestHandler completed" );
    }
    /*************************************************************************************************
     * BackgroundRequestHandler - Destroys egl context and informs the native layer on 
     * background run  
     * @param aIsNotify: Is thread notification is necessary
     */
    private void BackgroundRequestHandler( boolean aIsNotify )
    {    	
    	// AGA NOTE:: Should be reduced when stable    	
		WazeLog.w( "WAZE BackgroundRequestHandler" );
		
		if ( mState != SCREENMGR_STATE_BACKGROUND_REQUEST )
		{
			WazeLog.e( "WAZE BackgroundRequestHandler. Invalid state " + mState );
			return;
		}
		
		mNativeManager.BackgroundRun( true );	
		FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
		canvas.DestroyCanvas();

    	mState = SCREENMGR_STATE_BACKGROUND;
    	
    	if ( aIsNotify )
    	{
	    	/*
	    	 * Notify the UI thread of request handling completion
	    	 */
	    	synchronized( this )
	    	{
	    		// Log.w( "WAZE", "Notifying background request completion" );
	    		this.notify();
	    	}
    	}
    	// AGA NOTE:: Should be reduced when stable    	
    	WazeLog.w( "WAZE BackgroundRequestHandler completed" );
    }
    /*************************************************************************************************
     * ResizeRequestHandler - 
     * 1. Recreates the EGL surface
     * 2. Informs the native layer on canvas change
     */
    private void ResizeRequestHandler( boolean aIsNotify )
    {
//    	Log.w( "WAZE", "ResizeRequestHandler" );
    	// AGA NOTE:: Should be reduced when stable    	
		WazeLog.w( "WAZE ResizeRequestHandler" );
		
		if ( mState != SCREENMGR_STATE_RESIZE_REQUEST )
		{
			WazeLog.e( "WAZE ResizeRequestHandler. Invalid state " + mState );
			return;
		}
		
		FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
		
		// Create the canvas
		canvas.ResizeCanvas();
		// Update with orientation change if needed
		canvas.CanvasOrientationUpdate();
		
		mState = SCREENMGR_STATE_FOREGROUND;
		if ( aIsNotify )
    	{
	    	/*
	    	 * Notify the UI thread of request handling completion
	    	 */
	    	synchronized( this )
	    	{
	    		// Log.w( "WAZE", "Notifying resize request completion" );
	    		this.notify();
	    	}
    	}
    	// AGA NOTE:: Should be reduced when stable		
		WazeLog.w( "WAZE ResizeRequestHandler completed" );
    }
    
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */	
	private final int SCREENMGR_STATE_FOREGROUND_REQUEST = 0;
	private final int SCREENMGR_STATE_FOREGROUND = 1;
	private final int SCREENMGR_STATE_BACKGROUND_REQUEST = 2;
	private final int SCREENMGR_STATE_BACKGROUND = 3;
	private final int SCREENMGR_STATE_RESIZE_REQUEST = 4;
	private final int SCREENMGR_STATE_APP_SHUTDOWN = 5;
	
	private final long SCREENMGR_WAIT_TIMEOUT = 5000L;	// 5 seconds is a huge timeout
	
	FreeMapNativeManager mNativeManager = null;
	private volatile int mState = SCREENMGR_STATE_FOREGROUND;		// Thread  safe

}
