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
    public void onResume( WazeScreenManagerCallback aFgCallback )
    {    	
    	ForegroundRequest( aFgCallback );
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
		ResizeRequest();
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
    	SetState( SCREENMGR_STATE_APP_SHUTDOWN );
    }

    /*************************************************************************************************
     * class WazeEditBoxCallback - Provides an interface for the callback to be called upon action/enter event arrival 
     * Called on the native thread
     */
    public static interface WazeScreenManagerCallback 
    {   
    	public void Callback();
    }

    
    public void ForegroundRequest()
    {
    	ForegroundRequest( null );
    }
    /*************************************************************************************************
     * ForegroundRequest - checks state and posts the foreground request to be accomplished on the native thread
     * @param aFgCallback - Callback to be called upon foreground request full completion  
     */
    public void ForegroundRequest( final WazeScreenManagerCallback aFgCallback )
    {
    	DebugLog( "ForegroundRequest: State - " + curStateStr() );
    	/*
    	 * If already in foreground or in foreground request handling - nothing to do
    	 */
    	if ( mState == SCREENMGR_STATE_FOREGROUND ||
    			mState == SCREENMGR_STATE_FOREGROUND_REQUEST || 
    			mState == SCREENMGR_STATE_APP_SHUTDOWN )
    	{
    		return;
    	}
    	
    	if ( !mNativeManager.getInitializedStatus() )
    	{
    		SetState( SCREENMGR_STATE_FOREGROUND );
    		return;
    	}
    	
    	SetState( SCREENMGR_STATE_FOREGROUND_REQUEST );
    	
    	if ( mNativeManager.IsNativeThread() )
    	{
    		ForegroundRequestHandler( false, aFgCallback );
    	}
    	else
    	{
	    	/*
	    	 * Post the request
	    	 */
	    	Runnable request = new Runnable() {			
				public void run() {
					ForegroundRequestHandler( true, aFgCallback );				
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
    	int prevState = mState;
    	
		DebugLog( "BackgroundRequest: State - " + curStateStr() );
    	
    	/*
    	 * If already in background or in background request handling or shutting down - nothing to do
    	 */
    	if ( mState == SCREENMGR_STATE_BACKGROUND ||
    			mState == SCREENMGR_STATE_BACKGROUND_REQUEST ||     			
    			mState == SCREENMGR_STATE_APP_SHUTDOWN )
    		return;
    	
    	
    	if ( !mNativeManager.getInitializedStatus() )
    	{
    		SetState(  SCREENMGR_STATE_BACKGROUND );
    		return;
    	}
    	
        try 
        {
        	SetState( SCREENMGR_STATE_BACKGROUND_REQUEST );
        	synchronized( this ) {
	    		/*
	    		 * Notify the native thread to stop waiting for surface
	    		 * because the new background event is arrived
	    		 */
	    		this.notify();
		    	if ( prevState == SCREENMGR_STATE_FOREGROUND_REQUEST ||
		    			prevState == SCREENMGR_STATE_RESIZE_REQUEST )
		    	{
		    		DebugLog( "BackgroundRequest: Waiting ..." );
	    			this.wait();
		    	}
	    	}
        	/*
        	 * Reassign state if was changed by handler
        	 */
        	SetState( SCREENMGR_STATE_BACKGROUND_REQUEST );
        }
        catch( Exception ex )
        {
        	Log.e( "WAZE", "BackgroundRequest: Error while waiting for the foreground state" );
        	WazeLog.e( "BackgroundRequest: Error while waiting for the foreground state", ex );
        }
    	
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
			mNativeManager.PostRunnable( request );
	
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
    	DebugLog( "BackgroundRequest: Completed. State - " + curStateStr() );
    }
    
    /*************************************************************************************************
     * ResizeRequest - checks state and posts the resize request to be accomplished on the native thread  
     */
    public void ResizeRequest()
    {
    	int prevState = mState;
    	DebugLog( "ResizeRequest: State - " + curStateStr() );
    	
		if ( mState != SCREENMGR_STATE_FOREGROUND && mState != SCREENMGR_STATE_FOREGROUND_REQUEST )
		{
			WazeLog.e( "WAZE ResizeRequest. Invalid state: " + curStateStr() );
			return;
		}
		
		
		if ( !mNativeManager.getInitializedStatus() )
    	{
    		return;
    	}
		
		try 
        {
        	synchronized( this ) {
	    		/*
	    		 * Notify the native thread to stop waiting for surface
	    		 * because the new surface should be obtained
	    		 */
        		// AGA TODO:: Check this flow
	    		this.notify();

	    		if ( prevState == SCREENMGR_STATE_FOREGROUND_REQUEST ||
	    				prevState == SCREENMGR_STATE_RESIZE_REQUEST )
		    	{
		    		DebugLog( "ResizeRequest: Waiting ..." );
		    		// Wait for the previous request processing completion
	    			this.wait();
		    	}
		    	// Foreground event 
		    	if ( prevState == SCREENMGR_STATE_FOREGROUND_REQUEST )
		    		return;
	    	}
        	/*
        	 * Reassign state if was changed by handler
        	 */
        	SetState( SCREENMGR_STATE_RESIZE_REQUEST );
        }
        catch( Exception ex )
        {
        	Log.e( "WAZE", "ResizeRequest: Error while waiting for the foreground state" );
        	WazeLog.e( "ResizeRequest: Error while waiting for the foreground state", ex );
        }
	        
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
    private void ForegroundRequestHandler( boolean aIsNotify, final WazeScreenManagerCallback aFgCallback )
    {
//    	Log.w( "WAZE", "ForegroundRequestHandler" );
    	// AGA NOTE:: Should be reduced when stable
		WazeLog.w( "WAZE ForegroundRequestHandler" );
		DebugLog( "ForegroundRequestHandler: State - " + curStateStr() );
		
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
	        	DebugLog( "ForegroundRequestHandler. Start waiting. State - " + curStateStr() + ". App ready: " + appView.IsReady() );
	        	while( !appView.IsReady() && ( mState == SCREENMGR_STATE_FOREGROUND_REQUEST ) )
	        	{
//	        		Log.w( "WAZE", "ForegroundRequestHandler: Waiting for the surface to be ready" );
	        		wait( SCREENMGR_WAIT_TIMEOUT );
	        	}
	        	DebugLog( "ForegroundRequestHandler. End waiting. State - " + curStateStr() + ". App ready: " + appView.IsReady() );
	        	
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
    	if ( mState == SCREENMGR_STATE_FOREGROUND_REQUEST )
    	{
    		long endTime, startTime = android.os.SystemClock.elapsedRealtime();
	    	// Create the canvas
			if ( mNativeManager.getNativeCanvas().CreateCanvas() )
			{
				// Get it back
				mNativeManager.BackgroundRun( false );
				
				SetState( SCREENMGR_STATE_FOREGROUND );
		    	
			}
			else
			{
				// AGA NOTE:: Check this
				SetState( SCREENMGR_STATE_BACKGROUND );
			}
	    	// AGA NOTE:: Should be reduced when stable		
	    	WazeLog.w( "ForegroundRequestHandler completed" );
	    	
	    	endTime = android.os.SystemClock.elapsedRealtime();

			DebugLog( "ForegroundRequestHandler time: " + ( endTime - startTime ) );
    	}
    	else
    	{
	    	// AGA NOTE:: Should be reduced when stable		
	    	WazeLog.w( "ForegroundRequestHandler: State was changed to: " + mState );
    	}
    	DebugLog( "ForegroundRequestHandler finilizing: State - " + curStateStr() );
    	if ( aIsNotify )
    	{
	    	/*
	    	 * Notify UI thread of completion
	    	 */
	    	synchronized( this ) {
		        	this.notify();
	    	}	    	
    	}
    	if ( aFgCallback != null )
    	{
    		aFgCallback.Callback();
    	}
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
		
		DebugLog( "BackgroundRequestHandler: State - " + curStateStr() );
		
		if ( mState != SCREENMGR_STATE_BACKGROUND_REQUEST )
		{
			WazeLog.e( "WAZE BackgroundRequestHandler. Invalid state " + mState );
			return;
		}
		
		mNativeManager.BackgroundRun( true );	
		FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
		canvas.DestroyCanvas();

		SetState( SCREENMGR_STATE_BACKGROUND );
    	
    	if ( aIsNotify )
    	{
	    	/*
	    	 * Notify the UI thread of request handling completion
	    	 */
	    	synchronized( this ) {
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
		
		if ( mState == SCREENMGR_STATE_RESIZE_REQUEST )
		{
		
			FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
			
			// Create the canvas
			canvas.ResizeCanvas();
			// Update with orientation change if needed
			canvas.CanvasOrientationUpdate();
			
			SetState(  SCREENMGR_STATE_FOREGROUND );
		}
		else
		{
			WazeLog.e( "WAZE ResizeRequestHandler. State: " + curStateStr() );
		}
		
		if ( aIsNotify )
    	{
	    	/*
	    	 * Notify the UI thread of request handling completion
	    	 */
	    	synchronized( this ) {
	    		// Log.w( "WAZE", "Notifying resize request completion" );
	    		this.notify();
	    	}
    	}
		
    	// AGA NOTE:: Should be reduced when stable		
		WazeLog.w( "WAZE ResizeRequestHandler completed" );
    }
    
    private void SetState( final int aState )
    {
    	DebugLog( "SetState. New state: " + getStateDescription( aState ) + ". Old: " + curStateStr() );
    	if ( mStateLocked )
    		return;
    	
    	mState = aState;
    }
    
//    private void StateLock()
//    {
//    	mStateLocked = true;
//    }
//    private void StateUnLock()
//    {
//    	mStateLocked = false;
//    }

    /*************************************************************************************************
     * String description of the state 
     * 
     * 
     */
    private String getStateDescription( int aState )
    {
    	String res = "Unknown state";
    	switch( aState )
    	{
    		case SCREENMGR_STATE_APP_START:
    			res = "APP_START";
    			break;
    		case SCREENMGR_STATE_FOREGROUND_REQUEST: 
    			res = "FOREGROUND_REQUEST";
    			break;
    		case SCREENMGR_STATE_FOREGROUND: 
    			res = "FOREGROUND";
    			break;
    		case SCREENMGR_STATE_BACKGROUND_REQUEST: 
    			res = "BACKGROUND_REQUEST";
    			break;
    		case SCREENMGR_STATE_BACKGROUND: 
    			res = "BACKGROUND";
    			break;
    		case SCREENMGR_STATE_RESIZE_REQUEST: 
    			res = "RESIZE_REQUEST";
    			break;
    		case SCREENMGR_STATE_APP_SHUTDOWN: 
    			res = "APP_SHUTDOWN";
    			break;
    	}
    	return res;
    }
    private String curStateStr()
    {
    	return  getStateDescription( mState );
    }
    
    private void DebugLog( String aStr )
    {
    	if ( SCREENMGR_DEBUG_LOG_ENABLED )
    		Log.w( SCREENMGR_DEBUG_TAG, aStr );
    }
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */	
    private final int SCREENMGR_STATE_APP_START = 0;
	private final int SCREENMGR_STATE_FOREGROUND_REQUEST = 1;
	private final int SCREENMGR_STATE_FOREGROUND = 2;
	private final int SCREENMGR_STATE_BACKGROUND_REQUEST = 3;
	private final int SCREENMGR_STATE_BACKGROUND = 4;
	private final int SCREENMGR_STATE_RESIZE_REQUEST = 5;
	private final int SCREENMGR_STATE_APP_SHUTDOWN = 6;
	
	private final long SCREENMGR_WAIT_TIMEOUT = 5000L;	// 5 seconds is a huge timeout
	
	FreeMapNativeManager mNativeManager = null;
	private volatile int mState = SCREENMGR_STATE_FOREGROUND;		// Thread  safe
	private volatile boolean mStateLocked = false;					// Indicates if state can be changed
	// DEBUG VARS
	private final static boolean SCREENMGR_DEBUG_LOG_ENABLED = false;
	private final static String SCREENMGR_DEBUG_TAG = "WAZE DEBUG SCRMGR";
}
