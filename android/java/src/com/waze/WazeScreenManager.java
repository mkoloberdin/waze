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

import java.util.ArrayList;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


final public class WazeScreenManager 
{
	/*************************************************************************************************
     *================================= Public interface section =================================
     */
	public WazeScreenManager()
	{	
		mCanvasReadyEvents = new ArrayList<Runnable>();
	}
	
	public WazeScreenManager( FreeMapNativeManager aNativeMgr )
	{	
		mNativeManager = aNativeMgr;
		mCanvasReadyEvents = new ArrayList<Runnable>();
	}
	
	public void setNativeManager( FreeMapNativeManager aMgr )
	{
		mNativeManager = aMgr;
	}
	
    /*************************************************************************************************
     * Activity onResume event notification 
     */
    public void onResume()
    {    	
    	DebugLog( "onResume: Starting." );
    	
    	if ( mRunning )
    		return;

    	synchronized( this )
    	{
    		notifyAll();

    		if ( mNativeManager.IsNativeThread() )
        	{
    			onResumeHandler();
        	}
        	else
        	{
    	    	final Runnable request = new Runnable() {			
    				public void run() {
    					onResumeHandler();				
    				}
    	    	};
    			mNativeManager.PostRunnable( request );
    		}    	
    		
    		try
    		{
    			while ( !mRunning )
    			{
    				this.wait();
    			}
    		}
    		catch( Exception ex )
	        {
	        	Log.e( "WAZE", "onResume: Error while waiting for the onResume request completion" );
	        	WazeLog.e( "onResume: Error while waiting for the onResume request completion", ex );
	        	// AGA TODO :: Check this
	        	Thread.currentThread().interrupt();
	        }
	        DebugLog( "onResume: Finilizing. " );
    	}

    }
    
    /*************************************************************************************************
     * Activity onResume event handler 
     */
    public void onResumeHandler()
    {
    	synchronized( this )
    	{
    		DebugLog( "onResumeHandler: Starting." );
    		mRunning = true;
    		if ( mHasSurface )
    		{
    			FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
				
    			// Create the canvas surface
				canvas.CreateCanvas( FreeMapAppService.getMainView(), mSurfaceWidth, mSurfaceHeight );
				
				// AGA TODO:: Check success creation
				mNativeManager.BackgroundRun( false );
				
				// Fire Events
				CallOnCanvasReadyEvents();
				
				canvas.RedrawCanvas();
	    		// Second time
	    		canvas.RedrawCanvas( 500L );

			}
    		this.notifyAll();
    	}
    }
    /*************************************************************************************************
     * Activity onPause event notification 
     */
    public void onPause()
    {    	
    	DebugLog( "onPause: Starting." );
    	
    	boolean pauseBeforeInitialization = ( ( mSurfaceWidth == -1 ) && ( mSurfaceHeight == -1 ) );
    	
    	if ( !mRunning || mShuttingDown )
    		return;

    	synchronized( this )
    	{
    		notifyAll();

    		if ( pauseBeforeInitialization )
    		{
    			DebugLog( "onPause: pauseBeforeInitialization." );
    			mRunning = false;
    			return;
    		}
    		
    		if ( mNativeManager.IsNativeThread() )
        	{
    			onPauseHandler();
        	}
        	else
        	{
    	    	final Runnable request = new Runnable() {			
    				public void run() {
    					onPauseHandler();				
    				}
    	    	};
    			mNativeManager.PostRunnable( request );
    		}    	
    		
    		try
    		{
    			while ( mRunning )
    			{
    				this.wait();
    			}
    		}
    		catch( Exception ex )
	        {
	        	Log.e( "WAZE", "onPause: Error while waiting for the onPause request completion" );
	        	WazeLog.e( "onPause: Error while waiting for the onPause request completion", ex );
	        	// AGA TODO :: Check this
	        	Thread.currentThread().interrupt();
	        }
	        DebugLog( "onPause: Finilizing." );
    	}
    }
    
    /*************************************************************************************************
     * Activity onResume event handler 
     */
    public void onPauseHandler()
    {
    	synchronized( this )
    	{
    		mRunning = false;
    		mNativeManager.BackgroundRun( true );	
    		FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
    		canvas.DestroyCanvas();
    		this.notifyAll();
    	}
    }

