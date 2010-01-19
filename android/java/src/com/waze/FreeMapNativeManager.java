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

import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Timer;

import android.content.Context;
import android.graphics.Canvas;
import android.location.LocationManager;
import android.media.AudioManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
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
    static public FreeMapNativeManager Start( FreeMapAppActivity aAppActivity,
            FreeMapAppView aAppView )
    {
        if (mNativeManager != null)
            return mNativeManager;

        // Creating the singleton instance
        mNativeManager = new FreeMapNativeManager();
        // Starting the context thread
        mNativeThread = mNativeManager.new NativeThread("Native Thread");
        mNativeThread.start();

        while (!mNativeManager.mAppLooperReady);

        mNativeManager.mAppActivity = aAppActivity;
        mNativeManager.mAppView = aAppView;
  
        mNativeManager.PostUIMessage(FreeMapUIEvent.UI_EVENT_START);

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
     * 
     */
    public boolean IsMenuEnabled()
    {
        return mIsMenuEnabled;
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
     * Screen refresh flag accessor
     * 
     */
    public boolean getScreenRefresh()
    {
        return mAppScreenRefreshFlag;
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
     * Returns the status of the battery at the moment Wrapper for the access
     * from the native side
     */
    public int getBatteryLevel()
    {
        return mAppActivity.getBatteryLevel();
    }

    /*************************************************************************************************
     * Show soft keyboard with the given action
     * @param aAction - show EditorInfo for values
     * @param aCloseOnAction - if true the keyboard is closed when action is pressed
     */
    public void ShowSoftKeyboard( int aAction, int aCloseOnAction )
    {
        Runnable showKeyboard = new Runnable()
        {
            public void run()
            {                
                mAppView.ShowSoftInput();
            }
        };
        // Set the action
        mAppView.setImeAction( aAction );
        // Set if close on action
        mAppView.setImeCloseOnAction( aCloseOnAction == 1 );
        // Run the keyboard
        mAppView.post( showKeyboard );
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
                mAppView.HideSoftInput();
            }
        };
        mAppView.post( hideKeyboard );
    }
    
    /*************************************************************************************************
     * Screen refresh flag modifier
     * 
     */
    public void setScreenRefresh( boolean aVal )
    {
        synchronized (mAppScreenRefreshFlag)
        {
            mAppScreenRefreshFlag = aVal;
            if (mAppScreenRefreshFlag)
            {

                // If canvas buffer was not ready
                // post the view that it can be drawn now
                if (!getCanvasBufReady())
                {
                    Runnable lWillDraw = new Runnable()
                    {
                        public void run()
                        {
                            mAppView.setWillNotDraw(false);
                        }
                    };
                    // Post the view
                    mAppView.post(lWillDraw);
                }
                // Invalidate the window
                mAppView.postInvalidate();
            }
        }
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
     * ForceNewCanvas() - Force the new canvas configuration
     * 
     */
    public void ForceNewCanvas()
    {
        Runnable lNewCanvas = new Runnable()
        {
            public void run()
            {
                if (mNativeCanvas != null && !getShutDownStatus())
                {
                    mNativeCanvas.ForceNewCanvas();
                }
            }
        };
        mUIMsgDispatcher.postAtFrontOfQueue(lNewCanvas);
    }

    /*************************************************************************************************
     * getAppActivity() - Activity reference accessor
     * 
     */
    public FreeMapAppActivity getAppActivity()
    {
        return mAppActivity;
    }

    /*************************************************************************************************
     * setAppActivity() - Activity reference modifier
     * 
     */
    public void setAppActivity( FreeMapAppActivity aAppActivity )
    {
        mAppActivity = aAppActivity;
    }
    
    /*************************************************************************************************
     * setCanvas() - Canvas reference modifier
     * 
     */
    public void setCanvas( Canvas aCanvas )
    {
        mNativeCanvas.setCanvas(aCanvas);
    }

    /*************************************************************************************************
     * getAppView() - View reference accessor
     * 
     */
    public FreeMapAppView getAppView()
    {
        return mAppView;
    }

    /*************************************************************************************************
     * Wrapper for the canvas update
     * 
     */
    public boolean UpdateCanvas( Canvas aCanvas )
    {
        if (mNativeCanvas != null)
        {
            return mNativeCanvas.UpdateCanvas(aCanvas);
        }
        return false;
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
     * async UI messages creator
     * thread. Creates message that can be sent to the native thread
     * 
     * @param aMsgId
     *            - Message id of the posted message
     * 
     */
    public Message GetUIMessage( FreeMapUIEvent aEvent )
    {
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
                case UI_EVENT_STARTUP_NOSDCARD:
                {
                	if ( FreeMapResources.CheckSDCardAvailable() )
                	{
                		AppStart();
                	}
                	else                		
                	{
	                    // Stopping the service
	                    FreeMapAppService.ShutDown();
	                    // Finish the activity
	                    mAppActivity.finish();
	                    // Stop the loop
	                    Looper.myLooper().quit();
	
	                    // Restore the system settings
	                    RestoreSystemSettings();
	
	                    // Finish the VM
	                    System.exit(0);
                	}

                    break;
                }

                case UI_EVENT_FORCE_NEW_CANVAS:
                {
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
     * Application shutdown
     * 
     */
    public void ShutDown()
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
        // Stopping the service
        FreeMapAppService.ShutDown();
        // Finish the activity
        mAppActivity.finish();
        // Stop the loop
        Looper.myLooper().quit();

        // Copy the native log
        // FreeMapResources.CopyLogToSD();

        // Stop wake up monitor
        mBackLightManager.StopWakerMonitor();
        
        // Restore the system settings
        RestoreSystemSettings();

        // Finish the VM
        System.exit(0);
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
        mBackLightManager.StopWakerMonitor();
    }
    
    /*************************************************************************************************
     * Setting the new volume
     */
    public void SetVolume( int aLvl, int aMinLvl, int aMaxLvl )
    {
        int newSysVol;
        int maxSysVol;

        // Volume level
        AudioManager audioMngr = (AudioManager) mAppActivity
                .getSystemService(FreeMapAppActivity.AUDIO_SERVICE);
        maxSysVol = audioMngr.getStreamMaxVolume(AudioManager.STREAM_MUSIC);

        // Linear transformation
        newSysVol = (aLvl - aMinLvl) * ((maxSysVol - 0) / (aMaxLvl - aMinLvl));

        audioMngr.setStreamVolume(AudioManager.STREAM_MUSIC, newSysVol, 0);
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
        // Screen timeout
//        Settings.System.putInt(mAppActivity.getContentResolver(),
//                Settings.System.SCREEN_OFF_TIMEOUT, mSysValScreenTimeout);
        // Volume level
        AudioManager audioMngr = (AudioManager) mAppActivity
                .getSystemService(FreeMapAppActivity.AUDIO_SERVICE);
        audioMngr.setStreamVolume(AudioManager.STREAM_MUSIC, mSysValVolume, 0);

    }

    /*************************************************************************************************
     * LoadSystemSettings() - Loads the necessary system settings
     * Thread safe
     */
    synchronized public void LoadSystemSettings()
    {
        try
        {
            // Screen timeout
            mSysValScreenTimeout = Settings.System.getInt(mAppActivity
                    .getContentResolver(), Settings.System.SCREEN_OFF_TIMEOUT);
            // Volume level
            AudioManager audioMngr = (AudioManager) mAppActivity
                    .getSystemService(FreeMapAppActivity.AUDIO_SERVICE);
            mSysValVolume = audioMngr
                    .getStreamVolume(AudioManager.STREAM_MUSIC);
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
        LoadSystemSettings();

        // Init the resources provider
        FreeMapResources.InitContext(mAppActivity);
        
        try
        {
            // Wait for the initialization of the View dimensions
            while (mAppView.getMeasuredWidth() == 0)
            {
                Thread.sleep(50L);
            }
            // Initialize the canvas. At this stage the things can be drawn on
            // the screen - the onDraw events can be handled
            mNativeCanvas = new FreeMapNativeCanvas(this);
            // Giving a time to draw the splash
            // Wait for the initialization of the View dimensions
            while (getScreenRefresh())
            {
                Thread.sleep(200L);
                mAppView.postInvalidate();
            }
            // Show the splash for the 1 sec
            Thread.sleep(1000L);
            
            // Check sdcard available
            if( !FreeMapResources.CheckSDCardAvailable() )
            {
            	return;
            }
            
        }
        catch (InterruptedException ex)
        {
            WazeLog.w( "Interrupted while sleeping in InitNativeManager", ex );
        }
        
        // Backlight management
        mBackLightManager = new FreeMapBackLightManager( this );
        mBackLightManager.StartWakeMonitor( FreeMapBackLightManager.WAKE_REFRESH_PERIOD_DEFAULT );
        
        // Extracting the resources if necessary
        FreeMapResources.ExtractResources();

        // Loading the native library
        LoadNativeLib( FreeMapResources.mPkgDir + FreeMapResources.mLibFile);

        // Initialize the library
        InitNativeManagerNTV(FreeMapResources.mAppDir );

        // Set if the run is the upgrade run
        SetUpgradeRunNTV( FreeMapResources.mUpgradeRun );
        
        // Initialize the native canvas after the library is loaded
        mNativeCanvas.InitNativeCanvas();

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
	        AppStartNTV();
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
            WazeLog.e( "Error: Could not load library " + aTargetFile, ule );
        }
    }

    /*************************************************************************************************
     * Initialize the GPS
     */
    private void InitGPS()
    {
        // Location manager
        mLocationManager = (LocationManager) mAppActivity
                .getSystemService(Context.LOCATION_SERVICE);

        // Register the listener
        mLocationListner = new FreeMapNativeLocListener( mLocationManager );

        // Request updates anyway
        mLocationManager.requestLocationUpdates( LocationManager.NETWORK_PROVIDER, 0, 0,
                mLocationListner);
        mLocationManager.requestLocationUpdates( LocationManager.GPS_PROVIDER, 0, 0,
                mLocationListner);
        
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
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void NativeMsgDispatcherNTV( int aMsgId );

    private native void InitNativeManagerNTV( String aLibPath );

    private native void AppStartNTV(); // Starts the application

    private native void AppShutDownNTV(); // Shutdowns the application
    
    private native void ShowGpsDisabledWarningNTV(); // Shows the user the warning in case of the disabled GPS

    private native void BackLightMonitorResetNTV();	// Resets the backlight monitor
    
    private native void SetUpgradeRunNTV( byte value );	// Sets if this run is the first(upgrade or installation) run
    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */
	private FreeMapAppActivity mAppActivity; // Main application activity
	private FreeMapAppView mAppView; // Main application view

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
	                                             // indicator
    private boolean mAppLooperReady = false;
    private Boolean mAppScreenRefreshFlag  = true;
	private boolean mAppCanvasBufReady = true; // Indicates whether the canvas
	                                            // buffer is ready

	private boolean mIsMenuEnabled = false;		// Indicates whether the menu is enabled or not
	
	private static final int NATIVE_THREAD_PRIORITY = -18;
	private static final long CAMERA_PREVIEW_TIMEOUT = 30000L; // Camera preview
                                                                // timeout
                                                                // before
                                                                // returning to
                                                                // the main flow
    //private static final String LOG_TAG = "FreeMapNativeManager";

    private static NativeThread mNativeThread;
    private static FreeMapNativeManager mNativeManager         = null;

	private int mSysValScreenTimeout; // System value for the screen timeout
	private int mSysValVolume; // System value for the media volume
	
}
