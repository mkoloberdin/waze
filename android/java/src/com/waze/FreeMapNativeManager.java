/**  
 * FreeMapNativeManager.java
 * Application layer (singleton) working with the native side. Provides the interface for the java layer
 * It calls the native side functions in which the main application functionality is implemented
 *
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
 *   @see FreeMapNativeManager_JNI.h/c
 */

package com.waze;

import java.io.File;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import com.waze.WazeEditBox.WazeEditBoxCallback;
import com.waze.WazeLayoutManager.WazeRect;
import com.waze.WazeLayoutManager;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.LocationManager;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Debug;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.StatFs;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

public final class FreeMapNativeManager
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */

    // Represents the UI events
    public enum FreeMapUIEvent
    {
        UI_EVENT_START, // Start application message
        UI_EVENT_FORCE_NEW_CANVAS, // Force new canvas
        UI_EVENT_KEY_DOWN, // Key down event
        UI_EVENT_TOUCH, // Touch event
        UI_EVENT_STARTUP_NOSDCARD, // Starting up with no sdcard
        UI_EVENT_STARTUP_GPUERROR, // Error access GPU
        UI_EVENT_LOW_MEMORY, // Low memory warning
        UI_EVENT_SCREENSHOT, // Screen shot manager request
        UI_EVENT_NATIVE,
        UI_PRIORITY_EVENT_NATIVE; 

        // Get the UI event
        public static FreeMapUIEvent FromInt( int aCode )
        {
            return FreeMapUIEvent.values()[aCode];
        }

        // Get the code for the UI event
        public static int ToInt( FreeMapUIEvent aEvent )
        {
            return aEvent.ordinal();
        }

    };

    /*************************************************************************************************
     * Entire application entry point.
     * 
     */
    static public FreeMapNativeManager Start()
    {
        if (mNativeManager != null)
            return mNativeManager;

        // Creating the singleton instance
        mNativeManager = new FreeMapNativeManager();
        // Starting the context thread
        mNativeThread = mNativeManager.new NativeThread("Native Thread");
        mNativeThread.start();

        while (!mNativeManager.mAppLooperReady);            
        
        mNativeManager.PostUIMessage(FreeMapUIEvent.UI_EVENT_START);

        if ( TEMPERATURE_PROFILER_ENABLED )
        	mNativeManager.InitTemperatureProfiler();
        
        return mNativeManager;
    }

    /*************************************************************************************************
     * Application timer
     * 
     */
    public Timer getTimer()
    {
        return mTimer;
    }
    
    /*************************************************************************************************
     * Menu enabled indicator inspector
     */
    public boolean IsMenuEnabled()
    {
        return mIsMenuEnabled;
    }
    /*************************************************************************************************
     * Application start flag inspector
     */
    public boolean IsAppStarted()
    {
        return mAppStarted;
    }

    /*************************************************************************************************
     * Checks if the current thread is the native one ( the thread  
     * that Native Manager lives in its context )
     */
    public boolean IsNativeThread()
    {
    	return ( Thread.currentThread().getId() == mNativeThread.getId() );
    }
    /*************************************************************************************************
     * Menu enabled indicator modifier
     * 
     */
    public void SetIsMenuEnabled( int aValue )
    {
        mIsMenuEnabled = ( aValue != 0 );
    }
    
    /*************************************************************************************************
     * Shutdown flag
     * 
     */
    public boolean getShutDownStatus()
    {
        return mAppShutDownFlag;
    }

    /*************************************************************************************************
     * Initialized flag
     * 
     */
    public boolean getInitializedStatus()
    {
        return mAppInitializedFlag;
    }

    /*************************************************************************************************
     * Canvas buffer ready flag accessor
     * 
     */
    public boolean getCanvasBufReady()
    {
        return mAppCanvasBufReady;
    }

    /*************************************************************************************************
     * Canvas buffer ready flag modifier
     * 
     */
    public void setCanvasBufReady( boolean aVal )
    {
        mAppCanvasBufReady = aVal;
    }

    /*************************************************************************************************
     * Inspector of the menu manager
     * 
     */
    public WazeMenuManager getMenuManager()
    {
        return mMenuManager;
    }

      
    /*************************************************************************************************
     * Wrapper for the native layer background run notification
     * 
     */
    public void BackgroundRun( boolean aValue )
    {    	
    	int value = aValue ? 1 : 0;
    	SetBackgroundRunNTV( value );
    }
    
    /*************************************************************************************************
     * UrlHandler
     * Checks if waze should take an action on the passed url 
     */
    public boolean UrlHandler( String aUrl )
    {    	
    	final String url = aUrl;
    	if ( url != null && url.startsWith( WAZE_URL_PATTERN ) )
    	{
    		if ( IsNativeThread() )
    		{
    			UrlHandlerNTV( url );
    		}
    		else
    		{
    			/*
    	    	 * Post the request
    	    	 */
    	    	Runnable request = new Runnable() {			
    				public void run() {
    					UrlHandlerNTV( url );				
    				}
    			};
    			PostRunnable( request );
    		}
    		return true;
    	}
    	return false;
    }

    /*************************************************************************************************
     * PostRunnable
     * Posts the runnable on the main dispatcher queue ( at the end )
     */
    public void PostRunnable( Runnable aMsg )
    {    	
    	mUIMsgDispatcher.post( aMsg );
    }
    /*************************************************************************************************
     * PostRunnable
     * Posts the runnable on the main dispatcher queue ( at the end ) with the delay
     */
    public void PostRunnable( Runnable aMsg, long aDelayMillis )
    {    	
    	mUIMsgDispatcher.postDelayed( aMsg, aDelayMillis );
    }
    /*************************************************************************************************
     * PostRunnableAtFront
     * Posts the runnable on the main dispatcher queue ( at the front )
     */
    public void PostRunnableAtFront( Runnable aMsg )
    {    
    	mUIMsgDispatcher.postAtFrontOfQueue( aMsg );
    }     
    
    /*************************************************************************************************
     * Returns the status of the battery at the moment Wrapper for the access
     * from the native side
     */
    public int getBatteryLevel()
    {
    	final FreeMapPowerManager pwrMgr = FreeMapAppService.getPowerManager();
        return pwrMgr.getCurrentLevel();
    }
    
    /*************************************************************************************************
     * Native canvas accessor
     * 
     */
    FreeMapNativeCanvas getNativeCanvas()
    {
    	return mNativeCanvas;
    }

    /*************************************************************************************************
     * Show soft keyboard with the given action
     * @param aAction - show EditorInfo for values
     * @param aCloseOnAction - if true the keyboard is closed when action is pressed
     */
    public void ShowSoftKeyboard( int aAction, int aCloseOnAction )
    {
    	final WazeMainView view = getMainView();
        Runnable showKeyboard = new Runnable()
        {
            public void run()
            {                
            	view.ShowSoftInput();
            }
        };
        // Set the action
        view.setImeAction( aAction );
        // Set if close on action
        view.setImeCloseOnAction( aCloseOnAction == 1 );
        // Run the keyboard
        view.post( showKeyboard );
    }

    /*************************************************************************************************
     * Shows contacts book 
     * @param void
     * @param void
     */
    public void ShowContacts()
    {
    	FreeMapAppService.ShowContacts();
    }

    /*************************************************************************************************
     * ShowEditBox - Show edit box with the soft input
     * @param aAction - show EditorInfo for values
     * @param aStayOnAction - if false the keyboard is closed when action is pressed
     * @param aText - The initial text to be shown in the text box
     * @param aCbContext - The context for the callback
     * @param aTopMargin - Top margin from the top canvas coordinate for the edit box
     */
    public void ShowEditBox( final int aAction, final int aStayOnAction, final byte[] aText, final long aCbContext, final int aTopMargin, final int aFlags )
    {
    	final WazeLayoutManager layoutMgr = FreeMapAppService.getMainActivity().getLayoutMgr();
    	// Prepare the parameters for the editbox
    	final WazeEditBoxCallback editBoxCb = new WazeEditBoxCallback( aCbContext ) 
        {			
			@Override public void CallbackDone(int aResult, String aText, long aCbContext ) {
					EditBoxCallbackNTV( aResult, aText, aCbContext );
			}
		};

		// Should be set on the UI thread 
		Runnable showEditBox = new Runnable()
        {
            public void run()
            {
        		WazeEditBox editBox = layoutMgr.CreateEditBox();
        		
        		// Set the action
        		editBox.setEditBoxAction( aAction );
                // Set if close on action
        		editBox.setEditBoxStayOnAction( aStayOnAction == 1 );
        		// Set the value
        		editBox.setEditBoxValue( new String( aText ) );
        		// Set the callback
        		editBox.setEditBoxCallback( editBoxCb );
        		// Set the flags
        		editBox.setEditBoxFlags( aFlags );
        		
                layoutMgr.ShowEditBox( aTopMargin );
            }
        };
        getAppActivity().runOnUiThread( showEditBox );
    }
    /*************************************************************************************************
     * Hiding the edit box view
     * @param none
     */
    public void HideEditBox()
    {
        // Show the edit box
		final WazeLayoutManager layoutMgr = FreeMapAppService.getMainActivity().getLayoutMgr();
        Runnable hideEditBox = new Runnable()
        {
            public void run()
            {                
                layoutMgr.HideEditBox();
            }
        };
        getAppActivity().runOnUiThread( hideEditBox );
    }
    /*************************************************************************************************
     * Hiding the soft keyboard
     * @param none
     */
    public void HideSoftKeyboard()
    {
        Runnable hideKeyboard = new Runnable()
        {
            public void run()
            {
                getMainView().HideSoftInput();
            }
        };
        getMainView().post( hideKeyboard );
    }

    /*************************************************************************************************
     * Adds an element to the touch event queue
     * 
     */
    public void setTouchEvent( MotionEvent aEvent )
    {
        synchronized (mUITouchEventQueue)
        {
            mUITouchEventQueue.add(aEvent);
        }
    }

    /*************************************************************************************************
     * Adds an element to the key event queue
     * 
     */
    public void setKeyEvent( KeyEvent aEvent )
    {
        synchronized (mUIKeyEventQueue)
        {
            mUIKeyEventQueue.add(aEvent);
        }
    }

    /*************************************************************************************************
     * Flush if this called force performing the UI handlers
     * 
     */
    public void Flush()
    {
        // TODO:: Check if its possible to force the message loop to be flushed
    }

    /*************************************************************************************************
     * CreateNewCanvasOGL() - Force the new canvas for the OPENGL
     * 
     */
    public void PostCreateCanvas()
    {
        Runnable lForceCanvas = new Runnable()
        {
            public void run()
            {
                if ( mNativeCanvas != null && !getShutDownStatus() )
                {
                    mNativeCanvas.CreateCanvas();
                }
            }
        };
        mUIMsgDispatcher.postAtFrontOfQueue( lForceCanvas );
    }

    /*************************************************************************************************
     * DestroyCanvasOGL() - Destroy the canvas for the OPENGL
     * 
     */
    public void PostDestroyCanvas()
    {
        Runnable lDestroyCanvas = new Runnable()
        {
            public void run()
            {
                if ( mNativeCanvas != null && !getShutDownStatus() )
                {
                    mNativeCanvas.DestroyCanvas();
                }
            }
        };
        mUIMsgDispatcher.postAtFrontOfQueue( lDestroyCanvas );
    }

    /*************************************************************************************************
     * getAppActivity() - Activity accessor (wrapper)
     * 
     */
    public FreeMapAppActivity getAppActivity()
    {
        return FreeMapAppService.getMainActivity();
    }


    /*************************************************************************************************
     * getAppView() - Main view reference accessor (wrapper)
     * 
     */
    public WazeMainView getMainView()
    {
        return FreeMapAppService.getMainView();
    }

     /*************************************************************************************************
     * Interface function for the async native messages to be posted to the
     * current thread. Adds the message to the mNativeMsgDispatcher queue
     * 
     * @param aMsgId
     *            - Message id of the posted message
     * 
     */
    public void PostNativeMessage( int aMsgId )
    {
        Message msg = Message.obtain( mUIMsgDispatcher, 
                            FreeMapUIEvent.ToInt(FreeMapUIEvent.UI_EVENT_NATIVE), aMsgId, 0 );
        msg.sendToTarget();
    }
    
    /*************************************************************************************************
     * Interface function for the async native messages to be posted to the
     * current thread. Adds the message to the mNativeMsgDispatcher queue 
     * 
     * @param aMsgId - Message id of the posted message
     * @param aObjParam - object parameter reference
     * 
     */
    public void PostNativeMessage( int aMsgId, IFreeMapMessageParam aObjParam )
    {
        Message msg = Message.obtain( mUIMsgDispatcher, 
                            FreeMapUIEvent.ToInt(FreeMapUIEvent.UI_EVENT_NATIVE), aMsgId, 0 );
        msg.obj = aObjParam;
        msg.sendToTarget();
    }

    /*************************************************************************************************
     * Interface function for the async native messages to be posted to the
     * current thread. Adds the message to the priority queue 
     * 
     * @param aMsgId - Message id of the posted message
     * @param aObjParam - object parameter reference
     * 
     */
    public void PostPriorityNativeMessage( int aMsgId, IFreeMapMessageParam aObjParam )
    {
        Message msg = Message.obtain( mUIMsgDispatcher, 
                            FreeMapUIEvent.ToInt(FreeMapUIEvent.UI_PRIORITY_EVENT_NATIVE), aMsgId, 0 );
        msg.obj = aObjParam;
        
        synchronized (mPriorityEventQueue) {	
        	mPriorityEventQueue.add(msg);
        }
        
        msg.sendToTarget();
    }
    
    /*************************************************************************************************
     * Interface function for the async UI messages to be posted to the current
     * thread. Adds the message to the mUIMsgDispatcher queue
     * 
     * @param aMsgId
     *            - Message id of the posted message
     * 
     */
    public void PostUIMessage( FreeMapUIEvent aEvent )
    {
        // Log.i( "Posting message", "Thread ID: " + String.valueOf(
        // Thread.currentThread().getId() ) + " Posting message: " +
        // String.valueOf( FreeMapUIEvent.ToInt( aEvent ) ) );
        // Posting a message
        Message.obtain( mUIMsgDispatcher, FreeMapUIEvent.ToInt(aEvent) ).sendToTarget();
    }
    /*************************************************************************************************
     * Interface function for the async UI messages to be posted to the current
     * thread. Adds the message to the mUIMsgDispatcher queue
     * 
     * @param aMsgId
     *            - Message id of the posted message
     * @param aParam            
     * 			  - Additional message parameter
     */
    public void PostUIMessage( FreeMapUIEvent aEvent, int aParam )
    {
        // Log.i( "Posting message", "Thread ID: " + String.valueOf(
        // Thread.currentThread().getId() ) + " Posting message: " +
        // String.valueOf( FreeMapUIEvent.ToInt( aEvent ) ) );
        // Posting a message
        Message msg = Message.obtain( mUIMsgDispatcher, FreeMapUIEvent.ToInt(aEvent) );
        msg.arg1 = aParam;
        msg.sendToTarget();
    }
    /*************************************************************************************************
     * async UI messages creator
     * thread. Creates message that can be sent to the native thread
     * 
     * @param aMsgId
     *            - Message id of the posted message
     * @param arg0 - message param #1           
     * @param arg1 - message param #2
     */
    public Message GetUIMessage( FreeMapUIEvent aEvent )
    {
        // Log.i( "Posting message", "Thread ID: " + String.valueOf(
        // Thread.currentThread().getId() ) + " Posting message: " +
        // String.valueOf( FreeMapUIEvent.ToInt( aEvent ) ) );
        // Posting a message
        return Message.obtain( mUIMsgDispatcher, FreeMapUIEvent.ToInt(aEvent) );
    }

    /*************************************************************************************************
     * This class implements the message handler loop for the current thread
     * 
     * @author Alex Agranovich
     */
    public final class NativeThread extends HandlerThread
    {
        public NativeThread(String aThreadName)
        {
            super(aThreadName, NATIVE_THREAD_PRIORITY);
            // this.setPriority( MAX_PRIORITY );
        }

        @Override
        protected void onLooperPrepared()
        {
            // UI message dispatcher
            mUIMsgDispatcher = new FreeMapUIMsgDispatcher();

//            Log.i(LOG_TAG, "Starting native thread. Id: "
//                    + String.valueOf(this.getId()));
            mAppLooperReady = true;
        }

    }
    
    private class CheckSDCardCallback implements DialogInterface.OnClickListener
    {
    	public void onClick( DialogInterface dialog, int which )
    	{
    		WazeMsgBox.Notify();    		
    	}
    }
    /*************************************************************************************************
     * Extraction of the native library and other resources from the APK package
     * 
     */
    public boolean CheckSDCardAvailable()
    {
    	File file = new File("/sdcard/");
    	// Check if SD card can be accessed
    	if ( !file.exists() || !file.canWrite() )
    	{
    		WazeMsgBox msgBox = WazeMsgBox.getInstance();
    		msgBox.setBlocking( true );
    		msgBox.Show( "Error", "Waze can't access your SD card. Make sure it isn't mounted.", "Ok", 
    									null, new CheckSDCardCallback(), null );    		
    		return false;
    	}
    	
    	// Check if there is enough space
    	StatFs fs = new StatFs( file.getPath() );
    	long memSize = ( long ) fs.getAvailableBlocks() * ( long ) fs.getBlockSize();
    	Log.i( "WAZE", "Available space at " + file.getPath()+ ": " + memSize );
    	if ( memSize < STORAGE_SPACE_LOW_BOUND )
    	{
    		WazeMsgBox msgBox = WazeMsgBox.getInstance();
    		msgBox.setBlocking( true );
    		msgBox.Show( "Insufficient Memory", "Waze needs at least 5Mb of free space on your sdcard.\nPlease free some space to continue.",
    				"Ok", null, new CheckSDCardCallback(), null );
    		
    		return false;
    	}
    	
    	return true;
    }
    
    /*************************************************************************************************
     * This class implements the UI message handler loop for the current thread
     * 
     * @author Alex Agranovich
     */
    public class FreeMapUIMsgDispatcher extends Handler
    {

        @Override
        public void handleMessage( Message aMsg )
        {
            if (  getShutDownStatus() ) // Don't handle the message during the shutdown
                return;
            
            // First handle all the proirity events
            handlePriorityEvents();
            
            FreeMapUIEvent lEvent = FreeMapUIEvent.FromInt(aMsg.what);
            long startTime = System.currentTimeMillis();
            long delta;
            String eventName = "";
            
            switch (lEvent)
            {
                case UI_EVENT_START:
                {
                    AppStart();
                    break;
                }
                case UI_EVENT_STARTUP_GPUERROR:
                {
            		AppLayerShutDown();                	
            		break;
                }
                case UI_EVENT_STARTUP_NOSDCARD:
                {
                	if ( CheckSDCardAvailable() )
                	{
                		AppStart();
                	}
                	else                		
                	{
                		if ( IsAppStarted() )
                		{
                			ShutDown();
                		}
                		else
                		{
                			AppLayerShutDown();
                		}
                	}

                    break;
                }
                case UI_EVENT_SCREENSHOT:
                {
                	( new WazeScreenShotManager() ).Capture( getMainView(), mNativeCanvas.getGL() );
                    break;
                }

                case UI_EVENT_FORCE_NEW_CANVAS:
                {
                    break;
                }
                case UI_EVENT_LOW_MEMORY:
                {
                	String memory_usage = new String( "Memory usage native heap. Used: " + Debug.getNativeHeapAllocatedSize() +
    		                ". Free: " + Debug.getNativeHeapFreeSize() +
    		                ". Total: " + Debug.getNativeHeapSize() );
                	WazeLog.w( new String( "Android system reported low memory !!! " ) + memory_usage );
                	break;
                }
                	
                case UI_EVENT_KEY_DOWN:
                case UI_EVENT_TOUCH:
                case UI_PRIORITY_EVENT_NATIVE:
                	// These are handled in the priority handler
                    break;
                    
                case UI_EVENT_NATIVE:
                {
                    // 1. If the object parameter is not passed - the assumption is that the message 
                    //    is active
                    // 2. If the object parameter is passed - check if the object is active
                    boolean isActive = true;
                    IFreeMapMessageParam prm = (IFreeMapMessageParam) aMsg.obj;
                    if ( prm != null )
                    {
                        isActive = prm.IsActive();
                        eventName = "Timer Event";
                    }
                    else
                    {
                    	eventName = "IO event";
                    }
                    if ( isActive )
                    {
                        NativeMsgDispatcherNTV( aMsg.arg1 );
                    }
                    break;
                }
            }
            delta = System.currentTimeMillis() - startTime;
            
            if ( delta > 500 && getInitializedStatus() )
            {
                 WazeLog.w( "WAZE PROFILER " + "EXCEPTIONAL TIME FOR " + eventName + " HANDLING TIME: " + delta );
            }
        }

		private void handlePriorityEvents() {
			while (true) {
		
				// touch events
				MotionEvent lTouchEvent = null;
				
				synchronized (mUITouchEventQueue) {
					if (!mUITouchEventQueue.isEmpty()) {
						lTouchEvent = mUITouchEventQueue.remove(0);
					}
				}
				
				if (lTouchEvent != null) {

            
					//Handle the event
					mNativeCanvas.OnTouchEventHandler(lTouchEvent);

					//eventName = "Touch Event";
					continue;
				}

                KeyEvent lKeyEvent = null;
                // Take the element from the queue head
                synchronized (mUIKeyEventQueue)
                {
                	if (!mUIKeyEventQueue.isEmpty()) {
                		lKeyEvent = mUIKeyEventQueue.remove(0);
                	}
                }
                
                if (lKeyEvent != null) {
                	// Handle the event
                	mNativeCanvas.OnKeyDownHandler(lKeyEvent);
                    //eventName = "Key Down Event";
                	continue;
                }
				
                Message msg = null;
                synchronized (mPriorityEventQueue) {
					if (!mPriorityEventQueue.isEmpty()) {
						msg = mPriorityEventQueue.remove(0);
					}
				}
                
                if (msg != null) {
                	FreeMapUIEvent lEvent = FreeMapUIEvent.FromInt(msg.what);
                	long startTime = System.currentTimeMillis();
                	long delta;
                	String eventName = "";
                
                	switch (lEvent)
                	{
                		case UI_PRIORITY_EVENT_NATIVE:
                		{
                			// 1. If the object parameter is not passed - the assumption is that the message 
                			//    is active
                			// 2. If the object parameter is passed - check if the object is active
                			boolean isActive = true;
                			IFreeMapMessageParam prm = (IFreeMapMessageParam) msg.obj;
                			if ( prm != null )
                			{
                				isActive = prm.IsActive();
                				eventName = "Timer Event";
                			}
                			else
                			{
                				eventName = "IO event";
                			}
                			if ( isActive )
                			{
                				NativeMsgDispatcherNTV( msg.arg1 );
                			}
                		}
                		break;
                		
                		default:
                			WazeLog.e("Unknown priority event - " + lEvent);
                	}
                		
                    delta = System.currentTimeMillis() - startTime;
                        
                    if ( delta > 500 )
                    {
                         WazeLog.w( "WAZE PROFILER " + "EXCEPTIONAL TIME FOR " + eventName + " HANDLING TIME: " + delta );
                    }                		
                	
                	continue;
                }
                
                // no events
                break;
			}	
		}
    }

    /*************************************************************************************************
     * Application shutdown entry point
     * 
     */
    public void ShutDown()
    {
    	if ( IsNativeThread() )
    	{
    		ShutDownRun();
    	}
    	else
    	{
    		Runnable shutDown = new Runnable()
    		{
    			public void run()
    			{
    				ShutDownRun();
    			}
    		};
    		this.PostRunnableAtFront( shutDown );
    	}
    	
    }
    
    /*************************************************************************************************
     * ShutDownRun() - Closes the application
     *   @param void
     *   @return void
     */
    private void ShutDownRun()
    {
        WazeLog.i( "Finalizing the application ..." );
        mAppShutDownFlag = true;

        // Shutdown the GPS manager
        mLocationManager.removeUpdates(mLocationListner);

        // Shutdown the native layer
        AppShutDownNTV();
        // Shutting down the timer manager
        mTimerManager.ShutDown();
        // Shutdown the media service
        mSoundManager.ShutDown();
        mSoundManager = null;
        
        // Stop wake up monitor
        mBackLightManager.StopWakerMonitor();        
                
        // Shutting down the android application layer
		AppLayerShutDown();
    }

    /*************************************************************************************************
     * Setting the backlight on
     */
    public void SetBackLightOn( int aAlwaysOn )
    {
        mAppBackLightAlwaysOn = ( aAlwaysOn == 1 );
        if ( mAppBackLightAlwaysOn )
            mBackLightManager.StartWakeMonitor( FreeMapBackLightManager.WAKE_REFRESH_PERIOD_DEFAULT );
        else
            mBackLightManager.StopWakerMonitor();
    }
    /*************************************************************************************************
     * Setting the backlight using the current setting
     */
    public void SetBackLightOn()
    {
        int alwaysOn = (mAppBackLightAlwaysOn) ? 1 : 0;
        SetBackLightOn( alwaysOn );
        BackLightMonitorResetNTV();
    }

    /*************************************************************************************************
     * Suspends backlight on monitor
     */
    public void StopBackLightOn()
    {
    	if ( mBackLightManager != null )
    	{
    		mBackLightManager.StopWakerMonitor();
    	}
    }
    
    /*************************************************************************************************
     * Setting the new volume
     */
    public void SetVolume( int aLvl, int aMinLvl, int aMaxLvl )
    {
        int newSysVol;
        int maxSysVol;

        // Volume level
        AudioManager audioMngr = (AudioManager) getAppActivity().getSystemService(FreeMapAppActivity.AUDIO_SERVICE);
        maxSysVol = audioMngr.getStreamMaxVolume(AudioManager.STREAM_MUSIC);

        // Linear transformation
        newSysVol = (aLvl - aMinLvl) * ((maxSysVol - 0) / (aMaxLvl - aMinLvl));
        
        audioMngr.setStreamVolume( AudioManager.STREAM_MUSIC, newSysVol, 0 );
    }
    /*************************************************************************************************
     * Setting the system volume
     */
    public void SetSysVolume( int aSysVol )
    {
        // Volume level
        AudioManager audioMngr = (AudioManager) getAppActivity().getSystemService( FreeMapAppActivity.AUDIO_SERVICE );
       
        audioMngr.setStreamVolume( AudioManager.STREAM_MUSIC, aSysVol, 0 );
    }
    /*************************************************************************************************
     * Minimize application. If aDelay >= 0 returns to the main activity after
     * aDelay milliseconds
     */
    public void MinimizeApplication( int aDelay )
    {
        FreeMapAppService.ShowHomeWindow(aDelay);
    }

    /*************************************************************************************************
     * Maximize the application
     */
    public void MaximizeApplication()
    {
        FreeMapAppService.ShowMainActivityWindow(0);
    }

    /*************************************************************************************************
     * Show the dialer
     */
    public void ShowDilerWindow()
    {
        FreeMapAppService.ShowDilerWindow(-1);
    }

    /*************************************************************************************************
     * GetThumbnail
     */
    public int GetThumbnail( int aThumbWidth, int aThumbHeight, int aBuf[] )
    {
        IntBuffer buf = IntBuffer.wrap(aBuf);
        int res = -1;
        int[] thumb = FreeMapCameraPreView.GetThumbnail(aThumbWidth, aThumbHeight);
        if ( thumb != null )
        {
            buf.put( thumb );
            res = 0;
        }
        return res;
    }

    /*************************************************************************************************
     * Take camera picture
     */
    public int TakePicture( int aImageWidth, int aImageHeight,
            int aImageQuality, byte aImageFolder[], byte aImageFile[] )
    {
        int retVal = -1;        
        // Set the configuration
        FreeMapCameraPreView.CaptureConfig(aImageWidth, aImageHeight, aImageQuality,
                        new String(aImageFolder), new String(aImageFile));
               

        // Set background
        WazeScreenManager screenManager = FreeMapAppService.getScreenManager();
        screenManager.BackgroundRequest();

        // Run the preview
        FreeMapAppService.ShowCameraPreviewWindow();

        // Make this call synchronous
        try
        {
            synchronized (mNativeManager)
            {
                mNativeManager.wait(CAMERA_PREVIEW_TIMEOUT);
            }
            // 1st case: There is no response from user during the defined
            // timeout when
            // the flow returned to the main activity/thread => capture status
            // is false
            // 2nd case: The capture is unsuccessful => capture status is false
            // 3rd case: The user returned to the main activity => capture
            // status is false
            // 4th case: The capture was successful => capture status is true
            if (FreeMapCameraPreView.getCaptureStatus())
            {
                FreeMapCameraPreView.SaveToFile();
                retVal = 0;
            }
            // Show the main activity
            FreeMapAppService.ShowMainActivityWindow(0);

        }
        catch (Exception ex)
        {
            WazeLog.e( "Error waiting for the capturing. ", ex );
            ex.printStackTrace();
        }
        return retVal;
    }

    /*************************************************************************************************
     * Static notification routine
     */
    public static void Notify( long aDelay )
    {
        // Sleep if necessary
        if (aDelay > 0)
        {
            try
            {
                Thread.sleep(aDelay);
            }
            catch (Exception ex)
            {
                WazeLog.e("Error waiting for the manager notification. ", ex );
                ex.printStackTrace();
            }
        }
        // Notify the object
        if (mNativeManager != null)
        {
            synchronized (mNativeManager)
            {
                mNativeManager.notify();
            }
        }
    }
    
    /*************************************************************************************************
     * Canvas is just created - handle the necessary operations
     * 
     */
    public void FirstSwapEvent()
    {    	
    	if ( getAppActivity() != null )
    	{
    		final WazeLayoutManager mgr = getAppActivity().getLayoutMgr();
    		final Runnable hideProgress = new Runnable() {
    			public void run()
    			{
    				mgr.HideProgressView();
    			}
        	};
        	getAppActivity().runOnUiThread( hideProgress );
    	}
    }

    /*************************************************************************************************
     * Market waze page. To be called from the native side.
     * Posts the call on the UI thread 
     */
    public void MarketRate()
    {    	
    	WazeLog.d( "Calling market page for Waze" );
    	final Activity appCtx = getAppActivity();
    	final Intent i = new Intent( Intent.ACTION_VIEW, Uri.parse("market://details?id=com.waze") );
    	Runnable showMarketWaze = new Runnable() {
			public void run()
			{
				appCtx.startActivity( i );
			}
    	};
    	appCtx.runOnUiThread( showMarketWaze );
    }
    
    /*************************************************************************************************
     * Wrapper for the web view to be called from the native side.
     * Posts the call on the UI thread 
     */
    public void ShowWebView( final byte[] aUrl, final int aMinX, final int aMinY, final int aMaxX, final int aMaxY, final int aFlags )
    {
    	WazeLog.d( "URL to load: " + aUrl );
    	final WazeRect rect = new WazeRect( aMinX, aMinY, aMaxX, aMaxY );    	
    	Runnable showWebView = new Runnable() {
			public void run()
			{
				WazeLayoutManager mgr = getAppActivity().getLayoutMgr();
				mgr.ShowWebView( new String( aUrl ), rect, aFlags );
			}
    	};
    	getAppActivity().runOnUiThread( showWebView );
    }
    
    /*************************************************************************************************
     * Wrapper for the web view to be called from the native side.
     * Posts the call on the UI thread 
     */
    public void LoadUrl( byte[] aUrl )
    {
    	final String url = new String( aUrl );
    	final FreeMapAppActivity activity = getAppActivity();
    	WazeLog.d( "URL to load: " + url );
    	
    	Runnable loadUrl = new Runnable() {
			public void run()
			{
				WazeLayoutManager mgr = activity.getLayoutMgr();
				WazeWebView webView = mgr.getWebView();
				webView.loadUrl( url );
			}
    	};
    	activity.runOnUiThread( loadUrl );
    }
    
    
    /*************************************************************************************************
     * Wrapper for the web view close function to be called from the native side.
     * Posts the call on the UI thread 
     */
    public void HideWebView()
    {
    	final FreeMapAppActivity activity = getAppActivity();
    	Runnable hideWebView = new Runnable() {
			public void run()
			{
				WazeLayoutManager mgr = activity.getLayoutMgr();
				mgr.HideWebView();
			}
    	};
    	activity.runOnUiThread( hideWebView );
    }
    
    /*************************************************************************************************
     * Screen timeout modifier
     */
    public void setScreenTimeout( int aVal )
    {
        mSysValScreenTimeout = aVal;
    }
    
    /*************************************************************************************************
     * RestoreSystemSettings() - resets the system configuration to the initial
     * state
     * Thread safe
     */
    synchronized public void RestoreSystemSettings()
    {
        // Volume level
//        SetSysVolume( mSysValVolume );        
    	StopBackLightOn();
    }

    /*************************************************************************************************
     * SaveAppSettings() - Saves the app settings 
     * Thread safe
     */
    synchronized public void SaveAppSettings()
    {
        // Screen timeout
//        Settings.System.putInt(mAppActivity.getContentResolver(),
//                Settings.System.SCREEN_OFF_TIMEOUT, mSysValScreenTimeout);
        // Volume level
    	  // Volume level
        AudioManager audioMngr = (AudioManager) getAppActivity().getSystemService( FreeMapAppActivity.AUDIO_SERVICE );
        mAppMediaVolume = audioMngr.getStreamVolume( AudioManager.STREAM_MUSIC );        
    }

    /*************************************************************************************************
     * RestoreAppSettings() - Restore the app settings 
     * Thread safe
     */
    synchronized public void RestoreAppSettings()
    {
    	SetBackLightOn();
//    	SetSysVolume( mAppMediaVolume );
    }

    /*************************************************************************************************
     * LoadSystemSettings() - Loads the necessary system settings
     * Thread safe
     */
    synchronized public void SaveSystemSettings()
    {
        try
        {
            // Screen timeout
            mSysValScreenTimeout = Settings.System.getInt( getAppActivity().getContentResolver(), Settings.System.SCREEN_OFF_TIMEOUT);
            // Volume level
            AudioManager audioMngr = (AudioManager) getAppActivity().getSystemService(FreeMapAppActivity.AUDIO_SERVICE);
            mSysValVolume = audioMngr.getStreamVolume(AudioManager.STREAM_MUSIC);
        }
        catch (SettingNotFoundException ex)
        {
            WazeLog.w( "Setting load error ", ex );
        }
    }

    
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */

    /*************************************************************************************************
     * Initialization of the native manager
     * 
     * @param aAppActivity
     *            - application activity
     * @param aAppView
     *            - application view
     * 
     */
    private void InitNativeManager()
    {
        // Initialize the  timer
        mTimer = new Timer("FreeMap Timer");
        
        // Load the system settings
        SaveSystemSettings();

        try
        {
        	synchronized ( this ) {
			    while ( getMainView() == null || !getMainView().IsReady() )
			    {
			    	this.wait();
			    }
        	}
        }
        catch ( Exception ex )
        {
        	Log.w( "WAZE", "Error waiting for the surface view" );
        	ex.printStackTrace();
        }

        // Check sdcard available
        if( !CheckSDCardAvailable() )
        {
			AppLayerShutDown();
        	return;
        }
        
        // Backlight management
        mBackLightManager = new FreeMapBackLightManager( this );
        mBackLightManager.StartWakeMonitor( FreeMapBackLightManager.WAKE_REFRESH_PERIOD_DEFAULT );

        // Extracting the resources if necessary
        boolean extracted = FreeMapResources.ExtractResources();

        /*
         * Splash time if there was no extraction
         */
        if ( !extracted )
        	android.os.SystemClock.sleep( 500 );        
        
        // Loading the native library
        LoadNativeLib( FreeMapResources.mPkgDir + FreeMapResources.mLibFile);
        
        int sdkBuildVersion = Integer.parseInt( android.os.Build.VERSION.SDK );
        

        String manufacturer = new String( "not available" );
        if ( sdkBuildVersion > 3 )
        	manufacturer = CompatabilityWrapper.getManufacturer();
        
    	// Initialize the library
    	InitNativeManagerNTV( FreeMapResources.mAppDir, sdkBuildVersion, android.os.Build.DEVICE, 
        		android.os.Build.MODEL, manufacturer );

        
        // Set if the run is the upgrade run
        SetUpgradeRunNTV( FreeMapResources.mUpgradeRun );
        
        // Canvas initialization
        mNativeCanvas = new FreeMapNativeCanvas(this);       
        // Initialize the native canvas if the view is ready        
        if ( getMainView()!=null && getMainView().IsReady() )
        {
        	mNativeCanvas.CreateCanvas();
        }
        else
        {
        	Log.e("WAZE", "Critical error: Canvas can't be created - surface view is not ready" );
        	WazeLog.f( "Critical error: Canvas can't be created - surface view is not ready" );
        }
        // Initialize the logger - pushes the native log to the java one
        // FreeMapLogTail logTail = new FreeMapLogTail(false);
        // logTail.Open( FreeMapResources.mAppDir + FreeMapResources.mResDir +
        // FreeMapResources.mLogFile );
        // logTail.Open( FreeMapResources.mSDCardDir + FreeMapResources.mLogFile
        // );
        // logTail.StartLogTail();

        // Timers management
        mTimerManager = new FreeMapNativeTimerManager( this );

        // Sound management
        mSoundManager = new FreeMapNativeSoundManager( this );

        // Menu manager
        mMenuManager = new WazeMenuManager( this );
        
        // Location management
        InitGPS();

        // Profiler
        if ( MEMORY_PROFILER_ENABLED )
        	InitMemoryProfiler();
        if( CPU_PROFILER_ENABLED )
        	WazeCPUProfiler.Start();
        
        // Message box engine
        WazeMsgBox.InitNativeLayer();
        
        mAppInitializedFlag = true;
    }

    /*************************************************************************************************
     * Initialization of the native manager Starting the application on the
     * native side 
     * 
     */
    private void AppStart()
    { 
    	// Initialize the manager - load native library, initialize native layer
        // objects : Canvas, Timers, Location, Sound 
        InitNativeManager();        
        if ( getInitializedStatus() )
        {
        	// Application start
        	WazeLog.w( "Starting the application!!!" );
        	AppStartNTV( FreeMapAppService.getUrl() );
        	mAppStarted = true;
	        WarnGpsDisabled();
        } 
    }

    /*************************************************************************************************
     * Loading the native library
     * 
     */
    private void LoadNativeLib( String aTargetFile )
    {
        try
        {           
            System.load(aTargetFile);
            WazeLog.i( "Library ... " + aTargetFile + "is loaded");
        }
        catch (UnsatisfiedLinkError ule)
        {
            Log.e( "WAZE", "Error: Could not load library " + aTargetFile + " - exiting! " + ule.getMessage() );
            ule.printStackTrace();
            AppLayerShutDown();
        }
    }

    /*************************************************************************************************
     * Initialize the GPS
     */
    private void InitGPS()
    {
        // Location manager
        mLocationManager = (LocationManager) FreeMapAppService.getAppContext().getSystemService(Context.LOCATION_SERVICE);

        // Register the listener
        mLocationListner = new FreeMapNativeLocListener( mLocationManager );

        // Request updates anyway
        mLocationManager.requestLocationUpdates( LocationManager.NETWORK_PROVIDER, 0, 0, mLocationListner);
        mLocationManager.requestLocationUpdates( LocationManager.GPS_PROVIDER, 0, 0, mLocationListner);        
    }

    /*************************************************************************************************
     *  Shows warning to the user in case of disabled gps 
     */
    private void WarnGpsDisabled()
    {    
        if ( !mLocationManager.isProviderEnabled( LocationManager.GPS_PROVIDER ) )
        {
            // Log the problem
            WazeLog.w( "GPS is disabled! Warning is shown to the user" );
            ShowGpsDisabledWarningNTV();
        }
    }
    
    private FreeMapNativeManager()
    {
        mUITouchEventQueue = new ArrayList<MotionEvent>();
        mUIKeyEventQueue = new ArrayList<KeyEvent>();
        mPriorityEventQueue = new ArrayList<Message>();
    }

    /*************************************************************************************************
     *  Android application layer shutdown routine 
     */
    private void AppLayerShutDown()
    {
        WazeScreenManager screenManager = FreeMapAppService.getScreenManager();
        screenManager.onShutDown();
        
        // Unregistering the temperature profiler
        if ( TEMPERATURE_PROFILER_ENABLED )
        	CloseTemperatureProfiler();
        // Stopping the service
        FreeMapAppService.ShutDown();
        // Finish the activity
         getAppActivity().finish();
         
        // Stop the loop
         Looper.myLooper().quit();

        // Restore the system settings
        RestoreSystemSettings();
    	
        // Finish the VM
        System.exit(0);
    }

    /*************************************************************************************************
     * CompatabilityWrapper the compatability wrapper class used to overcome dalvik delayed  
     * class loading. Supposed to be loaded only for the matching os version  
     */
    private static final class CompatabilityWrapper
    {
    	/*
    	 * For 1.6+ only
    	 */
    	public static String getManufacturer()
    	{
    		return android.os.Build.MANUFACTURER;
    	}
    }

    /*************************************************************************************************
     * InitMemoryProfiler()
     *   @param void
     *   @return void
     */
    private void InitMemoryProfiler()
    {
    	final Runnable memoryLog = new Runnable() {
			
			public void run() {
            	final String memory_usage = new String( "WAZE MEMORY PROFILER. Global heap used [Kb]: " + Debug.getGlobalAllocSize()/1000 +  
		        		" Native heap. Used [Kb]: " + Debug.getNativeHeapAllocatedSize()/1000 +
		                ". Free [Kb]: " + Debug.getNativeHeapFreeSize()/1000 +
		                ". Total [Kb]: " + Debug.getNativeHeapSize()/1000 +
		                ". External [Kb]: " + Debug.getThreadExternalAllocSize()/1000 );

				WazeLog.w( memory_usage );
				
			}
		};
    	TimerTask task = new TimerTask() {
		    @Override	 
		    public void run()
		    {
		    	FreeMapNativeManager.this.PostRunnable( memoryLog );            	            	
		    }
	    };
	    mNativeManager.getTimer().scheduleAtFixedRate( task, 0, MEMORY_PROFILER_PERIOD );
	}

    /*************************************************************************************************
     * InitTemperatureProfiler()
     *   @param void
     *   @return void
     */
    private void InitTemperatureProfiler()
    {
    	final SensorManager mgr = (SensorManager) getAppActivity().getSystemService( Context.SENSOR_SERVICE );
    	final Sensor sensor = mgr.getDefaultSensor( Sensor.TYPE_TEMPERATURE );
    	mLastTemperatureSampleTime = 0;
    	if ( sensor != null )
    	{
    		mTemperatureEventListener = new SensorEventListener()
			{
	    		public final String mSensorName = sensor.getName();
				public void onSensorChanged(SensorEvent arg0) {
//					Log.w( "WAZE DEBUG", "WAZE TEMPERATURE PROFILER: " + arg0.values[0] + ". Sensor: " + mSensorName );
					if ( !mAppInitializedFlag )
						return;
					
					long curTime = android.os.SystemClock.elapsedRealtime();
					if (  ( curTime - mLastTemperatureSampleTime ) > TEMPERATURE_PROFILER_PERIOD )
					{
							WazeLog.w( "WAZE TEMPERATURE PROFILER. Current temperature: " + arg0.values[0] + 
														". Sensor: " + mSensorName );
							mLastTemperatureSampleTime = curTime;
					}					
				}
				
				public void onAccuracyChanged(Sensor arg0, int arg1) {
				}
			};
			
	    	mgr.registerListener( mTemperatureEventListener, sensor, SensorManager.SENSOR_DELAY_NORMAL, mUIMsgDispatcher );    		
    	}
    }
    /*************************************************************************************************
     * CloseTemperatureProfiler()
     *   @param void
     *   @return void
     */
    private void CloseTemperatureProfiler()
    {
    	final SensorManager mgr = (SensorManager) getAppActivity().getSystemService( Context.SENSOR_SERVICE );
    	mgr.unregisterListener( mTemperatureEventListener );
    }
    
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    
    private native boolean UrlHandlerNTV( String aUrl );	// Handles custom urls ( true - if handled )
    
    private native void NativeMsgDispatcherNTV( int aMsgId );

    private native void InitNativeManagerNTV( String aLibPath, int aBuildSdkVersion, String aDeviceName,
    		String aDeviceModel, String aDeviceManufacturer );

    private native void AppStartNTV( String aUrl ); // Starts the application
    
    private native void AppShutDownNTV(); // Shutdowns the application
    
    private native void ShowGpsDisabledWarningNTV(); // Shows the user the warning in case of the disabled GPS

    private native void BackLightMonitorResetNTV();	// Resets the backlight monitor
    
    private native void SetUpgradeRunNTV( byte value );	// Sets if this run is the first(upgrade or installation) run
    
    private native void SetBackgroundRunNTV( int value );	// Sets true if the application is currently at the background
    
    private native void EditBoxCallbackNTV( int res, String aText, long aCbContext );	// Returns text and action result
    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
	
	private FreeMapNativeCanvas mNativeCanvas; // Native canvas

	private FreeMapBackLightManager mBackLightManager;
	
	private Timer                mTimer;                 // Application timer
	
	private FreeMapNativeTimerManager mTimerManager; // Application timers -
                                                        // will be used natively
	private FreeMapNativeSoundManager mSoundManager; // Sound manager - will be
                                                    // used natively
                                                    // private FreeMapNativeMsgDispatcher mNativeMsgDispatcher; // The native
                                                    // events handler
	private WazeMenuManager mMenuManager;			// Menu manager
	
	private FreeMapUIMsgDispatcher mUIMsgDispatcher; // The UI messages
	                                                    // dispatcher

	private ArrayList<MotionEvent> mUITouchEventQueue; // The event queue of the
	                                                        // touch events
	private ArrayList<KeyEvent> mUIKeyEventQueue; // The event queue of the key
	                                                            // events

	private ArrayList<Message> mPriorityEventQueue; // The event queue of the key
    // events
	
	private LocationManager mLocationManager; // Location manager attribute
	
	private FreeMapNativeLocListener mLocationListner; // Current listener

    private boolean mAppBackLightAlwaysOn = false; // Backlight always on indicator

	private boolean mAppShutDownFlag = false; // Shutdown process indicator
	private boolean mAppInitializedFlag = false; // Initialization process
	                                        
	private boolean mAppStarted = false;// Indicator if the application is started
	
    private boolean mAppLooperReady = false;
    private boolean mAppCanvasBufReady = true; // Indicates whether the canvas
	                                            // buffer is ready

	private boolean mIsMenuEnabled = true;		// Indicates whether the menu is enabled or not
	
	private static final String WAZE_URL_PATTERN = "waze://";
	private static final int NATIVE_THREAD_PRIORITY = -18;
	private static final long CAMERA_PREVIEW_TIMEOUT = 60000L; // Camera preview
                                                                // timeout
                                                                // before
                                                                // returning to
                                                                // the main flow
    //private static final String LOG_TAG = "FreeMapNativeManager";

    private static NativeThread mNativeThread;
    private static FreeMapNativeManager mNativeManager         = null;

    private int mAppMediaVolume = -1; // Application media volume level
    
	private int mSysValScreenTimeout; // System value for the screen timeout
	private int mSysValVolume; // System value for the media volume
	
	private SensorEventListener mTemperatureEventListener;
	private long mLastTemperatureSampleTime;
    /*************************************************************************************************
     *================================= Constants section =================================
     */
	private final static boolean MEMORY_PROFILER_ENABLED = true;
	private final static boolean CPU_PROFILER_ENABLED = true;
	private final static boolean TEMPERATURE_PROFILER_ENABLED = true;
	private final static long MEMORY_PROFILER_PERIOD = 120000L;		// In milliseconds	
	private final static long TEMPERATURE_PROFILER_PERIOD = 120000L;		// In milliseconds
	
	private final static long STORAGE_SPACE_LOW_BOUND = 5000000L;		// In bytes
}