    /*************************************************************************************************
     * onSurfaceCreated event notification 
     */
    public void onSurfaceCreated( final SurfaceView aView )
    {
    	DebugLog( "onSurfaceCreated: Starting." );
    	
//    	if ( !mRunning )
//    		return;
    	
		mHasSurface = true;
    	
    	synchronized( this )
    	{
    		notifyAll();

    		if ( mNativeManager.IsNativeThread() )
        	{
    			onSurfaceCreatedHandler( aView );
        	}
        	else
        	{
    	    	final Runnable request = new Runnable() {			
    				public void run() {
    					onSurfaceCreatedHandler( aView );				
    				}
    	    	};
    			mNativeManager.PostRunnable( request );
    		}    	
    		
    		try
    		{
    			while ( mSurfaceWait )
    			{
    				this.wait();
    			}
    		}
    		catch( Exception ex )
	        {
	        	Log.e( "WAZE", "onSurfaceCreated: Error while waiting for the onSurfaceCreated request completion" );
	        	WazeLog.e( "onSurfaceCreated: Error while waiting for the onSurfaceCreated request completion", ex );
	        	// AGA TODO :: Check this
	        	Thread.currentThread().interrupt();
	        }
	        DebugLog( "onSurfaceCreated: Finilizing." );
    	}
    }	
    /*************************************************************************************************
     * onSurfaceCreated event notification handler  
     */
    public void onSurfaceCreatedHandler( final SurfaceView aView )
    {
    	synchronized( this )
    	{
			FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
			
			// AGA TODO:: Check success creation
			if ( mRunning )
			{
				if ( mSurfaceWidth > 0 && mSurfaceHeight > 0 )
				{
					canvas.CreateCanvas( aView, mSurfaceWidth, mSurfaceHeight );
					mNativeManager.BackgroundRun( false );
				}
				
				// Fire Events
				CallOnCanvasReadyEvents();
	
	    		canvas.RedrawCanvas();
	    		// Second time
	    		canvas.RedrawCanvas( 500L );
	
			}
			else
			{
				canvas.PrepareCanvas( aView );
			}
			
			mSurfaceWait = false;    		
    		this.notifyAll();
    	}    	
    }	
    /*************************************************************************************************
     * onSurfaceChanged event notification 
     */
    public void onSurfaceChanged( final SurfaceView aView, int aWidth, int aHeight )
    {
    	DebugLog( "onSurfaceChanged: Starting." );
    	
//    	if ( !mRunning )
//    		return;
    	
    	mSurfaceChanged = ( ( mSurfaceWidth != aWidth ) || ( mSurfaceHeight != aHeight ) );
    	
    	if ( mSurfaceChanged )
    	{
    		mSurfaceWidth = aWidth;
    		mSurfaceHeight = aHeight;
    	}
    	else
    	{    		
    		return;
    	}
    	
    	synchronized( this )
    	{
    		notifyAll();
    		
    		if ( mNativeManager.IsNativeThread() )
        	{
    			onSurfaceChangedHandler( aView );
        	}
        	else
        	{
    	    	final Runnable request = new Runnable() {			
    				public void run() {
    					onSurfaceChangedHandler( aView );				
    				}
    	    	};
    			mNativeManager.PostRunnable( request );
    		}    	
    		
    		try
    		{
    			while ( mSurfaceChanged )
    			{
    				this.wait();
    			}
    		}
    		catch( Exception ex )
	        {
	        	Log.e( "WAZE", "onSurfaceChanged: Error while waiting for the onSurfaceChanged request completion" );
	        	WazeLog.e( "onSurfaceChanged: Error while waiting for the onSurfaceChanged request completion", ex );
	        	// AGA TODO :: Check this
	        	Thread.currentThread().interrupt();
	        }
	        DebugLog( "onSurfaceChanged: Finilizing." );
    	}
    }
    
    
    /*************************************************************************************************
     * onSurfaceChanged event notification handler 
     */
    public void onSurfaceChangedHandler( final SurfaceView aView )
    {
    	synchronized( this )
    	{
    		if ( mHasSurface )
    		{
    			FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
    			if ( mRunning )
    			{
	    	        // Destroy the surface - not necessary for the newer systems. Done to be on
	    	        // the safe side with all the devices
					canvas.DestroyCanvasSurface();
					
					// Create the canvas
					canvas.ResizeCanvas( aView, mSurfaceWidth, mSurfaceHeight );
	
					if ( mNativeManager.getIsBackgroundRun() )
					{
						mNativeManager.BackgroundRun( false );
					}
					
					// Fire Events
					CallOnCanvasReadyEvents();
		    		
		    		canvas.RedrawCanvas();
		    		
		    		// Second time
		    		canvas.RedrawCanvas( 500L );
    			}
    			else
    			{
    				canvas.PrepareCanvas( aView );
    			}
    		}
    		
    		mSurfaceChanged = false;	
    		this.notifyAll();
    		
    	}    	
    }
    
    /*************************************************************************************************
     * onSurfaceDestroyed event notification 
     */
    public void onSurfaceDestroyed()
    {
    	DebugLog( "onSurfaceDestroyed: Starting." );
    	
    	if ( !mHasSurface || mShuttingDown )
    		return;
    	
    	mHasSurface = false;
    	
    	synchronized( this )
    	{
    		notifyAll();
    		
    		if ( mNativeManager.IsNativeThread() )
        	{
    			onSurfaceDestroyedHandler();
        	}
        	else
        	{
    	    	final Runnable request = new Runnable() {			
    				public void run() {
    					onSurfaceDestroyedHandler();				
    				}
    	    	};
    			mNativeManager.PostRunnable( request );
    		}    	
    		
    		try
    		{
    			while ( !mSurfaceWait )
    			{
    				this.wait();
    			}
    		}
    		catch( Exception ex )
	        {
	        	Log.e( "WAZE", "onSurfaceDestroyed: Error while waiting for the onSurfaceDestroyed request completion" );
	        	WazeLog.e( "onSurfaceDestroyed: Error while waiting for the onSurfaceDestroyed request completion", ex );
	        	// AGA TODO :: Check this
	        	Thread.currentThread().interrupt();
	        }
	        DebugLog( "onSurfaceDestroyed: Finilizing." );
    	}
    }

    /*************************************************************************************************
     * onSurfaceDestroyed event notification handler 
     */
    public void onSurfaceDestroyedHandler()
    {
    	synchronized( this )
    	{
    		
    		mNativeManager.BackgroundRun( true );
    		
			FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
			
			// Destroying EGL surface only
			canvas.DestroyCanvasSurface();

			mSurfaceWait = true;
		    
			this.notifyAll();
    	}    	
    }
    
    /*************************************************************************************************
     * Screen ready indicator
     */
    public boolean IsReady()
    {
    	boolean res;
    	res = mHasSurface && mRunning && ( mSurfaceWidth > 0 ) && ( mSurfaceHeight > 0 ) ;
    	return res;
    }

    /*************************************************************************************************
     * Screen running indicator
     */
    public boolean IsRunning()
    {
    	return mRunning;
    }
    /*************************************************************************************************
     * Screen surface ready indicator
     */
    public boolean IsSurfaceReady()
    {
    	boolean res;
    	res = mHasSurface && ( mSurfaceWidth > 0 ) && ( mSurfaceHeight > 0 );
    	return res;
    }

    /*************************************************************************************************
     * Application is shutting down 
     */
    public void onShutDown()
    {
    	mShuttingDown = true;
    	FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
    	if ( canvas != null )
    		canvas.DestroyCanvas();
    }
    
    /*************************************************************************************************
     * Registration of the canvas ready events
     */
    public void registerOnCanvasReadyEvent( Runnable aEvent )
    {
    	mCanvasReadyEvents.add( aEvent );
    }
    
	/*************************************************************************************************
     *================================= Private interface section =================================
     */
    
    private void CallOnCanvasReadyEvents()
    {
    	FreeMapNativeCanvas canvas = mNativeManager.getNativeCanvas();
    	
    	if ( !mRunning ||
    		 !mHasSurface || 
			 canvas == null || 
			 !canvas.getCanvasReady() ||
			 mNativeManager.getIsBackgroundRun() ||
			 mSurfaceWidth < 0 || mSurfaceHeight < 0 )
    	{
    		return;
    	}
    	
    	while ( mCanvasReadyEvents.size() > 0 )
    	{
    		Runnable event = mCanvasReadyEvents.remove( 0 );
    		FreeMapNativeManager.Post( event );
    	}    	
    }

    /*************************************************************************************************
     * String description of the state 
     * 
     * 
     */
    
    private void DebugLog( String aStr )
    {
    	final String strLog = aStr + ". Running: " + mRunning + ". Surface changed " + mSurfaceChanged + 
		". Surface wait: " + mSurfaceChanged + ". Has Surface: " + mHasSurface +		
		". Dims: (" + mSurfaceWidth + ", " + mSurfaceHeight + ")";
    	
    	if ( SCREENMGR_DEBUG_LOG_ENABLED )
    	{
    		if ( mNativeManager != null && mNativeManager.getInitializedStatus() )
    			WazeLog.ww( SCREENMGR_DEBUG_TAG + " " + strLog );
    		else
    			Log.w( SCREENMGR_DEBUG_TAG, strLog );
    	}
    		
    }
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */	
    
    private volatile boolean mShuttingDown = false;
    private volatile boolean mRunning = true;
    private volatile boolean mSurfaceChanged = false;
    private volatile boolean mSurfaceWait = false;
    private volatile boolean mHasSurface = false;
    
    private volatile int mSurfaceWidth = -1;
    private volatile int mSurfaceHeight = -1;
    
    private ArrayList<Runnable> mCanvasReadyEvents = null;
    
	FreeMapNativeManager mNativeManager = null;

	// DEBUG VARS
	private final static boolean SCREENMGR_DEBUG_LOG_ENABLED = false;
	private final static String SCREENMGR_DEBUG_TAG = "WAZE DEBUG SCRMGR";
}

